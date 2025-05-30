/**
    \file OptionsUtilsCode/options_security_utils.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

// RELIANCE 20160930
#include "baseinfra/OptionsUtils/security_utils.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"

namespace HFSAT {
void OptionsSecurityUtils::GetMVInfo(const std::string& shortcode_, int yyyymmdd_, const std::string& r_opt_type_,
                                     bool is_currency_, int max_) {
  // /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/mmyy/ddmmfo_0000.md ( yesterdaysdate )
  // /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$todaysdate
  std::string c_last_close_;
  std::string c_expiry_ = "99999999";
  std::string c_num_itm_;
  std::string c_current_stepvalue_;
  std::string c_prev_stepvalue_;
  double c_atm_strike_ = 99999999;
  std::string c_lot_size_;

  std::map<double, double, std::greater<double>> volume_2_distance_;

  std::string b_expiry_;
  std::string b_strike_;
  std::string b_pc_;
  std::string b_volume_;
  std::string b_value_;

  //  int y_yyyymmdd_ = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("NSE", yyyymmdd_);
  std::ostringstream date_string_oss_;
  date_string_oss_ << yyyymmdd_;
  std::string date_ = date_string_oss_.str();

  std::ostringstream nse_contract_specs_file_name_oss_;
  std::string nse_contract_specs_file_name_ = "";
  nse_contract_specs_file_name_oss_ << NSE_CONTRACT_FILENAME_PREFIX << "." << yyyymmdd_;
  nse_contract_specs_file_name_ = std::string("/spare/local/tradeinfo/") + nse_contract_specs_file_name_oss_.str();

  if (FileUtils::ExistsAndReadable(nse_contract_specs_file_name_)) {
    std::ifstream nse_contract_file_;
    nse_contract_file_.open(nse_contract_specs_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_contract_file_.is_open()) {
      while (nse_contract_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_contract_file_.getline(readline_buffer_, sizeof(readline_buffer_));
        PerishableStringTokenizer st_(readline_buffer_, sizeof(readline_buffer_));

        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if (tokens_.size() >= 9) {
          std::string c_shortcode_;
          HFSAT::PerishableStringTokenizer::TrimString(tokens_[1], c_shortcode_, ' ');
          std::string t_expiry_;
          HFSAT::PerishableStringTokenizer::TrimString(tokens_[5], t_expiry_, ' ');
          if (c_shortcode_.compare(shortcode_) == 0 && stoi(c_expiry_) >= stoi(t_expiry_)) {
            c_expiry_ = t_expiry_;
            HFSAT::PerishableStringTokenizer::TrimString(tokens_[2], c_last_close_, ' ');
            HFSAT::PerishableStringTokenizer::TrimString(tokens_[3], c_lot_size_, ' ');
            HFSAT::PerishableStringTokenizer::TrimString(tokens_[6], c_num_itm_, ' ');
            HFSAT::PerishableStringTokenizer::TrimString(tokens_[7], c_current_stepvalue_, ' ');
            HFSAT::PerishableStringTokenizer::TrimString(tokens_[8], c_prev_stepvalue_, ' ');
            c_atm_strike_ = round(stod(c_last_close_) / stod(c_current_stepvalue_)) * stod(c_current_stepvalue_);
          }
        }
      }
    }
    nse_contract_file_.close();
  } else {
    std::cerr << "Fatal error - count could not read NSE contract file " << nse_contract_specs_file_name_ << "\n";
  }

  std::ostringstream nse_bhav_copy_file_name_oss_;
  std::string nse_bhav_copy_file_name_ = "";
  if (is_currency_) {
    nse_bhav_copy_file_name_oss_ << NSE_CD_BHAVCOPY_FILENAME_PREFIX << "/" << date_.substr(4, 2) << date_.substr(2, 2)
                                 << "/" << date_.substr(6, 2) << date_.substr(4, 2) << "cd_0000.md";
  } else {
    nse_bhav_copy_file_name_oss_ << NSE_BHAVCOPY_FILENAME_PREFIX << "/" << date_.substr(4, 2) << date_.substr(2, 2)
                                 << "/" << date_.substr(6, 2) << date_.substr(4, 2) << "fo_0000.md";
  }
  nse_bhav_copy_file_name_ = std::string("/spare/local/tradeinfo/") + nse_bhav_copy_file_name_oss_.str();

  if (FileUtils::ExistsAndReadable(nse_bhav_copy_file_name_)) {
    std::ifstream nse_bhavcopy_file_;
    nse_bhavcopy_file_.open(nse_bhav_copy_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_bhavcopy_file_.is_open()) {
      while (nse_bhavcopy_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_bhavcopy_file_.getline(readline_buffer_, sizeof(readline_buffer_));
        std::vector<char*> tokens_;
        char readline_buffer_copy_[1024];
        memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
        strcpy(readline_buffer_copy_, readline_buffer_);

        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, ",", tokens_);
        // interested in 1, 2, 3, 4, 5, 11, 12
        // OPTSTK RELIANCE 27OCT2016 00001080.00 PE 1124 0000150.33
        if (tokens_.size() >= 15 && tokens_[0][0] != '#') {
          std::string b_type_;
          HFSAT::PerishableStringTokenizer::TrimString(tokens_[1], b_type_, ' ');
          std::string b_shortcode_;
          HFSAT::PerishableStringTokenizer::TrimString(tokens_[2], b_shortcode_, ' ');

          if ((b_type_ == "OPTSTK" || b_type_ == "OPTIDX" || b_type_ == "OPTCUR") &&
              (b_shortcode_.compare(shortcode_) == 0)) {
            b_expiry_ = NSESecurityDefinitions::GetDateInYYYYMMDD(tokens_[3]);
            if (b_expiry_.compare(c_expiry_) == 0) {
              HFSAT::PerishableStringTokenizer::TrimString(tokens_[11], b_volume_, ' ');
              if (stoi(b_volume_) > 0) {
                HFSAT::PerishableStringTokenizer::TrimString(tokens_[4], b_strike_, ' ');
                HFSAT::PerishableStringTokenizer::TrimString(tokens_[5], b_pc_, ' ');
                HFSAT::PerishableStringTokenizer::TrimString(tokens_[12], b_value_, ' ');
                double volume_ = stod(b_volume_) / stod(c_lot_size_);
                double distance_ = (stod(b_strike_) - c_atm_strike_) / stod(c_current_stepvalue_);
                if (b_pc_.compare(r_opt_type_) == 0) volume_2_distance_[volume_] = distance_;
              }
            }
          } else {
            continue;
          }
        }
      }
    }
    nse_bhavcopy_file_.close();
    std::cout << yyyymmdd_ << " " << c_expiry_ << " " << c_atm_strike_ << " " << c_current_stepvalue_ << " ";
    int count_ = 0;
    for (const auto& p : volume_2_distance_) {
      std::cout << " (" << p.first << ", " << p.second << ")";
      count_++;
      if (count_ >= max_) {
        break;
      }
    }
    std::cout << "\n";
  } else {
    std::cerr << "Fatal error - could not read NSE BhavCopy file " << nse_bhav_copy_file_name_ << "\n";
  }
}

void GetOTMOptionContracts(const std::string& base_code_, int yyyymmdd_, const std::string& opt_type_,
                           bool is_currency_, int max_) {
  // strike schema (T) => nse_contracts (T) => X contracts
  // previous_day bhav copy => sort by volume based => figure out what to with Y not in X
  // after dec, mar, jun, sep expiry
}
}
