/* delta_hedge_bounds.hpp
 *
 *  Created on: 09-Dec-2015
 *      Author: raghuram
 */

#ifndef TOOLS_DELTA_HEDGE_BOUNDS_HPP_
#define TOOLS_DELTA_HEDGE_BOUNDS_HPP_
#include <vector>

#include "OptionTools/Greeks/option.hpp"

namespace HFSAT {
class Delta_bounds {
  double cost_;
  double risk_aversion_;
  OptionType_t t = HFSAT::CALL;
  double K = 0;
  double F = 0;
  double T = 0;
  double Sigma = 0;
  double R = 0;
  double Gamma = 0;
  double S = 0;

  double Sigma_m = 0;
  double H_0 = 0;
  double H_w = 0;
  double H_sigma = 0;

  double lower_bound = 0;
  double upper_bound = 0;

  double Delta = 0;
  double Delta_m = 0;

 public:
  Delta_bounds(const double cost, const double risk_aversion) : cost_(cost), risk_aversion_(risk_aversion) {}
  std::vector<double> get_limits(Option& t_option, std::string method = "A") {
    t_option.computeGreeks();

    t = t_option.Type();
    K = t_option.Strike();
    F = t_option.Future();
    T = t_option.Days() / 365.0;
    Sigma = t_option.Stdev();
    R = t_option.Rate();
    Gamma = t_option.Gamma();
    S = F * std::exp(-R * T);
    Delta = t_option.Delta();

    if (method.compare("B") == 0) {
      H_0 = (cost_ / risk_aversion_ * S * Sigma * Sigma * T);
      H_w = 1.08 * std::pow(cost_, 0.31) * std::pow(Gamma / risk_aversion_, 0.5) / std::pow(Sigma, 0.25);
      H_sigma = 6.85 * std::pow(cost_, 0.78) * std::pow(risk_aversion_ * S * S * Gamma, 0.15) / std::pow(Sigma, 0.25);

      Sigma_m = std::pow(Sigma * Sigma * (1 + H_sigma), 0.5);

      Option temp_option(t, K, F, t_option.Days(), Sigma_m, R);
      temp_option.computeGreeks();
      Delta_m = temp_option.Delta();

      lower_bound = Delta_m - (H_w + H_0);
      upper_bound = Delta_m + (H_w + H_0);

      std::vector<double> bounds = {lower_bound, upper_bound, Delta};

      return (bounds);
    }

    if (method.compare("A") == 0) {
      H_0 = 0;
      H_w = std::pow(1.5 * std::exp(-R * T) * cost_ * S * Gamma * Gamma / risk_aversion_, 1.0 / 3.0);
      H_sigma = 0;

      Sigma_m = std::pow(Sigma * Sigma * (1 + H_sigma), 0.5);

      Option temp_option(t, K, F, t_option.Days(), Sigma_m, R);
      temp_option.computeGreeks();
      Delta_m = temp_option.Delta();

      lower_bound = Delta_m - (H_w + H_0);
      upper_bound = Delta_m + (H_w + H_0);

      std::vector<double> bounds = {lower_bound, upper_bound, Delta};

      return (bounds);
    }
  }
};
}
#endif /* TOOLS_DELTA_HEDGE_BOUNDS_HPP_ */
