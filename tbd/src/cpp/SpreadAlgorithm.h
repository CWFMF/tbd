/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"

namespace tbd
{
using HorizontalAdjustment = std::function<double(double)>;

HorizontalAdjustment horizontal_adjustment(
  const AspectSize slope_azimuth,
  const SlopeSize slope);

class SpreadAlgorithm
{
public:
  [[nodiscard]] virtual OffsetSet calculate_offsets(
    HorizontalAdjustment correction_factor,
    double head_raz,
    double head_ros,
    double back_ros,
    double length_to_breadth) const
    noexcept = 0;
};

class BaseSpreadAlgorithm
  : public SpreadAlgorithm
{
public:
  BaseSpreadAlgorithm(const double max_angle,
                      const double cell_size,
                      const double min_ros)
    : max_angle_(max_angle), cell_size_(cell_size), min_ros_(min_ros)
  {
  }
protected:
  double max_angle_;
  double cell_size_;
  double min_ros_;
};

class OriginalSpreadAlgorithm
  : public BaseSpreadAlgorithm
{
public:
  using BaseSpreadAlgorithm::BaseSpreadAlgorithm;
  [[nodiscard]] OffsetSet calculate_offsets(
    HorizontalAdjustment correction_factor,
    double head_raz,
    double head_ros,
    double back_ros,
    double length_to_breadth) const noexcept override;
};

class WidestEllipseAlgorithm
  : public BaseSpreadAlgorithm
{
public:
  using BaseSpreadAlgorithm::BaseSpreadAlgorithm;
  [[nodiscard]] OffsetSet calculate_offsets(
    HorizontalAdjustment correction_factor,
    double head_raz,
    double head_ros,
    double back_ros,
    double length_to_breadth) const noexcept override;
};
}
