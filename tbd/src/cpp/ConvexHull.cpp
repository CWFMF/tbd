/* Copyright (c) 2005-2022, Jordan Evens */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "ConvexHull.h"
#include <numbers>

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
