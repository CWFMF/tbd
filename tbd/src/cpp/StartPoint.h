// Copyright (c) 2020-2021, Queen's Printer for Ontario.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once
#include <tuple>
#include "Point.h"
namespace tbd
{
namespace topo
{
/**
 * \brief A Point that has sunrise and sunset times for each day.
 */
class StartPoint : public Point
{
  /**
   * \brief Array of tuple for sunrise/sunset times by day
   */
  array<tuple<double, double>, MAX_DAYS> days_;
public:
  /**
   * \brief Constructor
   * \param latitude Latitude (decimal degrees)
   * \param longitude Longitude (decimal degrees)
   */
  StartPoint(double latitude, double longitude) noexcept;
  ~StartPoint() noexcept = default;
  /**
   * \brief Copy constructor
   * \param rhs StartPoint to copy from
   */
  StartPoint(const StartPoint& rhs) noexcept = default;
  /**
   * \brief Move constructor
   * \param rhs StartPoint to move from
   */
  StartPoint(StartPoint&& rhs) noexcept = default;
  /**
   * \brief Copy assignment
   * \param rhs StartPoint to copy from
   * \return This, after assignment
   */
  StartPoint& operator=(const StartPoint& rhs) = default;
  /**
   * \brief Move assignment
   * \param rhs StartPoint to move from
   * \return This, after assignment
   */
  StartPoint& operator=(StartPoint&& rhs) noexcept;
  /**
   * \brief Sunrise time
   * \param day Day
   * \return Sunrise time on give day
   */
  [[nodiscard]] constexpr double dayStart(const size_t day) const
  {
    return get<0>(days_.at(day));
  }
  /**
   * \brief Sunset time
   * \param day Day
   * \return Sunset time on give day
   */
  [[nodiscard]] constexpr double dayEnd(const size_t day) const
  {
    return get<1>(days_.at(day));
  }
};
}
}