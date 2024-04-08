/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "stdafx.h"
#include "Cell.h"
namespace tbd::sim
{
/**
 * \brief A specific Event scheduled in a specific Scenario.
 */
class Event
{
public:
  /**
   * \brief Cell representing no location
   */
  static constexpr topo::Cell NoLocation{};
  // HACK: use type, so we can sort without having to give different times to them
  /**
   * \brief Type of Event
   */
  enum Type
  {
    SAVE,
    END_SIMULATION,
    NEW_FIRE,
    FIRE_SPREAD,
  };
  /**
   * \brief Make simulation end event
   * \param time Time to schedule for
   * \return Event created
   */
  [[nodiscard]] static constexpr Event makeEnd(const double time)
  {
    return {time, NoLocation, 0, END_SIMULATION, 0, 0};
  }
  /**
   * \brief Make new fire event
   * \param time Time to schedule for
   * \param cell Cell to start new fire in
   * \return Event created
   */
  [[nodiscard]] static Event constexpr makeNewFire(const double time,
                                                   const topo::Cell& cell)
  {
    return {time, cell, 0, NEW_FIRE, 0, 0};
  }
  /**
   * \brief Make simulation save event
   * \param time Time to schedule for
   * \return Event created
   */
  [[nodiscard]] static Event constexpr makeSave(const double time)
  {
    return {time, NoLocation, 0, SAVE, 0, 0};
  }
  /**
   * \brief Make fire spread event
   * \param time Time to schedule for
   * \return Event created
   */
  [[nodiscard]] static Event constexpr makeFireSpread(const double time)
  {
    return makeFireSpread(time, 0);
  }
  /**
   * \brief Make fire spread event
   * \param time Time to schedule for
   * \param intensity Intensity to spread with (kW/m)
   * \return Event created
   */
  [[nodiscard]] static Event constexpr makeFireSpread(
    const double time,
    const IntensitySize intensity)
  {
    return makeFireSpread(time, intensity, NoLocation);
  }
  /**
   * \brief Make fire spread event
   * \param time Time to schedule for
   * \param intensity Intensity to spread with (kW/m)
   * \param cell Cell to spread in
   * \return Event created
   */
  [[nodiscard]] static Event constexpr makeFireSpread(
    const double time,
    const IntensitySize intensity,
    const topo::Cell& cell)
  {
    return makeFireSpread(time, intensity, cell, 254);
  }
  /**
   * \brief Make fire spread event
   * \param time Time to schedule for
   * \param intensity Intensity to spread with (kW/m)
   * \param cell Cell to spread in
   * \return Event created
   */
  [[nodiscard]] static Event constexpr makeFireSpread(
    const double time,
    const IntensitySize intensity,
    const topo::Cell& cell,
    const CellIndex source)
  {
    return {time, cell, source, FIRE_SPREAD, intensity, 0};
  }
  ~Event() = default;
  /**
   * \brief Move constructor
   * \param rhs Event to move from
   */
  Event(Event&& rhs) noexcept = default;
  /**
   * \brief Copy constructor
   * \param rhs Event to copy from
   */
  Event(const Event& rhs) = delete;
  /**
   * \brief Move assignment
   * \param rhs Event to move from
   * \return This, after assignment
   */
  Event& operator=(Event&& rhs) noexcept = default;
  /**
   * \brief Copy assignment
   * \param rhs Event to copy from
   * \return This, after assignment
   */
  Event& operator=(const Event& rhs) = delete;
  /**
   * \brief Time of Event (decimal days)
   * \return Time of Event (decimal days)
   */
  [[nodiscard]] constexpr double time() const
  {
    return time_;
  }
  /**
   * \brief Type of Event
   * \return Type of Event
   */
  [[nodiscard]] constexpr Type type() const
  {
    return type_;
  }
  /**
   * \brief Duration that Event Cell has been burning (decimal days)
   * \return Duration that Event Cell has been burning (decimal days)
   */
  [[nodiscard]] constexpr double timeAtLocation() const
  {
    return time_at_location_;
  }
  /**
   * \brief Burn Intensity (kW/m)
   * \return Burn Intensity (kW/m)
   */
  [[nodiscard]] constexpr IntensitySize intensity() const
  {
    return intensity_;
  }
  /**
   * \brief Cell Event takes place in
   * \return Cell Event takes place in
   */
  [[nodiscard]] constexpr const topo::Cell& cell() const
  {
    return cell_;
  }
  /**
   * \brief CellIndex for relative Cell that spread into from
   * \return CellIndex for relative Cell that spread into from
   */
  [[nodiscard]] constexpr CellIndex source() const
  {
    return source_;
  }
private:
  /**
   * \brief Constructor
   * \param time Time to schedule for
   * \param cell CellIndex for relative Cell that spread into from
   * \param source Source that Event is coming from
   * \param type Type of Event
   * \param intensity Intensity to spread with (kW/m)
   * \param time_at_location Duration that Event Cell has been burning (decimal days)
   */
  constexpr Event(const double time,
                  const topo::Cell& cell,
                  const CellIndex source,
                  const Type type,
                  const IntensitySize intensity,
                  const double time_at_location)
    : time_(time),
      time_at_location_(time_at_location),
      cell_(cell),
      type_(type),
      intensity_(intensity),
      source_(source)
  {
  }
  /**
   * \brief Time to schedule for
   */
  double time_;
  /**
   * \brief Duration that Event Cell has been burning (decimal days)
   */
  double time_at_location_;
  /**
   * \brief Cell to spread in
   */
  topo::Cell cell_;
  /**
   * \brief Type of Event
   */
  Type type_;
  /**
   * \brief Burn Intensity (kW/m)
   */
  IntensitySize intensity_;
  /**
   * \brief CellIndex for relative Cell that spread into from
   */
  CellIndex source_;
};
}
