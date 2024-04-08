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
#include <algorithm>
#include <string>
#include <tiffio.h>
#include <geotiff/geotiffio.h>
#include <utility>
#include <vector>
#include "Grid.h"
#include "Util.h"
#include "Settings.h"
#include "FuelType.h"
namespace tbd::data
{
/**
 * \brief A GridData<T, V, const vector<T>> that cannot change once initialized.
 * \tparam T The initialization value type.
 * \tparam V The initialized value type.
 */
template <class T, class V = T>
class ConstantGrid
  : public GridData<T, V, const vector<T>>
{
public:
  /**
   * \brief Value for grid at given Location.
   * \param location Location to get value for.
   * \return Value at grid Location.
   */
  [[nodiscard]] constexpr T at(const Location& location) const noexcept override
  {
#ifdef DEBUG_GRIDS
    logging::check_fatal(location.row() >= this->rows() || location.column() >= this->columns(), "Out of bounds (%d, %d)", location.row(), location.column());
#endif
    // return at(location.hash());
    return this->data.at(location.hash());
  }
  /**
   * \brief Value for grid at given Location.
   * \param hash HashSize hash for Location to get value for.
   * \return Value at grid Location.
   */
  //  [[nodiscard]] constexpr T at(const HashSize hash) const noexcept
  //  {
  //    return this->data.at(hash);
  //  }
  /**
   * \brief Throw an error because ConstantGrid can't change values.
   */
  // ! @cond Doxygen_Suppress
  void set(const Location&, const T) override
  // ! @endcond
  {
    throw runtime_error("Cannot change ConstantGrid");
  }
  ~ConstantGrid() = default;
  ConstantGrid(const ConstantGrid& rhs) noexcept = delete;
  ConstantGrid(ConstantGrid&& rhs) noexcept = delete;
  ConstantGrid& operator=(const ConstantGrid& rhs) noexcept = delete;
  ConstantGrid& operator=(ConstantGrid&& rhs) noexcept = delete;
  /**
   * \brief Constructor
   * \param cell_size Cell width and height (m)
   * \param rows Number of rows
   * \param columns Number of columns
   * \param no_data Value to use for no data
   * \param nodata Integer value that represents no data
   * \param xllcorner Lower left corner X coordinate (m)
   * \param yllcorner Lower left corner Y coordinate (m)
   * \param xurcorner Upper right corner X coordinate (m)
   * \param yurcorner Upper right corner Y coordinate (m)
   * \param proj4 Proj4 projection definition
   * \param data Data to set as grid data
   */
  ConstantGrid(const double cell_size,
               const Idx rows,
               const Idx columns,
               const T no_data,
               const V nodata,
               const double xllcorner,
               const double yllcorner,
               const double xurcorner,
               const double yurcorner,
               string&& proj4,
               vector<T>&& data)
    : GridData<T, V, const vector<T>>(cell_size,
                                      rows,
                                      columns,
                                      no_data,
                                      nodata,
                                      xllcorner,
                                      yllcorner,
                                      xurcorner,
                                      yurcorner,
                                      std::forward<string>(proj4),
                                      std::move(data))
  {
  }
  /**
   * \brief Constructor
   * \param cell_size Cell width and height (m)
   * \param rows Number of rows
   * \param columns Number of columns
   * \param no_data Value to use for no data
   * \param nodata Integer value that represents no data
   * \param xllcorner Lower left corner X coordinate (m)
   * \param yllcorner Lower left corner Y coordinate (m)
   * \param proj4 Proj4 projection definition
   * \param initialization_value Value to initialize entire grid with
   */
  ConstantGrid(const double cell_size,
               const Idx rows,
               const Idx columns,
               const T& no_data,
               const V nodata,
               const double xllcorner,
               const double yllcorner,
               const string& proj4,
               const T& initialization_value) noexcept
    : ConstantGrid(cell_size,
                   rows,
                   columns,
                   no_data,
                   nodata,
                   xllcorner,
                   yllcorner,
                   proj4,
                   std::move(vector<T>(static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS,
                                       initialization_value)))
  {
  }
  /**
   * \brief Read a section of a TIFF into a ConstantGrid
   * \param filename File name to read from
   * \param tif Pointer to open TIFF denoted by filename
   * \param gtif Pointer to open geotiff denoted by filename
   * \param point Point to center ConstantGrid on
   * \param convert Function taking int and nodata int value that returns T
   * \return ConstantGrid containing clipped data for TIFF
   */
  [[nodiscard]] static ConstantGrid<T, V>* readTiff(const string& filename,
                                                    TIFF* tif,
                                                    GTIF* gtif,
                                                    const topo::Point& point,
                                                    std::function<T(V, V)> convert)
  {
    logging::info("Reading file %s", filename.c_str());
#ifdef DEBUG_GRIDS
    // auto min_value = std::numeric_limits<int16_t>::max();
    // auto max_value = std::numeric_limits<int16_t>::min();
    auto min_value = std::numeric_limits<V>::max();
    auto max_value = std::numeric_limits<V>::min();
#endif
    logging::debug("Reading a raster where T = %s, V = %s",
                   typeid(T).name(),
                   typeid(V).name());
    logging::debug("Raster type V has limits %ld, %ld",
                   std::numeric_limits<V>::min(),
                   std::numeric_limits<V>::max());
    const GridBase grid_info = read_header(tif, gtif);
    int tile_width;
    int tile_length;
    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tile_width);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &tile_length);
    void* data;
    uint32_t count;
    TIFFGetField(tif, TIFFTAG_GDAL_NODATA, &count, &data);
    logging::check_fatal(0 == count, "NODATA value is not set in input");
    logging::debug("NODATA value is '%s'", static_cast<char*>(data));
    const auto nodata_orig = stoi(string(static_cast<char*>(data)));
    logging::debug("NODATA value is originally parsed as %d", nodata_orig);
    const auto nodata = static_cast<V>(stoi(string(static_cast<char*>(data))));
    logging::debug("NODATA value is parsed as %d", nodata);
    auto actual_rows = grid_info.calculateRows();
    auto actual_columns = grid_info.calculateColumns();
    const auto coordinates = grid_info.findFullCoordinates(point, true);
    auto min_column = max(static_cast<FullIdx>(0),
                          static_cast<FullIdx>(std::get<1>(*coordinates) - static_cast<FullIdx>(MAX_COLUMNS) / static_cast<FullIdx>(2)));
    if (min_column + MAX_COLUMNS >= actual_columns)
    {
      min_column = max(static_cast<FullIdx>(0), actual_columns - MAX_COLUMNS);
    }
    // make sure we're at the start of a tile
    const auto tile_column = tile_width * static_cast<FullIdx>(min_column / tile_width);
    const auto max_column = static_cast<FullIdx>(min(min_column + MAX_COLUMNS - 1, actual_columns));
#ifdef DEBUG_GRIDS
    logging::check_fatal(min_column < 0, "Column can't be less than 0");
    logging::check_fatal(max_column - min_column > MAX_COLUMNS, "Can't have more than %d columns", MAX_COLUMNS);
    logging::check_fatal(max_column > actual_columns, "Can't have more than actual %d columns", actual_columns);
#endif
    auto min_row = max(static_cast<FullIdx>(0),
                       static_cast<FullIdx>(std::get<0>(*coordinates) - static_cast<FullIdx>(MAX_ROWS) / static_cast<FullIdx>(2)));
    if (min_row + MAX_COLUMNS >= actual_rows)
    {
      min_row = max(static_cast<FullIdx>(0), actual_rows - MAX_ROWS);
    }
    const auto tile_row = tile_width * static_cast<FullIdx>(min_row / tile_width);
    const auto max_row = static_cast<FullIdx>(min(min_row + MAX_ROWS - 1, actual_rows));
#ifdef DEBUG_GRIDS
    logging::check_fatal(min_row < 0, "Row can't be less than 0 but is %d", min_row);
    logging::check_fatal(max_row - min_row > MAX_ROWS, "Can't have more than %d rows but have %d", MAX_ROWS, max_row - min_row);
    logging::check_fatal(max_row > actual_rows, "Can't have more than actual %d rows", actual_rows);
#endif
    T no_data = convert(nodata, nodata);
    vector<T> values(static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS, no_data);
    logging::verbose("%s: malloc start", filename.c_str());
    int bps = std::numeric_limits<V>::digits + (1 * std::numeric_limits<V>::is_signed);
    int bps_file;
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps_file);
    logging::check_fatal(bps != bps_file,
                         "Raster %s type is not expected type (%ld bits instead of %ld)",
                         filename.c_str(),
                         bps_file,
                         bps);
