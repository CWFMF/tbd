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
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include "Location.h"
#include "Log.h"
#include "Point.h"

using tbd::topo::Location;
/**
 * \brief Provides hash function for Location.
 */
template <>
struct std::hash<Location>
{
  /**
   * \brief Get hash value for a Location
   * \param location Location to get value for
   * \return Hash value for a Location
   */
  [[nodiscard]] constexpr tbd::HashSize operator()(
    const Location& location) const noexcept
  {
    return location.hash();
  }
};
namespace tbd::data
{
/**
 * \brief The base class with information for a grid of data with geographic coordinates.
 */
class GridBase
{
public:
  virtual ~GridBase() = default;
  /**
   * \brief Move constructor
   * \param rhs GridBase to move from
   */
  GridBase(GridBase&& rhs) noexcept = default;
  /**
   * \brief Copy constructor
   * \param rhs GridBase to copy from
   */
  GridBase(const GridBase& rhs) = default;
  /**
   * \brief Copy assignment
   * \param rhs GridBase to copy from
   * \return This, after assignment
   */
  GridBase& operator=(const GridBase& rhs) = default;
  /**
   * \brief Move assignment
   * \param rhs GridBase to move from
   * \return This, after assignment
   */
  GridBase& operator=(GridBase&& rhs) noexcept = default;
  /**
   * \brief Cell size used for GridBase.
   * \return Cell height and width in meters.
   */
  [[nodiscard]] constexpr double cellSize() const noexcept
  {
    return cell_size_;
  }
  /**
   * \brief Number of rows in the GridBase.
   * \return Number of rows in the GridBase.
   */
  [[nodiscard]] constexpr FullIdx calculateRows() const noexcept
  {
    return static_cast<FullIdx>((yurcorner() - yllcorner()) / cellSize()) - 1;
    // // HACK: just get rid of -1 for now because it seems weird
    // return static_cast<FullIdx>((yurcorner() - yllcorner()) / cellSize());
  }
  /**
   * \brief Number of columns in the GridBase.
   * \return Number of columns in the GridBase.
   */
  [[nodiscard]] constexpr FullIdx calculateColumns() const noexcept
  {
    return static_cast<FullIdx>((xurcorner() - xllcorner()) / cellSize()) - 1;
    // // HACK: just get rid of -1 for now because it seems weird
    // return static_cast<FullIdx>((xurcorner() - xllcorner()) / cellSize());
  }
  /**
   * \brief Lower left corner X coordinate in meters.
   * \return Lower left corner X coordinate in meters.
   */
  [[nodiscard]] constexpr double xllcorner() const noexcept
  {
    return xllcorner_;
  }
  /**
   * \brief Lower left corner Y coordinate in meters.
   * \return Lower left corner Y coordinate in meters.
   */
  [[nodiscard]] constexpr double yllcorner() const noexcept
  {
    return yllcorner_;
  }
  /**
   * \brief Upper right corner X coordinate in meters.
   * \return Upper right corner X coordinate in meters.
   */
  [[nodiscard]] constexpr double xurcorner() const noexcept
  {
    return xurcorner_;
  }
  /**
   * \brief Upper right corner Y coordinate in meters.
   * \return Upper right corner Y coordinate in meters.
   */
  [[nodiscard]] constexpr double yurcorner() const noexcept
  {
    return yurcorner_;
  }
  /**
   * \brief Proj4 string defining coordinate system for this grid. Must be a UTM projection.
   * \return Proj4 string defining coordinate system for this grid.
   */
  [[nodiscard]] constexpr const string& proj4() const noexcept
  {
    return proj4_;
  }
  /**
   * \brief Central meridian of UTM zone for this grid.
   * \return Central meridian of UTM zone for this grid.
   */
  [[nodiscard]] constexpr double meridian() const noexcept
  {
    return meridian_;
  }
  /**
   * \brief UTM zone represented by proj4 string for this grid.
   * \return UTM zone represented by proj4 string for this grid.
   */
  [[nodiscard]] constexpr double zone() const noexcept
  {
    return zone_;
  }
  /**
   * \brief Constructor
   * \param cell_size Cell width and height (m)
   * \param xllcorner Lower left corner X coordinate (m)
   * \param yllcorner Lower left corner Y coordinate (m)
   * \param xurcorner Upper right corner X coordinate (m)
   * \param yurcorner Upper right corner Y coordinate (m)
   * \param proj4 Proj4 projection definition
   */
  GridBase(double cell_size,
           double xllcorner,
           double yllcorner,
           double xurcorner,
           double yurcorner,
           string&& proj4) noexcept;
  /**
   * \brief Default constructor
   */
  GridBase() noexcept;
  /**
   * \brief Create .prj file in directory with base name for file
   * \param dir Directory to create in
   * \param base_name base file name for .prj file
   */
  void createPrj(const string& dir, const string& base_name) const;
  /**
   * \brief Find Coordinates for Point
   * \param point Point to translate to Grid Coordinate
   * \param flipped Whether or not Grid data is flipped along x axis
   * \return Coordinates for Point translated to Grid
   */
  [[nodiscard]] unique_ptr<Coordinates> findCoordinates(
    const topo::Point& point,
    bool flipped) const;
  /**
   * \brief Find FullCoordinates for Point
   * \param point Point to translate to Grid Coordinate
   * \param flipped Whether or not Grid data is flipped along x axis
   * \return Coordinates for Point translated to Grid
   */
  [[nodiscard]] unique_ptr<FullCoordinates> findFullCoordinates(
    const topo::Point& point,
    bool flipped) const;
private:
  /**
   * \brief Proj4 string defining projection.
   */
  string proj4_;
  /**
   * \brief Cell height and width in meters.
   */
  double cell_size_;
  /**
   * \brief Lower left corner X coordinate in meters.
   */
  double xllcorner_;
  /**
   * \brief Lower left corner Y coordinate in meters.
   */
  double yllcorner_;
  /**
   * \brief Upper right corner X coordinate in meters.
   */
  double xurcorner_;
  /**
   * \brief Upper right corner Y coordinate in meters.
   */
  double yurcorner_;
  /**
   * \brief Central meridian of projection in degrees.
   */
  double meridian_;
  /**
   * \brief UTM zone of projection.
   */
  double zone_;
};
/**
 * \brief A GridBase with an associated type of data.
 * \tparam T Type of data after conversion from initialization type.
 * \tparam V Type of data used as an input when initializing.
 */
template <class T, class V = T>
class Grid
  : public GridBase
{
public:
  /**
   * \brief Number of rows in the GridBase.
   * \return Number of rows in the GridBase.
   */
  [[nodiscard]] constexpr Idx rows() const noexcept
  {
    return rows_;
  }
  /**
   * \brief Number of columns in the GridBase.
   * \return Number of columns in the GridBase.
   */
  [[nodiscard]] constexpr Idx columns() const noexcept
  {
    return columns_;
  }
  /**
   * \brief Value used for grid locations that have no data.
   * \return Value used for grid locations that have no data.
   */
  [[nodiscard]] constexpr V nodata() const noexcept
  {
    return nodata_;
  }
  /**
   * \brief Value representing no data
   * \return Value representing no data
   */
  constexpr T noData() const noexcept
  {
    return no_data_;
  }
  /**
   * \brief Value used to represent no data at a Location.
   * \return Value used to represent no data at a Location.
   */
  [[nodiscard]] constexpr double noDataInt() const noexcept
  {
    return nodata_;
  }
  // NOTE: only use this for simple types because it's returning by value
  /**
   * \brief Value for grid at given Location.
   * \param location Location to get value for.
   * \return Value at grid Location.
   */
  [[nodiscard]] virtual T at(const Location& location) const = 0;
  // NOTE: use set instead of at to avoid issues with bool
  /**
   * \brief Set value for grid at given Location.
   * \param location Location to set value for.
   * \param value Value to set at grid Location.
   * \return None
   */
  virtual void set(const Location& location, T value) = 0;
protected:
  /**
   * \brief Constructor
   * \param cell_size Cell width and height (m)
   * \param rows Number of rows
   * \param columns Number of columns
   * \param no_data Value that represents no data
   * \param nodata Integer value that represents no data
   * \param xllcorner Lower left corner X coordinate (m)
   * \param yllcorner Lower left corner Y coordinate (m)
   * \param proj4 Proj4 projection definition
   */
  Grid(const double cell_size,
       const Idx rows,
       const Idx columns,
       T no_data,
       const V nodata,
       const double xllcorner,
       const double yllcorner,
       const double xurcorner,
       const double yurcorner,
       string&& proj4) noexcept
    : GridBase(cell_size,
               xllcorner,
               yllcorner,
               xurcorner,
               yurcorner,
               std::forward<string>(proj4)),
      nodata_(nodata),
      no_data_(no_data),
      rows_(rows),
      columns_(columns)
  {
#ifdef DEBUG_GRIDS
    logging::check_fatal(rows > MAX_ROWS, "Too many rows (%d > %d)", rows, MAX_ROWS);
    logging::check_fatal(columns > MAX_COLUMNS, "Too many columns (%d > %d)", columns, MAX_COLUMNS);
#endif
  }
  /**
   * \brief Construct based on GridBase and no data value
   * \param grid_info GridBase defining Grid area
   * \param no_data Value that represents no data
   */
  Grid(const GridBase& grid_info, V no_data) noexcept
    : Grid(grid_info.cellSize(),
           static_cast<Idx>(grid_info.calculateRows()),
           static_cast<Idx>(grid_info.calculateColumns()),
           no_data,
           to_string(no_data),
           grid_info.xllcorner(),
           grid_info.yllcorner(),
           grid_info.xurcorner(),
           grid_info.yurcorner(),
           grid_info.proj4())
  {
  }
private:
  /**
   * \brief Value used to represent no data at a Location.
   */
  V nodata_;
  /**
   * \brief Value to use for representing no data at a Location.
   */
  T no_data_;
  /**
   * \brief Number of rows in the grid.
   */
  Idx rows_{};
  /**
   * \brief Number of columns in the grid.
   */
  Idx columns_{};
};
/**
 * \brief A Grid that defines the data structure used for storing values.
 * \tparam T Type of data after conversion from initialization type.
 * \tparam V Type of data used as an input when initializing.
 * \tparam D The data type that stores the values.
 */
template <class T, class V, class D>
class GridData
  : public Grid<T, V>
{
public:
  using Grid<T, V>::Grid;
  /**
   * \brief Constructor
   * \param cell_size Cell width and height (m)
   * \param rows Number of rows
   * \param columns Number of columns
   * \param no_data Value that represents no data
   * \param nodata Integer value that represents no data
   * \param xllcorner Lower left corner X coordinate (m)
   * \param yllcorner Lower left corner Y coordinate (m)
   * \param xurcorner Upper right corner X coordinate (m)
   * \param yurcorner Upper right corner Y coordinate (m)
   * \param proj4 Proj4 projection definition
   * \param data Data to populate GridData with
   */
  GridData(const double cell_size,
           const Idx rows,
           const Idx columns,
           const T no_data,
           const V nodata,
           const double xllcorner,
           const double yllcorner,
           const double xurcorner,
           const double yurcorner,
           string&& proj4,
           D&& data)
    : Grid<T, V>(cell_size,
                 rows,
                 columns,
                 no_data,
                 nodata,
                 xllcorner,
                 yllcorner,
                 xurcorner,
                 yurcorner,
                 std::forward<string>(proj4)),
      data(std::forward<D>(data))
  {
  }
  ~GridData() = default;
  /**
   * \brief Copy constructor
   * \param rhs GridData to copy from
   */
  GridData(const GridData& rhs)
    : Grid<T, V>(rhs), data(rhs.data)
  {
  }
  /**
   * \brief Move constructor
   * \param rhs GridData to move from
   */
  GridData(GridData&& rhs) noexcept
    : Grid<T, V>(rhs), data(std::move(rhs.data))
  {
  }
  /**
   * \brief Copy assignment
   * \param rhs GridData to copy from
   * \return This, after assignment
   */
  GridData& operator=(const GridData& rhs) noexcept
  {
    if (this != &rhs)
    {
      Grid<T, V>::operator=(rhs);
      data = rhs.data;
    }
    return *this;
  }
  /**
   * \brief Move assignment
   * \param rhs GridData to copy from
   * \return This, after assignment
   */
  GridData& operator=(GridData&& rhs) noexcept
  {
    if (this != &rhs)
    {
      Grid<T, V>::operator=(rhs);
      data = std::move(rhs.data);
    }
    return *this;
  }
  /**
   * \brief Size of data structure storing values
   * \return Size of data structure storing values
   */
  [[nodiscard]] size_t size() const
  {
    return data.size();
  }
  // HACK: use public access so that we can get to the keys
  /**
   * \brief Structure that holds data represented by this GridData
   */
  D data;
};
void write_ascii_header(ofstream& out,
                        double num_columns,
                        double num_rows,
                        double xll,
                        double yll,
                        double cell_size,
                        double no_data);
template <class R>
[[nodiscard]] R with_tiff(const string& filename, function<R(TIFF*, GTIF*)> fct)
{
  logging::debug("Reading file %s", filename.c_str());
  // suppress warnings about geotiff tags that aren't found
  TIFFSetWarningHandler(nullptr);
  auto tif = GeoTiffOpen(filename.c_str(), "r");
  logging::check_fatal(!tif, "Cannot open file %s as a TIF", filename.c_str());
  auto gtif = GTIFNew(tif);
  logging::check_fatal(!gtif, "Cannot open file %s as a GEOTIFF", filename.c_str());
  //  try
  //  {
  R result = fct(tif, gtif);
  if (tif)
  {
    XTIFFClose(tif);
  }
  if (gtif)
  {
    GTIFFree(gtif);
  }
  GTIFDeaccessCSV();
  return result;
  //  }
  //  catch (std::exception&)
  //  {
  //    return logging::fatal<R>("Unable to process file %s", filename.c_str());
  //  }
}
GridBase read_header(TIFF* tif, GTIF* gtif);
GridBase read_header(const string& filename);
}
