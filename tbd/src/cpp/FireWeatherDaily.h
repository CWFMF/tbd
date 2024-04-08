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
#include <map>
#include <set>
#include <vector>
#include "FuelLookup.h"
#include "FWI.h"
#include "FireWeather.h"
namespace tbd::wx
{
/**
 * \brief A stream of weather that gets used by a Scenario every Iteration.
 */
class FireWeatherDaily
  : public FireWeather
{
public:
  /**
   * \brief Destructor
   */
  virtual ~FireWeatherDaily() = default;
  /**
   * \brief Constructor
   * \param used_fuels set of FuelTypes that are used in the simulation
   * \param data map of Day to FwiWeather to use for weather stream
   */
  FireWeatherDaily(const set<const fuel::FuelType*>& used_fuels,
                   const map<Day, FwiWeather>& data);
  /**
   * \brief Move constructor
   * \param rhs FireWeatherDaily to move from
   */
  FireWeatherDaily(FireWeatherDaily&& rhs) = default;
  FireWeatherDaily(const FireWeatherDaily& rhs) = delete;
  /**
   * \brief Move assignment
   * \param rhs FireWeatherDaily to move from
   * \return This, after assignment
   */
  FireWeatherDaily& operator=(FireWeatherDaily&& rhs) noexcept = default;
  FireWeatherDaily& operator=(const FireWeatherDaily& rhs) = delete;
};
}
