/* Copyright (c) 2005-2022, Jordan Evens */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once

#include "InnerPos.h"

/**
 * Maximum number of points to allow in a cell before condensing them.
 */
constexpr auto MAX_BEFORE_CONDENSE = 3;

/**
 * Calculates distance from point a to point b (squared I think? - we only
 * care about relative values, so no need to do sqrt)
 * @param a First point
 * @param b Second point
 * @return 'distance' from point a to point b
 */
inline constexpr double distPtPt(const tbd::sim::InnerPos& a, const tbd::sim::InnerPos& b) noexcept;

/**
 * Find a convex hull for the points in the given vector and modify the
 * input to only have the hull points on return
 * @param a Points to find a convex hull for
 */
void hull(vector<tbd::sim::InnerPos>& a) noexcept;
