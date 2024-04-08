/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "FireSpread.h"
#include "FuelLookup.h"
#include "FuelType.h"
#include "Scenario.h"
#include "Settings.h"
#include "unstable.h"
namespace tbd::sim
{
SlopeTableArray make_slope_table() noexcept
{
  // HACK: slope can be infinite, but anything > max is the same as max
  SlopeTableArray result{};
  for (size_t i = 0; i <= MAX_SLOPE_FOR_FACTOR; ++i)
  {
    result.at(i) = exp(3.533 * pow(i / 100.0, 1.2));
  }
  static_assert(result.size() == MAX_SLOPE_FOR_DISTANCE + 1);
  for (size_t i = MAX_SLOPE_FOR_FACTOR + 1; i < result.size(); ++i)
  {
    result.at(i) = result.at(MAX_SLOPE_FOR_FACTOR);
  }
  return result;
}
const SlopeTableArray SpreadInfo::SlopeTable = make_slope_table();
int calculate_nd_for_point(const int elevation, const topo::Point& point) noexcept
{
  return static_cast<int>(truncl(
    elevation < 0
      ? 0.5 + 151.0 * point.latitude() / (23.4 * exp(-0.0360 * (150 - point.longitude())) + 46.0)
      : 0.5 + 142.1 * point.latitude() / (33.7 * exp(-0.0351 * (150 - point.longitude())) + 43.0)
          + 0.0172 * elevation));
}
static double calculate_standard_back_isi_wsv(const double v) noexcept
{
  return 0.208 * exp(-0.05039 * v);
}
static const util::LookupTable<&calculate_standard_back_isi_wsv> STANDARD_BACK_ISI_WSV{};
static double calculate_standard_wsv(const double v) noexcept
{
  return v < 40.0
         ? exp(0.05039 * v)
         : 12.0 * (1.0 - exp(-0.0818 * (v - 28)));
}
static const util::LookupTable<&calculate_standard_wsv> STANDARD_WSV{};
SpreadInfo::SpreadInfo(const Scenario& scenario,
                       const double time,
                       const topo::SpreadKey& key,
                       const int nd,
                       const wx::FwiWeather* weather)
  : SpreadInfo(scenario, time, key, nd, weather, scenario.weather_daily(time))
{
}
double SpreadInfo::initial(SpreadInfo& spread,
                           const wx::FwiWeather& weather,
                           double& ffmc_effect,
                           double& wsv,
                           bool& is_crown,
                           double& sfc,
                           double& rso,
                           double& raz,
                           const fuel::FuelType* const fuel,
                           bool has_no_slope,
                           double heading_sin,
                           double heading_cos,
                           double bui_eff,
                           double min_ros,
                           double critical_surface_intensity)
{
  ffmc_effect = spread.ffmcEffect();
  // needs to be non-const so that we can update if slopeEffect changes direction
  raz = spread.wind().heading();
  const auto isz = 0.208 * ffmc_effect;
  wsv = spread.wind().speed().asDouble();
  if (!has_no_slope)
  {
    const auto isf1 = fuel->calculateIsf(spread, isz);
    // const auto isf = (0.0 == isf1) ? isz : isf1;
    // we know const auto isz = 0.208 * ffmc_effect;
    auto wse = 0.0 == isf1 ? 0 : log(isf1 / isz) / 0.05039;
    if (wse > 40)
    {
      wse = 28.0 - log(1.0 - min(0.999 * 2.496 * ffmc_effect, isf1) / (2.496 * ffmc_effect)) / 0.0818;
    }
    // we know that at->raz is already set to be the wind heading
    const auto wsv_x = spread.wind().wsvX() + wse * heading_sin;
    const auto wsv_y = spread.wind().wsvY() + wse * heading_cos;
    wsv = sqrt(wsv_x * wsv_x + wsv_y * wsv_y);
    raz = (0 == wsv) ? 0 : acos(wsv_y / wsv);
    if (wsv_x < 0)
    {
      raz = util::RAD_360 - raz;
    }
  }
  spread.raz_ = tbd::wx::Direction(raz, true);
  const auto isi = isz * STANDARD_WSV(wsv);
  // FIX: make this a member function so we don't need to preface head_ros_
  spread.head_ros_ = fuel->calculateRos(spread.nd(),
                                        weather,
                                        isi)
                   * bui_eff;
  if (min_ros > spread.head_ros_)
  {
    spread.head_ros_ = -1;
  }
  else
  {
    sfc = fuel->surfaceFuelConsumption(spread);
    rso = fuel::FuelType::criticalRos(sfc, critical_surface_intensity);
    is_crown = fuel::FuelType::isCrown(critical_surface_intensity,
                                       fuel::fire_intensity(sfc, spread.head_ros_));
    if (is_crown)
    {
      spread.head_ros_ = fuel->finalRos(spread,
                                        isi,
                                        fuel->crownFractionBurned(spread.head_ros_, rso),
                                        spread.head_ros_);
    }
  }
  return spread.head_ros_;
}
SpreadInfo::SpreadInfo(const Scenario& scenario,
                       const double time,
                       const topo::SpreadKey& key,
                       const int nd,
                       const wx::FwiWeather* weather,
                       const wx::FwiWeather* weather_daily)
  : key_(key),
    weather_(weather),
    time_(time),
    nd_(nd)
{
  // HACK: use weather_daily to figure out probability of spread but hourly for ROS
  max_intensity_ = -1;
  const auto slope_azimuth = topo::Cell::aspect(key_);
  const auto fuel = fuel::fuel_by_code(topo::Cell::fuelCode(key_));
  const auto has_no_slope = 0 == percentSlope();
  double heading_sin = 0;
  double heading_cos = 0;
  if (!has_no_slope)
  {
    const auto heading = util::to_heading(
      util::to_radians(static_cast<double>(slope_azimuth)));
    heading_sin = _sin(heading);
    heading_cos = _cos(heading);
  }
  // HACK: only use BUI from hourly weather for both calculations
  const auto bui_eff = fuel->buiEffect(bui().asDouble());
  const auto min_ros = Settings::deterministic()
                       ? Settings::minimumRos()
                       : std::max(scenario.spreadThresholdByRos(time_),
                                  Settings::minimumRos());
  // FIX: gets calculated when not necessary sometimes
  const auto critical_surface_intensity = fuel->criticalSurfaceIntensity(*this);
  double ffmc_effect;
  double wsv;
  bool is_crown;
  double sfc;
  double rso;
  double raz;
  if (min_ros > SpreadInfo::initial(
        *this,
        *weather_daily,
        ffmc_effect,
        wsv,
        is_crown,
        sfc,
        rso,
        raz,
        fuel,
        has_no_slope,
        heading_sin,
        heading_cos,
        bui_eff,
        min_ros,
        critical_surface_intensity)
      || sfc < COMPARE_LIMIT)
  {
    return;
  }
  // Now use hourly weather for actual spread calculations
  if (min_ros > SpreadInfo::initial(*this,
                                    *weather,
                                    ffmc_effect,
                                    wsv,
                                    is_crown,
                                    sfc,
                                    rso,
                                    raz,
                                    fuel,
                                    has_no_slope,
                                    heading_sin,
                                    heading_cos,
                                    bui_eff,
                                    min_ros,
                                    critical_surface_intensity)
      || sfc < COMPARE_LIMIT)
  {
    // no spread with hourly weather
    // NOTE: only would happen if FFMC hourly is lower than FFMC daily?
    return;
  }
  const auto back_isi = ffmc_effect * STANDARD_BACK_ISI_WSV(wsv);
  auto back_ros = fuel->calculateRos(nd,
                                     *weather,
                                     back_isi)
                * bui_eff;
  if (is_crown)
  {
    back_ros = fuel->finalRos(*this,
                              back_isi,
                              fuel->crownFractionBurned(back_ros, rso),
                              back_ros);
  }
  // do everything we can to avoid calling trig functions unnecessarily
  const auto b_semi = has_no_slope ? 0 : _cos(atan(percentSlope() / 100.0));
  const auto slope_radians = util::to_radians(slope_azimuth);
  // do check once and make function just return 1.0 if no slope
  const auto no_correction = [](const double) noexcept { return 1.0; };
  const auto do_correction = [b_semi, slope_radians](const double theta) noexcept {
    // never gets called if isInvalid() so don't check
    // figure out how far the ground distance is in map distance horizontally
    auto angle_unrotated = theta - slope_radians;
    if (util::to_degrees(angle_unrotated) == 270 || util::to_degrees(angle_unrotated) == 90)
    {
      // CHECK: if we're going directly across the slope then horizontal distance is same as spread distance
      return 1.0;
    }
    const auto tan_u = tan(angle_unrotated);
    const auto y = b_semi / sqrt(b_semi * tan_u * (b_semi * tan_u) + 1.0);
    const auto x = y * tan_u;
    // CHECK: Pretty sure you can't spread farther horizontally than the spread distance, regardless of angle?
    return min(1.0, sqrt(x * x + y * y));
  };
  const auto correction_factor = has_no_slope
                                 ? std::function<double(double)>(no_correction)
                                 : std::function<double(double)>(do_correction);
  const auto cell_size = scenario.cellSize();
  const auto add_offset = [this, cell_size, min_ros](const double direction,
                                                     const double ros) {
    if (ros < min_ros)
    {
      return false;
    }
    // spreading, so figure out offset from current point
    const auto ros_cell = ros / cell_size;
    offsets_.emplace_back(ros_cell * _sin(direction), ros_cell * _cos(direction));
    return true;
  };
  // if not over spread threshold then don't spread
  // HACK: assume there is no fuel where a crown fire's sfc is < COMPARE_LIMIT and its fc is >
  double ros{};
  // HACK: set ros in boolean if we get that far so that we don't have to repeat the if body
  if (!add_offset(raz, ros = (head_ros_ * correction_factor(raz))))
  {
    // mark as invalid
    head_ros_ = -1;
    return;
  }
  auto fc = sfc;
  // don't need to re-evaluate if crown with new head_ros_ because it would only go up if is_crown
  if (fuel->canCrown() && is_crown)
  {
    // wouldn't be crowning if ros is 0 so that's why this is in an else
    fc += fuel->crownConsumption(fuel->crownFractionBurned(head_ros_, rso));
  }
  // max intensity should always be at the head
  max_intensity_ = fuel::fire_intensity(fc, ros);
  const auto a = (head_ros_ + back_ros) / 2.0;
  const auto c = a - back_ros;
  const auto flank_ros = a / fuel->lengthToBreadth(wsv);
  const auto a_sq = a * a;
  const auto flank_ros_sq = flank_ros * flank_ros;
  const auto a_sq_sub_c_sq = a_sq - (c * c);
  const auto ac = a * c;
  const auto calculate_ros =
    [a, c, ac, flank_ros, a_sq, flank_ros_sq, a_sq_sub_c_sq](const double theta) noexcept {
      const auto cos_t = _cos(theta);
      const auto cos_t_sq = cos_t * cos_t;
      const auto f_sq_cos_t_sq = flank_ros_sq * cos_t_sq;
      // 1.0 = cos^2 + sin^2
      //    const auto sin_t_sq = 1.0 - cos_t_sq;
      const auto sin_t = _sin(theta);
      const auto sin_t_sq = sin_t * sin_t;
      return abs((a * ((flank_ros * cos_t * sqrt(f_sq_cos_t_sq + a_sq_sub_c_sq * sin_t_sq) - ac * sin_t_sq) / (f_sq_cos_t_sq + a_sq * sin_t_sq)) + c) / cos_t);
    };
  const auto add_offsets =
    [&correction_factor, &add_offset, raz, min_ros](
      const double angle_radians,
      const double ros_flat) {
      if (ros_flat < min_ros)
      {
        return false;
      }
      auto direction = util::fix_radians(angle_radians + raz);
      // spread is symmetrical across the center axis, but needs to be adjusted if on a slope
      // intentionally don't use || because we want both of these to happen all the time
      auto added = add_offset(direction, ros_flat * correction_factor(direction));
      direction = util::fix_radians(raz - angle_radians);
      added |= add_offset(direction, ros_flat * correction_factor(direction));
      return added;
    };
  const auto add_offsets_calc_ros =
    [&add_offsets, &calculate_ros](const double angle_radians) { return add_offsets(angle_radians, calculate_ros(angle_radians)); };
  bool added = add_offset(raz, head_ros_);
  constexpr size_t STEP = 10;
  size_t i = STEP;
  while (added && i < 90)
  {
    added = add_offsets_calc_ros(util::to_radians(i));
    i += STEP;
  }
  if (added)
  {
    added = add_offsets(util::to_radians(90), flank_ros * sqrt(a_sq_sub_c_sq) / a);
    i = 90 + STEP;
    while (added && i < 180)
    {
      added = add_offsets_calc_ros(util::to_radians(i));
      i += STEP;
    }
    if (added)
    {
      // only use back ros if every other angle is spreading since this should be lowest
      //  180
      if (back_ros < min_ros)
      {
        return;
      }
      const auto direction = util::fix_radians(util::RAD_180 + raz);
      static_cast<void>(!add_offset(direction, back_ros * correction_factor(direction)));
    }
  }
}
// double SpreadInfo::calculateSpreadProbability(const double ros)
// {
//   // note: based off spread event probability from wotton
//   return 1 / (1 + exp(1.64 - 0.16 * ros));
// }
}