#ifdef DEBUG_GRIDS
    int bps_int16_t = std::numeric_limits<int16_t>::digits + (1 * std::numeric_limits<int16_t>::is_signed);
    logging::debug("Size of pointer to int is %ld vs %ld", sizeof(int16_t*), sizeof(V*));
    logging::debug("Raster %s calculated bps for type V is %ld; tif says bps is %ld; int16_t is %ld",
                   filename.c_str(),
                   bps,
                   bps_file,
                   bps_int16_t);
#endif
    const auto tile_size = TIFFTileSize(tif);
    logging::debug("Tile size for reading %s is %ld", filename.c_str(), tile_size);
    const auto buf = _TIFFmalloc(tile_size);
    logging::verbose("%s: read start", filename.c_str());
    const tsample_t smp{};
    logging::debug("Want to clip grid to (%d, %d) => (%d, %d) for a %dx%d raster",
                   min_row,
                   min_column,
                   max_row,
                   max_column,
                   actual_rows,
                   actual_columns);
    for (auto h = tile_row; h <= max_row; h += tile_length)
    {
      for (auto w = tile_column; w <= max_column; w += tile_width)
      {
        TIFFReadTile(tif, buf, static_cast<uint32_t>(w), static_cast<uint32_t>(h), 0, smp);
        for (auto y = 0; (y < tile_length) && (y + h <= max_row); ++y)
        {
          // read in so that (0, 0) has a hash of 0
          const auto y_row = static_cast<HashSize>((h - min_row) + y);
          const auto actual_row = (max_row - min_row) - y_row;
          if (actual_row >= 0 && actual_row < MAX_ROWS)
          {
            for (auto x = 0; (x < tile_width) && (x + w <= max_column); ++x)
            {
              const auto offset = y * tile_width + x;
              const auto actual_column = ((w - min_column) + x);
              if (actual_column >= 0 && actual_column < MAX_ROWS)
              {
                const auto cur_hash = actual_row * MAX_COLUMNS + actual_column;
                auto cur = *(static_cast<V*>(buf) + offset);
#ifdef DEBUG_GRIDS
                min_value = min(cur, min_value);
                max_value = max(cur, max_value);
#endif
                // try
                // {
                values.at(cur_hash) = convert(cur, nodata);
                // logging::check_fatal(Settings::fuelLookup().values.at(cur_hash));
                // }
                // // catch (const std::out_of_range& err)
                // catch (const std::exception& err)
                // {
                //   logging::error("Error trying to read tiff");
                //   logging::debug("cur = %d", cur);
                //   logging::debug("nodata = %d", nodata);
                //   logging::debug("T = %s, V = %s", typeid(T).name(), typeid(V).name());
                //   if constexpr (std::is_same_v<T, const tbd::fuel::FuelType*>)
                //   {
                //     auto f = static_cast<const tbd::fuel::FuelType*>(values.at(cur_hash));
                //     logging::debug("fuel %s has code %d",
                //         tbd::fuel::FuelType::safeName(f),
                //         tbd::fuel::FuelType::safeCode(f));
                //   }
                //   logging::warning(err.what());
                //   // logging::fatal(err.what());
                //   tbd::sim::Settings::fuelLookup().listFuels();
                //   throw err;
                // }
              }
            }
          }
        }
      }
    }
    logging::verbose("%s: read end", filename.c_str());
    _TIFFfree(buf);
    logging::verbose("%s: free end", filename.c_str());
    const auto new_xll = grid_info.xllcorner() + (static_cast<double>(min_column) * grid_info.cellSize());
    const auto new_yll = grid_info.yllcorner()
                       + (static_cast<double>(actual_rows) - static_cast<double>(max_row))
                           * grid_info.cellSize();
