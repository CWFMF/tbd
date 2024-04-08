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
#include "Util.h"
#include "Log.h"
namespace tbd::topo
{
/**
 * \brief A location with a row and column.
 */
class Location
{
public:
  Location() = default;
  /**
   * \brief Construct using hash of row and column
   * \param hash HashSize derived form row and column
   */
// NOTE: do this so that we don't get warnings about unused variables in release mode
#ifdef NDEBUG
  explicit constexpr Location(const Idx, const Idx, const HashSize hash) noexcept
#else
  explicit Location(const Idx row, const Idx column, const HashSize hash) noexcept
#endif
    : topo_data_(hash & HashMask)
  {
#ifdef DEBUG_GRIDS
    logging::check_fatal((row != unhashRow(topo_data_))
                           || column != unhashColumn(topo_data_),
                         "Hash is incorrect (%d, %d)",
                         row,
                         column);
#endif
  }
  /**
   * \brief Constructor
   * \param row Row
   * \param column Column
   */
#ifdef NDEBUG
  constexpr
#endif
    Location(const Idx row, const Idx column) noexcept
    : Location(row, column, doHash(row, column) & HashMask)
  {
#ifdef DEBUG_GRIDS
    logging::check_fatal(row >= MAX_ROWS || column >= MAX_COLUMNS, "Location out of bounds (%d, %d)", row, column);
#endif
  }
  /**
   * \brief Row
   * \return Row
   */
  [[nodiscard]] constexpr Idx row() const noexcept
  {
    return unhashRow(hash());
  }
  /**
   * \brief Column
   * \return Column
   */
  [[nodiscard]] constexpr Idx column() const noexcept
  {
    return unhashColumn(hash());
  }
  /**
   * \brief Hash derived from row and column
   * \return Hash derived from row and column
   */
  [[nodiscard]] constexpr HashSize hash() const noexcept
  {
    // can get away with just casting because all the other bits are outside this area
    return static_cast<HashSize>(topo_data_);
  }
  /**
   * \brief Equality operator
   * \param rhs Location to compare to
   * \return Whether or not these are equivalent
   */
  [[nodiscard]] constexpr bool operator==(const Location& rhs) const noexcept
  {
    return hash() == rhs.hash();
  }
  /**
   * \brief Inequality operator
   * \param rhs Location to compare to
   * \return Whether or not these are not equivalent
   */
  [[nodiscard]] constexpr bool operator!=(const Location& rhs) const noexcept
  {
    return !(*this == rhs);
  }
  /**
   * \brief Full stored hash that may contain data from subclasses
   * \return Full stored hash that may contain data from subclasses
   */
  [[nodiscard]] constexpr Topo fullHash() const
  {
    return topo_data_;
  }
protected:
  /**
   * \brief Stored hash that contains row and column data
   */
  Topo topo_data_;
  /**
   * \brief Number of bits to use for storing one coordinate of location data
   */
  static constexpr uint32_t XYBits = std::bit_width<uint32_t>(MAX_ROWS - 1);
  static_assert(util::pow_int<XYBits, size_t>(2) == MAX_ROWS);
  static_assert(util::pow_int<XYBits, size_t>(2) == MAX_COLUMNS);
  /**
   * \brief Number of bits to use for storing location data
   */
  static constexpr uint32_t LocationBits = XYBits * 2;
  /**
   * \brief Hash mask for bits being used for location data
   */
  static constexpr Topo ColumnMask = util::bit_mask<XYBits, Topo>();
  /**
   * \brief Hash mask for bits being used for location data
   */
  static constexpr Topo HashMask = util::bit_mask<LocationBits, Topo>();
  static_assert(HashMask >= static_cast<size_t>(MAX_COLUMNS) * MAX_ROWS - 1);
  static_assert(HashMask <= std::numeric_limits<HashSize>::max());
  /**
   * \brief Construct with given hash that may contain data from subclasses
   * \param topo Hash to store
   */
  explicit constexpr Location(const Topo& topo) noexcept
    : topo_data_(topo)
  {
  }
  /**
   * \brief Create a hash from given values
   * \param row Row
   * \param column Column
   * \return Hash
   */
  [[nodiscard]] static constexpr HashSize doHash(
    const Idx row,
    const Idx column) noexcept
  {
    return (static_cast<HashSize>(row) << XYBits) + static_cast<HashSize>(column);
  }
  /**
   * \brief Row from hash
   * \return Row from hash
   */
  [[nodiscard]] static constexpr Idx unhashRow(const Topo row) noexcept
  {
    // don't need to use mask since bits just get shifted out
    return static_cast<Idx>(row >> XYBits);
  }
  /**
   * \brief Column
   * \return Column
   */
  [[nodiscard]] static constexpr Idx unhashColumn(const Topo column) noexcept
  {
    return static_cast<Idx>(column & ColumnMask);
  }
};
inline bool operator<(const Location& lhs, const Location& rhs)
{
  return lhs.hash() < rhs.hash();
}
inline bool operator>(const Location& lhs, const Location& rhs)
{
  return rhs < lhs;
}
inline bool operator<=(const Location& lhs, const Location& rhs)
{
  return !(lhs > rhs);
}
inline bool operator>=(const Location& lhs, const Location& rhs)
{
  return !(lhs < rhs);
}
}
