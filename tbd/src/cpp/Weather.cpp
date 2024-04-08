/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

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