#ifdef DEBUG_GRIDS
    logging::check_fatal(new_yll < grid_info.yllcorner(),
                         "New yllcorner is outside original grid");
#endif
    logging::verbose("Translated lower left is (%f, %f) from (%f, %f)",
                     new_xll,
                     new_yll,
                     grid_info.xllcorner(),
                     grid_info.yllcorner());
    const auto num_rows = max_row - min_row + 1;
    const auto num_columns = max_column - min_column + 1;
    auto result = new ConstantGrid<T, V>(grid_info.cellSize(),
                                         num_rows,
                                         num_columns,
                                         no_data,
                                         nodata,
                                         new_xll,
                                         new_yll,
                                         new_xll + (static_cast<double>(num_columns) + 1) * grid_info.cellSize(),
                                         new_yll + (static_cast<double>(num_rows) + 1) * grid_info.cellSize(),
                                         string(grid_info.proj4()),
                                         std::move(values));
    auto new_location = result->findCoordinates(point, true);
#ifdef DEBUG_GRIDS
    logging::check_fatal(nullptr == new_location, "Invalid location after reading");
#endif
    logging::note("Coordinates are (%d, %d => %f, %f)",
                  std::get<0>(*new_location),
                  std::get<1>(*new_location),
                  std::get<0>(*new_location) + std::get<2>(*new_location) / 1000.0,
                  std::get<1>(*new_location) + std::get<3>(*new_location) / 1000.0);
