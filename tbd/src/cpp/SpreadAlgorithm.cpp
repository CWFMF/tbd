/* Copyright (c) 2020,  Queen's Printer for Ontario */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "SpreadAlgorithm.h"
#include "Util.h"
#include "unstable.h"
#include "CellPoints.h"
namespace tbd
{
using util::to_degrees;
HorizontalAdjustment horizontal_adjustment(
  const AspectSize slope_azimuth,
  const SlopeSize slope)
{
  // do everything we can to avoid calling trig functions unnecessarily
  constexpr auto no_correction = [](const MathSize) noexcept { return 1.0; };
  if (0 == slope)
  {
    // do check once and make function just return 1.0 if no slope
    return no_correction;
  }
  const auto b_semi = _cos(atan(slope / 100.0));
  const auto slope_radians = util::to_radians(slope_azimuth);
  const auto do_correction = [b_semi, slope_radians](const MathSize theta) noexcept {
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
  return do_correction;
}
[[nodiscard]] OffsetSet OriginalSpreadAlgorithm::calculate_offsets(
  HorizontalAdjustment correction_factor,
  MathSize tfc,
  MathSize head_raz,
  MathSize head_ros,
  MathSize back_ros,
  MathSize length_to_breadth) const noexcept
{
  OffsetSet offsets{};
  auto prev_min = std::numeric_limits<MathSize>::max();
  auto prev_angle = std::numeric_limits<MathSize>::max();
  auto have_prev = false;
  auto odd = true;
  const auto add_offset =
    [this, &offsets, tfc, &prev_min, &prev_angle, &have_prev, &odd](
      const MathSize direction,
      const MathSize ros) {
      if (odd)
      {
        if (have_prev)
        {
          // can't use exact > because math and rounding may break things
          // logging::check_fatal(
          //   ((abs(ros - prev_min) > __FLT_EPSILON__) && ros > prev_min),
          //   "Fire gets faster towards rear at %f with ros of %f",
          //   to_degrees(direction),
          //   ros);
          if (
            ((abs(ros - prev_min) > __FLT_EPSILON__) && ros > prev_min))
          {
            logging::error(
              "Fire speed increase from %f to %f between angle %f and %f",
              prev_min,
              ros,
              to_degrees(prev_angle),
              to_degrees(direction));
          }
          else
          {
            logging::note(
              "ROS decrease from %f to %f between angle %f and %f",
              ((ros < prev_min) ? "decrease" : "consistent"),
              prev_min,
              ros,
              to_degrees(prev_angle),
              to_degrees(direction));
          }
        }
        have_prev = true;
        prev_min = ros;
        prev_angle = direction;
      }
      odd = !odd;
      // if (ros < min_ros_)
      // {
      //   // return false;
      //   // NOTE: always return true so we can ensure we go around ellipse entirely for testing
      //   return true;
      // }
      const auto ros_cell = ros / cell_size_;
      const auto intensity = fuel::fire_intensity(tfc, ros);
      // spreading, so figure out offset from current point
      offsets.emplace_back(
        intensity,
        ros,
        Direction(direction, true),
        Offset{
          static_cast<DistanceSize>(ros_cell * _sin(direction)),
          static_cast<DistanceSize>(ros_cell * _cos(direction))});
      return true;
    };
  // if not over spread threshold then don't spread
  // HACK: set ros in boolean if we get that far so that we don't have to repeat the if body
  if (!add_offset(head_raz, head_ros * correction_factor(head_raz)))
  {
    return offsets;
  }
  const auto a = (head_ros + back_ros) / 2.0;
  const auto c = a - back_ros;
  const auto flank_ros = a / length_to_breadth;
  const auto a_sq = a * a;
  const auto flank_ros_sq = flank_ros * flank_ros;
  const auto a_sq_sub_c_sq = a_sq - (c * c);
  const auto ac = a * c;
  logging::note(
    "Calculating offsets for HROS %f, BROS %f, L:B %f, FROS %f",
    head_ros,
    back_ros,
    length_to_breadth,
    flank_ros);
  const auto calculate_ros =
    [a, c, ac, flank_ros, a_sq, flank_ros_sq, a_sq_sub_c_sq](const MathSize theta) noexcept {
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
    [this, &correction_factor, &add_offset, head_raz](
      const MathSize angle_radians,
      const MathSize ros_flat) {
      if (ros_flat < min_ros_)
      {
        return false;
      }
      auto direction = util::fix_radians(angle_radians + head_raz);
      // spread is symmetrical across the center axis, but needs to be adjusted if on a slope
      // intentionally don't use || because we want both of these to happen all the time
      auto added = add_offset(direction, ros_flat * correction_factor(direction));
      direction = util::fix_radians(head_raz - angle_radians);
      added |= add_offset(direction, ros_flat * correction_factor(direction));
      return added;
    };
  const auto add_offsets_calc_ros =
    [&add_offsets, &calculate_ros](const MathSize angle_radians) { return add_offsets(angle_radians, calculate_ros(angle_radians)); };
  // bool added = add_offset(head_raz, head_ros);
  bool added = add_offset(head_raz, head_ros);
  MathSize i = max_angle_;
  while (added && i < 90)
  {
    added = add_offsets_calc_ros(util::to_radians(i));
    i += max_angle_;
  }
  if (added)
  {
    added = add_offsets_calc_ros(util::to_radians(90 - __DBL_EPSILON__));
    // NOTE: this must be simplified from the original formula
    added = add_offsets(util::to_radians(90), flank_ros * sqrt(a_sq_sub_c_sq) / a);
    added = add_offsets_calc_ros(util::to_radians(90 + __DBL_EPSILON__));
    i = 90 + max_angle_;
    while (added && i < 180)
    {
      added = add_offsets_calc_ros(util::to_radians(i));
      i += max_angle_;
    }
    if (added)
    {
      // only use back ros if every other angle is spreading since this should be lowest
      //  180
      if (back_ros >= min_ros_)
      {
        const auto direction = util::fix_radians(util::RAD_180 + head_raz);
        static_cast<void>(!add_offset(direction, back_ros * correction_factor(direction)));
      }
    }
  }
  return offsets;
}
[[nodiscard]] OffsetSet
  WidestEllipseAlgorithm::calculate_offsets(
    const HorizontalAdjustment correction_factor,
    const MathSize tfc,
    const MathSize head_raz,
    const MathSize head_ros,
    const MathSize back_ros,
    const MathSize length_to_breadth) const noexcept
{
  OffsetSet offsets{};
  const auto add_offset =
    [this, &offsets, tfc](
      const MathSize direction,
      const MathSize ros) {
#ifdef DEBUG_POINTS
      const auto s0 = offsets.size();
#endif
      if (ros < min_ros_)
      {
        // might not be correct depending on slope angle correction
        // #ifdef DEBUG_POINTS
        //       // should never be empty since head_ros must have been high enough
        //       logging::check_fatal(offsets.empty(), "offsets.empty()");
        // #endif
        return false;
      }
      const auto ros_cell = ros / cell_size_;
      const auto intensity = fuel::fire_intensity(tfc, ros);
      // spreading, so figure out offset from current point
      offsets.emplace_back(
        intensity,
        ros,
        Direction(direction, true),
        Offset{
          static_cast<DistanceSize>(ros_cell * _sin(direction)),
          static_cast<DistanceSize>(ros_cell * _cos(direction))});
    // // HACK: avoid bounds check
    // offsets.emplace_back(ros_cell * _sin(direction), ros_cell * _cos(direction), false);
#ifdef DEBUG_POINTS
      const auto s1 = offsets.size();
      logging::check_equal(s0 + 1, s1, "offsets.size()");
      logging::check_fatal(offsets.empty(), "offsets.empty()");
#endif
      return true;
    };
  // if not over spread threshold then don't spread
  // HACK: set ros in boolean if we get that far so that we don't have to repeat the if body
  if (!add_offset(head_raz, head_ros * correction_factor(head_raz)))
  {
    // might not be correct depending on slope angle correction
    // #ifdef DEBUG_POINTS
    //     // if (head_ros >= min_ros_)
    //     {
    //       logging::check_fatal(
    //         offsets.empty(),
    //         "Empty when ros of %f >= %f",
    //         head_ros,
    //         min_ros_);
    //     }
    // #endif
    return offsets;
  }
#ifdef DEBUG_POINTS
  logging::check_fatal(offsets.empty(), "offsets.empty()");
#endif
  const auto a = (head_ros + back_ros) / 2.0;
  const auto c = a - back_ros;
  const auto flank_ros = a / length_to_breadth;
  const auto a_sq = a * a;
  const auto flank_ros_sq = flank_ros * flank_ros;
  const auto a_sq_sub_c_sq = a_sq - (c * c);
  const auto ac = a * c;
  const auto calculate_ros =
    [a, c, ac, flank_ros, a_sq, flank_ros_sq, a_sq_sub_c_sq](const MathSize theta) noexcept {
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
    [this, &correction_factor, &add_offset, head_raz](
      const MathSize angle_radians,
      const MathSize ros_flat) {
      if (ros_flat < min_ros_)
      {
        return false;
      }
      auto direction = util::fix_radians(angle_radians + head_raz);
      // spread is symmetrical across the center axis, but needs to be adjusted if on a slope
      // intentionally don't use || because we want both of these to happen all the time
      auto added = add_offset(direction, ros_flat * correction_factor(direction));
      direction = util::fix_radians(head_raz - angle_radians);
      added |= add_offset(direction, ros_flat * correction_factor(direction));
      return added;
    };
  const auto add_offsets_calc_ros =
    [&add_offsets, &calculate_ros](const MathSize angle_radians) { return add_offsets(angle_radians, calculate_ros(angle_radians)); };
  // bool added = add_offset(head_raz, head_ros);
  bool added = true;
#define STEP_X 0.2
#define STEP_MAX util::to_radians(max_angle_)
  // MathSize step_x = STEP_X;
  // MathSize step_x = STEP_X / length_to_breadth;
  MathSize step_x = STEP_X / pow(length_to_breadth, 0.5);
  MathSize theta = 0;
  MathSize angle = 0;
  MathSize last_theta = 0;
  MathSize cur_x = 1.0;
  // MathSize last_angle = 0;
  // widest point should be at origin, which is 'c' away from origin
  MathSize widest = atan2(flank_ros, c);
  // printf("head_ros = %f, back_ros = %f, flank_ros = %f, c = %f, widest = %f\n",
  //        head_ros,
  //        back_ros,
  //        flank_ros,
  //        c,
  //        util::to_degrees(widest));
  // MathSize step = 1;
  // MathSize last_step = 0;
  size_t num_angles = 0;
  MathSize widest_x = _cos(widest);
  MathSize step_max = STEP_MAX / pow(length_to_breadth, 0.5);
  while (added && cur_x > (STEP_MAX / 4.0))
  {
    ++num_angles;
    theta = min(acos(cur_x), last_theta + step_max);
    angle = ellipse_angle(length_to_breadth, theta);
    added = add_offsets_calc_ros(angle);
    cur_x = _cos(theta);
    // printf("cur_x = %f, theta = %f, angle = %f, last_theta = %f, last_angle = %f\n",
    //        cur_x,
    //        util::to_degrees(theta),
    //        util::to_degrees(angle),
    //        util::to_degrees(last_theta),
    //        util::to_degrees(last_angle));
    last_theta = theta;
    // last_angle = angle;
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
    angle = ellipse_angle(length_to_breadth, (util::RAD_090 + theta) / 2.0);
    added = add_offsets_calc_ros(angle);
    // always just do one between the last angle and 90
    theta = util::RAD_090;
    ++num_angles;
    angle = ellipse_angle(length_to_breadth, theta);
    added = add_offsets(util::RAD_090, flank_ros * sqrt(a_sq_sub_c_sq) / a);
    cur_x = _cos(theta);
    // printf("cur_x = %f, theta = %f, angle = %f, last_theta = %f, last_angle = %f\n",
    //        cur_x,
    //        util::to_degrees(theta),
    //        util::to_degrees(angle),
    //        util::to_degrees(last_theta),
    //        util::to_degrees(last_angle));
    last_theta = theta;
    // last_angle = angle;
  }
  // just because 5 seems good for the front and 10 for the back
  // step_max = 2.0 * STEP_MAX;
  cur_x -= (step_x / 2.0);
  // trying to pick less rear points
  // step_x *= length_to_breadth;
  step_x *= length_to_breadth;
  // just trying random things now
  // MathSize max_angle = util::RAD_180 - (pow(length_to_breadth, 1.5) * STEP_MAX);
  MathSize max_angle = util::RAD_180 - (length_to_breadth * step_max);
  MathSize min_x = _cos(max_angle);
  while (added && cur_x >= min_x)
  {
    ++num_angles;
    theta = max(acos(cur_x), last_theta + step_max);
    angle = ellipse_angle(length_to_breadth, theta);
    if (angle > max_angle)
    {
      break;
      // // compromise and put a point in the middle
      // theta = (theta + last_theta) / 2.0;
      // angle = ellipse_angle(length_to_breadth, theta);
    }
    added = add_offsets_calc_ros(angle);
    cur_x = _cos(theta);
    // printf("cur_x = %f, theta = %f, angle = %f, last_theta = %f, last_angle = %f\n",
    //        cur_x,
    //        util::to_degrees(theta),
    //        util::to_degrees(angle),
    //        util::to_degrees(last_theta),
    //        util::to_degrees(last_angle));
    last_theta = theta;
    // last_angle = angle;
    cur_x -= step_x;
  }
  if (added)
  {
    // only use back ros if every other angle is spreading since this should be lowest
    //  180
    if (back_ros >= min_ros_)
    {
      const auto direction = util::fix_radians(util::RAD_180 + head_raz);
      static_cast<void>(!add_offset(direction, back_ros * correction_factor(direction)));
    }
  }
#ifdef DEBUG_POINTS
  if (head_ros >= min_ros_)
  {
    logging::check_fatal(
      offsets.empty(),
      "Empty when ros of %f >= %f",
      head_ros,
      min_ros_);
  }
#endif
  return offsets;
}
}
