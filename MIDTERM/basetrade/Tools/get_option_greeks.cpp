#include <iostream>
#include <getopt.h>
#include <string>
#include <cmath>
#include <iomanip>

#include "dvctrade/OptionsHelper/option.hpp"
using namespace HFSAT;

static struct option input_options_[] = {{"help", no_argument, 0, 'h'},
                                         {"type", required_argument, 0, 'a'},
                                         {"strike", required_argument, 0, 'b'},
                                         {"future", required_argument, 0, 'c'},
                                         {"years", required_argument, 0, 'd'},
                                         {"stdev", required_argument, 0, 'e'},
                                         {"rate", required_argument, 0, 'f'},
                                         {"price", required_argument, 0, 'g'},
                                         {0, 0, 0, 0}};

void print_usage(const char* prg_name) {
  printf("This is the option_greeks exec \n");
  printf(
      "Usage: %s --type <1/-1> --strike <option_strike> --future <future_price> --years <years_to_expiry> -- "
      "stdev <annualized_volatility(not in percentage)>  --rate <annulized_interest_rate(not in percentage)>\n",
      prg_name);
    std::cout << "or" << std::endl;
     printf(
      "Usage: %s --type <1/-1> --strike <option_strike> --future <future_price> --years <years_to_expiry> -- "
      "price <option_price> --rate <annulized_interest_rate(not in percentage)>\n",
      prg_name);  


}

int main(int argc, char** argv) {
  double F = -1, K = -1, r = 0, s = 0, price = 0;
  OptionType_t t = HFSAT::CALL;
  double T = 0;
  bool read_F = false, read_K = false, read_T = false, read_r = false, read_s = false, read_t = false, read_P = false;
  int c;
  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "", input_options_, &option_index);
    if (c == -1) break;
    switch (c) {
      case 'h':
        print_usage(argv[0]);
        exit(0);

      case 'a':
        read_t = true;
        if (atoi(optarg) != 1) t = HFSAT::PUT;
        break;

      case 'b':
        read_K = true;
        K = atof(optarg);
        break;

      case 'c':
        read_F = true;
        F = atof(optarg);
        break;

      case 'd':
        read_T = true;
        T = atof(optarg);
        break;

      case 'e':
        read_s = true;
        s = atof(optarg);
        break;

      case 'f':
        read_r = true;
        r = atof(optarg);
        break;

      case 'g':
        read_P = true;
        price = atof(optarg);
        break;

      case '?':
        print_usage(argv[0]);
        exit(0);
        break;
    }
  }

  if (!(read_t && read_K && read_F && read_T && (read_s || read_P) && read_r)) {
    print_usage(argv[0]);
    printf(" All arguments were not specified .. Exiting \n");
    exit(0);
  }

  // currently all prices are assumed positive which holds for NSE options
  if (K < 1e-5 || F < 1e-5 || T < 0 || T > 10000 || (s < 1e-5 && price < 1e-5) || s > 1000 || r < 0 || r > 100) {
    printf("Error in parameter input K = %f,\tF = %f,\tT = %f,\ts = %f,\tr = %f\n", K, F, T, s, r);
    exit(0);
  }
  Option* t_option_;
  if (read_s) {
    t_option_ = new Option(t, K, F, T, s, r, true);
  } else {
    t_option_ = new Option(price, t, K, F, T, r, true);
  }

  t_option_->computeGreeks();

  std::cout << "price " << t_option_->Price() << std::endl;
  std::cout << "vol " << t_option_->Stdev() << std::endl;
  std::cout << "delta " << t_option_->Delta() << std::endl;
  std::cout << "gamma " << t_option_->Gamma() << std::endl;
  std::cout << "theta " << t_option_->Theta() << std::endl;
  std::cout << "vega " << t_option_->Vega() << std::endl;
  std::cout << "rho " << t_option_->Rho() << std::endl;
  std::cout << "vanna " << t_option_->Vanna() << std::endl;
  std::cout << "vomma " << t_option_->Vomma() << std::endl;
  std::cout << "charm " << t_option_->Charm() << std::endl;
}
