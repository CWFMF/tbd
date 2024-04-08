// Copyright (c) 2020-2021, Queen's Printer for Ontario.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once
#include <algorithm>
#include <numeric>
#include <vector>
#include "Log.h"
#include "Settings.h"
#include "Util.h"
namespace tbd
{
namespace util
{
/**
 * \brief Student's T critical values
 */
static constexpr array<double, 100> T_VALUES{
  3.078,
  1.886,
  1.638,
  1.533,
  1.476,
  1.440,
  1.415,
  1.397,
  1.383,
  1.372,
  1.363,
  1.356,
  1.350,
  1.345,
  1.341,
  1.337,
  1.333,
  1.330,
  1.328,
  1.325,
  1.323,
  1.321,
  1.319,
  1.318,
  1.316,
  1.315,
  1.314,
  1.313,
  1.311,
  1.310,
  1.309,
  1.309,
  1.308,
  1.307,
  1.306,
  1.306,
  1.305,
  1.304,
  1.304,
  1.303,
  1.303,
  1.302,
  1.302,
  1.301,
  1.301,
  1.300,
  1.300,
  1.299,
  1.299,
  1.299,
  1.298,
  1.298,
  1.298,
  1.297,
  1.297,
  1.297,
  1.297,
  1.296,
  1.296,
  1.296,
  1.296,
  1.295,
  1.295,
  1.295,
  1.295,
  1.295,
  1.294,
  1.294,
  1.294,
  1.294,
  1.294,
  1.293,
  1.293,
  1.293,
  1.293,
  1.293,
  1.293,
  1.292,
  1.292,
  1.292,
  1.292,
  1.292,
  1.292,
  1.292,
  1.292,
  1.291,
  1.291,
  1.291,
  1.291,
  1.291,
  1.291,
  1.291,
  1.291,
  1.291,
  1.291,
  1.290,
  1.290,
  1.290,
  1.290,
  1.290};
/**
 * \brief Provides statistics calculation for vectors of values.
 */
class Statistics
{
public:
  /**
   * \brief Minimum value
   * \return Minimum value
   */
  [[nodiscard]] double min() const noexcept
  {
    return percentiles_[0];
  }
  /**
   * \brief Maximum value
   * \return Maximum value
   */
  [[nodiscard]] double max() const noexcept
  {
    return percentiles_[100];
  }
  /**
   * \brief Median value
   * \return Median value
   */
  [[nodiscard]] double median() const noexcept
  {
    return percentiles_[50];
  }
  /**
   * \brief Mean (average) value
   * \return Mean (average) value
   */
  [[nodiscard]] double mean() const noexcept
  {
    return mean_;
  }
  /**
   * \brief Standard Deviation
   * \return Standard Deviation
   */
  [[nodiscard]] double standardDeviation() const noexcept
  {
    return standard_deviation_;
  }
  /**
   * \brief Sample Variance
   * \return Sample Variance
   */
  [[nodiscard]] double sampleVariance() const noexcept
  {
    return sample_variance_;
  }
  /**
   * \brief Number of data points in the set
   * \return Number of data points in the set
   */
  [[nodiscard]] size_t n() const noexcept
  {
    return n_;
  }
  /**
   * \brief Value for given percentile
   * \param i Percentile to retrieve value for
   * \return Value for given percentile
   */
  [[nodiscard]] double percentile(const uint8_t i) const noexcept
  {
#ifdef DEBUG_STATISTICS
    logging::check_fatal(static_cast<size_t>(i) >= percentiles_.size(),
                         "Invalid percentile %d requested",
                         i);
#endif
    return percentiles_.at(i);
  }
  /**
   * \brief 80% Confidence Interval
   * \return 80% Confidence Interval
   */
  [[nodiscard]] double confidenceInterval80() const
  {
    return confidenceInterval(1.28);
  }
  /**
   * \brief 90% Confidence Interval
   * \return 90% Confidence Interval
   */
  [[nodiscard]] double confidenceInterval90() const
  {
    return confidenceInterval(1.645);
  }
  /**
   * \brief 95% Confidence Interval
   * \return 95% Confidence Interval
   */
  [[nodiscard]] double confidenceInterval95() const
  {
    return confidenceInterval(1.96);
  }
  /**
   * \brief 98% Confidence Interval
   * \return 98% Confidence Interval
   */
  [[nodiscard]] double confidenceInterval98() const
  {
    return confidenceInterval(2.33);
  }
  /**
   * \brief 99% Confidence Interval
   * \return 99% Confidence Interval
   */
  [[nodiscard]] double confidenceInterval99() const
  {
    return confidenceInterval(2.58);
  }
  /**
   * \brief Calculates statistics on a vector of values
   * \param values Values to use for calculation
   */
  explicit Statistics(vector<double> values)
  {
    // values should already be sorted
    //  std::sort(values.begin(), values.end());
    n_ = values.size();
    min_ = values[0];
    max_ = values[n_ - 1];
    median_ = values[n_ / 2];
    const auto total_sum = std::accumulate(values.begin(),
                                           values.end(),
                                           0.0,
                                           [](const double t, const double x) { return t + x; });
    mean_ = total_sum / n_;
    for (size_t i = 0; i < percentiles_.size(); ++i)
    {
      const auto pos = std::min(n_ - 1,
                                static_cast<size_t>(truncl(
                                  (static_cast<double>(i) / (percentiles_.size() - 1)) * n_)));
      // note("For %d values %dth percentile is at %d", n_, i, pos);
      percentiles_[i] = values[pos];
    }
    const auto total = std::accumulate(values.begin(),
                                       values.end(),
                                       0.0,
                                       [this](const double t, const double x) { return t + pow_int<2>(x - mean_); });
    standard_deviation_ = sqrt(total / n_);
    sample_variance_ = total / (n_ - 1);
#ifdef DEBUG_STATISTICS
    logging::check_fatal(min_ != percentiles_[0],
                         "Expected min to be %f not %f",
                         min_,
                         percentiles_[0]);
    logging::check_fatal(max_ != percentiles_[100],
                         "Expected max to be %f not %f",
                         max_,
                         percentiles_[100]);
    logging::check_fatal(median_ != percentiles_[50],
                         "Expected median to be %f not %f",
                         median_,
                         percentiles_[50]);
#endif
  }
  /**
   * \brief Calculate Student's T value
   * \return Student's T value
   */
  [[nodiscard]] double studentsT() const noexcept
  {
    const auto result = T_VALUES[std::min(T_VALUES.size(), n()) - 1]
                      * sqrt(sampleVariance() / n()) / abs(mean());
    // printf("%ld %f %f %f\n", n(), mean(), sampleVariance(), result);
    return result;
  }
  /**
   * \brief Whether or not we have less than the relative error and can be confident in the results
   * \param relative_error Relative Error that is required
   * \return If Student's T value is less than the relative error
   */
  [[nodiscard]] bool isConfident(const double relative_error) const noexcept
  {
    const auto st = studentsT();
    const auto re = relative_error / (1 + relative_error);
    // printf("%f <= %f is %s\n", st, re, ((st <= re) ? "true" : "false"));
    return st <= re;
  }
  /**
   * \brief Estimate how many more runs are required to achieve desired confidence
   * \param cur_runs Current number of runs completed
   * \param relative_error Relative Error to achieve to be confident
   * \return Number of runs still required
   */
  [[nodiscard]] size_t runsRequired(
    // const size_t cur_runs,
    const double relative_error) const
  {
    const auto re = relative_error / (1 + relative_error);
    const std::function<double(size_t)> fct = [this](const size_t i) noexcept {
      return T_VALUES[std::min(T_VALUES.size(), i) - 1]
           * sqrt(sampleVariance() / i) / abs(mean());
    };
    const auto cur_runs = n();
    return binary_find_checked(cur_runs, 10 * cur_runs, re, fct) - cur_runs;
  }
private:
  /**
   * \brief Calculate Confidence Interval for given z value
   * \param z Z value to calculate for
   * \return Confidence Interval
   */
  [[nodiscard]] double confidenceInterval(const double z) const
  {
    return z * mean_ / sqrt(n_);
  }
  /**
   * \brief Number of values
   */
  size_t n_;
  /**
   * \brief Minimum value
   */
  double min_;
  /**
   * \brief Maximum value
   */
  double max_;
  /**
   * \brief Mean (average) value
   */
  double mean_;
  /**
   * \brief Median value
   */
  double median_;
  /**
   * \brief Standard Deviation
   */
  double standard_deviation_;
  /**
   * \brief Sample variance
   */
  double sample_variance_;
  /**
   * \brief Array of all integer percentile values
   */
  array<double, 101> percentiles_{};
};
}
}
