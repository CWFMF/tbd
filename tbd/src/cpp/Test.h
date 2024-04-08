/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
namespace tbd::sim
{
static const double TEST_GRID_SIZE = 100.0;
static const char TEST_PROJ4[] =
  "+proj=tmerc +lat_0=0.000000000 +lon_0=-90.000000000"
  " +k=0.999600 +x_0=500000.000 +y_0=0.000 +a=6378137.000 +b=6356752.314 +units=m";
static const double TEST_XLLCORNER = 324203.990666;
static const double TEST_YLLCORNER = 12646355.311160;
/**
 * \brief Runs test cases for constant weather and fuel based on given arguments.
 * \param argc
 * \param argv
 * \return
 */
int test(int argc, const char* const argv[]);
}
