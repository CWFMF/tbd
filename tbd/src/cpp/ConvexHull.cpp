/* Copyright (c) 2005-2022, Jordan Evens */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "ConvexHull.h"
#include <numbers>

// hull to condense points
#define DO_HULL
// use quick hull instead of regular if hulling
#define QUICK_HULL

#ifdef QUICK_HULL
/**
 * Implementation of the quickhull algorithm to find a convex hull.
 * @param a Points to find hull for
 * @param hullPoints Points already in the hull
 * @param n1 First point
 * @param n2 Second point
 */
void quickHull(const vector<tbd::sim::InnerPos>& a,
               vector<tbd::sim::InnerPos>& hullPoints,
               const tbd::sim::InnerPos& n1,
               const tbd::sim::InnerPos& n2) noexcept;
#endif

constexpr double MIN_X = std::numeric_limits<double>::min();
constexpr double MAX_X = std::numeric_limits<double>::max();
// const double TAN_PI_8 = std::tan(std::numbers::pi / 8);
// const double LEN_PI_8 = TAN_PI_8 / 2;
constexpr double DIST_22_5 = 0.2071067811865475244008443621048490392848359376884740365883398689;
constexpr double P_0_5 = 0.5 + DIST_22_5;
constexpr double M_0_5 = 0.5 - DIST_22_5;

inline constexpr double distPtPt(const tbd::sim::InnerPos& a, const tbd::sim::InnerPos& b) noexcept
{
  return (std::pow((b.x - a.x), 2) + std::pow((b.y - a.y), 2));
}

