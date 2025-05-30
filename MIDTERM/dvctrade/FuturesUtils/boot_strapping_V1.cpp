/**
   \file FuturesUtils/boot_strapping_V1.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <string.h>
#include <stdio.h>

#include <boost/date_time/gregorian/gregorian.hpp>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

// BASETRADEINFO
// ignoring value_date for now -> once the rate is fixed ( on expiry date, it comes into existence from +2 biz days
// usually )
// the term is taken to be 3 calender months :: investigate if simple 90 days works better

int main(int argc, char** argv) {
  if (argc < 5) {
    std::cerr << "Usage: " << argv[0]
              << " basename_ input_date_ getthisexpirypx_ fromthisexpirypx1_ fromthisexpirypx2_ " << std::endl;
    exit(0);
  }

  std::string basename_ = argv[1];

  std::string yyyymmdd_ = argv[2];
  boost::gregorian::date input_date_(boost::gregorian::from_undelimited_string(argv[2]));

  int getthisexpirypx_ = atoi(argv[3]);  // basically array index
  int fromthisexpirypx1_ = atoi(argv[4]);
  int fromthisexpirypx2_ = atoi(argv[5]);

  std::vector<boost::gregorian::date> last_trading_date_;
  boost::gregorian::date ltd_;

  std::vector<double> prices_map_;

  std::vector<int> stub_;
  std::vector<int> term_;
  int total_day_count_ = 36500;

  std::string t_ltd_filename_ = "";
  std::string t_px_filename_ = "";

  std::ostringstream t_temp_oss_;
  std::ifstream t_ltd_file_;
  std::ifstream t_px_file_;

  boost::gregorian::months m1(1);
  boost::gregorian::months m2(2);
  boost::gregorian::months m3(3);

  int nSpots = 3;
  int nFutures = 8;

  stub_.push_back(0);
  term_.push_back((((input_date_ + m1) - input_date_).days()));
  last_trading_date_.push_back(input_date_);

  stub_.push_back(0);
  term_.push_back((((input_date_ + m2) - input_date_).days()));
  last_trading_date_.push_back(input_date_);

  stub_.push_back(0);
  term_.push_back((((input_date_ + m3) - input_date_).days()));
  last_trading_date_.push_back(input_date_);

  t_temp_oss_ << std::string(BASETRADEINFODIR) << "TMX/tmx-bax-ltd.txt";
  t_ltd_filename_ = t_temp_oss_.str();

  if (!HFSAT::FileUtils::exists(t_ltd_filename_)) {
    std::cerr << "there is no last trading dates holder file \n";
    exit(0);
  } else {
    t_ltd_file_.open(t_ltd_filename_.c_str(), std::ifstream::in);
    if (t_ltd_file_.is_open()) {
      char line_buffer_[1024];
      std::string line_read_ = "";

      while (t_ltd_file_.good()) {
        memset(line_buffer_, 0, 1024);
        line_read_ = "";
        t_temp_oss_.str(std::string());
        t_temp_oss_.clear();

        t_ltd_file_.getline(line_buffer_, 1024);
        line_read_ = line_buffer_;

        if (line_read_.find("#") != std::string::npos) continue;  // skip comments

        HFSAT::PerishableStringTokenizer st_(line_buffer_, 1024);
        const std::vector<const char*>& sub_tokens_ = st_.GetTokens();

        if (sub_tokens_.size() == 1 && strlen(sub_tokens_[0]) == 8 && atoi(sub_tokens_[0]) > atoi(yyyymmdd_.c_str())) {
          ltd_ = boost::gregorian::from_undelimited_string(sub_tokens_[0]);

          last_trading_date_.push_back(ltd_);
          stub_.push_back((ltd_ - input_date_).days());         // stub
          term_.push_back(((ltd_ + m3) - input_date_).days());  // term
        }
      }

      t_ltd_file_.close();
    }
  }

  // read pricec
  t_temp_oss_ << std::string(BASETRADEINFODIR) << "bax_lpx.csv";
  t_px_filename_ = t_temp_oss_.str();

  if (!HFSAT::FileUtils::exists(t_px_filename_)) {
    std::cerr << "there is no prices holder file \n";
    exit(0);
  } else {
    t_px_file_.open(t_px_filename_.c_str(), std::ifstream::in);
    if (t_px_file_.is_open()) {
      char line_buffer_[1024];
      std::string line_read_ = "";

      while (t_px_file_.good()) {
        memset(line_buffer_, 0, 1024);

        t_px_file_.getline(line_buffer_, 1024);
        line_read_ = line_buffer_;

        if (line_read_.find("#") != std::string::npos || line_read_.find(yyyymmdd_) == std::string::npos)
          continue;  // skip comments

        HFSAT::PerishableStringTokenizer st_(line_buffer_, 1024);
        const std::vector<const char*>& sub_tokens_ = st_.GetTokens();

        if (sub_tokens_.size() == (nSpots + nFutures + 1)) {
          for (unsigned int i = 1; i <= nSpots; i++) {
            prices_map_.push_back(atof(sub_tokens_[i]));
          }

          for (unsigned int i = nSpots + 1; i < (nSpots + nFutures + 1); i++) {
            prices_map_.push_back(100 - atof(sub_tokens_[i]));
          }
        }
      }

      t_px_file_.close();
    }
  }

  std::vector<double> df_forward_;
  std::vector<double> df_stub_;
  std::vector<double> df_;
  std::vector<double> zero_rate_;

  for (unsigned int j = 0; j < prices_map_.size(); j++) {
    //      std::cout << j << std::endl ;

    df_forward_.push_back(1 / (1 + (prices_map_[j]) * (term_[j] - stub_[j]) / (total_day_count_)));

    //      std::cout << "df_forward_ " << df_forward_[ j ] << std::endl ;

    if (stub_[j] == 0) {
      //  std::cout << "stub_ " << stub_[ j ] << std::endl ;
      df_stub_.push_back(1.0);
      df_.push_back(df_stub_[j] * df_forward_[j]);
      zero_rate_.push_back(prices_map_[j]);

    } else {
      //  std::cout << "stub_ " << stub_[ j ] << std::endl ;
      // hopefully term is already sorted
      double inter_rate_ = 0.0;

      unsigned int i = j - 1;
      while (i >= 0) {
        // not interpolating/extrpolating between 0 and first spot, instead using available spot
        if (i == 0 && stub_[j] <= term_[i]) {
          inter_rate_ = zero_rate_[i];
          df_stub_.push_back(1 / (1 + (inter_rate_ * stub_[j]) / (total_day_count_)));
          df_.push_back(df_stub_[j] * df_forward_[j]);
          zero_rate_.push_back((1 / df_[j] - 1) * total_day_count_ / term_[j]);
          break;
          // std::cerr << "error :: all cases not handled \n" ;
          // return 0 ;
        } else if (stub_[j] > term_[i]) {
          if (j - i == 1)  // extrpolate i and i - 1
          {
            inter_rate_ = zero_rate_[i - 1] +
                          ((zero_rate_[i] - zero_rate_[i - 1]) / (term_[i] - term_[i - 1])) * (stub_[j] - term_[i - 1]);
            df_stub_.push_back(1 / (1 + (inter_rate_ * stub_[j]) / (total_day_count_)));
            df_.push_back(df_stub_[j] * df_forward_[j]);
            zero_rate_.push_back((1 / df_[j] - 1) * total_day_count_ / term_[j]);
            break;
          } else  // interpolate between i and i + 1
          {
            inter_rate_ = zero_rate_[i] +
                          ((zero_rate_[i + 1] - zero_rate_[i]) / (term_[i + 1] - term_[i])) * (stub_[j] - term_[i]);
            df_stub_.push_back(1 / (1 + (inter_rate_ * stub_[j]) / (total_day_count_)));
            df_.push_back(df_stub_[j] * df_forward_[j]);
            zero_rate_.push_back((1 / df_[j] - 1) * total_day_count_ / term_[j]);
            break;
          }

        } else if (stub_[j] == term_[i]) {
          df_stub_.push_back(df_[i]);
          df_.push_back(df_stub_[j] * df_forward_[j]);
          zero_rate_.push_back((1 / df_[j] - 1) * total_day_count_ / term_[j]);
          break;
        }

        i--;
      }
    }
  }

  for (int i = 0; i < zero_rate_.size(); i++) {
    std::cout << stub_[i] << "\t" << term_[i] << "\t" << df_stub_[i] << "\t" << df_forward_[i] << "\t" << df_[i] << "\t"
              << zero_rate_[i] << std::endl;
  }

  return 1;
}
