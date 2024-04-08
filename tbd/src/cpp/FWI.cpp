/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "FWI.h"
#include "Log.h"
#define CHECK_CALCULATION 1
#ifndef DEBUG_FWI_WEATHER
#undef CHECK_CALCULATION
#endif
#define CHECK_EPSILON 0.1
// adapted from http://www.columbia.edu/~rf2426/index_files/FWI.vba
//******************************************************************************************
//
// Description: VBA module containing functions to calculate the components of
//              the Canadian Fire Weather Index system, as described in
//
//      Van Wagner, C.E. 1987. Development and structure of the Canadian Forest Fire
//      Weather Index System. Canadian Forest Service, Ottawa, Ont. For. Tech. Rep. 35.
//      37 p.
//
//      Equation numbers from VW87 are listed throughout, to the right of the equations
//      in the code.
//
//      A more recent technical description can be found in:
//      http://www.cawcr.gov.au/publications/technicalreports/CTR_010.pdf
//
//      This module is essentially a direct C to VBA translation of Kerry Anderson's
//      fwi84.c code. The latitude adjustments were developed by Marty Alexander, and used
//      over Indonesia in the paper:
//
//      Field, R.D., Y. Wang, O. Roswintiarti, and Guswanto. A drought-based predictor of
//      recent haze events in western Indonesia. Atmospheric Environment, 38, 1869-1878,
//      2004.
//
//      A technical description of the latitude adjustments can be found in Appendix 3 of:
//      http://cfs.nrcan.gc.ca/pubwarehouse/pdfs/29152.pdf
//
//      Robert Field, robert.field@utoronto.ca
//******************************************************************************************
namespace tbd::wx
{
const Ffmc Ffmc::Zero = Ffmc(0);
const Dmc Dmc::Zero = Dmc(0);
const Dc Dc::Zero = Dc(0);
const Bui Bui::Zero = Bui(0);
const Isi Isi::Zero = Isi(0);
const Fwi Fwi::Zero = Fwi(0);
// HACK: can't use the ::Zero fields for these because we don't know when they initialize
const FwiWeather FwiWeather::Zero{
  Temperature(0),
  RelativeHumidity(0),
  Wind(Direction(0, false), Speed(0)),
  Precipitation(0),
  Ffmc::Zero,
  Dmc::Zero,
  Dc::Zero,
  Isi::Zero,
  Bui::Zero,
  Fwi::Zero};
// The following two functions refer to the MEA day length adjustment 'note'.
//
//******************************************************************************************
// Function Name: DayLengthFactor
// Description: Calculates latitude/date dependent day length factor for Drought Code
// Parameters:
//      Latitude is latitude in decimal degrees of calculation location
//      Month is integer (1..12) value of month of year for calculation
//
//******************************************************************************************
static double day_length_factor(const double latitude, const int month) noexcept
{
  static constexpr double LfN[] = {
    -1.6,
    -1.6,
    -1.6,
    0.9,
    3.8,
    5.8,
    6.4,
    5.0,
    2.4,
    0.4,
    -1.6,
    -1.6};
  static constexpr double LfS[] = {
    6.4,
    5.0,
    2.4,
    0.4,
    -1.6,
    -1.6,
    -1.6,
    -1.6,
    -1.6,
    0.9,
    3.8,
    5.8};
  //'    '/* Use Northern hemisphere numbers */
  //'   '/* something goes wrong with >= */
  if (latitude > 15.0)
  {
    return LfN[month];
  }
  //'    '/* Use Equatorial numbers */
  if ((latitude <= 15.0) && (latitude > -15.0))
  {
    return 1.39;
  }
  //'    '/* Use Southern hemisphere numbers */
  if (latitude <= -15.0)
  {
    return LfS[month];
  }
  return logging::fatal<double>("Unable to calculate DayLengthFactor");
}
using MonthArray = array<double, 12>;
static constexpr MonthArray DAY_LENGTH46_N{
  6.5,
  7.5,
  9.0,
  12.8,
  13.9,
  13.9,
  12.4,
  10.9,
  9.4,
  8.0,
  7.0,
  6.0};
static constexpr MonthArray DAY_LENGTH20_N{
  7.9,
  8.4,
  8.9,
  9.5,
  9.9,
  10.2,
  10.1,
  9.7,
  9.1,
  8.6,
  8.1,
  7.8};
static constexpr MonthArray DAY_LENGTH20_S{
  10.1,
  9.6,
  9.1,
  8.5,
  8.1,
  7.8,
  7.9,
  8.3,
  8.9,
  9.4,
  9.9,
  10.2};
static constexpr MonthArray DAY_LENGTH40_S{
  11.5,
  10.5,
  9.2,
  7.9,
  6.8,
  6.2,
  6.5,
  7.4,
  8.7,
  10.0,
  11.2,
  11.8};
//******************************************************************************************
// Function Name: DayLength
// Description: Calculates latitude/date dependent day length for DMC calculation
// Parameters:
//      Latitude is latitude in decimal degrees of calculation location
//      Month is integer (1..12) value of month of year for calculation
//
//******************************************************************************************
static constexpr double day_length(const double latitude, const int month) noexcept
{
  //'''/* Day Length Arrays for four different latitude ranges '*/
  //''/*
  //'    Use four ranges which respectively span:
  //'        - 90N to 33 N
  //'        - 33 N to 0
  //'        - 0 to -30
  //'        - -30 to -90
  ///
  if ((latitude <= 90) && (latitude > 33.0))
  {
    return DAY_LENGTH46_N.at(static_cast<size_t>(month) - 1);
  }
  if ((latitude <= 33.0) && (latitude > 15.0))
  {
    return DAY_LENGTH20_N.at(static_cast<size_t>(month) - 1);
  }
  if ((latitude <= 15.0) && (latitude > -15.0))
  {
    return 9.0;
  }
  if ((latitude <= -15.0) && (latitude > -30.0))
  {
    return DAY_LENGTH20_S.at(static_cast<size_t>(month) - 1);
  }
  if ((latitude <= -30.0) && (latitude >= -90.0))
  {
    return DAY_LENGTH40_S.at(static_cast<size_t>(month) - 1);
  }
  return logging::fatal<double>("Unable to calculate DayLength");
}
double find_m(const Temperature& temperature, const RelativeHumidity& rh, const Speed& wind, const double mo) noexcept
{
  //'''/* 4  '*/
  const auto ed = 0.942 * pow(rh.asDouble(), 0.679) + 11.0 * exp((rh.asDouble() - 100.0) / 10.0) + 0.18 * (21.1 - temperature.asDouble()) * (1.0 - exp(-0.115 * rh.asDouble()));
  if (mo > ed)
  {
    //'''/* 6a '*/
    const auto ko = 0.424 * (1.0 - pow(rh.asDouble() / 100.0, 1.7)) + 0.0694 * sqrt(wind.asDouble()) * (1.0 - util::pow_int<8>(rh.asDouble() / 100.0));
    //'''/* 6b '*/
    const auto kd = ko * 0.581 * exp(0.0365 * temperature.asDouble());
    //'''/* 8  '*/
    return ed + (mo - ed) * pow(10.0, -kd);
  }
  //'''/* 5  '*/
  const auto ew = 0.618 * pow(rh.asDouble(), 0.753) + 10.0 * exp((rh.asDouble() - 100.0) / 10.0) + 0.18 * (21.1 - temperature.asDouble()) * (1.0 - exp(-0.115 * rh.asDouble()));
  if (mo < ew)
  {
    //'''/* 7a '*/
    const auto kl = 0.424 * (1.0 - pow((100.0 - rh.asDouble()) / 100.0, 1.7)) + 0.0694 * sqrt(wind.asDouble()) * (1 - util::pow_int<8>((100.0 - rh.asDouble()) / 100.0));
    //'''/* 7b '*/
    const auto kw = kl * 0.581 * exp(0.0365 * temperature.asDouble());
    //'''/* 9  '*/
    return ew - (ew - mo) * pow(10.0, -kw);
  }
  return mo;
}
//******************************************************************************************
// Function Name: FFMC
// Description: Calculates today's Fine Fuel Moisture Code
// Parameters:
//    temperature is the 12:00 LST temperature in degrees celsius
//    rh is the 12:00 LST relative humidity in %
//    wind is the 12:00 LST wind speed in kph
//    rain is the 24-hour accumulated rainfall in mm, calculated at 12:00 LST
//    ffmc_previous is the previous day's FFMC
//******************************************************************************************
static double calculate_ffmc(const Temperature& temperature,
                             const RelativeHumidity& rh,
                             const Speed& wind,
                             const Precipitation& rain,
                             const Ffmc& ffmc_previous) noexcept
{
  //'''/* 1  '*/
  auto mo = 147.2 * (101.0 - ffmc_previous.asDouble()) / (59.5 + ffmc_previous.asDouble());
  if (rain.asDouble() > 0.5)
  {
    //'''/* 2  '*/
    const auto rf = rain.asDouble() - 0.5;
    //'''/* 3a '*/
    auto mr = mo + 42.5 * rf * (exp(-100.0 / (251.0 - mo))) * (1 - exp(-6.93 / rf));
    if (mo > 150.0)
    {
      //'''/* 3b '*/
      mr += 0.0015 * util::pow_int<2>(mo - 150.0) * sqrt(rf);
    }
    if (mr > 250.0)
    {
      mr = 250.0;
    }
    mo = mr;
  }
  const auto m = find_m(temperature, rh, wind, mo);
  //'''/* 10 '*/
  return (59.5 * (250.0 - m) / (147.2 + m));
}
Ffmc::Ffmc(const Temperature& temperature,
           const RelativeHumidity& rh,
           const Speed& wind,
           const Precipitation& rain,
           const Ffmc& ffmc_previous) noexcept
  : Ffmc(calculate_ffmc(temperature, rh, wind, rain, ffmc_previous))
{
}
//******************************************************************************************
// Function Name: DMC
// Description: Calculates today's Duff Moisture Code
// Parameters:
//    temperature is the 12:00 LST temperature in degrees celsius
//    rh is the 12:00 LST relative humidity in %
//    rain is the 24-hour accumulated rainfall in mm, calculated at 12:00 LST
//    dmc_previous is the previous day's DMC
//    latitude is the latitude in decimal degrees of the location for calculation
//    month is the month of Year (1..12) for the current day's calculations.
//******************************************************************************************
static double calculate_dmc(const Temperature& temperature,
                            const RelativeHumidity& rh,
                            const Precipitation& rain,
                            const Dmc& dmc_previous,
                            const int month,
                            const double latitude) noexcept
{
  auto previous = dmc_previous.asDouble();
  if (rain.asDouble() > 1.5)
  {
    //'''/* 11  '*/
    const auto re = 0.92 * rain.asDouble() - 1.27;
    //'''/* 12  '*/
    //    const auto mo = 20.0 + exp(5.6348 - previous / 43.43);
    // Alteration to Eq. 12 to calculate more accurately
    const auto mo = 20 + 280 / exp(0.023 * previous);
    const auto b = (previous <= 33.0)
                   ?   //'''/* 13a '*/
                     100.0 / (0.5 + 0.3 * previous)
                   : ((previous <= 65.0)
                        ?   //'''/* 13b '*/
                        14.0 - 1.3 * (log(previous))
                        :   //'''/* 13c '*/
                        6.2 * log(previous) - 17.2);
    //'''/* 14  '*/
    const auto mr = mo + 1000.0 * re / (48.77 + b * re);
    //'''/* 15  '*/
    //    const auto pr = 244.72 - 43.43 * log(mr - 20.0);
    // Alteration to Eq. 15 to calculate more accurately
    const auto pr = 43.43 * (5.6348 - log(mr - 20));
    previous = max(pr, 0.0);
  }
  const auto k = (temperature.asDouble() > -1.1)
                 ? 1.894 * (temperature.asDouble() + 1.1) * (100.0 - rh.asDouble()) * day_length(latitude, month) * 0.0001
                 : 0.0;
  //'''/* 17  '*/
  return (previous + k);
}
Dmc::Dmc(const Temperature& temperature,
         const RelativeHumidity& rh,
         const Precipitation& rain,
         const Dmc& dmc_previous,
         const int month,
         const double latitude) noexcept
  : Dmc(calculate_dmc(temperature, rh, rain, dmc_previous, month, latitude))
{
}
//******************************************************************************************
// Function Name: DC
// Description: Calculates today's Drought Code
// Parameters:
//    temperature is the 12:00 LST temperature in degrees celsius
//    rain is the 24-hour accumulated rainfall in mm, calculated at 12:00 LST
//    dc_previous is the previous day's DC
//    latitude is the latitude in decimal degrees of the location for calculation
//    month is the month of Year (1..12) for the current day's calculations.
//******************************************************************************************
static double calculate_dc(const Temperature& temperature,
                           const Precipitation& rain,
                           const Dc& dc_previous,
                           const int month,
                           const double latitude) noexcept
{
  auto previous = dc_previous.asDouble();
  if (rain.asDouble() > 2.8)
  {
    //'/* 18  */
    const auto rd = 0.83 * (rain.asDouble()) - 1.27;
    //'/* 19  */
    const auto qo = 800.0 * exp(-previous / 400.0);
    //'/* 20  */
    const auto qr = qo + 3.937 * rd;
    //'/* 21  */
    const auto dr = 400.0 * log(800.0 / qr);
    previous = (dr > 0.0) ? dr : 0.0;
  }
  const auto lf = day_length_factor(latitude, month - 1);
  //'/* 22  */
  const auto v = max(0.0,
                     (temperature.asDouble() > -2.8)
                       ? 0.36 * (temperature.asDouble() + 2.8) + lf
                       : lf);
  //'/* 23  */
  const auto d = previous + 0.5 * v;
  // HACK: don't allow negative values
  return max(0.0, d);
}
Dc::Dc(const Temperature& temperature,
       const Precipitation& rain,
       const Dc& dc_previous,
       const int month,
       const double latitude) noexcept
  : Dc(calculate_dc(temperature, rain, dc_previous, month, latitude))
{
}
//******************************************************************************************
// Function Name: ISI
// Description: Calculates today's Initial Spread Index
// Parameters:
//    wind is the 12:00 LST wind speed in kph
//    ffmc is the current day's FFMC
//******************************************************************************************
static double calculate_isi(const Speed& wind, const Ffmc& ffmc) noexcept
{
  //'''/* 24  '*/
  const auto f_wind = exp(0.05039 * wind.asDouble());
  //'''/* 1   '*/
  const auto m = 147.2 * (101 - ffmc.asDouble()) / (59.5 + ffmc.asDouble());
  //'''/* 25  '*/
  const auto f_f = 91.9 * exp(-0.1386 * m) * (1.0 + pow(m, 5.31) / 49300000.0);
  //'''/* 26  '*/
  return (0.208 * f_wind * f_f);
}
Isi::Isi(const Speed& wind, const Ffmc& ffmc) noexcept
  : Isi(calculate_isi(wind, ffmc))
{
}
Isi::Isi(double
#ifdef CHECK_CALCULATION
           value
#endif
         ,
         const Speed& wind,
         const Ffmc& ffmc) noexcept
  : Isi(wind, ffmc)
{
#ifdef CHECK_CALCULATION
  logging::check_fatal(abs((*this - Isi(value)).asDouble()) >= CHECK_EPSILON,
                       "ISI is incorrect %f, %f => %f not %f",
                       wind.asDouble(),
                       ffmc.asDouble(),
                       asDouble(),
                       Isi(value).asDouble());
#endif
}
//******************************************************************************************
// Function Name: BUI
// Description: Calculates today's Buildup Index
// Parameters:
//    DMC is the current day's Duff Moisture Code
//    DC is the current day's Drought Code
//******************************************************************************************
static double calculate_bui(const Dmc& dmc, const Dc& dc) noexcept
{
  if (dmc.asDouble() <= 0.4 * dc.asDouble())
  {
    // HACK: this isn't normally part of it, but it's division by 0 without this
    if (0 == dc.asDouble())
    {
      return 0;
    }
    //'''/* 27a '*/
    return max(0.0,
               0.8 * dmc.asDouble() * dc.asDouble() / (dmc.asDouble() + 0.4 * dc.asDouble()));
  }
  //'''/* 27b '*/
  return max(0.0,
             dmc.asDouble() - (1.0 - 0.8 * dc.asDouble() / (dmc.asDouble() + 0.4 * dc.asDouble())) * (0.92 + pow(0.0114 * dmc.asDouble(), 1.7)));
}
Bui::Bui(double
#ifdef CHECK_CALCULATION
           value
#endif
         ,
         const Dmc& dmc,
         const Dc& dc) noexcept
  : Bui(dmc, dc)
{
#ifdef CHECK_CALCULATION
  logging::check_fatal(abs((*this - Bui(value)).asDouble()) >= CHECK_EPSILON,
                       "BUI is incorrect %f, %f => %f not %f",
                       dmc.asDouble(),
                       dc.asDouble(),
                       asDouble(),
                       Bui(value).asDouble());
#endif
}
Bui::Bui(const Dmc& dmc, const Dc& dc) noexcept
  : Bui(calculate_bui(dmc, dc))
{
}
//******************************************************************************************
// Function Name: FWI
// Description: Calculates today's Fire Weather Index
// Parameters:
//    ISI is current day's ISI
//    BUI is the current day's BUI
//******************************************************************************************
static double calculate_fwi(const Isi& isi, const Bui& bui) noexcept
{
  const auto f_d = (bui.asDouble() <= 80.0)
                   ?   //'''/* 28a '*/
                     0.626 * pow(bui.asDouble(), 0.809) + 2.0
                   :   //'''/* 28b '*/
                     1000.0 / (25.0 + 108.64 * exp(-0.023 * bui.asDouble()));
  //'''/* 29  '*/
  const auto b = 0.1 * isi.asDouble() * f_d;
  if (b > 1.0)
  {
    //'''/* 30a '*/
    return exp(2.72 * pow(0.434 * log(b), 0.647));
  }
  //'''/* 30b '*/
  return b;
}
Fwi::Fwi(double
#ifdef CHECK_CALCULATION
           value
#endif
         ,
         const Isi& isi,
         const Bui& bui) noexcept
  : Fwi(isi, bui)
{
#ifdef CHECK_CALCULATION
  logging::check_fatal(abs((*this - Fwi(value)).asDouble()) >= CHECK_EPSILON,
                       "FWI is incorrect %f, %f => %f not %f",
                       isi.asDouble(),
                       bui.asDouble(),
                       asDouble(),
                       Fwi(value).asDouble());
#endif
}
Fwi::Fwi(const Isi& isi, const Bui& bui) noexcept
  : Fwi(calculate_fwi(isi, bui))
{
}
//******************************************************************************************
// Function Name: DSR
// Description: Calculates today's Daily Severity Rating
// Parameters:
//    FWI is current day's FWI
//******************************************************************************************
static double calculate_dsr(const Fwi& fwi) noexcept
{
  //'''/* 41 '*/
  return (0.0272 * pow(fwi.asDouble(), 1.77));
}
Dsr::Dsr(const Fwi& fwi) noexcept
  : Dsr(calculate_dsr(fwi))
{
}
inline double stod(const string* const str)
{
  return stod(*str);
}
FwiWeather read(istringstream* iss,
                string* str)
{
  // PREC
  util::getline(iss, str, ',');
  logging::extensive("PREC is %s", str->c_str());
  const Precipitation prec(stod(str));
  // TEMP
  util::getline(iss, str, ',');
  logging::extensive("TEMP is %s", str->c_str());
  const Temperature temp(stod(str));
  // RH
  util::getline(iss, str, ',');
  logging::extensive("RH is %s", str->c_str());
  const RelativeHumidity rh(stod(str));
  // WS
  util::getline(iss, str, ',');
  logging::extensive("WS is %s", str->c_str());
  const Speed ws(stod(str));
  // WD
  util::getline(iss, str, ',');
  logging::extensive("WD is %s", str->c_str());
  const Direction wd(stod(str), false);
  const Wind wind(wd, ws);
  // FIX: pretend we're checking these but the flag is unset for now
  util::getline(iss, str, ',');
  logging::extensive("FFMC is %s", str->c_str());
  const Ffmc ffmc(stod(str));
  util::getline(iss, str, ',');
  logging::extensive("DMC is %s", str->c_str());
  const Dmc dmc(stod(str));
  util::getline(iss, str, ',');
  logging::extensive("DC is %s", str->c_str());
  const Dc dc(stod(str));
  util::getline(iss, str, ',');
  logging::extensive("ISI is %s", str->c_str());
  const Isi isi(stod(str), ws, ffmc);
  util::getline(iss, str, ',');
  logging::extensive("BUI is %s", str->c_str());
  const Bui bui(stod(str), dmc, dc);
  util::getline(iss, str, ',');
  logging::extensive("FWI is %s", str->c_str());
  const Fwi fwi(stod(str), isi, bui);
  return {temp, rh, wind, prec, ffmc, dmc, dc, isi, bui, fwi};
}
FwiWeather::FwiWeather(istringstream* iss,
                       string* str)
  : FwiWeather(read(iss, str))
{
}
double ffmc_effect(const Ffmc& ffmc) noexcept
{
  const auto v = ffmc.asDouble();
  // FIX: use higher precision constant
  const auto mc = 147.2 * (101.0 - v) / (59.5 + v);
  return 91.9 * exp(-0.1386 * mc) * (1 + pow(mc, 5.31) / 49300000.0);
}
FwiWeather::FwiWeather(const Temperature& temp,
                       const RelativeHumidity& rh,
                       const Wind& wind,
                       const Precipitation& prec,
                       const Ffmc& ffmc,
                       const Dmc& dmc,
                       const Dc& dc,
                       const Isi& isi,
                       const Bui& bui,
                       const Fwi& fwi) noexcept
  : Weather(temp, rh, wind, prec),
    ffmc_(ffmc),
    dmc_(dmc),
    dc_(dc),
    // HACK: recalculate so that we can check that things are within tolerances
    isi_(Isi(isi.asDouble(), wind.speed(), ffmc)),
    bui_(Bui(bui.asDouble(), dmc, dc)),
    fwi_(Fwi(fwi.asDouble(), isi, bui)),
    // FIX: this is duplicated in ffmc_effect
    mc_ffmc_pct_(147.2 * (101 - ffmc.asDouble()) / (59.5 + ffmc.asDouble())),
    mc_dmc_pct_(exp((dmc.asDouble() - 244.72) / -43.43) + 20),
    ffmc_effect_(ffmc_effect(ffmc))
{
}
FwiWeather::FwiWeather(const FwiWeather& yesterday,
                       const int month,
                       const double latitude,
                       const Temperature& temp,
                       const RelativeHumidity& rh,
                       const Wind& wind,
                       const Precipitation& prec)
  : FwiWeather(temp,
               rh,
               wind,
               prec,
               Ffmc(temp, rh, wind.speed(), prec, yesterday.ffmc()),
               Dmc(temp, rh, prec, yesterday.dmc(), month, latitude),
               Dc(temp, prec, yesterday.dc(), month, latitude))
{
}
FwiWeather::FwiWeather(const FwiWeather& wx,
                       const Wind& wind,
                       const Ffmc& ffmc,
                       const Isi& isi) noexcept
  : FwiWeather(wx.temp(),
               wx.rh(),
               wind,
               wx.prec(),
               ffmc,
               wx.dmc(),
               wx.dc(),
               isi,
               wx.bui(),
               Fwi(isi, wx.bui()))
{
}
FwiWeather::FwiWeather(const FwiWeather& wx, const Wind& wind, const Ffmc& ffmc) noexcept
  : FwiWeather(wx, wind, ffmc, Isi(wind.speed(), ffmc))
{
}
FwiWeather::FwiWeather(const FwiWeather& wx, const Speed& ws, const Ffmc& ffmc) noexcept
  : FwiWeather(wx, Wind(wx.wind().direction(), ws), ffmc)
{
}
FwiWeather::FwiWeather() noexcept
  : FwiWeather(Zero)
{
}
FwiWeather::FwiWeather(const Temperature& temp,
                       const RelativeHumidity& rh,
                       const Wind& wind,
                       const Precipitation& prec,
                       const Ffmc& ffmc,
                       const Dmc& dmc,
                       const Dc& dc,
                       const Isi& isi,
                       const Bui& bui) noexcept
  : FwiWeather(temp, rh, wind, prec, ffmc, dmc, dc, isi, bui, Fwi(isi, bui))
{
}
FwiWeather::FwiWeather(const Temperature& temp,
                       const RelativeHumidity& rh,
                       const Wind& wind,
                       const Precipitation& prec,
                       const Ffmc& ffmc,
                       const Dmc& dmc,
                       const Dc& dc) noexcept
  : FwiWeather(temp, rh, wind, prec, ffmc, dmc, dc, Isi(wind.speed(), ffmc), Bui(dmc, dc))
{
}
}