#ifndef DO_HULL
void hull(vector<tbd::sim::InnerPos>&) noexcept
{
  return;
}
#else
#ifndef QUICK_HULL
void hull(vector<tbd::sim::InnerPos>& a) noexcept
{
  if (a.size() > MAX_BEFORE_CONDENSE)
  {
    size_t n_pos = 0;
    auto n = numeric_limits<double>::max();
    size_t ne_pos = 0;
    auto ne = numeric_limits<double>::max();
    size_t e_pos = 0;
    auto e = numeric_limits<double>::max();
    size_t se_pos = 0;
    auto se = numeric_limits<double>::max();
    size_t s_pos = 0;
    auto s = numeric_limits<double>::max();
    size_t sw_pos = 0;
    auto sw = numeric_limits<double>::max();
    size_t w_pos = 0;
    auto w = numeric_limits<double>::max();
    size_t nw_pos = 0;
    auto nw = numeric_limits<double>::max();
    size_t ssw_pos = 0;
    auto ssw = numeric_limits<double>::max();
    size_t sse_pos = 0;
    auto sse = numeric_limits<double>::max();
    size_t nnw_pos = 0;
    auto nnw = numeric_limits<double>::max();
    size_t nne_pos = 0;
    auto nne = numeric_limits<double>::max();
    size_t wsw_pos = 0;
    auto wsw = numeric_limits<double>::max();
    size_t ese_pos = 0;
    auto ese = numeric_limits<double>::max();
    size_t wnw_pos = 0;
    auto wnw = numeric_limits<double>::max();
    size_t ene_pos = 0;
    auto ene = numeric_limits<double>::max();
    // should always be in the same cell so do this once
    const auto cell_x = static_cast<tbd::Idx>(a[0].x);
    const auto cell_y = static_cast<tbd::Idx>(a[0].y);
    for (size_t i = 0; i < a.size(); ++i)
    {
      const auto& p = a[i];
      const auto x = p.x - cell_x;
      const auto y = p.y - cell_y;
      // north is closest to point (0.5, 1.0)
      const auto cur_n = ((x - 0.5) * (x - 0.5)) + ((1 - y) * (1 - y));
      if (cur_n < n)
      {
        n_pos = i;
        n = cur_n;
      }
      // south is closest to point (0.5, 0.0)
      const auto cur_s = ((x - 0.5) * (x - 0.5)) + (y * y);
      if (cur_s < s)
      {
        s_pos = i;
        s = cur_s;
      }
      // northeast is closest to point (1.0, 1.0)
      const auto cur_ne = ((1 - x) * (1 - x)) + ((1 - y) * (1 - y));
      if (cur_ne < ne)
      {
        ne_pos = i;
        ne = cur_ne;
      }
      // southwest is closest to point (0.0, 0.0)
      const auto cur_sw = (x * x) + (y * y);
      if (cur_sw < sw)
      {
        sw_pos = i;
        sw = cur_sw;
      }
      // east is closest to point (1.0, 0.5)
      const auto cur_e = ((1 - x) * (1 - x)) + ((y - 0.5) * (y - 0.5));
      if (cur_e < e)
      {
        e_pos = i;
        e = cur_e;
      }
      // west is closest to point (0.0, 0.5)
      const auto cur_w = (x * x) + ((y - 0.5) * (y - 0.5));
      if (cur_w < w)
      {
        w_pos = i;
        w = cur_w;
      }
      // southeast is closest to point (1.0, 0.0)
      const auto cur_se = ((1 - x) * (1 - x)) + (y * y);
      if (cur_se < se)
      {
        se_pos = i;
        se = cur_se;
      }
      // northwest is closest to point (0.0, 1.0)
      const auto cur_nw = (x * x) + ((1 - y) * (1 - y));
      if (cur_nw < nw)
      {
        nw_pos = i;
        nw = cur_nw;
      }
      // south-southwest is closest to point (0.5 - 0.207, 0.0)
      const auto cur_ssw = ((x - M_0_5) * (x - M_0_5)) + (y * y);
      if (cur_ssw < ssw)
      {
        ssw_pos = i;
        ssw = cur_ssw;
      }
      // south-southeast is closest to point (0.5 + 0.207, 0.0)
      const auto cur_sse = ((x - P_0_5) * (x - P_0_5)) + (y * y);
      if (cur_sse < sse)
      {
        sse_pos = i;
        sse = cur_sse;
      }
      // north-northwest is closest to point (0.5 - 0.207, 1.0)
      const auto cur_nnw = ((x - M_0_5) * (x - M_0_5)) + ((1 - y) * (1 - y));
      if (cur_nnw < nnw)
      {
        nnw_pos = i;
        nnw = cur_nnw;
      }
      // north-northeast is closest to point (0.5 + 0.207, 1.0)
      const auto cur_nne = ((x - P_0_5) * (x - P_0_5)) + ((1 - y) * (1 - y));
      if (cur_nne < nne)
      {
        nne_pos = i;
        nne = cur_nne;
      }
      // west-southwest is closest to point (0.0, 0.5 - 0.207)
      const auto cur_wsw = (x * x) + ((y - M_0_5) * (y - M_0_5));
      if (cur_wsw < wsw)
      {
        wsw_pos = i;
        wsw = cur_wsw;
      }
      // west-northwest is closest to point (0.0, 0.5 + 0.207)
      const auto cur_wnw = (x * x) + ((y - P_0_5) * (y - P_0_5));
      if (cur_wnw < wnw)
      {
        wnw_pos = i;
        wnw = cur_wnw;
      }
      // east-southeast is closest to point (1.0, 0.5 - 0.207)
      const auto cur_ese = ((1 - x) * (1 - x)) + ((y - M_0_5) * (y - M_0_5));
      if (cur_ese < ese)
      {
        ese_pos = i;
        ese = cur_ese;
      }
      // east-northeast is closest to point (1.0, 0.5 + 0.207)
      const auto cur_ene = ((1 - x) * (1 - x)) + ((y - P_0_5) * (y - P_0_5));
      if (cur_ene < ene)
      {
        ene_pos = i;
        ene = cur_ene;
      }
    }
    a = {
      a[n_pos],
      a[ne_pos],
      a[e_pos],
      a[se_pos],
      a[s_pos],
      a[sw_pos],
      a[w_pos],
      a[nw_pos],
      a[ssw_pos],
      a[sse_pos],
      a[nnw_pos],
      a[nne_pos],
      a[wsw_pos],
      a[ese_pos],
      a[wnw_pos],
      a[ene_pos]};
    tbd::logging::check_fatal(a.size() > 16, "Expected <= 16 points but have %ld", a.size());
  }
  else
  {
    tbd::logging::note("Called when shouldn't have");
  }
}
#else
void hull(vector<tbd::sim::InnerPos>& a) noexcept
{
  vector<tbd::sim::InnerPos> hullPoints{};
  tbd::sim::InnerPos maxPos{MIN_X, MIN_X};
  tbd::sim::InnerPos minPos{MAX_X, MAX_X};

  for (const auto p : a)
  {
    if (p.x > maxPos.x)
    {
      maxPos = p;
    }
    // don't use else if because first point should be set for both
    if (p.x < minPos.x)
    {
      minPos = p;
    }
  }

  // get rid of max & min nodes & call quickhull
  if (maxPos != minPos)
  {
    a.erase(std::remove(a.begin(), a.end(), maxPos), a.end());
    a.erase(std::remove(a.begin(), a.end(), minPos), a.end());
    quickHull(a, hullPoints, minPos, maxPos);
    quickHull(a, hullPoints, maxPos, minPos);
    // make sure we have unique points
    std::sort(hullPoints.begin(), hullPoints.end());
    hullPoints.erase(std::unique(hullPoints.begin(), hullPoints.end()), hullPoints.end());
    std::swap(a, hullPoints);
  }
}

