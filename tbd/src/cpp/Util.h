/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "stdafx.h"
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef TIFFTAG_GDAL_NODATA
#define TIFFTAG_GDAL_NODATA 42113
#endif

/**
 * Open file and register GeoTIFF tags so we can read and write properly
 * @param filename Name of file to open
 * @param mode Mode to open file with
 * @return Handle to open TIFF with fields registered
 */
TIFF* GeoTiffOpen(const char* const filename, const char* const mode);

namespace tbd
{
namespace util
{
/**
 * \brief Convert day and hour to double representing time
 * \tparam T Type used for representing day
 * \param day Day
 * \param hour Hour
 * \return double representing time at day and hour
 */
template <typename T>
[[nodiscard]] double to_time(const T day, const int hour) noexcept
{
  return day + hour / (1.0 * DAY_HOURS);
}
/**
 * \brief Convert time index to double representing time
 * \param t_index index for array of times
 * \return double representing time at day and hour
 */
[[nodiscard]] constexpr double to_time(const size_t t_index) noexcept
{
  return static_cast<double>(t_index) / DAY_HOURS;
}
/**
 * \brief Convert day and hour into time index
 * \tparam T Type used for representing day
 * \param day Day
 * \param hour Hour
 * \return index for array of times
 */
template <typename T>
[[nodiscard]] size_t time_index(const T day, const int hour) noexcept
{
  return static_cast<size_t>(day) * DAY_HOURS + hour;
}
/**
 * \brief Convert day and hour into time index since min_date
 * \param day Day
 * \param hour Hour
 * \param min_date Date at time index 0
 * \return index for array of times
 */
template <typename T>
[[nodiscard]] size_t time_index(const T day,
                                const int hour,
                                const Day min_date) noexcept
{
  return time_index(day, hour) - static_cast<size_t>(DAY_HOURS) * min_date;
}
/**
 * \brief Convert double into time index
 * \param time time to get time index for
 * \return index for array of times
 */
[[nodiscard]] constexpr size_t time_index(const double time) noexcept
{
  return static_cast<size_t>(time * DAY_HOURS);
}
/**
 * \brief Convert double into time index since min_date
 * \param time Time to convert to time index
 * \param min_date Date at time index 0
 * \return index for array of times
 */
[[nodiscard]] constexpr size_t time_index(const double time,
                                          const Day min_date) noexcept
{
  return time_index(time) - static_cast<size_t>(DAY_HOURS) * min_date;
}
/**
 * \brief Return the passed value as given type
 * \tparam T Type to return
 * \param value Value to return
 * \return Value casted to type T
 */
template <class T>
[[nodiscard]] T no_convert(int value, int) noexcept
{
  return static_cast<T>(value);
}
/**
 * \brief Ensure that value lies between 0 and 2 * PI
 * \param theta value to ensure is within bounds
 * \return value within range of (0, 2 * PI]
 */
[[nodiscard]] constexpr double fix_radians(const double theta)
{
  if (theta > M_2_X_PI)
  {
    return theta - M_2_X_PI;
  }
  if (theta < 0)
  {
    return theta + M_2_X_PI;
  }
  return theta;
}
/**
 * \brief Convert degrees to radians
 * \param degrees Angle in degrees
 * \return Angle in radians
 */
[[nodiscard]] constexpr double to_radians(const double degrees) noexcept
{
  return fix_radians(degrees / M_RADIANS_TO_DEGREES);
}
// only calculate this once and reuse it
/**
 * \brief 360 degrees in radians
 */
static constexpr double RAD_360 = to_radians(360);
/**
 * \brief 270 degrees in radians
 */
static constexpr double RAD_270 = to_radians(270);
/**
 * \brief 180 degrees in radians
 */
static constexpr double RAD_180 = to_radians(180);
/**
 * \brief 90 degrees in radians
 */
static constexpr double RAD_090 = to_radians(90);
/**
 * \brief Convert radians to degrees
 * \param radians Value in radians
 * \return Value in degrees
 */
[[nodiscard]] constexpr double to_degrees(const double radians)
{
  return fix_radians(radians) * M_RADIANS_TO_DEGREES;
}
/**
 * \brief Convert Bearing to Heading (opposite angle)
 * \param azimuth Bearing
 * \return Heading
 */
[[nodiscard]] constexpr double to_heading(const double azimuth)
{
  return fix_radians(azimuth + RAD_180);
}
/**
 * \brief Read from a stream until delimiter is found
 * \tparam Elem Elem for stream
 * \tparam Traits Traits for stream
 * \tparam Alloc Allocator for stream
 * \param stream Stream to read from
 * \param str gstring to read into
 * \param delimiter Delimiter to stop at
 * \return
 */
template <class Elem,
          class Traits,
          class Alloc>
std::basic_istream<Elem, Traits>& getline(
  std::basic_istream<Elem, Traits>* stream,
  std::basic_string<Elem, Traits, Alloc>* str,
  const Elem delimiter)
{
  // make template so we can use pointers directly
  return getline(*stream, *str, delimiter);
}
/**
 * \brief Check if a directory exists
 * \param dir Directory to check existence of
 * \return Whether or not the directory exists
 */
[[nodiscard]] bool directory_exists(const char* dir) noexcept;
/**
 * \brief Get a list of files in the given directory matching the given regex
 * \param name Directory to search for files
 * \param v vector to put found file names into
 * \param match regular expression to match in file names
 */
void read_directory(const wstring& name,
                    vector<string>* v,
                    const wstring& match);
/**
 * \brief Get a list of files in the given directory
 * \param name Directory to search for files
 * \param v vector to put found file names into
 */
void read_directory(const wstring& name, vector<string>* v);
/**
 * \brief Get a list of rasters in the given directory for the specified year
 * \param dir Root directory to look for rasters in
 * \param year Year to use rasters for if available, else default
 * \return List of rasters in the directory
 */
[[nodiscard]] vector<string> find_rasters(const string& dir, int year);
/**
 * \brief Make the given directory if it does not exist
 * \param dir Directory to create
 */
void make_directory(const char* dir) noexcept;
/**
 * \brief Make the given directory and any parent directories that do not exist
 * \param dir Directory to create
 */
void make_directory_recursive(const char* dir) noexcept;
/**
 * \brief Return base raised to the power N
 * \tparam N Power to raise to
 * \tparam T Type passed and returned
 * \param base Base to raise to power N
 * \return base raised to power N
 */
template <unsigned int N, class T>
[[nodiscard]] constexpr T pow_int(const T& base)
{
  // https://stackoverflow.com/questions/16443682/c-power-of-integer-template-meta-programming
  return N == 0
         ? 1
       : N % 2 == 0
         ? pow_int<N / 2, T>(base) * pow_int<N / 2, T>(base)
         : base * pow_int<(N - 1) / 2, T>(base) * pow_int<(N - 1) / 2, T>(base);
}
/**
 * \brief Makes a bit mask of all 1's the specified size
 * \tparam N Number of bits
 * \tparam T Type to return
 * \return Bit mask of all 1's of specified size
 */
template <unsigned int N, class T>
[[nodiscard]] constexpr T bit_mask()
{
  return static_cast<T>(pow_int<N, int64_t>(2) - 1);
}
/**
 * \brief Round value to specified precision
 * \tparam N Precision to round to
 * \param value Value to round
 * \return Value after rounding
 */
template <unsigned int N>
[[nodiscard]] double round_to_precision(const double value) noexcept
{
  // HACK: this can't actually make the value be the precision we want due to
  // floating point storage, but we can round it to what it would be if it were
  // that precision
  static const auto b = pow_int<N, int64_t>(10);
  return round(value * b) / b;
}
/**
 * \brief Convert date and time into internal represenation
 * \param year year
 * \param month month
 * \param day day of month
 * \param hour hour
 * \param minute minute
 */
tm to_tm(const int year,
         const int month,
         const int day,
         const int hour,
         const int minute);
double to_time(const tm& t);
double to_time(const int year,
               const int month,
               const int day,
               const int hour,
               const int minute);
/**
 * \brief Read a date from the given stream
 * \param iss Stream to read from
 * \param str string to read date into
 * \param t tm to parse date into
 */
void read_date(istringstream* iss, string* str, tm* t);
/**
 * \brief Provides the ability to determine how many times something is used during a simulation.
 */
class UsageCount
{
  /**
   * \brief How many times this has been used
   */
  atomic<size_t> count_;
  /**
   * \brief What this is tracking usage of
   */
  string for_what_;
public:
  ~UsageCount();
  /**
   * \brief Constructor
   * \param for_what What this is tracking usage of
   */
  explicit UsageCount(string for_what) noexcept;
  UsageCount(UsageCount&& rhs) noexcept = delete;
  UsageCount(const UsageCount& rhs) noexcept = delete;
  UsageCount& operator=(UsageCount&& rhs) noexcept = delete;
  UsageCount& operator=(const UsageCount& rhs) noexcept = delete;
  /**
   * \brief Increment operator
   * \return Value after increment
   */
  UsageCount& operator++() noexcept;
};
// https://stackoverflow.com/questions/15843525/how-do-you-insert-the-value-in-a-sorted-vector
/**
 * \brief Insert value into vector in sorted order even if it is already present
 * \tparam T Type of value to insert
 * \param vec vector to insert into
 * \param item Item to insert
 * \return iterator result of insert
 */
template <typename T>
[[nodiscard]] typename std::vector<T>::iterator
  insert_sorted(std::vector<T>* vec, T const& item)
{
  return vec->insert(std::upper_bound(vec->begin(), vec->end(), item), item);
}
/**
 * \brief Insert value into vector in sorted order if it does not already exist
 * \tparam T Type of value to insert
 * \param vec vector to insert into
 * \param item Item to insert
 */
template <typename T>
void insert_unique(std::vector<T>* vec, T const& item)
{
  const auto i = std::lower_bound(vec->begin(), vec->end(), item);
  if (i == vec->end() || *i != item)
  {
    vec->insert(i, item);
  }
}
}
/**
 * \brief Do binary search over function for T that results in value
 * \tparam T Type of input values
 * \tparam V Result type of fct
 * \param lower Lower bound
 * \param upper Upper bound
 * \param value Value to look for
 * \param fct Function taking T and returning V
 * \return T value that results in closest to value
 * \return
 */
template <typename T, typename V>
T binary_find(const T lower,
              const T upper,
              const double value,
              const std::function<V(T)>& fct)
{
  const auto mid = lower + (upper - lower) / 2;
  if (lower == upper)
  {
    return lower;
  }
  if (fct(mid) < value)
  {
    return binary_find(lower, mid, value, fct);
  }
  return binary_find(mid + 1, upper, value, fct);
}
/**
 * \brief Check lower and upper limits before doing binary search over function for T that results in value
 * \tparam T Type of input values
 * \tparam V Result type of fct
 * \param lower Lower bound to check
 * \param upper Upper bound to check
 * \param value Value to look for
 * \param fct Function taking T and returning V
 * \return T value that results in closest to value
 */
template <typename T, typename V>
T binary_find_checked(const T lower,
                      const T upper,
                      const double value,
                      const std::function<V(T)>& fct)
{
  if (fct(lower) < value)
  {
    return lower;
  }
  if (fct(upper) >= value)
  {
    return upper;
  }
  return binary_find(lower, upper, value, fct);
}
/**
 * \brief Determine month and day that a day of the year represents
 * \param year Year to determine for
 * \param day_of_year Day of year to determine for
 * \param month Month that was determined
 * \param day_of_month Day of month that was determined
 */
void month_and_day(const int year, const size_t day_of_year, size_t* month, size_t* day_of_month);
/**
 * \brief Determine if year is a leap year
 * \param year Year to determine for
 * \return Whether or not the given year is a leap year
 */
[[nodiscard]] bool is_leap_year(const int year);
/**
 * Make a nicely formatted timestamp string for the given simulation time
 * @param year Year time is for
 * @param time Simulation time (fractional day of year)
 * @return YYYY-mm-dd HH:00 time string for given time
 */
[[nodiscard]] string make_timestamp(const int year, const double time);
/**
 * Convert circle angle to the angle that would be on an ellipse with
 * given length-to-breadth ratio
 * @param length_to_breadth length-to-breadth ratio
 * @param theta direction to convert to ellipse direction (radians)
 */
[[nodiscard]] inline double ellipse_angle(const double length_to_breadth,
                                          const double theta)
{
  return (util::fix_radians(
    atan2(sin(theta) / length_to_breadth,
          cos(theta))));
}
}
