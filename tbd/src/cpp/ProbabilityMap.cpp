/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "ProbabilityMap.h"
#include "FBP45.h"
#include "IntensityMap.h"
#include "Model.h"
namespace tbd::sim
{
ProbabilityMap::ProbabilityMap(const double time,
                               const double start_time,
                               const int min_value,
                               const int low_max,
                               const int med_max,
                               const int max_value,
                               const data::GridBase& grid_info)
  : all_(data::GridMap<size_t>(grid_info, 0)),
    high_(data::GridMap<size_t>(grid_info, 0)),
    med_(data::GridMap<size_t>(grid_info, 0)),
    low_(data::GridMap<size_t>(grid_info, 0)),
    time_(time),
    start_time_(start_time),
    min_value_(min_value),
    max_value_(max_value),
    low_max_(low_max),
    med_max_(med_max),
    perimeter_(nullptr)
{
}
ProbabilityMap* ProbabilityMap::copyEmpty() const
{
  return new ProbabilityMap(time_,
                            start_time_,
                            min_value_,
                            low_max_,
                            med_max_,
                            max_value_,
                            all_);
}
void ProbabilityMap::setPerimeter(const topo::Perimeter* const perimeter)
{
  perimeter_ = perimeter;
}
void ProbabilityMap::addProbabilities(const ProbabilityMap& rhs)
{
#ifndef DEBUG_PROBAILITY
  logging::check_fatal(rhs.time_ != time_, "Wrong time");
  logging::check_fatal(rhs.start_time_ != start_time_, "Wrong start time");
  logging::check_fatal(rhs.min_value_ != min_value_, "Wrong min value");
  logging::check_fatal(rhs.max_value_ != max_value_, "Wrong max value");
  logging::check_fatal(rhs.low_max_ != low_max_, "Wrong low max value");
  logging::check_fatal(rhs.med_max_ != med_max_, "Wrong med max value");
#endif
  lock_guard<mutex> lock(mutex_);
  if (Settings::saveIntensity())
  {
    for (auto&& kv : rhs.low_.data)
    {
      low_.data[kv.first] += kv.second;
    }
    for (auto&& kv : rhs.med_.data)
    {
      med_.data[kv.first] += kv.second;
    }
    for (auto&& kv : rhs.high_.data)
    {
      high_.data[kv.first] += kv.second;
    }
  }
  for (auto&& kv : rhs.all_.data)
  {
    all_.data[kv.first] += kv.second;
  }
  for (auto size : rhs.sizes_)
  {
    static_cast<void>(util::insert_sorted(&sizes_, size));
  }
}
void ProbabilityMap::addProbability(const IntensityMap& for_time)
{
  lock_guard<mutex> lock(mutex_);
  std::for_each(
    for_time.cbegin(),
    for_time.cend(),
    [this](auto&& kv) {
      const auto k = kv.first;
      const auto v = kv.second;
      all_.data[k] += 1;
      if (Settings::saveIntensity())
      {
        if (v >= min_value_ && v <= low_max_)
        {
          low_.data[k] += 1;
        }
        else if (v > low_max_ && v <= med_max_)
        {
          med_.data[k] += 1;
        }
        else if (v > med_max_ && v <= max_value_)
        {
          high_.data[k] += 1;
        }
        else
        {
          logging::fatal("Value %d doesn't fit into any range", v);
        }
      }
    });
  const auto size = for_time.fireSize();
  static_cast<void>(util::insert_sorted(&sizes_, size));
}
vector<double> ProbabilityMap::getSizes() const
{
  return sizes_;
}
util::Statistics ProbabilityMap::getStatistics() const
{
  return util::Statistics{getSizes()};
}
size_t ProbabilityMap::numSizes() const noexcept
{
  return sizes_.size();
}
void ProbabilityMap::show() const
{
  // even if we only ran the actuals we'll still have multiple scenarios
  // with different randomThreshold values
  const auto day = static_cast<int>(time_ - floor(start_time_));
  const auto s = getStatistics();
  logging::note(
    "Fire size at end of day %d: %0.1f ha - %0.1f ha (mean %0.1f ha, median %0.1f ha)",
    day,
    s.min(),
    s.max(),
    s.mean(),
    s.median());
}
void ProbabilityMap::saveSizes(const string& base_name) const
{
  ofstream out;
  out.open(Settings::outputDirectory() + base_name + ".csv");
  auto sizes = getSizes();
  if (!sizes.empty())
  {
    // don't want to modify original array so that we can still lookup in correct order
    sort(sizes.begin(), sizes.end());
  }
  for (const auto& s : sizes)
  {
    out << s << "\n";
  }
  out.close();
}
string make_string(const char* name, const tm& t, const int day)
{
  constexpr auto mask = "%s_%03d_%04d-%02d-%02d";
  static constexpr size_t OutLength = 100;
  char tmp[OutLength];
  sprintf(tmp,
          mask,
          name,
          day,
          t.tm_year + 1900,
          t.tm_mon + 1,
          t.tm_mday);
  return string(tmp);
};

void ProbabilityMap::saveAll(const Model& model,
                             const tm& start_time,
                             const double time,
                             const double start_day,
                             const bool is_interim) const
{
  lock_guard<mutex> lock(mutex_);
  auto t = start_time;
  auto ticks = mktime(&t);
  const auto day = static_cast<int>(round(time));
  ticks += (static_cast<size_t>(day) - t.tm_yday - 1) * DAY_SECONDS;
  t = *localtime(&ticks);
  auto fix_string = [&t, &day, &is_interim](string prefix) {
    auto text = (is_interim ? "interim_" : "") + prefix;
    return make_string(text.c_str(), t, day);
  };
  if (sim::Settings::runAsync())
  {
    vector<std::future<void>> results{};
    if (Settings::saveProbability())
    {
      results.push_back(async(launch::async,
                              &ProbabilityMap::saveTotal,
                              this,
                              fix_string("probability")));
    }
    if (Settings::saveOccurrence())
    {
      results.push_back(async(launch::async,
                              &ProbabilityMap::saveTotalCount,
                              this,
                              fix_string("occurrence")));
    }
    if (Settings::saveIntensity())
    {
      results.push_back(async(launch::async,
                              &ProbabilityMap::saveLow,
                              this,
                              fix_string("intensity_L")));
      results.push_back(async(launch::async,
                              &ProbabilityMap::saveModerate,
                              this,
                              fix_string("intensity_M")));
      results.push_back(async(launch::async,
                              &ProbabilityMap::saveHigh,
                              this,
                              fix_string("intensity_H")));
    }
    results.push_back(async(launch::async,
                            &ProbabilityMap::saveSizes,
                            this,
                            fix_string("sizes")));
    for (auto& result : results)
    {
      result.wait();
    }
  }
  else
  {
    if (Settings::saveProbability())
    {
      saveTotal(fix_string("probability"));
    }
    if (Settings::saveOccurrence())
    {
      saveTotalCount(fix_string("occurrence"));
    }
    if (Settings::saveIntensity())
    {
      saveLow(fix_string("intensity_L"));
      saveModerate(fix_string("intensity_M"));
      saveHigh(fix_string("intensity_H"));
    }
    saveSizes(fix_string("sizes"));
  }
  const auto nd = model.nd(day);
  logging::note("Fuels for day %d are %s green-up and grass has %d%% curing",
                day - static_cast<int>(start_day),
                fuel::calculate_is_green(nd) ? "after" : "before",
                fuel::calculate_grass_curing(nd));
}
void ProbabilityMap::saveTotal(const string& base_name) const
{
  auto with_perim = all_;
  if (nullptr != perimeter_)
  {
    for (auto loc : perimeter_->burned())
    {
      // make initial perimeter cells 2* so that probability ends up as 2
      with_perim.data[loc] *= 2;
    }
  }
  with_perim.saveToProbabilityFile<float>(Settings::outputDirectory(), base_name, static_cast<float>(numSizes()));
}
void ProbabilityMap::saveTotalCount(const string& base_name) const
{
  all_.saveToProbabilityFile<uint32_t>(Settings::outputDirectory(), base_name, 1);
}
void ProbabilityMap::saveHigh(const string& base_name) const
{
  high_.saveToProbabilityFile<float>(Settings::outputDirectory(), base_name, static_cast<float>(numSizes()));
}
void ProbabilityMap::saveModerate(const string& base_name) const
{
  med_.saveToProbabilityFile<float>(Settings::outputDirectory(), base_name, static_cast<float>(numSizes()));
}
void ProbabilityMap::saveLow(const string& base_name) const
{
  low_.saveToProbabilityFile<float>(Settings::outputDirectory(), base_name, static_cast<float>(numSizes()));
}
void ProbabilityMap::reset()
{
  all_.clear();
  low_.clear();
  med_.clear();
  high_.clear();
  sizes_.clear();
}
}
