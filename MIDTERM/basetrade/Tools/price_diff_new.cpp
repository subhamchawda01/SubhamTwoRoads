#include <iostream>
#include <fstream>

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225

int main(int argc, char **argv) {
  if (argc < 4) {
    std::cerr << "expecting :\n"
              << " $EXEC SHORTCODE TRADINGDATE START_UNIX_TIME_STAMP END_UNIX_TIME_STAMP" << '\n';
    exit(0);
  }

  std::string _this_shortcode_ = argv[1];

  int tradingdate_ = 0;

  // bool non_self_book_enabled_ = true;

  int start_unix_time_ = 0;
  int end_unix_time_ = 0;

  double start_price = 0;

  tradingdate_ = atoi(argv[2]);
  if ((tradingdate_ < MIN_YYYYMMDD) || (tradingdate_ > MAX_YYYYMMDD)) {
    std::cerr << tradingdate_ << " not logical date" << std::endl;
    exit(0);
  }

  start_unix_time_ = atoi(argv[3]);
  end_unix_time_ = atoi(argv[4]);

  std::string config_file = "/home/ssanapala/";
  config_file.append(_this_shortcode_);
  config_file.append("_sec_marketupdate_");
  config_file.append(argv[2]);

  std::ifstream config;  // format: reference/incremental side_a/side_b IP PORT IP_CODE
  config.open(config_file.c_str());

  int time = 0;
  double mid_price = 0;
  double minimum_price_increment;
  config >> minimum_price_increment;
  // std::cout << start_unix_time_ << std::endl;
  // std::cout << end_unix_time_ << std::endl;
  // std::cout << minimum_price_increment << std::endl;
  while (config >> time) {
    config >> mid_price;
    //    std::cout << mid_price << std::endl;
    if (time == start_unix_time_)
      start_price = mid_price;

    else if (time == end_unix_time_) {
      //    std::cout << mid_price << std::endl;
      std::cout << ((start_price - mid_price) / minimum_price_increment) << std::endl;
      break;
    }
  }
}
