/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "FireSpread.h"
#include "FuelType.h"
#include "FireWeather.h"
#include "Log.h"
namespace tbd::fuel
{
double InvalidFuel::buiEffect(double) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
double InvalidFuel::crownConsumption(double) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
double InvalidFuel::calculateRos(const int, const wx::FwiWeather&, double) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
double InvalidFuel::calculateIsf(const SpreadInfo&, double) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
double InvalidFuel::surfaceFuelConsumption(const SpreadInfo&) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
double InvalidFuel::lengthToBreadth(double) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
double InvalidFuel::finalRos(const SpreadInfo&, double, double, double) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
double InvalidFuel::criticalSurfaceIntensity(const SpreadInfo&) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
double InvalidFuel::crownFractionBurned(double, double) const noexcept
{
  return logging::fatal<double>("Invalid fuel type in fuel map");
}
double InvalidFuel::probabilityPeat(double) const noexcept
{
  return logging::fatal<double>("Invalid fuel type in fuel map");
}
double InvalidFuel::survivalProbability(const wx::FwiWeather&) const noexcept
{
  return logging::fatal<double>("Invalid fuel type in fuel map");
}
}