void quickHull(const vector<tbd::sim::InnerPos>& a,
               vector<tbd::sim::InnerPos>& hullPoints,
               const tbd::sim::InnerPos& n1,
               const tbd::sim::InnerPos& n2) noexcept
{
  // printf("Running quick hull\n");
  // exit(-1);
  double maxD = -1.0;   // just make sure it's not >= 0
  tbd::sim::InnerPos maxPos{MIN_X, MIN_X};
  vector<tbd::sim::InnerPos> usePoints{};
  // worst case scenario
  usePoints.reserve(a.size());

  // since we do distLinePt so often, calculate the parts that are always the same
  const auto abX = (n2.x - n1.x);
  const auto abY = (n2.y - n1.y);
  /* so instead of:
   * return ( (b->x - a->x)*(a->y - p->y) - (a->x - p->x)*(b->y - a->y) );
   * we can do the equivalent of:
   * return ( abX*(a->y - p->y) - (a->x - p->x)*abY );
   * for distance from the line n1n2 to the current point
   */

  for (const auto p : a)
  {
    // loop through points, looking for furthest
    const auto d = (abX * (n1.y - p.y) - (n1.x - p.x) * abY);
    if (d >= 0)
    {
      if (d > maxD)
      {
        // if further away
        if (maxD >= 0)
        {
          // already have one, so add old one to the list
          // NOTE: delayed add instead of erasing maxPos later
          usePoints.emplace_back(maxPos);
        }
        // update max dist
        maxD = d;
        maxPos = p;
      }
      else
      {
        // only use in next step if on positive side of line
        usePoints.emplace_back(p);
      }
    }
  }
  if (maxD > 0
      || (0 == maxD
          // we have co-linear points
          //  if either of these isn't true then this must be an edge
          && (distPtPt(n1, maxPos) < distPtPt(n1, n2))
          && (distPtPt(maxPos, n2) < distPtPt(n1, n2))))
  {
    // this is not an edge, so recurse on the lines between n1, n2, & maxPos
    quickHull(usePoints, hullPoints, n1, maxPos);
    quickHull(usePoints, hullPoints, maxPos, n2);
  }
  else
  {
    // n1 -> n2 must be an edge
    hullPoints.emplace_back(n1);
    // Must add n2 as the first point of a different line
  }
}
#endif
#endif
