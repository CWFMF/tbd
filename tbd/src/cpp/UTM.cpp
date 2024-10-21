/* Copyright (c) Queen's Printer for Ontario, 2020. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "UTM.h"
#include "Point.h"
#include "Log.h"
#include "Util.h"
#include "unstable.h"
namespace tbd::topo
{
// UTM.c
// Original Javascript by Chuck Taylor
// Port to C++ by Alex Hajnal
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
using util::to_radians;
using util::pow_int;
/* Ellipsoid model constants (actual values here are for WGS84) */
constexpr auto SM_A = 6378137.0;
constexpr auto SM_B = 6356752.314;
constexpr auto UTM_SCALE_FACTOR = 0.9996;
MathSize arc_length_of_meridian(const MathSize phi) noexcept
{
  const auto n = (SM_A - SM_B) / (SM_A + SM_B);
  const auto alpha = ((SM_A + SM_B) / 2.0) * (1.0 + (pow_int<2>(n) / 4.0) + (pow_int<4>(n) / 64.0));
  const auto beta = (-3.0 * n / 2.0) + (9.0 * pow_int<3>(n) / 16.0) + (-3.0 * pow_int<5>(n) / 32.0);
  const auto gamma = (15.0 * pow_int<2>(n) / 16.0) + (-15.0 * pow_int<4>(n) / 32.0);
  const auto delta = (-35.0 * pow_int<3>(n) / 48.0) + (105.0 * pow_int<5>(n) / 256.0);
  const auto epsilon = (315.0 * pow_int<4>(n) / 512.0);
  /* Now calculate the sum of the series and return */
  return alpha * (phi + (beta * _sin(2.0 * phi)) + (gamma * _sin(4.0 * phi)) + (delta * _sin(6.0 * phi)) + (epsilon * _sin(8.0 * phi)));
}
constexpr MathSize utm_central_meridian(const MathSize zone)
{
  return to_radians(utm_central_meridian_deg(zone));
}
constexpr MathSize utm_central_meridian_deg(const int zone)
{
  return -183.0 + (static_cast<MathSize>(zone) * 6.0);
}
MathSize footpoint_latitude(const MathSize y) noexcept
{
  /* Precalculate n (Eq. 10.18) */
  const auto n = (SM_A - SM_B) / (SM_A + SM_B);
  /* Precalculate alpha_ (Eq. 10.22) */
  /* (Same as alpha in Eq. 10.17) */
  const auto alpha = ((SM_A + SM_B) / 2.0) * (1 + (pow_int<2>(n) / 4) + (pow_int<4>(n) / 64));
  /* Precalculate row_ (Eq. 10.23) */
  const auto y_alpha = y / alpha;
  /* Precalculate beta_ (Eq. 10.22) */
  const auto beta = (3.0 * n / 2.0) + (-27.0 * pow_int<3>(n) / 32.0) + (269.0 * pow_int<5>(n) / 512.0);
  /* Precalculate gamma_ (Eq. 10.22) */
  const auto gamma = (21.0 * pow_int<2>(n) / 16.0) + (-55.0 * pow_int<4>(n) / 32.0);
  /* Precalculate delta_ (Eq. 10.22) */
  const auto delta = (151.0 * pow_int<3>(n) / 96.0) + (-417.0 * pow_int<5>(n) / 128.0);
  /* Precalculate epsilon_ (Eq. 10.22) */
  const auto epsilon = (1097.0 * pow_int<4>(n) / 512.0);
  /* Now calculate the sum of the series (Eq. 10.21) */
  return y_alpha + (beta * _sin(2.0 * y_alpha))
       + (gamma * _sin(4.0 * y_alpha))
       + (delta * _sin(6.0 * y_alpha))
       + (epsilon * _sin(8.0 * y_alpha));
}
void map_lat_lon_to_xy(const MathSize phi,
                       const MathSize lambda,
                       const MathSize lambda0,
                       MathSize* x,
                       MathSize* y) noexcept
{
  const auto ep2 = (pow_int<2>(SM_A) - pow_int<2>(SM_B)) / pow_int<2>(SM_B);
  const auto c = _cos(phi);
  const auto nu2 = ep2 * pow_int<2>(c);
  const auto n = pow_int<2>(SM_A) / (SM_B * sqrt(1 + nu2));
  const auto t = tan(phi);
  const auto t2 = t * t;
  const auto l = lambda - lambda0;
  /* Precalculate coefficients for l**n in the equations below
     so a normal human being can read the expressions for easting
     and northing
     -- l**1 and l**2 have coefficients of 1.0 */
  const auto l3_coefficient = 1.0 - t2 + nu2;
  const auto l4_coefficient = 5.0 - t2 + 9 * nu2 + 4.0 * (nu2 * nu2);
  const auto l5_coefficient = 5.0 - 18.0 * t2 + (t2 * t2) + 14.0 * nu2 - 58.0 * t2 * nu2;
  const auto l6_coefficient = 61.0 - 58.0 * t2 + (t2 * t2) + 270.0 * nu2 - 330.0 * t2 * nu2;
  const auto l7_coefficient = 61.0 - 479.0 * t2 + 179.0 * (t2 * t2) - (t2 * t2 * t2);
  const auto l8_coefficient = 1385.0 - 3111.0 * t2 + 543.0 * (t2 * t2) - (t2 * t2 * t2);
  /* Calculate easting (x) */
  *x = n * c * l
     + (n / 6.0 * pow_int<3>(c) * l3_coefficient * pow_int<3>(l))
     + (n / 120.0 * pow_int<5>(c) * l5_coefficient * pow_int<5>(l))
     + (n / 5040.0 * pow_int<7>(c) * l7_coefficient * pow_int<7>(l));
  /* Calculate northing (y) */
  *y = arc_length_of_meridian(phi)
     + (t / 2.0 * n * pow_int<2>(c) * pow_int<2>(l))
     + (t / 24.0 * n * pow_int<4>(c) * l4_coefficient * pow_int<4>(l))
     + (t / 720.0 * n * pow_int<6>(c) * l6_coefficient * pow_int<6>(l))
     + (t / 40320.0 * n * pow_int<8>(c) * l8_coefficient * pow_int<8>(l));
}
//   The local variables Nf, nuf2, tf, and tf2 serve the same purpose as
//   N, nu2, t, and t2 in MapLatLonToXY, but they are computed with respect
//   to the footpoint latitude phi_f.
//
//   x1_fraction, x2_fraction, x2_polynomial, x3_polynomial, etc. are to enhance
//   readability and to optimize computations.
void map_xy_to_lat_lon(const MathSize x,
                       const MathSize y,
                       const MathSize lambda0,
                       MathSize* phi,
                       MathSize* lambda) noexcept
{
  /* Get the value of phi_f, the footpoint latitude. */
  const auto phi_f = footpoint_latitude(y);
  const auto ep2 = (pow_int<2>(SM_A) - pow_int<2>(SM_B)) / pow_int<2>(SM_B);
  const auto cf = _cos(phi_f);
  const auto nuf2 = ep2 * pow_int<2>(cf);
  /* Precalculate Nf and initialize nf_power */
  const auto nf = pow_int<2>(SM_A) / (SM_B * sqrt(1 + nuf2));
  auto nf_power = nf;
  const auto tf = tan(phi_f);
  const auto tf2 = tf * tf;
  const auto tf4 = tf2 * tf2;
  /* Precalculate fractional coefficients for x**n in the equations
     below to simplify the expressions for latitude and longitude. */
  const auto x1_fraction = 1.0 / (nf_power * cf);
  nf_power *= nf; /* now equals Nf**2) */
  const auto x2_fraction = tf / (2.0 * nf_power);
  nf_power *= nf; /* now equals Nf**3) */
  const auto x3_fraction = 1.0 / (6.0 * nf_power * cf);
  nf_power *= nf; /* now equals Nf**4) */
  const auto x4_fraction = tf / (24.0 * nf_power);
  nf_power *= nf; /* now equals Nf**5) */
  const auto x5_fraction = 1.0 / (120.0 * nf_power * cf);
  nf_power *= nf; /* now equals Nf**6) */
  const auto x6_fraction = tf / (720.0 * nf_power);
  nf_power *= nf; /* now equals Nf**7) */
  const auto x7_fraction = 1.0 / (5040.0 * nf_power * cf);
  nf_power *= nf; /* now equals Nf**8) */
  const auto x8_fraction = tf / (40320.0 * nf_power);
  /* Precalculate polynomial coefficients for x**n.
     -- x**1 does not have a polynomial coefficient. */
  const auto x2_polynomial = -1.0 - nuf2;
  const auto x3_polynomial = -1.0 - 2 * tf2 - nuf2;
  const auto x4_polynomial = 5.0 + 3.0 * tf2 + 6.0 * nuf2 - 6.0 * tf2 * nuf2 - 3.0 * (nuf2 * nuf2)
                           - 9.0 * tf2 * (nuf2 * nuf2);
  const auto x5_polynomial = 5.0 + 28.0 * tf2 + 24.0 * tf4 + 6.0 * nuf2 + 8.0 * tf2 * nuf2;
  const auto x6_polynomial = -61.0 - 90.0 * tf2 - 45.0 * tf4 - 107.0 * nuf2 + 162.0 * tf2 * nuf2;
  const auto x7_polynomial = -61.0 - 662.0 * tf2 - 1320.0 * tf4 - 720.0 * (tf4 * tf2);
  const auto x8_polynomial = 1385.0 + 3633.0 * tf2 + 4095.0 * tf4 + 1575 * (tf4 * tf2);
  /* Calculate latitude */
  *phi = phi_f + x2_fraction * x2_polynomial * (x * x)
       + x4_fraction * x4_polynomial * pow_int<4>(x)
       + x6_fraction * x6_polynomial * pow_int<6>(x)
       + x8_fraction * x8_polynomial * pow_int<8>(x);
  /* Calculate longitude */
  *lambda = lambda0 + x1_fraction * x
          + x3_fraction * x3_polynomial * pow_int<3>(x)
          + x5_fraction * x5_polynomial * pow_int<5>(x)
          + x7_fraction * x7_polynomial * pow_int<7>(x);
}
int lat_lon_to_utm(const Point& point, MathSize* x, MathSize* y) noexcept
{
  const auto zone = static_cast<int>((point.longitude() + 180.0) / 6) + 1;
  map_lat_lon_to_xy(to_radians(point.latitude()),
                    to_radians(point.longitude()),
                    utm_central_meridian(zone),
                    x,
                    y);
  /* Adjust easting and northing for UTM system. */
  *x = (*x) * UTM_SCALE_FACTOR + 500000.0;
  *y = (*y) * UTM_SCALE_FACTOR;
  if (*y < 0.0)
    *y += 10000000.0;
  return zone;
}
void lat_lon_to_utm(const Point& point, const MathSize zone, MathSize* x, MathSize* y) noexcept
{
  map_lat_lon_to_xy(to_radians(point.latitude()),
                    to_radians(point.longitude()),
                    utm_central_meridian(zone),
                    x,
                    y);
  tbd::logging::note("Converted lat/lon to utm %f,%f", *x, *y);
  /* Adjust easting and northing for UTM system. */
  *x = (*x) * UTM_SCALE_FACTOR + 500000.0;
  *y = (*y) * UTM_SCALE_FACTOR;
  if (*y < 0.0)
    *y += 10000000.0;
}
void utm_to_lat_lon(MathSize x,
                    MathSize y,
                    const int zone,
                    const bool is_southern_hemisphere,
                    MathSize* lat,
                    MathSize* lon) noexcept
{
  x -= 500000.0;
  x /= UTM_SCALE_FACTOR;
  /* If in southern hemisphere, adjust y accordingly. */
  if (is_southern_hemisphere)
    y -= 10000000.0;
  y /= UTM_SCALE_FACTOR;
  const auto central_meridian = utm_central_meridian(zone);
  map_xy_to_lat_lon(x, y, central_meridian, lat, lon);
}
}
