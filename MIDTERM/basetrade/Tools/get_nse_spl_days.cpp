/**
 *        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 *             Address:
 *                   Suite No 353, Evoma, #14, Bhattarhalli,
 *                   Old Madras Road, Near Garden City College,
 *                   KR Puram, Bangalore 560049, India
 *                   +91 80 4190 355
 **/

#include <stdio.h>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/Utils/holiday_manager.hpp"
#include "dvccode/Utils/exchange_names.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

void split(const std::string& s, char delim, std::vector<std::string>& elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " segment start_date_YYYYMMDD end_date_YYYYMMDD -e 1 " << std::endl;
    exit(0);
  }
  const char* segment_ = argv[1];
  int start_date_ = atoi(argv[2]);
  int end_date_ = atoi(argv[3]);

  int arg_index_ = 4;

  bool e_flag_ = false;
  int ndays_ = 0;

  std::vector<int> spl_dates_;

  while (arg_index_ < argc) {
    if (strcmp(argv[arg_index_], "-e") == 0) {
      arg_index_++;
      if (atoi(argv[arg_index_]) > 0) {
        ndays_ = atoi(argv[arg_index_]);
        arg_index_++;
        e_flag_ = true;
        continue;
      }
    }
    // dont know how to process the argv[arg_index_]
    arg_index_++;
  }

  if (e_flag_) {
    int start_year_ = start_date_ / 10000;
    int end_year_ = end_date_ / 10000;

    for (int year_ = start_year_; year_ <= end_year_; year_++) {
      std::ostringstream year_oss_;
      year_oss_ << year_;
      std::string nse_expiry_dates_file_ =
          std::string("/spare/local/tradeinfo/NSE_Files/expiry_dates_") + year_oss_.str();
      if (HFSAT::FileUtils::ExistsAndReadable(nse_expiry_dates_file_)) {
        std::ifstream expiry_fstream_;
        expiry_fstream_.open(nse_expiry_dates_file_, std::ifstream::in);
        char readline_buffer_[1024];
        if (expiry_fstream_.is_open()) {
          while (expiry_fstream_.good()) {
            memset(readline_buffer_, 0, sizeof(readline_buffer_));
            expiry_fstream_.getline(readline_buffer_, sizeof(readline_buffer_));
            HFSAT::PerishableStringTokenizer st_(readline_buffer_, sizeof(readline_buffer_));
            const std::vector<const char*>& tokens_ = st_.GetTokens();
            if (tokens_.size() == 2 && tokens_[0][0] != '#') {
              if (strcmp(tokens_[0], segment_) == 0) {
                HFSAT::VectorUtils::UniqueVectorAdd(spl_dates_, atoi(tokens_[1]));
                int intdate_ = HFSAT::DateTime::CalcPrevWeekDay(atoi(tokens_[1]));
                int i = 1;
                while (i < ndays_) {
                  while (HFSAT::HolidayManagerNoThrow::IsExchangeHoliday(HFSAT::EXCHANGE_KEYS::kExchSourceNSEStr,
                                                                         intdate_, true)) {
                    intdate_ = HFSAT::DateTime::CalcPrevWeekDay(intdate_);
                  }
                  HFSAT::VectorUtils::UniqueVectorAdd(spl_dates_, intdate_);
                  intdate_ = HFSAT::DateTime::CalcPrevWeekDay(intdate_);
                  i++;
                }
              }
            }
          }
        }
      }
    }
  }

  for (auto i = 0u; i < spl_dates_.size(); i++) {
    std::cout << spl_dates_[i] << " ";
  }
  std::cout << "\n";
}