#ifdef DEBUG_GRIDS
    logging::note("Values for %s range from %d to %d",
                  filename.c_str(),
                  min_value,
                  max_value);
#endif
    return result;
  }
  /**
   * \brief Read a section of a TIFF into a ConstantGrid
   * \param filename File name to read from
   * \param point Point to center ConstantGrid on
   * \param convert Function taking int and nodata int value that returns T
   * \return ConstantGrid containing clipped data for TIFF
   */
  [[nodiscard]] static ConstantGrid<T, V>* readTiff(const string& filename,
                                                    const topo::Point& point,
                                                    std::function<T(V, V)> convert)
  {
    return with_tiff<ConstantGrid<T, V>*>(
      filename,
      [&filename, &convert, &point](TIFF* tif, GTIF* gtif) { return readTiff(filename, tif, gtif, point, convert); });
  }
  /**
   * \brief Read a section of a TIFF into a ConstantGrid
   * \param filename File name to read from
   * \param point Point to center ConstantGrid on
   * \return ConstantGrid containing clipped data for TIFF
   */
  [[nodiscard]] static ConstantGrid<T, T>* readTiff(const string& filename,
                                                    const topo::Point& point)
  {
    return readTiff(filename, point, util::no_convert<T>);
  }
  /**
   * \brief Save contents to .asc file
   * \param dir Directory to save into
   * \param base_name File base name to use
   */
  void saveToAsciiFile(const string& dir,
                       const string& base_name) const
  {
    saveToAsciiFile(dir, base_name, [](V value) { return value; });
  }
  /**
   * \brief Save contents to .asc file
   * \tparam R Type to be written to .asc file
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param convert Function to convert from V to R
   */
  void saveToAsciiFile(const string& dir,
                       const string& base_name,
                       std::function<V(T)> convert) const
  {
    Idx min_row = 0;
    Idx num_rows = this->rows();
    Idx min_column = 0;
    Idx num_columns = this->columns();
    const double xll = this->xllcorner() + min_column * this->cellSize();
    // offset is different for y since it's flipped
    const double yll = this->yllcorner() + (min_row) * this->cellSize();
    logging::extensive("Lower left corner is (%f, %f)", xll, yll);
    ofstream out;
    out.open(dir + base_name + ".asc");
    write_ascii_header(out,
                       num_columns,
                       num_rows,
                       xll,
                       yll,
                       this->cellSize(),
                       static_cast<double>(this->noDataInt()));
    // need to output in reverse order since (0,0) is bottom left
    for (Idx r = num_rows - 1; r >= 0; --r)
    {
      for (Idx c = 0; c < num_columns; ++c)
      {
        const Location idx(static_cast<Idx>(r), static_cast<Idx>(c));
        // HACK: use + here so that it gets promoted to a printable number
        //       prevents char type being output as characters
        out << +(convert(this->at(idx)))
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
    saveToTiffFile(dir, base_name, [](V value) { return value; });
  }
  /**
   * \brief Save contents to .tif file
   * \tparam R Type to be written to .tif file
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param convert Function to convert from V to R
   */
  void saveToTiffFile(const string& dir,
                      const string& base_name,
                      std::function<V(T)> convert) const
  {
    Idx min_row = 0;
    //    Idx num_rows = this->rows();
    Idx min_column = 0;
    //    Idx num_columns = this->columns();
    const double xll = this->xllcorner() + min_column * this->cellSize();
    // offset is different for y since it's flipped
    const double yll = this->yllcorner() + (min_row) * this->cellSize();
    logging::extensive("Lower left corner is (%f, %f)", xll, yll);
    constexpr uint32_t tileWidth = 256;
    constexpr uint32_t tileHeight = 256;
    // ensure this is always divisible by tile size
    static_assert(0 == MAX_ROWS % tileWidth);
    static_assert(0 == MAX_COLUMNS % tileHeight);
    uint32_t width = this->columns();
    uint32_t height = this->rows();
    string filename = dir + base_name + ".tif";
    TIFF* tif = GeoTiffOpen(filename.c_str(), "w");
    auto gtif = GTIFNew(tif);
    logging::check_fatal(!gtif, "Cannot open file %s as a GEOTIFF", filename.c_str());
    const double xul = this->xllcorner();
    const double yul = this->yllcorner() + (this->cellSize() * this->rows());
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
    uint32_t bps = std::numeric_limits<V>::digits + (1 * std::numeric_limits<V>::is_signed);
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
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
    const auto tile_size = TIFFTileSize(tif);
    logging::debug("Tile size for writing %s is %ld", base_name.c_str(), tile_size);
    auto buf = (V*)_TIFFmalloc(tile_size);
    for (size_t i = 0; i < width; i += tileWidth)
    {
      for (size_t j = 0; j < height; j += tileHeight)
      {
        // need to put data from grid into buffer, but flipped vertically
        for (size_t x = 0; x < tileWidth; ++x)
        {
          for (size_t y = 0; y < tileHeight; ++y)
          {
            const size_t actual_x = i + x;
            const size_t actual_y = j + y;
            const size_t flipped_y = height - actual_y - 1;
            const size_t actual = actual_x + flipped_y * width;
            buf[x + y * tileWidth] = convert(this->data[actual]);
          }
        }
        logging::check_fatal(TIFFWriteTile(tif, buf, i, j, 0, 0) < 0, "Cannot write tile to %s", filename.c_str());
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
private:
  /**
   * \brief Constructor
   * \param grid_info GridBase defining Grid area
   * \param no_data Value that represents no data
   * \param values Values to initialize grid with
   */
  ConstantGrid(const GridBase& grid_info,
               const T& no_data,
               const V& nodata,
               vector<T>&& values)
    : ConstantGrid(grid_info.cellSize(),
                   no_data,
                   nodata,
                   grid_info.xllcorner(),
                   grid_info.yllcorner(),
                   grid_info.xurcorner(),
                   grid_info.yurcorner(),
                   string(grid_info.proj4()),
                   std::move(values))
  {
#ifdef DEBUG_GRIDS
    logging::check_fatal(
      this->data.size() != static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS,
      "Invalid grid size");
#endif
  }
};
}
