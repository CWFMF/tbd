/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "TimeUtil.h"
namespace tbd::util
{
void fix_tm(tm* t)
{
  const time_t t_t = mktime(t);
  t = localtime(&t_t);
}
}
