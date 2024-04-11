/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
namespace tbd::data
{
/**
 * \brief A wrapper around a double to ensure correct types are used.
 * \tparam T The derived class that this Index represents.
 */
template <class T>
class Index
{
  /**
   * \brief Value represented by this
   */
  double value_;
public:
  /**
   * \brief Destructor
   */
  ~Index() = default;
  /**
   * \brief Construct with a value of 0
   */
  constexpr Index() noexcept
    : value_(0)
  {
  }
  /**
   * \brief Construct with given value
   * \param value Value to assign
   */
  constexpr explicit Index(const double value) noexcept
    : value_(value)
  {
  }
  /**
   * \brief Move constructor
   * \param rhs Index to move from
   */
  constexpr Index(Index<T>&& rhs) noexcept = default;
  /**
   * \brief Copy constructor
   * \param rhs Index to copy from
   */
  constexpr Index(const Index<T>& rhs) noexcept = default;
  /**
   * \brief Move assignment
   * \param rhs Index to move from
   * \return This, after assignment
   */
  Index<T>& operator=(Index<T>&& rhs) noexcept = default;
  /**
   * \brief Copy assignment
   * \param rhs Index to copy from
   * \return This, after assignment
   */
  Index<T>& operator=(const Index<T>& rhs) noexcept = default;
  /**
   * \brief Equality operator
   * \param rhs Index to compare to
   * \return Whether or not these are equivalent
   */
  [[nodiscard]] constexpr bool operator==(const Index<T>& rhs) const noexcept
  {
    return value_ == rhs.value_;
  }
  /**
   * \brief Not equals operator
   * \param rhs Index to compare to
   * \return Whether or not these are not equivalent
   */
  [[nodiscard]] constexpr bool operator!=(const Index<T>& rhs) const noexcept
  {
    return !(*this == rhs);
  }
  /**
   * \brief Returns value as a double
   * \return double value for Index
   */
  [[nodiscard]] constexpr double asDouble() const noexcept
  {
    return value_;
  }
  /**
   * \brief Less than operator
   * \param rhs Index to compare to
   * \return Whether or not this is less than the provided Index
   */
  constexpr bool operator<(const Index<T> rhs) const noexcept
  {
    return value_ < rhs.value_;
  }
  /**
   * \brief Greater than operator
   * \param rhs Index to compare to
   * \return Whether or not this is greater than the provided Index
   */
  [[nodiscard]] constexpr bool operator>(const Index<T> rhs) const noexcept
  {
    return value_ > rhs.value_;
  }
  /**
   * \brief Less than or equal to operator
   * \param rhs Index to compare to
   * \return Whether or not this is less than or equal to the provided Index
   */
  [[nodiscard]] constexpr bool operator<=(const Index<T> rhs) const noexcept
  {
    return value_ <= rhs.value_;
  }
  /**
   * \brief Greater than or equal to operator
   * \param rhs Index to compare to
   * \return Whether or not this is greater than or equal to the provided Index
   */
  [[nodiscard]] constexpr bool operator>=(const Index<T> rhs) const noexcept
  {
    return value_ >= rhs.value_;
  }
  /**
   * \brief Addition operator
   * \param rhs Index to add value from
   * \return The value of this plus the value of the provided index
   */
  [[nodiscard]] constexpr Index<T> operator+(const Index<T> rhs) const noexcept
  {
    return Index<T>(value_ + rhs.value_);
  }
  /**
   * \brief Subtraction operator
   * \param rhs Index to add value from
   * \return The value of this minus the value of the provided index
   */
  [[nodiscard]] constexpr Index<T> operator-(const Index<T> rhs) const noexcept
  {
    return Index<T>(value_ - rhs.value_);
  }
  /**
   * \brief Addition assignment operator
   * \param rhs Index to add value from
   * \return This, plus the value of the provided Index
   */
  constexpr Index<T>& operator+=(const Index<T> rhs) noexcept
  {
    value_ += rhs.value_;
    return *this;
  }
  /**
   * \brief Subtraction assignment operator
   * \param rhs Index to add value from
   * \return This, minus the value of the provided Index
   */
  constexpr Index<T>& operator-=(const Index<T> rhs) noexcept
  {
    value_ -= rhs.value_;
    return *this;
  }
};
/**
 * \brief A result of calling log(x) for some value of x, pre-calculated at compile time.
 */
class LogValue
  : public Index<LogValue>
{
public:
  //! @cond Doxygen_Suppress
  using Index::Index;
  //! @endcond
};
// FIX: why is this log10 and not just log?????
// static constexpr LogValue LOG_0_7{-0.15490195998574316928778374140736};
// static constexpr LogValue LOG_0_75{-0.12493873660829995313244988619387};
// static constexpr LogValue LOG_0_8{-0.09691001300805641435878331582652};
// static constexpr LogValue LOG_0_85{-0.07058107428570726667356900039616};
// static constexpr LogValue LOG_0_9{-0.04575749056067512540994419348977};
// static constexpr LogValue LOG_1_0{0};
static constexpr LogValue LOG_0_7{log(0.7)};
static constexpr LogValue LOG_0_75{log(0.75)};
static constexpr LogValue LOG_0_8{log(0.8)};
static constexpr LogValue LOG_0_85{log(0.85)};
static constexpr LogValue LOG_0_9{log(0.9)};
static constexpr LogValue LOG_1_0{log(1.0)};
}
