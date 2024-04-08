/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Observer.h"
namespace tbd::sim
{
string IObserver::makeName(const string& base_name, const string& suffix)
{
  if (base_name.length() > 0)
  {
    return base_name + "_" + suffix;
  }
  return suffix;
}
ArrivalObserver::ArrivalObserver(const Scenario& scenario)
  : MapObserver<double>(scenario, 0.0, "arrival")
{
}
double ArrivalObserver::getValue(const Event& event) const noexcept
{
  return event.time();
}
SourceObserver::SourceObserver(const Scenario& scenario)
  : MapObserver<CellIndex>(scenario, static_cast<CellIndex>(255), "source")
{
}
CellIndex SourceObserver::getValue(const Event& event) const noexcept
{
  return event.source();
}
IntensityObserver::IntensityObserver(const Scenario& scenario, string suffix) noexcept
  : scenario_(scenario), suffix_(std::move(suffix))
{
}
void IntensityObserver::handleEvent(const Event&) noexcept
{
  // HACK: do nothing because Scenario tracks intensity
}
void IntensityObserver::save(const string& dir, const string& base_name) const
{
  scenario_.saveIntensity(dir, makeName(base_name, suffix_));
}
void IntensityObserver::reset() noexcept
{
  // HACK: do nothing because Scenario tracks intensity
}
}
