#include <iostream>
#include <getopt.h>
#include <string>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <math.h>
#include <vector>

#include "basetrade/OptionTools/ImpliedVolatility/lets_be_rational.hpp"

#define NUM_ITER 10

static struct option input_options_[] = {{"help", no_argument, 0, 'h'},
                                         {"price", required_argument, 0, 'a'},
                                         {"forward", required_argument, 0, 'b'},
                                         {"time_period", required_argument, 0, 'c'},
                                         {"strike", required_argument, 0, 'd'},
                                         {"type", required_argument, 0, 'e'},
                                         {"interest_rate", required_argument, 0, 'f'},
                                         {"dividends", required_argument, 0, 'g'},
                                         {0, 0, 0, 0}};

void print_usage(const char* prg_name) {
  printf("This is the get_implied_volatility exec \n");
  printf("Usage: %s --option <optional_val> [ --option <optional_val> ... ]\n", prg_name);
  printf("Valid options are:\n");
  printf("(1) This will return Implied Volatility Sigma: [ Beta, K, F, q, T ] -> Sigma\n");
  printf(
      "--price < option_price > --strike < strike_px > --forward < fwd_price > --type < 1/-1 > --time_period < "
      "in_fraction_years > --interest_rate =0 < in percentage > --dividends ="
      " < t_0 d_0 t_1 d_1 ...> \n");
  printf("(2) This will return Variance_Sqrt : [ Beta, K, F, q ] -> Sigma * Sqrt ( T ) \n");
  printf("--price < option_price > --strike < strike_px > --forward < fwd_price > --type < 1/-1 > \n");
}

bool CheckInputValidity(double beta, double x, double q) {
#define DELTA 0.00001
  if (!(fabs(q - 1) < DELTA || fabs(q + 1) < DELTA)) {
    std::cout << "Type must be either 1 ( for call ) or -1 ( for put ) \n";
    return false;
  }

  double lower_bound = 0;
  double upper_bound = exp(x / 2);

  // Call Option
  if (q > 0) {
    upper_bound = exp(x / 2);

    // ln(F/K) > 0 => F > K
    if (x > 0) {
      lower_bound = exp(x / 2) - exp(-x / 2);
    }
    // F < K
    else {
      lower_bound = 0;
    }
  }
  // Put Option
  else {
    upper_bound = exp(-x / 2);

    // F > K
    if (x > 0) {
      lower_bound = 0;
    }
    // F < K
    else {
      lower_bound = exp(-x / 2) - exp(x / 2);
    }
  }

  if (!(lower_bound <= beta && beta <= upper_bound)) {
    std::cout << std::setprecision(9) << "For given normalized_strike_px : " << x << " , value of beta must be between "
              << lower_bound << " and " << upper_bound << " while Input Beta( option_px/sqrt(F*K) ) : " << beta << "\n";
    return false;
  }
  return true;
}

int main(int argc, char** argv) {
  double F = -1.0, K = -1.0, q = 0, beta = -1.0, T = -1.0, R = 0, S = 0;
  bool read_F = false, read_K = false, read_q = false, read_beta = false, read_T = false;
  std::vector<double> dividends_time = {};
  std::vector<double> dividends_value = {};

  int c;
  /// parse input options
  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "", input_options_, &option_index);
    if (c == -1) break;
    switch (c) {
      case 'h':
        print_usage(argv[0]);
        exit(0);

      case 'a':
        read_beta = true;
        beta = atof(optarg);
        break;

      case 'b':
        read_F = true;
        F = atof(optarg);
        break;

      case 'c':
        read_T = true;
        T = atof(optarg);
        break;

      case 'd':
        read_K = true;
        K = atof(optarg);
        break;

      case 'e':
        read_q = true;
        q = atof(optarg);
        break;
      case 'f':
        R = atof(optarg) / 100;
        break;
      case 'g': {
        std::istringstream temp_iss(optarg);
        std::string date_token = "";
        std::string value_token = "";
        while (std::getline(temp_iss, date_token, ' ')) {
          dividends_time.push_back(atof(date_token.c_str()));
          std::getline(temp_iss, value_token, ' ');
          dividends_value.push_back(atof(value_token.c_str()));
        }
      } break;
      case '?':
        print_usage(argv[0]);
        exit(0);
        break;
    }
  }

  if (read_beta == false || read_q == false || read_F == false || read_K == false) {
    std::cout << " Error: beta, type, forward and strike interest_rate =0, dividends = null (date_0, value_0, date_1, "
                 "value_1... ) must be given as input..Exiting \n\n";
    print_usage(argv[0]);
    exit(0);
  }

  if (F <= 0 || K <= 0) {
    std::cout << "Error: Forward px or Strike px can't be less than/ equal to zero \n";
    exit(0);
  }

  S = F * exp(-R * T);
  double adjusted_K = K;

  for (auto i = 0u; i < dividends_time.size(); i++) {
    S += dividends_value[i] * exp(-R * dividends_time[i]);
    adjusted_K += dividends_value[i] * exp(R * (T - dividends_time[i]));
  }

  if (dividends_time.size() > 0) {
    F = S * std::exp(R * T);
    K = adjusted_K;
  }
  double normalized_strike_px = log(F / K);

  // discounting the price
  beta *= std::exp(R * T);

  // normalizing Beta
  beta /= sqrt(F * K);

  if (!CheckInputValidity(beta, normalized_strike_px, q)) {
    exit(0);
  }

  double variance_sqrt = HFSAT::NormalisedImpliedVolatilityFromTransformedRationalGuessWithLimitedIterations(
      beta,                  // option price
      normalized_strike_px,  // normalized strike px
      q,                     // 1/-1 -> call or put
      NUM_ITER               // num of interations
      );

  // Input type (1), will retun Volatility ( Sigma )
  if (read_T) {
    double sqrt_T = (double)sqrt(T);
    std::cout << "Volatility ( sigma ): " << variance_sqrt / sqrt_T << "\n";
  }
  // Input type (2)
  else {
    std::cout << "Variance_Sqrt ( sigma * sqrt(T) ): " << variance_sqrt << "\n";
  }

  // std::cout << "Beta : " << HFSAT::NormalisedBlack( normalized_strike_px, variance_sqrt, q ) << "\n";

  return 0;
}
