/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "Util.h"
// UTM.h
// Original Javascript by Chuck Taylor
// Port to C++ by Alex Hajnal
//
// *** THIS CODE USES 32-BIT doubleS BY DEFAULT ***
// *** For 64-bit double-precision edit UTM.h: undefine double_32 and define double_64
//
// This is a simple port of the code on the Geographic/UTM Coordinate Converter (1) page
// from Javascript to C++.
// Using this you can easily convert between UTM and WGS84 (latitude and longitude).
// Accuracy seems to be around 50 cm (I suspect rounding errors are limiting precision).
// This code is provided as-is and has been minimally tested; enjoy but use at your own
// risk!
// The license for UTM.cpp and UTM.h is the same as the original Javascript:
// "The C++ source code in UTM.cpp and UTM.h may be copied and reused without
// restriction."
//
// 1) http://home.hiwaay.net/~taylorc/toolbox/geography/geoutm.html
namespace tbd::topo
{
class Point;
/**
 * \brief Computes the ellipsoidal distance from the equator to a point at a given latitude.
 *
 * Reference: Hoffmann-Wellenhof, B., Lichtenegger, H., and Collins, J.,
 * GPS: Theory and Practice, 3rd ed.  New York: Springer-Verlag Wien, 1994.
 *
 * \param phi Latitude of the point, in radians.
 * \return The ellipsoidal distance of the point from the equator, in meters.
 */
[[nodiscard]] double arc_length_of_meridian(double phi) noexcept;
[[nodiscard]] constexpr double meridian_to_zone(const double meridian) noexcept
{
  return (meridian + 183.0) / 6.0;
}
/**
 * \brief Determines the central meridian for the given UTM zone.
 *
 * Range of the central meridian is the radian equivalent of [-177,+177].
 *
 * \param zone A double designating the UTM zone, range [1,60].
 * \return The central meridian for the given UTM zone, in degrees
 */
[[nodiscard]] constexpr double utm_central_meridian_deg(const double zone) noexcept
{
  return -183.0 + zone * 6.0;
}
/**
 * \brief Determines the central meridian for the given UTM zone.
 *
 * Range of the central meridian is the radian equivalent of [-177,+177].
 *
 * \param zone An integer value designating the UTM zone, range [1,60].
 * \return The central meridian for the given UTM zone, in radians
 */
[[nodiscard]] constexpr double utm_central_meridian(const int zone) noexcept
{
  return util::to_radians(utm_central_meridian_deg(zone));
}
/**
 * \brief Computes the footpoint latitude
 *
 * For use in converting transverse Mercator coordinates to ellipsoidal coordinates.
 *
 * Reference: Hoffmann-Wellenhof, B., Lichtenegger, H., and Collins, J.,
 * GPS: Theory and Practice, 3rd ed.  New York: Springer-Verlag Wien, 1994.
 *
 * \param y The UTM northing coordinate, in meters.
 * \return The footpoint latitude, in radians.
 */
[[nodiscard]] double footpoint_latitude(double y) noexcept;
/**
 * \brief Converts a latitude/longitude pair to Transverse Mercator x and y coordinates
 *
 * Converts a latitude/longitude pair to x and y coordinates in the
 * Transverse Mercator projection.  Note that Transverse Mercator is not
 * the same as UTM; a scale factor is required to convert between them.
 *
 * Reference: Hoffmann-Wellenhof, B., Lichtenegger, H., and Collins, J.,
 * GPS: Theory and Practice, 3rd ed.  New York: Springer-Verlag Wien, 1994.
 *
 * \param phi Latitude of the point, in radians.
 * \param lambda Longitude of the point, in radians.
 * \param lambda0 Longitude of the central meridian to be used, in radians.
 * \param x The x coordinate of the computed point.
 * \param y The y coordinate of the computed point.
 * \return None
 */
void map_lat_lon_to_xy(double phi,
                       double lambda,
                       double lambda0,
                       double* x,
                       double* y) noexcept;
/**
 * \brief Converts Transverse Mercator to latitude/longitude
 *
 * Converts x and y coordinates in the Transverse Mercator projection to
 * a latitude/longitude pair.  Note that Transverse Mercator is not
 * the same as UTM; a scale factor is required to convert between them.
 *
 * Reference: Hoffmann-Wellenhof, B., Lichtenegger, H., and Collins, J.,
 * GPS: Theory and Practice, 3rd ed.  New York: Springer-Verlag Wien, 1994.
 *
 * \param x The easting of the point, in meters.
 * \param y The northing of the point, in meters.
 * \param lambda0 Longitude of the central meridian to be used, in radians.
 * \param phi Latitude in radians.
 * \param lambda Longitude in radians.
 */
void map_xy_to_lat_lon(double x,
                       double y,
                       double lambda0,
                       double* phi,
                       double* lambda) noexcept;
/**
 * \brief Converts a latitude/longitude pair to x and y coordinates in the UTM projection.
 * \param point Point to convert coordinates from
 * \param x The x coordinate (easting) of the computed point. (in meters)
 * \param y The y coordinate (northing) of the computed point. (in meters)
 * \return The UTM zone used for calculating the values of x and y.
 */
[[nodiscard]] int lat_lon_to_utm(const Point& point, double* x, double* y) noexcept;
/**
 * \brief Converts a latitude/longitude pair to x and y coordinates in the UTM projection.
 * \param point Point to convert coordinates from
 * \param zone Zone to use for conversion
 * \param x The x coordinate (easting) of the computed point. (in meters)
 * \param y The y coordinate (northing) of the computed point. (in meters)
 */
void lat_lon_to_utm(const Point& point,
                    double zone,
                    double* x,
                    double* y) noexcept;
/**
 * \brief Convert UTM to latitude/longitude.
 *
 * Converts x and y coordinates in the Universal Transverse Mercator
 * projection to a latitude/longitude pair.
 *
 * The UTM zone parameter should be in the range [1,60].
 *
 * \param x The easting of the point, in meters.
 * \param y The northing of the point, in meters.
 * \param zone The UTM zone in which the point lies.
 * \param is_southern_hemisphere True if the point is in the southern hemisphere; false otherwise.
 * \param lat The latitude of the point, in radians.
 * \param lon The longitude of the point, in radians.
 */
void utm_to_lat_lon(double x,
                    double y,
                    int zone,
                    bool is_southern_hemisphere,
                    double* lat,
                    double* lon) noexcept;
}
