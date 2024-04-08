/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once

// if in debug mode then set everything, otherwise uncomment turning things off if trying to debug specific things
#define DEBUG_FUEL_VARIABLE
#define DEBUG_FWI_WEATHER
#define DEBUG_GRIDS
#define DEBUG_PROBAILITY
#define DEBUG_SIMULATION
#define DEBUG_STATISTICS
#define DEBUG_WEATHER

#ifdef NDEBUG

#undef DEBUG_FUEL_VARIABLE
#undef DEBUG_FWI_WEATHER
#undef DEBUG_GRIDS
#undef DEBUG_PROBAILITY
#undef DEBUG_SIMULATION
#undef DEBUG_STATISTICS
#undef DEBUG_WEATHER

#endif

#if defined(NDEBUG) || defined(DEBUG_FUEL_VARIABLE) || defined(DEBUG_FWI_WEATHER) || defined(DEBUG_GRIDS) || defined(DEBUG_PROBABILITY) || defined(DEBUG_FWI_WEATHER) || defined(DEBUG_FWI_WEATHER) || defined(DEBUG_FWI_WEATHER)
#define DEBUG_ANY
#endif

namespace tbd::debug
{
void show_debug_settings();
}
