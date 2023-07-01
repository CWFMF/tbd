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

#include "stdafx.h"
#include "Weather.h"
#include "Log.h"
namespace tbd::wx
{
const Temperature Temperature::Zero = Temperature(0);
const RelativeHumidity RelativeHumidity::Zero = RelativeHumidity(0);
const Direction Direction::Zero = Direction(0, false);
const Speed Speed::Zero = Speed(0);
const Wind Wind::Zero = Wind(Direction(0, false), Speed(0));
const Precipitation Precipitation::Zero =
  Precipitation(0);
}