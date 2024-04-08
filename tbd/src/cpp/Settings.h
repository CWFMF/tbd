/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <vector>
#include "FuelLookup.h"

namespace tbd
{
namespace sim
{
/**
 * \brief Difference minimum for doubles to be considered the same
 */
static const double COMPARE_LIMIT = 1.0E-20f;
/**
 * \brief Reads and provides access to settings for the simulation.
 */
class Settings
{
public:
  /**
   * \brief Set root directory and read settings from file
   * \param dirname Directory to use for settings and relative paths
   */
  static void setRoot(const char* dirname) noexcept;
  /**
   * \brief Path to directory that outputs are saved to
   * \return Name of file to save weather to
   */
  [[nodiscard]] static const char* outputDirectory() noexcept;
  /**
   * \brief Set path to directory that outputs are saved to
   * \param d Path to directory that outputs are saved to
   * \return None
   */
  static void setOutputDirectory(const string& d);
  /**
   * \brief Root directory that raster inputs are stored in
   * \return Root directory that raster inputs are stored in
   */
  [[nodiscard]] static const char* rasterRoot() noexcept;
  /**
   * \brief Fuel lookup table
   * \return Fuel lookup table
   */
  [[nodiscard]] static const fuel::FuelLookup& fuelLookup() noexcept;
  /**
   * \brief Whether or not to run things asynchronously where possible
   * \return Whether or not to run things asynchronously where possible
   */
  [[nodiscard]] static bool runAsync() noexcept;
  /**
   * \brief Set whether or not to run things asynchronously where possible
   * \param value Whether or not to run things asynchronously where possible
   * \return None
   */
  static void setRunAsync(bool value) noexcept;
  /**
   * \brief Whether or not to run deterministically (100% chance of spread & survival)
   * \return Whether or not to run deterministically (100% chance of spread & survival)
   */
  [[nodiscard]] static bool deterministic() noexcept;
  /**
   * \brief Set whether or not to run deterministically (100% chance of spread & survival)
   * \param value Whether or not to run deterministically (100% chance of spread & survival)
   * \return None
   */
  static void setDeterministic(bool value) noexcept;
  /**
   * \brief Whether or not to save grids as .asc
   * \return Whether or not to save grids as .asc
   */
  [[nodiscard]] static bool saveAsAscii() noexcept;
  /**
   * \brief Set whether or not to save grids as .asc
   * \param value Whether or not to save grids as .asc
   * \return None
   */
  static void setSaveAsAscii(bool value) noexcept;
  /**
   * \brief Whether or not to save intensity grids
   * \return Whether or not to save intensity grids
   */
  [[nodiscard]] static bool saveIntensity() noexcept;
  /**
   * \brief Set whether or not to save intensity grids
   * \param value Whether or not to save intensity grids
   * \return None
   */
  static void setSaveIntensity(bool value) noexcept;
  /**
   * \brief Whether or not to save probability grids
   * \return Whether or not to save probability grids
   */
  [[nodiscard]] static bool saveProbability() noexcept;
  /**
   * \brief Set whether or not to save probability grids
   * \param value Whether or not to save probability grids
   * \return None
   */
  static void setSaveProbability(bool value) noexcept;
  /**
   * \brief Whether or not to save occurrence grids
   * \return Whether or not to save occurrence grids
   */
  [[nodiscard]] static bool saveOccurrence() noexcept;
  /**
   * \brief Set whether or not to save occurrence grids
   * \param value Whether or not to save occurrence grids
   * \return None
   */
  static void setSaveOccurrence(bool value) noexcept;
  /**
   * \brief Whether or not to save simulation area grids
   * \return Whether or not to save simulation area grids
   */
  [[nodiscard]] static bool saveSimulationArea() noexcept;
  /**
   * \brief Set whether or not to save simulation area grids
   * \param value Whether or not to save simulation area grids
   * \return None
   */
  static void setSaveSimulationArea(bool value) noexcept;
  /**
   * \brief Minimum rate of spread before fire is considered to be spreading (m/min)
   * \return Minimum rate of spread before fire is considered to be spreading (m/min)
   */
  [[nodiscard]] static double minimumRos() noexcept;
  /**
   * \brief Maximum distance that the fire is allowed to spread in one step (# of cells)
   * \return Maximum distance that the fire is allowed to spread in one step (# of cells)
   */
  [[nodiscard]] static double maximumSpreadDistance() noexcept;
  /**
   * \brief Minimum Fine Fuel Moisture Code required for spread during the day
   * \return Minimum Fine Fuel Moisture Code required for spread during the day
   */
  [[nodiscard]] static double minimumFfmc() noexcept;
  /**
   * \brief Minimum Fine Fuel Moisture Code required for spread during the night
   * \return Minimum Fine Fuel Moisture Code required for spread during the night
   */
  [[nodiscard]] static double minimumFfmcAtNight() noexcept;
  /**
   * \brief Offset from sunrise at which the day is considered to start (hours)
   * \return Offset from sunrise at which the day is considered to start (hours)
   */
  [[nodiscard]] static double offsetSunrise() noexcept;
  /**
   * \brief Offset from sunrise at which the day is considered to end (hours)
   * \return Offset from sunrise at which the day is considered to end (hours)
   */
  [[nodiscard]] static double offsetSunset() noexcept;
  /**
   * \brief Default Percent Conifer to use for M1/M2 fuels where none is specified (%)
   * \return Percent of the stand that is composed of conifer (%)
   */
  [[nodiscard]] static int defaultPercentConifer() noexcept;
  /**
   * \brief Default Percent Dead Fir to use for M3/M4 fuels where none is specified (%)
   * \return Percent of the stand that is composed of dead fir (NOT percent of the fir that is dead) (%)
   */
  [[nodiscard]] static int defaultPercentDeadFir() noexcept;
  /**
   * \brief The maximum fire intensity for the 'low' range of intensity (kW/m)
   * \return The maximum fire intensity for the 'low' range of intensity (kW/m)
   */
  [[nodiscard]] static int intensityMaxLow() noexcept;
  /**
   * \brief The maximum fire intensity for the 'moderate' range of intensity (kW/m)
   * \return The maximum fire intensity for the 'moderate' range of intensity (kW/m)
   */
  [[nodiscard]] static int intensityMaxModerate() noexcept;
  /**
   * \brief Confidence required before simulation stops (% / 100)
   * \return Confidence required before simulation stops (% / 100)
   */
  [[nodiscard]] static double confidenceLevel() noexcept;
  /**
   * \brief Set confidence required before simulation stops (% / 100)
   * \return Set confidence required before simulation stops (% / 100)
   */
  static void setConfidenceLevel(const double value) noexcept;
  /**
   * \brief Maximum time simulation can run before it is ended and whatever results it has are used (s)
   * \return Maximum time simulation can run before it is ended and whatever results it has are used (s)
   */
  [[nodiscard]] static size_t maximumTimeSeconds() noexcept;
  /**
   * \brief Maximum number of simulations that can run before it is ended and whatever results it has are used
   * \return Maximum number of simulations that can run before it is ended and whatever results it has are used
   */
  [[nodiscard]] static size_t maximumCountSimulations() noexcept;
  /**
   * \brief Weight to give to Scenario part of thresholds
   * \return Weight to give to Scenario part of thresholds
   */
  [[nodiscard]] static double thresholdScenarioWeight() noexcept;
  /**
   * \brief Weight to give to daily part of thresholds
   * \return Weight to give to daily part of thresholds
   */
  [[nodiscard]] static double thresholdDailyWeight() noexcept;
  /**
   * \brief Weight to give to hourly part of thresholds
   * \return Weight to give to hourly part of thresholds
   */
  [[nodiscard]] static double thresholdHourlyWeight() noexcept;
  /**
   * \brief Days to output probability contours for (1 is start date, 2 is day after, etc.)
   * \return Days to output probability contours for (1 is start date, 2 is day after, etc.)
   */
  [[nodiscard]] static vector<int> outputDateOffsets();
  /**
   * \brief Set days to output probability contours for (1 is start date, 2 is day after, etc.)
   * \return None
   */
  static void setOutputDateOffsets(const char* value);
  /**
   * \brief Whatever the maximum value in the date offsets is
   * \return Whatever the maximum value in the date offsets is
   */
  [[nodiscard]] static int maxDateOffset() noexcept;
  Settings() = delete;
};
}
}
