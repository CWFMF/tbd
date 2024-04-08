/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "IntensityMap.h"
#include "Model.h"
#include "Perimeter.h"
#include "Weather.h"
namespace tbd::sim
{

// FIX: maybe this can be more generic but just want to keep allocated objects and reuse them
template <class K>
class GridMapCache
{
public:
  void release_map(unique_ptr<data::GridMap<K>> map) noexcept
  {
    map->clear();
    try
    {
      lock_guard<mutex> lock(mutex_);
      maps_.push_back(std::move(map));
    }
    catch (const std::exception& ex)
    {
      logging::fatal(ex);
      std::terminate();
    }
  }
  unique_ptr<data::GridMap<K>> acquire_map(const Model& model) noexcept
  {
    try
    {
      lock_guard<mutex> lock(mutex_);
      if (!maps_.empty())
      {
        auto result = std::move(maps_.at(maps_.size() - 1));
        maps_.pop_back();
        return result;
      }
      return model.environment().makeMap<K>(false);
    }
    catch (const std::exception& ex)
    {
      logging::fatal(ex);
      std::terminate();
    }
  }
protected:
  vector<unique_ptr<data::GridMap<K>>> maps_;
  mutex mutex_;
};

static GridMapCache<IntensitySize> CacheIntensitySize{};
static GridMapCache<double> CacheDouble{};
static GridMapCache<DegreesSize> CacheDegreesSize{};

// IntensityMap::IntensityMap(const Model& model, topo::Perimeter* perimeter) noexcept
//   : model_(model),
//     intensity_max_(acquire_map(model)),
//     is_burned_(model.getBurnedVector())
// {
//   if (nullptr != perimeter)
//   {
//     // logging::verbose("Converting perimeter to intensity");
//     // intensity_max_ = perimeter->burned_map();
//     // logging::verbose("Converting perimeter to is_burned");
//     // (*is_burned_) = perimeter->burned();
//     // // intensity_max_->set(location, intensity);
//     // // (*is_burned_).set(location.hash());
//     // applyPerimeter(*perimeter);
//     std::for_each(
//       std::execution::par_unseq,
//       perimeter->burned().begin(),
//       perimeter->burned().end(),
//       [this](const auto& location) {
//         auto intensity = 1;
//         //burn(location, intensity);
//         intensity_max_->set(location, intensity);
//         (*is_burned_).set(location.hash());
//       });
//   }
// }
IntensityMap::IntensityMap(const Model& model) noexcept
  : model_(model),
    intensity_max_(CacheIntensitySize.acquire_map(model)),
    rate_of_spread_at_max_(CacheDouble.acquire_map(model)),
    direction_of_spread_at_max_(CacheDegreesSize.acquire_map(model)),
    is_burned_(model.getBurnedVector())
{
}

IntensityMap::IntensityMap(const IntensityMap& rhs)
  // : IntensityMap(rhs.model_, nullptr)
  : IntensityMap(rhs.model_)
{
  *intensity_max_ = *rhs.intensity_max_;
  *rate_of_spread_at_max_ = *rhs.rate_of_spread_at_max_;
  *direction_of_spread_at_max_ = *rhs.direction_of_spread_at_max_;
  is_burned_ = rhs.is_burned_;
}

// IntensityMap::IntensityMap(IntensityMap&& rhs)
//   : IntensityMap(rhs.model_)
// {
//   *intensity_max_ = *rhs.intensity_max_;
//   is_burned_ = rhs.is_burned_;
// }

IntensityMap::~IntensityMap() noexcept
{
  model_.releaseBurnedVector(is_burned_);
  CacheIntensitySize.release_map(std::move(intensity_max_));
  CacheDouble.release_map(std::move(rate_of_spread_at_max_));
  CacheDegreesSize.release_map(std::move(direction_of_spread_at_max_));
}
void IntensityMap::applyPerimeter(const topo::Perimeter& perimeter) noexcept
{
  // logging::verbose("Attaining lock");
  // lock_guard<mutex> lock(mutex_);
  logging::verbose("Applying burned cells");
  std::for_each(
    std::execution::par_unseq,
    perimeter.burned().begin(),
    perimeter.burned().end(),
    [this](const auto& location) { ignite(location); });
}
// bool IntensityMap::canBurn(const HashSize hash) const
//{
//   return !hasBurned(hash);
// }
bool IntensityMap::canBurn(const topo::Cell& location) const
{
  return !hasBurned(location);
}
bool IntensityMap::hasBurned(const Location& location) const
{
  lock_guard<mutex> lock(mutex_);
  return (*is_burned_)[location.hash()];
  //  return hasBurned(location.hash());
}
// bool IntensityMap::hasBurned(const HashSize hash) const
//{
//   lock_guard<mutex> lock(mutex_);
//   return (*is_burned_)[hash];
// }
bool IntensityMap::isSurrounded(const Location& location) const
{
  // implement here so we can just lock once
  lock_guard<mutex> lock(mutex_);
  const auto x = location.column();
  const auto y = location.row();
  const auto min_row = static_cast<Idx>(max(y - 1, 0));
  const auto max_row = min(y + 1, this->rows() - 1);
  const auto min_column = static_cast<Idx>(max(x - 1, 0));
  const auto max_column = min(x + 1, this->columns() - 1);
  for (auto r = min_row; r <= max_row; ++r)
  {
    //    auto h = static_cast<size_t>(r) * MAX_COLUMNS + min_column;
    for (auto c = min_column; c <= max_column; ++c)
    {
      // actually check x, y too since we care if the cell itself is burned
      //      if (!(*is_burned_)[h])
      if (!(*is_burned_)[Location(r, c).hash()])
      {
        return false;
      }
      //      ++h;
    }
  }
  return true;
}
void IntensityMap::burn(const Location& location)
{
  lock_guard<mutex> lock(mutex_);
  intensity_max_->set(location, 1);
  (*is_burned_).set(location.hash());
}
void IntensityMap::ignite(const Location& location)
{
  update(location, 1, 0, tbd::wx::Direction::Zero);
}
void IntensityMap::update(const Location& location,
                          IntensitySize intensity,
                          double ros,
                          tbd::wx::Direction raz)
// void IntensityMap::update(const Location& location,
//                           const SpreadInfo& spread_info)
{
  lock_guard<mutex> lock(mutex_);
  // (*is_burned_).set(location.hash());
  const auto intensity_cur = intensity_max_->at(location);
  if (intensity_cur < intensity)
  {
    intensity_max_->set(location, intensity);
  }
  const auto ros_cur = rate_of_spread_at_max_->at(location);
  // update ros and direction if higher ros
  if (ros_cur < ros)
  {
    rate_of_spread_at_max_->set(location, ros);
    direction_of_spread_at_max_->set(location, static_cast<DegreesSize>(raz.asDegrees()));
  }
}
void IntensityMap::save(const string& dir, const string& base_name) const
{
  lock_guard<mutex> lock(mutex_);
  const auto name_ros = base_name + "_ros";
  const auto name_raz = base_name + "_raz";
  // static std::function<DegreesSize(tbd::wx::Direction)> fct_raz = [](tbd::wx::Direction raz) {
  //   return static_cast<DegreesSize>(raz.asDegrees());
  // };
  if (Settings::saveAsAscii())
  {
    intensity_max_->saveToAsciiFile(dir, base_name);
    rate_of_spread_at_max_->saveToAsciiFile(dir, name_ros);
    direction_of_spread_at_max_->saveToAsciiFile(dir, name_raz);
  }
  else
  {
    intensity_max_->saveToTiffFile(dir, base_name);
    rate_of_spread_at_max_->saveToTiffFile(dir, name_ros);
    direction_of_spread_at_max_->saveToTiffFile(dir, name_raz);
  }
}
double IntensityMap::fireSize() const
{
  lock_guard<mutex> lock(mutex_);
  return intensity_max_->fireSize();
}
map<Location, IntensitySize>::const_iterator
  IntensityMap::cend() const noexcept
{
  return intensity_max_->data.cend();
}
map<Location, IntensitySize>::const_iterator
  IntensityMap::cbegin() const noexcept
{
  return intensity_max_->data.cbegin();
}
}
