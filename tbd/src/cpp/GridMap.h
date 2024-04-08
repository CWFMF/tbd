/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <algorithm>
#include <limits>
#include <list>
#include <string>
#include <map>
#include <utility>
#include "Grid.h"
#include "ConstantGrid.h"
#include "Settings.h"
namespace tbd::data
{
/**
 * \brief A GridData that uses an unordered_map for storage.
 * \tparam T Type of data after conversion from initialization type.
 * \tparam V Type of data used as an input when initializing.
 */
template <class T, class V = T>
class GridMap
  : public GridData<T, V, map<Location, T>>
{
public:
  /**
   * \brief Determine if Location has a value
   * \param location Location to determine if present in GridMap
   * \return Whether or not a value is present for the Location
   */
  [[nodiscard]] bool contains(const Location& location) const
  {
    return this->data.end() != this->data.find(location);
  }
  /**
   * \brief Retrieve value at Location
   * \param location Location to get value for
   * \return Value at Location
   */
  [[nodiscard]] T at(const Location& location) const override
  {
    const auto value = this->data.find(location);
    if (value == this->data.end())
    {
      return this->noData();
    }
    return get<1>(*value);
  }
  /**
   * \brief Set value at Location
   * \param location Location to set value for
   * \param value Value to set at Location
   */
  void set(const Location& location, const T value) override
  {
    //    const auto seek = this->data.find(location);
    //    if (seek == this->data.end())
    //    {
    //      this->data.emplace(location, value);
    //    }
    //    else
    //    {
    //      this->data.at(location) = value;
    //    }
    this->data[location] = value;
    assert(at(location) == value);
  }
  ~GridMap() = default;
  /**
   * \brief Constructor
   * \param cell_size Cell width and height (m)
   * \param rows Number of rows
   * \param columns Number of columns
   * \param no_data Value that represents no data
   * \param nodata Integer value that represents no data
   * \param xllcorner Lower left corner X coordinate (m)
   * \param yllcorner Lower left corner Y coordinate (m)
   * \param xllcorner Upper right corner X coordinate (m)
   * \param yllcorner Upper right corner Y coordinate (m)
   * \param proj4 Proj4 projection definition
   */
  GridMap(const double cell_size,
          const Idx rows,
          const Idx columns,
          T no_data,
          const V nodata,
          const double xllcorner,
          const double yllcorner,
          const double xurcorner,
          const double yurcorner,
          string&& proj4)
    : GridData<T, V, map<Location, T>>(cell_size,
                                       rows,
                                       columns,
                                       no_data,
                                       nodata,
                                       xllcorner,
                                       yllcorner,
                                       xurcorner,
                                       yurcorner,
                                       std::forward<string>(proj4),
                                       map<Location, T>())
  {
    constexpr auto max_hash = numeric_limits<HashSize>::max();
    // HACK: we don't want overflow errors, but we want to play with the hash size
    const auto max_columns = static_cast<double>(max_hash) / static_cast<double>(this->rows());
    logging::check_fatal(this->columns() >= max_columns,
                         "Grid is too big for cells to be hashed - "
                         "recompile with a larger HashSize value");
    // HACK: reserve space for this based on how big our Idx is because that
    // tells us how many cells there could be
    // HACK: divide because we expect most perimeters to be fairly small, but we
    // want it to be reasonably large
    //    this->data.reserve(static_cast<size_t>(numeric_limits<Idx>::max() / 4));
    //    this->data.reserve(static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS);
  }
  /**
   * \brief Construct empty GridMap with same extent as given Grid
   * \param grid Grid to use extent from
   */
  explicit GridMap(const Grid<T, V>& grid)
    : GridMap<T, V>(grid.cellSize(),
                    grid.noData(),
                    grid.nodata(),
                    grid.xllcorner(),
                    grid.yllcorner(),
                    grid.xurcorner(),
                    grid.yurcorner(),
                    grid.proj4())
  {
  }
  /**
   * \brief Construct empty GridMap with same extent as given Grid
   * \param grid_info Grid to use extent from
   * \param no_data Value to use for no data
   */
  GridMap(const GridBase& grid_info, T no_data)
    : GridMap<T, V>(grid_info.cellSize(),
                    static_cast<Idx>(grid_info.calculateRows()),
                    static_cast<Idx>(grid_info.calculateColumns()),
                    no_data,
                    static_cast<int>(no_data),
                    grid_info.xllcorner(),
                    grid_info.yllcorner(),
                    grid_info.xurcorner(),
                    grid_info.yurcorner(),
                    string(grid_info.proj4()))
  {
  }
  /**
   * \brief Move constructor
   * \param rhs GridMap to move from
   */
  GridMap(GridMap&& rhs) noexcept
    : GridData<T, V, map<Location, T>>(std::move(rhs))
  {
    this->data = std::move(rhs.data);
  }
  /**
   * \brief Copy constructor
   * \param rhs GridMap to copy from
   */
  GridMap(const GridMap& rhs)
    : GridData<T, V, map<Location, T>>(rhs)
  {
    this->data = rhs.data;
  }
  /**
   * \brief Move assignment
   * \param rhs GridMap to move from
   * \return This, after assignment
   */
  GridMap& operator=(GridMap&& rhs) noexcept
  {
    if (this != &rhs)
    {
      this->data = std::move(rhs.data);
    }
    return *this;
  }
  /**
   * \brief Copy assignment
   * \param rhs GridMap to copy from
   * \return This, after assignment
   */
  GridMap& operator=(const GridMap& rhs)
  {
    if (this != &rhs)
    {
      this->data = rhs.data;
    }
    return *this;
  }
  /**
   * \brief Clear data from GridMap
   */
  void clear() noexcept
  {
    //    this->data.clear();
    this->data = {};
    //    this->data.reserve(static_cast<size_t>(numeric_limits<Idx>::max() / 4));
  }
  /**
   * \brief Save GridMap contents to .asc file
   * \param dir Directory to save into
   * \param base_name File base name to use
   */
  void saveToAsciiFile(const string& dir, const string& base_name) const
  {
    saveToAsciiFile<V>(dir, base_name, [](V value) { return value; });
  }
  /**
   * \brief Save GridMap contents to .asc file
   * \tparam R Type to be written to .asc file
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param convert Function to convert from V to R
   */
  template <class R>
  void saveToAsciiFile(const string& dir,
                       const string& base_name,
                       std::function<R(V value)> convert) const
  {
    Idx min_row = this->rows();
    int16_t max_row = 0;
    Idx min_column = this->columns();
    Idx max_column = 0;
    for (const auto& kv : this->data)
    {
      const Idx r = kv.first.row();
      const Idx c = kv.first.column();
      min_row = min(min_row, r);
      max_row = max(max_row, r);
      min_column = min(min_column, c);
      max_column = max(max_column, c);
    }
    // do this so that we take the center point when there's no data since it should
    // stay the same if the grid is centered on the fire
    if (min_row > max_row)
    {
      min_row = max_row = this->rows() / 2;
    }
    if (min_column > max_column)
    {
      min_column = max_column = this->columns() / 2;
    }
    logging::extensive("Lower left corner is (%d, %d)", min_column, min_row);
    logging::extensive("Upper right corner is (%d, %d)", max_column, max_row);
    const double xll = this->xllcorner() + min_column * this->cellSize();
    // offset is different for y since it's flipped
    const double yll = this->yllcorner() + (min_row) * this->cellSize();
    logging::extensive("Lower left corner is (%f, %f)", xll, yll);
    // HACK: make sure it's always at least 1
    const auto num_rows = static_cast<double>(max_row) - min_row + 1;
    const auto num_columns = static_cast<double>(max_column) - min_column + 1;
    ofstream out;
    out.open(dir + base_name + ".asc");
    write_ascii_header(out,
                       num_columns,
                       num_rows,
                       xll,
                       yll,
                       this->cellSize(),
                       static_cast<double>(this->noDataInt()));
    for (Idx ro = 0; ro < num_rows; ++ro)
    {
      // HACK: do this so that we always get at least one pixel in output
      // need to output in reverse order since (0,0) is bottom left
      const Idx r = static_cast<Idx>(max_row) - ro;
      for (Idx co = 0; co < num_columns; ++co)
      {
        const Location idx(static_cast<Idx>(r), static_cast<Idx>(min_column + co));
        // HACK: use + here so that it gets promoted to a printable number
        //       prevents char type being output as characters
        out << +((this->data.find(idx) != this->data.end())
                   ? convert(this->data.at(idx))
                   : this->noData())
            << " ";
      }
      out << "\n";
    }
    out.close();
    this->createPrj(dir, base_name);
  }
  /**
   * \brief Save contents to .tif file
   * \param dir Directory to save into
   * \param base_name File base name to use
   */
  void saveToTiffFile(const string& dir,
                      const string& base_name) const
  {
    saveToTiffFile<V>(dir, base_name, [](V value) { return value; });
  }
  /**
   * \brief Save GridMap contents to .tif file
   * \tparam R Type to be written to .tif file
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param convert Function to convert from V to R
   */
  template <class R>
  void saveToTiffFile(const string& dir,
                      const string& base_name,
                      std::function<R(V value)> convert) const
  {
    constexpr uint32_t tileWidth = 256;
    constexpr uint32_t tileHeight = 256;
    Idx min_row = this->rows();
    int16_t max_row = 0;
    Idx min_column = this->columns();
    Idx max_column = 0;
    for (const auto& kv : this->data)
    {
      const Idx r = kv.first.row();
      const Idx c = kv.first.column();
      min_row = min(min_row, r);
      max_row = max(max_row, r);
      min_column = min(min_column, c);
      max_column = max(max_column, c);
    }
    // do this so that we take the center point when there's no data since it should
    // stay the same if the grid is centered on the fire
    if (min_row > max_row)
    {
      min_row = max_row = this->rows() / 2;
    }
    if (min_column > max_column)
    {
      min_column = max_column = this->columns() / 2;
    }
    Idx c_min = 0;
    while (c_min + static_cast<Idx>(tileWidth) <= min_column)
    {
      c_min += static_cast<Idx>(tileWidth);
    }
    Idx c_max = c_min + static_cast<Idx>(tileWidth);
    while (c_max < max_column)
    {
      c_max += static_cast<Idx>(tileWidth);
    }
    min_column = c_min;
    max_column = c_max;
    Idx r_min = 0;
    while (r_min + static_cast<Idx>(tileHeight) <= min_row)
    {
      r_min += static_cast<Idx>(tileHeight);
    }
    Idx r_max = r_min + static_cast<Idx>(tileHeight);
    while (r_max < max_row)
    {
      r_max += static_cast<Idx>(tileHeight);
    }
    min_row = r_min;
    max_row = r_max;
    logging::extensive("(%d, %d) => (%d, %d)", min_column, min_row, max_column, max_row);
    logging::check_fatal((max_row - min_row) % tileHeight != 0, "Invalid start and end rows");
    logging::check_fatal((max_column - min_column) % tileHeight != 0, "Invalid start and end columns");
    logging::extensive("Lower left corner is (%d, %d)", min_column, min_row);
    logging::extensive("Upper right corner is (%d, %d)", max_column, max_row);
    const double xll = this->xllcorner() + min_column * this->cellSize();
    // offset is different for y since it's flipped
    const double yll = this->yllcorner() + (min_row) * this->cellSize();
    logging::extensive("Lower left corner is (%f, %f)", xll, yll);
    const auto num_rows = static_cast<size_t>(max_row - min_row);
    const auto num_columns = static_cast<size_t>(max_column - min_column);
    // ensure this is always divisible by tile size
    logging::check_fatal(0 != (num_rows % tileWidth), "%d rows not divisible by tiles", num_rows);
    logging::check_fatal(0 != (num_columns % tileHeight), "%d columns not divisible by tiles", num_columns);
    string filename = dir + base_name + ".tif";
    TIFF* tif = GeoTiffOpen(filename.c_str(), "w");
    auto gtif = GTIFNew(tif);
    logging::check_fatal(!gtif, "Cannot open file %s as a GEOTIFF", filename.c_str());
    const double xul = xll;
    const double yul = this->yllcorner() + (this->cellSize() * max_row);
    double tiePoints[6] = {
      0.0,
      0.0,
      0.0,
      xul,
      yul,
      0.0};
    double pixelScale[3] = {
      this->cellSize(),
      this->cellSize(),
      0.0};
    uint32_t bps = sizeof(R) * 8;
    // make sure to use floating point if values are
    if (std::is_floating_point<R>::value)
    {
      TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
    }
    int length = snprintf(nullptr, 0, "%f", this->noDataInt());
    char* str = static_cast<char*>(malloc(length + 1));
    snprintf(str, length + 1, "%f", this->noDataInt());
    TIFFSetField(tif, TIFFTAG_GDAL_NODATA, str);
    free(str);
    logging::extensive("%s takes %d bits", base_name.c_str(), bps);
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, num_columns);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, num_rows);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bps);
    TIFFSetField(tif, TIFFTAG_TILEWIDTH, tileWidth);
    TIFFSetField(tif, TIFFTAG_TILELENGTH, tileHeight);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    GTIFSetFromProj4(gtif, this->proj4().c_str());
    TIFFSetField(tif, TIFFTAG_GEOTIEPOINTS, 6, tiePoints);
    TIFFSetField(tif, TIFFTAG_GEOPIXELSCALE, 3, pixelScale);
    size_t tileSize = tileWidth * tileHeight;
    const auto buf_size = tileSize * sizeof(R);
    logging::extensive("%s has buffer size %d", base_name.c_str(), buf_size);
    R* buf = (R*)_TIFFmalloc(buf_size);
    for (size_t co = 0; co < num_columns; co += tileWidth)
    {
      for (size_t ro = 0; ro < num_rows; ro += tileHeight)
      {
        // NOTE: shouldn't need to check if writing outside of tile because we made bounds on tile edges above
        // need to put data from grid into buffer, but flipped vertically
        for (size_t x = 0; x < tileWidth; ++x)
        {
          for (size_t y = 0; y < tileHeight; ++y)
          {
            const Idx r = static_cast<Idx>(max_row) - (ro + y + 1);
            const Idx c = static_cast<Idx>(min_column) + co + x;
            const Location idx(r, c);
            const R value = convert(at(idx));
            buf[x + y * tileWidth] = value;
          }
        }
        logging::check_fatal(TIFFWriteTile(tif, buf, co, ro, 0, 0) < 0, "Cannot write tile to %s", filename.c_str());
      }
    }
    GTIFWriteKeys(gtif);
    if (gtif)
    {
      GTIFFree(gtif);
    }
    _TIFFfree(buf);
    TIFFClose(tif);
  }
  /**
   * \brief Save GridMap contents to .asc file as probability
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param divisor Number of simulations to divide by to calculate probability per cell
   */
  template <class R>
  void saveToProbabilityFile(const string& dir,
                             const string& base_name,
                             const R divisor) const
  {
    auto div = [divisor](V value) { return static_cast<R>(value / divisor); };
    if (tbd::sim::Settings::saveAsAscii())
    {
      saveToAsciiFile<R>(dir, base_name, div);
    }
    else
    {
      saveToTiffFile<R>(dir, base_name, div);
    }
  }
  /**
   * \brief Calculate area for cells that have a value (ha)
   * \return Area for cells that have a value (ha)
   */
  [[nodiscard]] double fireSize() const noexcept
  {
    // we know that every cell is a key, so we convert that to hectares
    const double per_width = (this->cellSize() / 100.0);
    // cells might have 0 as a value, but those shouldn't affect size
    return static_cast<double>(this->data.size()) * per_width * per_width;
  }
  /**
   * \brief Make a list of all Locations that are on the edge of cells with a value
   * \return A list of all Locations that are on the edge of cells with a value
   */
  [[nodiscard]] list<Location> makeEdge() const
  {
    list<Location> edge{};
    for (const auto& kv : this->data)
    {
      auto loc = kv.first;
      auto on_edge = false;
      for (Idx r = -1; !on_edge && r <= 1; ++r)
      {
        const Idx row_index = loc.row() + r;
        if (!(row_index < 0 || row_index >= this->rows()))
        {
          for (Idx c = -1; !on_edge && c <= 1; ++c)
          {
            const Idx col_index = loc.column() + c;
            if (!(col_index < 0 || col_index >= this->columns())
                && this->data.find(Location(row_index, col_index)) == this->data.end())
            {
              on_edge = true;
            }
          }
        }
      }
      if (on_edge)
      {
        edge.push_back(loc);
      }
    }
    logging::info("Created edge for perimeter with length %lu m",
                  static_cast<size_t>(this->cellSize() * edge.size()));
    return edge;
  }
  /**
   * \brief Make a list of all Locations that have a value
   * \return A list of all Locations that have a value
   */
  [[nodiscard]] list<Location> makeList() const
  {
    list<Location> result{this->data.size()};
    std::transform(this->data.begin(),
                   this->data.end(),
                   result.begin(),
                   [](const pair<const Location, const T>& kv) { return kv.first; });
    return result;
  }
};
}
