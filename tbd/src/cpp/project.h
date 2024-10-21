#include <proj.h>
#include <stdio.h>

#include "stdafx.h"

#include "Point.h"

using tbd::FullCoordinates;

unique_ptr<FullCoordinates> to_proj4(
  const string& proj4,
  const tbd::topo::Point& point,
  MathSize* x,
  MathSize* y)
{
  PJ_CONTEXT* C;
  PJ* P;
  PJ* norm;
  PJ_COORD a, b, c;

  /* or you may set C=PJ_DEFAULT_CTX if you are sure you will     */
  /* use PJ objects from only one thread                          */
  C = proj_context_create();

  P = proj_create_crs_to_crs(
    C,
    "EPSG:4326",
    proj4.c_str(),
    NULL);

  if (0 == P)
  {
    fprintf(stderr, "Failed to create transformation object.\n");
    return {};
  }

  /* This will ensure that the order of coordinates for the input CRS */
  /* will be longitude, latitude, whereas EPSG:4326 mandates latitude, */
  /* longitude */
  norm = proj_normalize_for_visualization(C, P);
  if (0 == norm)
  {
    fprintf(stderr, "Failed to normalize transformation object.\n");
    return {};
  }
  proj_destroy(P);
  P = norm;

  /* Given that we have used proj_normalize_for_visualization(), the order */
  /* of coordinates is longitude, latitude, and values are expressed in */
  /* degrees. */
  a = proj_coord(point.longitude(), point.latitude(), 0, 0);

  /* transform to UTM, then back to geographical */
  b = proj_trans(P, PJ_FWD, a);
  *x = b.enu.e;
  *y = b.enu.n;

  c = proj_trans(P, PJ_INV, b);
  printf("longitude: %f, latitude: %f => easting: %.3f, northing: %.3f => x: %.3f, y: %.3f => longitude: %g, latitude: %g\n",
         point.longitude(),
         point.latitude(),
         b.enu.e,
         b.enu.n,
         b.xy.x,
         b.xy.y,
         c.lp.lam,
         c.lp.phi);

  /* Clean up */
  proj_destroy(P);
  proj_context_destroy(C); /* may be omitted in the single threaded case */
  return {};
}
