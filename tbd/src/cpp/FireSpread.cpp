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
// hull to condesnse points
#define DO_HULL
// use quick hull instead of regular if hulling
// #define QUICK_HULL
// number of degrees between spread directions
// if not defined then use variable step degrees
// #define STEP

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
int calculate_nd_ref_for_point(const int elevation, const topo::Point& point) noexcept
{
  // NOTE: cffdrs R package store longitude West as a positive, so this would be `- long`
  const auto latn = elevation <= 0
                    ? (46.0 + 23.4 * exp(-0.0360 * (150 + point.longitude())))
                    : (43.0 + 33.7 * exp(-0.0351 * (150 + point.longitude())));
  // add 0.5 to round by truncating
  return static_cast<int>(truncl(
    0.5 + (elevation <= 0 ? 151.0 * (point.latitude() / latn) : 142.1 * (point.latitude() / latn) + 0.0172 * elevation)));
}
int calculate_nd_for_point(const Day day, const int elevation, const topo::Point& point)
{
  return static_cast<int>(abs(day - calculate_nd_ref_for_point(elevation, point)));
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
    spread.sfc_ = fuel->surfaceFuelConsumption(spread);
    rso = fuel::FuelType::criticalRos(spread.sfc_, critical_surface_intensity);
    const auto sfi = fuel::fire_intensity(spread.sfc_, spread.head_ros_);
    spread.is_crown_ = fuel::FuelType::isCrown(critical_surface_intensity,
                                               sfi);
    if (spread.is_crown_)
    {
      spread.head_ros_ = fuel->finalRos(spread,
                                        isi,
                                        fuel->crownFractionBurned(spread.head_ros_, rso),
                                        spread.head_ros_);
    }
  }
  return spread.head_ros_;
}
static double find_min_ros(const Scenario& scenario, const double time)
{
  return Settings::deterministic()
         ? Settings::minimumRos()
         : std::max(scenario.spreadThresholdByRos(time),
                    Settings::minimumRos());
}
SpreadInfo::SpreadInfo(const Scenario& scenario,
                       const double time,
                       const topo::SpreadKey& key,
                       const int nd,
                       const wx::FwiWeather* weather,
                       const wx::FwiWeather* weather_daily)
  : SpreadInfo(time,
               find_min_ros(scenario, time),
               scenario.cellSize(),
               key,
               nd,
               weather,
               weather_daily)
{
}
static topo::SpreadKey make_key(const SlopeSize slope,
                                const AspectSize aspect,
                                const char* fuel_name)
{
  const auto lookup = tbd::sim::Settings::fuelLookup();
  const auto key = topo::Cell::key(topo::Cell::hashCell(slope,
                                                        aspect,
                                                        fuel::FuelType::safeCode(lookup.byName(fuel_name))));
  const auto a = topo::Cell::aspect(key);
  const auto s = topo::Cell::slope(key);
  const auto fuel = fuel::fuel_by_code(topo::Cell::fuelCode(key));
  logging::check_fatal(s != slope, "Expected slope to be %d but got %d", slope, s);
  const auto aspect_expected = 0 == slope ? 0 : aspect;
  logging::check_fatal(a != aspect_expected, "Expected aspect to be %d but got %d", aspect_expected, a);
  logging::check_fatal(0 != strcmp(fuel->name(), fuel_name), "Expected fuel to be %s but got %s", fuel_name, fuel->name());
  return key;
}
SpreadInfo::SpreadInfo(
  const int year,
  const int month,
  const int day,
  const int hour,
  const int minute,
  const double latitude,
  const double longitude,
  const ElevationSize elevation,
  const SlopeSize slope,
  const AspectSize aspect,
  const char* fuel_name,
  const wx::FwiWeather* weather)
  : SpreadInfo(util::to_tm(year, month, day, hour, minute),
               latitude,
               longitude,
               elevation,
               slope,
               aspect,
               fuel_name,
               weather)
{
}
SpreadInfo::SpreadInfo(
  const tm& start_date,
  const double latitude,
  const double longitude,
  const ElevationSize elevation,
  const SlopeSize slope,
  const AspectSize aspect,
  const char* fuel_name,
  const wx::FwiWeather* weather)
  : SpreadInfo(util::to_time(start_date),
               0.0,
               100.0,
               slope,
               aspect,
               fuel_name,
               calculate_nd_for_point(start_date.tm_yday, elevation, tbd::topo::Point(latitude, longitude)),
               weather)
{
}
SpreadInfo::SpreadInfo(const double time,
                       const double min_ros,
                       const double cell_size,
                       const SlopeSize slope,
                       const AspectSize aspect,
                       const char* fuel_name,
                       const int nd,
                       const wx::FwiWeather* weather)
  : SpreadInfo(time, min_ros, cell_size, make_key(slope, aspect, fuel_name), nd, weather, weather)
{
}
SpreadInfo::SpreadInfo(const double time,
                       const double min_ros,
                       const double cell_size,
                       const topo::SpreadKey& key,
                       const int nd,
                       const wx::FwiWeather* weather)
  : SpreadInfo(time, min_ros, cell_size, key, nd, weather, weather)
{
}
SpreadInfo::SpreadInfo(const double time,
                       const double min_ros,
                       const double cell_size,
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
  const auto _bui = bui().asDouble();
  const auto bui_eff = fuel->buiEffect(_bui);
  // FIX: gets calculated when not necessary sometimes
  const auto critical_surface_intensity = fuel->criticalSurfaceIntensity(*this);
  double ffmc_effect;
  double wsv;
  double rso;
  double raz;
  if (min_ros > SpreadInfo::initial(
        *this,
        *weather_daily,
        ffmc_effect,
        wsv,
        rso,
        raz,
        fuel,
        has_no_slope,
        heading_sin,
        heading_cos,
        bui_eff,
        min_ros,
        critical_surface_intensity)
      || sfc_ < COMPARE_LIMIT)
  {
    return;
  }
  // Now use hourly weather for actual spread calculations
  // don't check again if pointing at same weather
  if (weather != weather_daily)
  {
    if ((min_ros > SpreadInfo::initial(*this,
                                       *weather,
                                       ffmc_effect,
                                       wsv,
                                       rso,
                                       raz,
                                       fuel,
                                       has_no_slope,
                                       heading_sin,
                                       heading_cos,
                                       bui_eff,
                                       min_ros,
                                       critical_surface_intensity)
         || sfc_ < COMPARE_LIMIT))
    {
      // no spread with hourly weather
      // NOTE: only would happen if FFMC hourly is lower than FFMC daily?
      return;
    }
  }
  const auto back_isi = ffmc_effect * STANDARD_BACK_ISI_WSV(wsv);
  auto back_ros = fuel->calculateRos(nd,
                                     *weather,
                                     back_isi)
                * bui_eff;
  if (is_crown_)
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
  tfc_ = sfc_;
  // don't need to re-evaluate if crown with new head_ros_ because it would only go up if is_crown_
  if (fuel->canCrown() && is_crown_)
  {
    // wouldn't be crowning if ros is 0 so that's why this is in an else
    cfb_ = fuel->crownFractionBurned(head_ros_, rso);
    cfc_ = fuel->crownConsumption(cfb_);
    tfc_ += cfc_;
  }
  // max intensity should always be at the head
  max_intensity_ = fuel::fire_intensity(tfc_, ros);
  const auto a = (head_ros_ + back_ros) / 2.0;
  const auto c = a - back_ros;
  const auto l_b = fuel->lengthToBreadth(wsv);
  const auto flank_ros = a / l_b;
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
  // bool added = add_offset(raz, head_ros_);
  bool added = true;
#define STEP_X 0.2
#define STEP_MAX_DEGREES 5.0
#define STEP_MAX util::to_radians(STEP_MAX_DEGREES)
  double step_x = STEP_X;
  // double step_x = STEP_X / l_b;
  double theta = 0;
  double angle = 0;
  double last_theta = 0;
  double cur_x = 1.0;
  double last_angle = 0;
  // widest point should be at origin, which is 'c' away from origin
  double widest = atan2(flank_ros, c);
  printf("head_ros_ = %f, back_ros = %f, flank_ros = %f, c = %f, widest = %f\n",
         head_ros_,
         back_ros,
         flank_ros,
         c,
         util::to_degrees(widest));
  // double step = 1;
  // double last_step = 0;
  size_t num_angles = 0;
  double widest_x = cos(widest);
  double step_max = STEP_MAX / pow(l_b, 0.5);
  while (added && cur_x > (STEP_MAX / 4.0))
  {
    ++num_angles;
    theta = min(acos(cur_x), last_theta + step_max);
    angle = ellipse_angle(l_b, theta);
    added = add_offsets_calc_ros(angle);
    cur_x = cos(theta);
    printf("cur_x = %f, theta = %f, angle = %f, last_theta = %f, last_angle = %f\n",
           cur_x,
           util::to_degrees(theta),
           util::to_degrees(angle),
           util::to_degrees(last_theta),
           util::to_degrees(last_angle));
    last_theta = theta;
    last_angle = angle;
    if (theta > (STEP_MAX / 2.0))
    {
      step_max = STEP_MAX;
    }
    cur_x -= step_x;
    if (cur_x > widest_x && abs(cur_x - widest_x) < step_x)
    {
      cur_x = widest_x;
    }
  }
  if (added)
  {
    angle = ellipse_angle(l_b, (util::RAD_090 + theta) / 2.0);
    added = add_offsets_calc_ros(angle);
    // always just do one between the last angle and 90
    theta = util::RAD_090;
    ++num_angles;
    angle = ellipse_angle(l_b, theta);
    added = add_offsets(util::RAD_090, flank_ros * sqrt(a_sq_sub_c_sq) / a);
    cur_x = cos(theta);
    printf("cur_x = %f, theta = %f, angle = %f, last_theta = %f, last_angle = %f\n",
           cur_x,
           util::to_degrees(theta),
           util::to_degrees(angle),
           util::to_degrees(last_theta),
           util::to_degrees(last_angle));
    last_theta = theta;
    last_angle = angle;
  }
  // just because 5 seems good for the front and 10 for the back
  step_max = 2.0 * STEP_MAX;
  cur_x -= (step_x / 2.0);
  // trying to pick less rear points
  // step_x *= l_b;
  step_x *= l_b;
  // just trying random things now
  // double max_angle = util::RAD_180 - (pow(l_b, 1.5) * STEP_MAX);
  double max_angle = util::RAD_180 - (l_b * step_max);
  double min_x = cos(max_angle);
  while (added && cur_x >= min_x)
  {
    ++num_angles;
    theta = max(acos(cur_x), last_theta + step_max);
    angle = ellipse_angle(l_b, theta);
    if (angle > max_angle)
    {
      break;
      // // compromise and put a point in the middle
      // theta = (theta + last_theta) / 2.0;
      // angle = ellipse_angle(l_b, theta);
    }
    added = add_offsets_calc_ros(angle);
    cur_x = cos(theta);
    printf("cur_x = %f, theta = %f, angle = %f, last_theta = %f, last_angle = %f\n",
           cur_x,
           util::to_degrees(theta),
           util::to_degrees(angle),
           util::to_degrees(last_theta),
           util::to_degrees(last_angle));
    last_theta = theta;
    last_angle = angle;
    cur_x -= step_x;
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
// double SpreadInfo::calculateSpreadProbability(const double ros)
// {
//   // note: based off spread event probability from wotton
//   return 1 / (1 + exp(1.64 - 0.16 * ros));
// }
}
