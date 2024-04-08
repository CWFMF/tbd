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
#include "debug_settings.h"

// #define VLD_FORCE_ENABLE
// #include "vld.h"
#define _USE_MATH_DEFINES
#define NOMINMAX
#include <ctime>
#include <chrono>
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <cstdint>
#include <string>
#include <cstring>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <geotiff/geo_normalize.h>
#include <geotiff/geotiff.h>
#include <geotiff/geovalues.h>
#include "tiff.h"
#include <tiffio.h>
#include <geotiff/xtiffio.h>
#include <sys/stat.h>
// unreferenced inline function has been removed
// Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
// function 'X' selected for automatic inline expansion
// function not inlined
// selected for automatic inline expansion
// Do not assign the result of an allocation or a function call with an owner<T> return value to a raw pointer, use owner<T> instead
// Do not delete a raw pointer that is not an owner<T>
// Return a scoped object instead of a heap-allocated if it has a move constructor
// Reset or explicitly delete an owner<T> pointer
// Do not assign to an owner<T> which may be in valid state
// Do not assign a raw pointer to an owner<T>
// Prefer scoped objects, don't heap-allocate unnecessarily
// Avoid calling new and delete explicitly, use std::make_unique<T> instead
// Global initializer calls a non-constexpr function
// Symbol is never tested for nullness, it can be marked as not_null
// Function hides a non-virtual function
// prefer to use gsl::at()
// Don't use a static_cast for arithmetic conversions. Use brace initialization, gsl::narrow_cast or gsl::narrow
// Don't use pointer arithmetic. Use span instead
// Only index into arrays using constant expressions
// No array to pointer decay
using std::abs;
using std::array;
using std::async;
using std::atomic;
using std::endl;
using std::fixed;
using std::function;
using std::future;
using std::get;
using std::getline;
using std::hash;
using std::ifstream;
using std::istringstream;
using std::launch;
using std::list;
using std::lock_guard;
using std::make_shared;
using std::make_tuple;
using std::make_unique;
using std::map;
using std::max;
using std::min;
using std::mt19937;
using std::mutex;
using std::numeric_limits;
using std::ofstream;
using std::ostream;
using std::ostringstream;
using std::pair;
using std::put_time;
using std::runtime_error;
using std::set;
using std::setprecision;
using std::shared_ptr;
using std::stod;
using std::stoi;
using std::stol;
using std::string;
using std::string_view;
using std::stringstream;
using std::to_string;
using std::to_wstring;
using std::tuple;
using std::uniform_real_distribution;
using std::unique_ptr;
using std::unordered_map;
using std::vector;
using std::wstring;
namespace tbd
{
/**
 * \brief Size of the hash of a Cell
 */
using HashSize = uint32_t;
/**
 * \brief Size of the index for a Cell
 */
using CellIndex = uint8_t;
/**
 * \brief A row or column index for a grid
 */
using Idx = int16_t;
/**
 * \brief A row or column index for a grid not in memory yet
 */
using FullIdx = int64_t;
/**
 * \brief Type used for perimeter raster values (uses [0, 1])
 */
// FIX: could try to get really fancy and use number of bits per pixel options
using PerimSize = uint8_t;
// using PerimSize = uint16_t;
/**
 * \brief Type used for fuel values (uses [0 - 999]?)
 */
// FIX: seriously does not like uint for some reason
using FuelSize = uint16_t;
// using FuelSize = int16_t;
/**
 * \brief Type used for aspect values (uses [0 - 359])
 */
using AspectSize = uint16_t;
/**
 * \brief Type used for elevation values (uses [? - 9800?])
 */
using ElevationSize = int16_t;
/**
 * \brief Type used for slope values (uses [0 - MAX_SLOPE_FOR_DISTANCE])
 */
using SlopeSize = uint8_t;
/**
 * \brief Type used for storing intensities
 */
using IntensitySize = uint16_t;
/**
 * \brief A day (0 - 366)
 */
using Day = uint16_t;
static constexpr Day MAX_DAYS = 366;
/**
 * \brief Maximum number of columns for an Environment
 */
static constexpr Idx MAX_COLUMNS = 2048;
/**
 * \brief Maximum number of rows for an Environment
 */
static constexpr Idx MAX_ROWS = MAX_COLUMNS;
// static_assert(static_cast<size_t>(MAX_ROWS) * (MAX_COLUMNS - 1) <= std::numeric_limits<Idx>::max());
/**
 * \brief Maximum aspect value (360 == 0)
 */
static constexpr auto MAX_ASPECT = 359;
/**
 * \brief Maximum slope that affects ISI - everything after this is the same factor
 */
static constexpr auto MAX_SLOPE_FOR_FACTOR = 60;
/**
 * \brief Maximum slope that can be stored - this is used in the horizontal distance calculation
 */
static constexpr auto MAX_SLOPE_FOR_DISTANCE = 127;
/**
 * \brief Number of all possible fuels in simulation
 */
static constexpr auto NUMBER_OF_FUELS = 56;
/**
 * \brief 2*pi
 */
static constexpr auto M_2_X_PI = 2.0 * M_PI;
/**
 * \brief 3/2*pi
 */
static constexpr auto M_3_X_PI_2 = 3.0 * M_PI_2;
/**
 * \brief Ratio of degrees to radians
 */
static constexpr auto M_RADIANS_TO_DEGREES = 180.0 / M_PI;
/**
 * \brief Number of hours in a day
 */
static constexpr int DAY_HOURS = 24;
/**
 * \brief Number of minutes in an hour
 */
static constexpr int HOUR_MINUTES = 60;
/**
 * \brief Number of seconds in a minute
 */
static constexpr int MINUTE_SECONDS = 60;
/**
 * \brief Number of minutes in a day
 */
static constexpr int DAY_MINUTES = DAY_HOURS * HOUR_MINUTES;
/**
 * \brief Number of seconds in a day
 */
static constexpr int DAY_SECONDS = DAY_MINUTES * MINUTE_SECONDS;
/**
 * \brief Number of hours in a year
 */
static constexpr int YEAR_HOURS = MAX_DAYS * DAY_HOURS;
/**
 * \brief Array of results of a function for all possible integer percent slopes
 */
using SlopeTableArray = array<double, MAX_SLOPE_FOR_DISTANCE + 1>;
/**
 * \brief Size of an angle in degrees
 */
using DegreesSize = uint16_t;
/**
 * \brief Array of results of a function for all possible integer angles in degrees
 */
using AngleTableArray = array<double, 361>;
/**
 * \brief Size to use for representing fuel types
 */
using FuelCodeSize = uint8_t;
/**
 * \brief Size to use for representing the data in a Cell
 */
using Topo = uint64_t;
/**
 * \brief Size to use for representing sub-coordinates for location within a Cell
 */
using SubSize = uint16_t;
/**
 * \brief Coordinates (row, column, sub-row, sub-column)
 */
using Coordinates = tuple<Idx, Idx, SubSize, SubSize>;
/**
 * \brief FullCoordinates (row, column, sub-row, sub-column)
 */
using FullCoordinates = tuple<FullIdx, FullIdx, SubSize, SubSize>;
/**
 * \brief Type of clock to use for times
 */
using Clock = std::chrono::steady_clock;
/**
 * \brief Offset from a position
 */
struct Offset
{
public:
  /**
   * \brief Offset in the x direction (column)
   */
  inline constexpr double x() const noexcept
  {
    return x_;
  }
  /**
   * \brief Offset in the y direction (row)
   */
  inline constexpr double y() const noexcept
  {
    return y_;
  }
  constexpr Offset(const double a, const double b) noexcept
    : x_(a),
      y_(b)
  {
  }
  constexpr Offset() noexcept
    : Offset(-1, -1)
  {
  }
  constexpr Offset(Offset&& rhs) noexcept = default;
  constexpr Offset(const Offset& rhs) noexcept = default;
  Offset& operator=(const Offset& rhs) noexcept = default;
  Offset& operator=(Offset&& rhs) noexcept = default;
private:
  /**
   * \brief Offset in the x direction (column)
   */
  double x_;
  /**
   * \brief Offset in the y direction (row)
   */
  double y_;
};
/**
 * \brief Collection of Offsets
 */
using OffsetSet = vector<Offset>;
}
