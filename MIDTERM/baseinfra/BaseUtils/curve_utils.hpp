/*
 * curve_utils.hpp
 *
 *  Created on: 12-Nov-2013
 *      Author: archit
 */

#include <iostream>
#include <fstream>
#include <strings.h>
#include <vector>
#include <boost/date_time/gregorian/gregorian.hpp>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "dvccode/Utils/exchange_names.hpp"
#include "dvccode/Utils/holiday_manager.hpp"

#ifndef CURVE_UTILS_HPP_
#define CURVE_UTILS_HPP_

#define P2Y_FILE "/spare/local/tradeinfo/p2y/"
#define YIELD_DATA "/spare/local/tradeinfo/p2y/yield_data"

namespace HFSAT {

class CurveUtils {
 public:
  // DV01 = - f'( y ) / 10000
  // Duration ( D ) = - f' ( y ) / p
  // DMac / DMod  = ( 1 + y / 2 ) * D
  // Convexity = f'' ( y ) / p
  // DMac = F / P * { T / ( 1 + y / 2 ) ^ 2T  + q / 2 * sum ( ( i / 2 ) / ( 1 + y / 2 ) ^ i ) }

  // P2Y
  /* US Treasuries:
       TU 2 ZT
       FV 5 ZF
       TY 10 ZN
       US Long 15 ZB
       WN Ultra 25 UB

       EU Treasuries:
       RX Bund 8.5 - 10.5 FGBL
       OE Bobl 4.5 - 5.5 FGBM
       DU Schatz 1.75 - 2.25 FGBS
       UB Buxl 24 - 35 FGBX
       IK BTP Italian FBTP
       OT OAT French FOAT

       UK Treasuries:
       WB short gilt 1.5 - 3.25
       WX medium gilt 4 -6
       G0  long gilt 8.75 - 13 LFR

       Canadian Treasuries:
       CN 10 CAD CGB
       CV 2 CAD CGF

       Japan Treasuries:
       JB 10 Yr JGB
   */

  static inline int get_nearest_index_from_increasing_vector(double _value_, std::vector<double> &sorted_vec_) {
    if (sorted_vec_.size() > 0) {
      int start_ = 0;
      int end_ = sorted_vec_.size();
      int mid_index_ = (start_ + end_) / 2;

      while (start_ < end_ && _value_ != sorted_vec_[mid_index_]) {
        if (_value_ < sorted_vec_[mid_index_]) {
          end_ = mid_index_ - 1;
          mid_index_ = (start_ + end_) / 2;
        } else if (_value_ > sorted_vec_[mid_index_]) {
          start_ = mid_index_ + 1;
          mid_index_ = (start_ + end_) / 2;
        }
      }
      return mid_index_;
    } else {
      return (-1);
    }
  }

  static inline int get_nearest_index_from_decreasing_vector(double _value_, std::vector<double> &sorted_vec_) {
    if (sorted_vec_.size() > 0) {
      int start_ = 0;
      int end_ = sorted_vec_.size();
      int mid_index_ = (start_ + end_) / 2;

      while (start_ < end_ && _value_ != sorted_vec_[mid_index_]) {
        if (_value_ > sorted_vec_[mid_index_]) {
          end_ = mid_index_ - 1;
          mid_index_ = (start_ + end_) / 2;
        } else if (_value_ < sorted_vec_[mid_index_]) {
          start_ = mid_index_ + 1;
          mid_index_ = (start_ + end_) / 2;
        }
      }

      int t_index_;
      t_index_ = abs(sorted_vec_[mid_index_ + 1] - _value_) > abs(sorted_vec_[mid_index_] - _value_) ? mid_index_
                                                                                                     : (mid_index_ + 1);
      t_index_ = abs(sorted_vec_[mid_index_ - 1] - _value_) > abs(sorted_vec_[t_index_] - _value_) ? t_index_
                                                                                                   : (mid_index_ - 1);
      return t_index_;
    } else {
      return (-1);
    }
  }

  static inline void load_PY_vectors(std::string _shc_, std::vector<double> &price_vec_,
                                     std::vector<double> &yield_vec_) {
    std::string p2y_filename_ = "";
    std::ifstream t_p2y_file_;
    p2y_filename_ = std::string(P2Y_FILE) + _shc_.substr(0, _shc_.find("_") + 1) + "p2y";
    t_p2y_file_.open(p2y_filename_.c_str(), std::ifstream::in);

    price_vec_.clear();

    if (t_p2y_file_.is_open()) {
      const int kBufferLen = 1024;
      char readline_buffer_[kBufferLen];
      bzero(readline_buffer_, kBufferLen);

      while (t_p2y_file_.good()) {
        bzero(readline_buffer_, kBufferLen);
        t_p2y_file_.getline(readline_buffer_, kBufferLen);
        PerishableStringTokenizer st_(readline_buffer_, kBufferLen);
        const std::vector<const char *> &tokens_ = st_.GetTokens();
        if (tokens_.size() == 2) {
          price_vec_.push_back(atof(tokens_[0]));
          yield_vec_.push_back(atof(tokens_[1]));
        }
      }
      t_p2y_file_.close();
    }
  }

  static inline void ReplaceDelimiter(char *_buffer_, const int _buffer_length_) {
    for (int i = 0; i < _buffer_length_; i++) {
      if (_buffer_[i] == '|') {
        _buffer_[i] = ' ';
      }
    }
  }

  static inline double gov_fut_dv01_new(const std::string &_shc_, int _yyyymmdd_) {
    std::string bbg_symbol_ = "";
    bool file_found_ = false;

    if (!_shc_.compare("ZN_0")) {
      bbg_symbol_ = "TY_UTILS";
      file_found_ = true;
    }
    if (!_shc_.compare("ZB_0")) {
      bbg_symbol_ = "US_UTILS";
      file_found_ = true;
    }
    if (!_shc_.compare("ZF_0")) {
      bbg_symbol_ = "FV_UTILS";
      file_found_ = true;
    }
    if (!_shc_.compare("UB_0")) {
      bbg_symbol_ = "WN_UTILS";
      file_found_ = true;
    }
    if (!_shc_.compare("ZT_0")) {
      bbg_symbol_ = "TU_UTILS";
      file_found_ = true;
    }
    if (!_shc_.compare("FGBM_0")) {
      bbg_symbol_ = "OE_UTILS";
      file_found_ = true;
    }
    if (!_shc_.compare("FGBL_0")) {
      bbg_symbol_ = "RX_UTILS";
      file_found_ = true;
    }
    if (!_shc_.compare("FGBS_0")) {
      bbg_symbol_ = "DU_UTILS";
      file_found_ = true;
    }
    if (!_shc_.compare("FGBX_0")) {
      bbg_symbol_ = "UB_UTILS";
      file_found_ = true;
    }
    if (!_shc_.compare("FBTP_0")) {
      bbg_symbol_ = "IK_UTILS";
      file_found_ = true;
    }
    if (!_shc_.compare("FOAT_0")) {
      bbg_symbol_ = "OT_UTILS";
      file_found_ = true;
    }
    if (!_shc_.compare("LFR_0")) {
      bbg_symbol_ = "G0_UTILS";
      file_found_ = true;
    }
    if (!_shc_.compare("CGB_0")) {
      bbg_symbol_ = "CN_UTILS";
      file_found_ = true;
    }

    std::string p2y_filename_ = "";
    std::ifstream t_p2y_file_;
    p2y_filename_ = std::string(P2Y_FILE) + bbg_symbol_;
    t_p2y_file_.open(p2y_filename_.c_str(), std::ifstream::in);
    double frsk_ = 0.0;
    std::map<int, double> date2frsk_map_;
    if (t_p2y_file_.is_open() && file_found_) {
      const int kBufferLen = 1024;
      char readline_buffer_[kBufferLen];
      bzero(readline_buffer_, kBufferLen);
      while (t_p2y_file_.good()) {
        bzero(readline_buffer_, kBufferLen);
        t_p2y_file_.getline(readline_buffer_, kBufferLen);
        ReplaceDelimiter(readline_buffer_, kBufferLen);  // replacing '|' with ' '
        PerishableStringTokenizer st_(readline_buffer_, kBufferLen);
        const std::vector<const char *> &tokens_ = st_.GetTokens();

        if (tokens_.size() == 9) {
          std::string date_ = tokens_[0];
          std::string t_out_date_ = "";
          for (size_t i = 0; i < date_.size(); ++i) {
            if (date_[i] != '-') t_out_date_ += date_[i];
          }  // changing the date yyyy-mm-dd yyyymmdd
          if (date2frsk_map_.find(atoi(t_out_date_.c_str())) == date2frsk_map_.end()) {
            date2frsk_map_[atoi(t_out_date_.c_str())] = atof(tokens_[3]);
          }
          //              if ( _yyyymmdd_ == atoi(t_out_date_.c_str()) )
          //              {
          //                 frsk_ = atof(tokens_[ 3 ] )  ;
          //              }
        }
      }
      t_p2y_file_.close();
    }

    double n2d = SecurityDefinitions::GetContractNumbersToDollars(_shc_, _yyyymmdd_);
    if (!date2frsk_map_.empty()) {
      frsk_ = date2frsk_map_.begin()->second;
      for (std::map<int, double>::iterator it = date2frsk_map_.begin(); it != date2frsk_map_.end(); it++) {
        // iterate in increasing order of dates
        if (it->first >= _yyyymmdd_) {
          // use dv01 just before _yyyymmdd_, as in real we would be having TODAY-1 dv01 in the file
          break;
        } else {
          frsk_ = it->second;
        }
      }
    } else {
      return n2d / 100;
    }

    return (frsk_ * n2d / 100);
  }

  static inline double gov_fut_dv01(std::string _shc_, double _price_, int _yyyymmdd_, std::vector<double> &price_vec_,
                                    std::vector<double> &yield_vec_) {
    if (price_vec_.size() == 0 || yield_vec_.size() == 0) {
      load_PY_vectors(_shc_, price_vec_, yield_vec_);
    }

    if (yield_vec_.size() > 0 && price_vec_.size() > 0) {
      int t_p2y_index_ = get_nearest_index_from_increasing_vector(_price_, price_vec_);  // should be one index

      int t_y2p_index_1 = get_nearest_index_from_decreasing_vector(yield_vec_[t_p2y_index_] + 0.01,
                                                                   yield_vec_);  // get 2 indices and do interpolation ?
      int t_y2p_index_2 = get_nearest_index_from_decreasing_vector(yield_vec_[t_p2y_index_] - 0.01,
                                                                   yield_vec_);  // get 2 indices and do interpolation ?

      double dv01 = 0.5 * ((_price_ - price_vec_[t_y2p_index_1]) + (price_vec_[t_y2p_index_2] - _price_));
      double n2d = SecurityDefinitions::GetContractNumbersToDollars(_shc_, _yyyymmdd_);

      return (dv01 * n2d);
    } else {
      return (-1);
    }
  }

  static inline double stirs_fut_dv01(const std::string &_shc_, int _yyyymmdd_, double _price_ = -1.00) {
    if (_shc_.find("DI1") != std::string::npos)
    // exact_version -> { GetDIContractNumbersToDollars ( _shc_ , date_ , _price_ + 0.1 ) +
    // GetDIContractNumbersToDollars ( _shc_ , date_ , _price_ - 0.1 ) } * 0.5
    // app_version -> 10 * GetDIReserves / 252
    {
      double term_ = (double(_get_term_(_yyyymmdd_, _shc_))) / 252;
      if (_price_ < 0.00) {
        return (10.0 * term_);
      } else {
        return (10.0 * term_) / std::pow(1 + 0.01 * _price_, term_ + 1);
      }
    }
    //      else if (  _shc_.find ( "LFI" ) != std::string::npos || _shc_.find ( "LFL" ) != std::string::npos ||
    //      _shc_.find ( "GE" ) != std::string::npos )
    // ( 1 + y / 100 * 90 / 360 * PA  ) = DV = > DV = ( 1 + y / 400 * PA )
    // 0.01 / 400 * PA
    // PA = 1mil for GE/LFI & 0.5 mil for LFL
    // 1/4 * 100 for GE / LFI & 0.5 * 1 / 4 * 100 for LFL
    // 25 for GE / LFI and 12.5 for LFL
    // yield = 100 - px => px = 100 - y => dp / dy = -1 => dv01 = 1 * n2d / 100
    else if (_shc_.find("GE") != std::string::npos) {
      double n2d = SecurityDefinitions::GetContractNumbersToDollars(_shc_, _yyyymmdd_);
      return n2d;  // GE n2d is already downscaled by 100
    } else {
      double n2d = SecurityDefinitions::GetContractNumbersToDollars(_shc_, _yyyymmdd_);
      return (n2d / 100.0);
    }
  }

  static inline double dv01(const std::string &_shc_, int _yyyymmdd_, double _price_ = -1.00) {
    if (_shc_.find("LFI") != std::string::npos || _shc_.find("LFL") != std::string::npos ||
        _shc_.find("GE") != std::string::npos || _shc_.find("DI1") != std::string::npos ||
        _shc_.substr(0, 3) == "I~~" || _shc_.substr(0, 3) == "L~~") {
      return (stirs_fut_dv01(_shc_, _yyyymmdd_, _price_));
    } else if (_shc_.find("VX") != std::string::npos || _shc_.find("FVS") != std::string::npos) {
      double n2d = SecurityDefinitions::GetContractNumbersToDollars(_shc_, _yyyymmdd_);
      return n2d;
    } else {
      //	  return ( gov_fut_dv01_new ( _shc_ , _price_ , _yyyymmdd_ , price_vec_ , yield_vec_ ) ) ;
      return (gov_fut_dv01_new(_shc_, _yyyymmdd_));
    }
  }

  static inline std::string _generic_to_specific_(int _input_date_, std::string _shc_) {
    if (_shc_.find("DI1") != std::string::npos) {
      int current_mm = (_input_date_ / 100) % 100;
      int current_yy = (_input_date_ / 10000) % 100;
      int contract_no = atoi(_shc_.substr(6).c_str());

      if (contract_no > 3) {
        current_mm = current_mm + 3;
        int skip_q = contract_no - 3;

        while (skip_q > 0) {
          current_mm++;
          while (current_mm % 3 != 0) {
            current_mm++;
          }
          skip_q--;
        }
      } else {
        current_mm = current_mm + contract_no;
      }
      current_mm++;

      if (current_mm > 12) {
        current_yy += current_mm / 12;
        current_mm = current_mm % 12;
      } else {
      }

      const std::string ExchMonthCode("FGHJKMNQUVXZ");
      std::stringstream ss;
      ss << "DI1" << ExchMonthCode[current_mm - 1] << current_yy;
      return ss.str();
    }
    return _shc_;
  }

  static inline void GetSpreadShortcodes(std::string structure_, std::vector<std::string> &structure_shortcode_vec_) {
    GetStructureShortcodes(structure_, structure_shortcode_vec_, "PriceBasedSpreadTrading");
  }

  static inline void GetStructureShortcodes(
      std::string structure_, std::vector<std::string> &structure_shortcode_vec_,
      const std::string &strategy_name_ = "StructuredPriceBasedAggressiveTrading") {
    if (IsSpreadStrategy(strategy_name_)) {
      std::ifstream t_spread_shc_file_;
      t_spread_shc_file_.open((std::string(SPREADINFO_DIR) + std::string("shortcode.txt")).c_str(), std::ifstream::in);
      if (t_spread_shc_file_.is_open()) {
        const int kReadLineBufferLen = 1024;
        char readline_buffer_[kReadLineBufferLen];
        bzero(readline_buffer_, kReadLineBufferLen);

        while (t_spread_shc_file_.good()) {
          bzero(readline_buffer_, kReadLineBufferLen);
          t_spread_shc_file_.getline(readline_buffer_, kReadLineBufferLen);
          PerishableStringTokenizer st_(readline_buffer_, kReadLineBufferLen);
          const std::vector<const char *> &tokens_ = st_.GetTokens();
          if (tokens_.size() >= 5 && strcmp(tokens_[0], structure_.c_str()) == 0) {
            //_spread_shc_ _shc1_ _size1_ _shc2_ _size2_ ...
            for (unsigned int i = 1; i <= (tokens_.size() - 1) / 2; i++) {
              std::string t_shc_ = tokens_[2 * i - 1];
              if (!HFSAT::VectorUtils::UniqueVectorAdd(structure_shortcode_vec_, t_shc_)) {
                std::cerr << "In file" << SPREADINFO_DIR << "shortcode.txt, SpreadTrading Shortcode " << structure_
                          << "has repeating shortcode: " << t_shc_ << std::endl;
                exit(1);
              }
            }
          }
        }
      }
    } else if (strategy_name_.compare("RiskBasedStructuredTrading") == 0) {
      std::ifstream t_lfi_shc_file_;
      std::string t_std_matrix_file_ =
          std::string(SPREADINFO_DIR) + structure_.substr(0, structure_.size() - 2) + "_stdev_matrix.txt";

      // file order:Isolated Outrights/Spreads/Flies
      t_lfi_shc_file_.open(t_std_matrix_file_.c_str(), std::ifstream::in);
      if (t_lfi_shc_file_.is_open()) {
        const int kLineBufferLen = 1024;
        char readline_buffer_[kLineBufferLen];
        bzero(readline_buffer_, kLineBufferLen);
        while (t_lfi_shc_file_.good()) {
          bzero(readline_buffer_, kLineBufferLen);
          t_lfi_shc_file_.getline(readline_buffer_, kLineBufferLen);
          PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
          const std::vector<const char *> &tokens_ = st_.GetTokens();
          if (tokens_.size() > 0 && strcmp(tokens_[0], structure_.c_str()) == 0) {
            for (unsigned int i = 1; i < tokens_.size(); i++) structure_shortcode_vec_.push_back(tokens_[i]);
          }
        }
        t_lfi_shc_file_.close();
      } else {
        std::cerr << "can't open file " << t_std_matrix_file_ << "\n";
        exit(1);
      }
    } else {
      std::ifstream t_structured_trading_risk_matrix_infile_;
      t_structured_trading_risk_matrix_infile_.open("/spare/local/tradeinfo/StructureInfo/structured_trading.txt",
                                                    std::ifstream::in);
      if (t_structured_trading_risk_matrix_infile_.is_open()) {
        const int kL1AvgBufferLen = 1024;
        char readline_buffer_[kL1AvgBufferLen];
        bzero(readline_buffer_, kL1AvgBufferLen);

        while (t_structured_trading_risk_matrix_infile_.good()) {
          bzero(readline_buffer_, kL1AvgBufferLen);
          t_structured_trading_risk_matrix_infile_.getline(readline_buffer_, kL1AvgBufferLen);
          PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
          const std::vector<const char *> &tokens_ = st_.GetTokens();
          if (tokens_.size() >= 1 && strcmp(tokens_[0], structure_.c_str()) == 0) {
            for (unsigned int i = 1; i < tokens_.size(); i++) {
              structure_shortcode_vec_.push_back(std::string(tokens_[i]));
            }
            return;
          }
        }
      }
    }
  }

  static inline double GetLastDayClosingPrice(int _date_, std::string _shortcode_) {
    std::ifstream t_last_day_closing_price_infile_;
    std::string t_last_day_closing_price_infilename_;

    double t_last_day_closing_price_ = 0.0;

    int this_YYYYMMDD_ = _date_;

    for (unsigned int ii = 0; ii < 40; ii++) {
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << "/spare/local/tradeinfo/StructureInfo/prices/prices." << this_YYYYMMDD_;
      t_last_day_closing_price_infilename_ = t_temp_oss_.str();

      if (FileUtils::exists(t_last_day_closing_price_infilename_)) {
        t_last_day_closing_price_infile_.open(t_last_day_closing_price_infilename_.c_str(), std::ifstream::in);

        if (t_last_day_closing_price_infile_.is_open()) {
          const int kLastDayClosingPriceBufferLen = 1024;
          char readline_buffer_[kLastDayClosingPriceBufferLen];
          bzero(readline_buffer_, kLastDayClosingPriceBufferLen);
          while (t_last_day_closing_price_infile_.good()) {
            bzero(readline_buffer_, kLastDayClosingPriceBufferLen);
            t_last_day_closing_price_infile_.getline(readline_buffer_, kLastDayClosingPriceBufferLen);
            PerishableStringTokenizer st_(readline_buffer_, kLastDayClosingPriceBufferLen);
            const std::vector<const char *> &tokens_ = st_.GetTokens();
            if (tokens_.size() >= 2) {
              std::string t_shortcode_ = tokens_[0];
              if (t_shortcode_.compare(_shortcode_) == 0) {
                t_last_day_closing_price_ = atof(tokens_[1]);
                t_last_day_closing_price_infile_.close();
                return t_last_day_closing_price_;
              }
            }
          }
        }
        t_last_day_closing_price_infile_.close();
      }
      this_YYYYMMDD_ = DateTime::CalcPrevDay(this_YYYYMMDD_);
    }
    return t_last_day_closing_price_;
  }

  static inline int _get_volume_(int _date_, std::string _shortcode_, int trading_start_mfm, int trading_end_mfm) {
    double t_avg_volume_ = 0.0;
    std::map<int, double> utc_time_to_vol_map_;
    SampleDataUtil::GetAvgForPeriod(_shortcode_, _date_, 60, trading_start_mfm, trading_end_mfm, "VOL",
                                    utc_time_to_vol_map_);

    std::ifstream t_recent_volume_infile_;
    std::string t_recent_volume_infilename_;

    for (auto it = utc_time_to_vol_map_.begin(); it != utc_time_to_vol_map_.end(); it++) {
      t_avg_volume_ += it->second;
    }
    return t_avg_volume_;
  }

  static inline double _get_pvalue_(int term_, double price_) {
    double p_value_ = 100000 / (pow((1 + price_ * 0.01), term_ / 252.0));
    return p_value_;
  }

  static inline double _get_pvalue_(std::string _shortcode_, int _term_, int _date_) {
    double price_ = GetLastDayClosingPrice(_date_, _shortcode_);
    if (abs(price_) < 0.01)  // if it 0
    {
      return 0.0;
    }
    double p_value_ = 100000 / (pow((1 + price_ * 0.01), _term_ / 252.0));
    return p_value_;
  }

  /**
   *
   * @param input_date
   * @param t_shortcode
   * @return
   */
  static inline int get_cme_fi_term(int input_date, const std::string &t_shortcode) {
    std::stringstream st;
    st << input_date;
    auto input_date_str = st.str();

    auto index = 2u;
    if (t_shortcode.substr(0, 3) == "ZTY" || t_shortcode.substr(0, 3) == "ZFY" || t_shortcode.substr(0, 3) == "ZNY" ||
        t_shortcode.substr(0, 3) == "ZBY" || t_shortcode.substr(0, 3) == "UBY") {
      index = 3;
    }
    // t_shortcode is exchange symbol actually

    std::string current_shortcode = t_shortcode.substr(0, t_shortcode.size() - index) + "_0";

    std::string yield_for_day_directory_path = std::string(YIELD_DATA) + std::string("/") + input_date_str;

    std::string yield_file_full_path = yield_for_day_directory_path + "/" + current_shortcode;

    std::ifstream t_yield_file_read;

    t_yield_file_read.open(yield_file_full_path.c_str(), std::ifstream::in);

    if (t_yield_file_read.is_open()) {
      const int kLineBufferLen = 2048;
      char readline_buffer[kLineBufferLen];

      while (t_yield_file_read.good()) {
        bzero(readline_buffer, kLineBufferLen);

        t_yield_file_read.getline(readline_buffer, kLineBufferLen);

        // read the csv file contents
        PerishableStringTokenizer tokenizer(readline_buffer, kLineBufferLen);
        std::vector<const char *> tokens = tokenizer.GetTokens();

        if (tokens.size() > 0) {
          // we can put assertion on exchange symbol mapping here
          std::string this_exch_symbol = std::string(tokens[0]);

          /// Removing the exchange symbol check here as in cases of ZN/ZNY both exchange symbols would point to same
          /// yield value
          // assert(this_exch_symbol == t_shortcode);

          int term_time = atoi(tokens[3]);
          return term_time;
        }
      }
    }

    // For now exiting rather than returning incorrect value
    st.str("");
    st << " GetTerm Called for " << t_shortcode << " but no entry in " << yield_file_full_path;
    ExitVerbose(kExitErrorCodeGeneral, st.str().c_str());

    return -1;
  }

  /**
   *
   * @param _input_date_
   * @param _shc_
   * @return
   */
  static inline int _get_term_(int _input_date_, const std::string &_shc_) {
    std::string this_shc = _shc_;
    std::replace(this_shc.begin(), this_shc.end(), ' ', '~');

    if (this_shc.substr(0, 2) == "ZT" || this_shc.substr(0, 2) == "ZF" || this_shc.substr(0, 2) == "ZN" ||
        this_shc.substr(0, 2) == "ZB" || this_shc.substr(0, 2) == "UB") {
      return get_cme_fi_term(_input_date_, _shc_);
    }

    if (this_shc.substr(0, 2) == "IR") {
      std::stringstream ss;
      ss << _input_date_;
      boost::gregorian::date sdate_ = boost::gregorian::from_undelimited_string(ss.str());
      std::string exp_month_ = "20" + this_shc.substr(4, 4) + "01";
      boost::gregorian::date edate_ = boost::gregorian::from_undelimited_string(exp_month_);
      const boost::gregorian::date_duration one_day_(1);
      while (edate_.day_of_week() != boost::gregorian::Thursday) {
        edate_ = edate_ + one_day_;
      }
      edate_ = edate_ + boost::gregorian::date_duration(7);
      return ((edate_ - sdate_).days());
    }

    if (this_shc.substr(0, 12) == "USD000UTSTOM") {
      std::stringstream ss;
      ss << _input_date_;
      boost::gregorian::date sdate_ = boost::gregorian::from_undelimited_string(ss.str());
      const boost::gregorian::date_duration one_day_date_duration_(1);
      sdate_ += one_day_date_duration_;
      int _term_ = 1;

      while (
          HolidayManagerNoThrow::IsProductHoliday("USD000UTSTOM", std::atoi((to_iso_string(sdate_)).c_str()), true)) {
        sdate_ += one_day_date_duration_;
        _term_ += 1;
      }
      return _term_;
    }

    if (this_shc.substr(0, 3) == "DI1") {
      return (SecurityDefinitions::GetDIReserves(_input_date_, this_shc));
    }

    if (this_shc.substr(0, 2) == "GE") {
      char _ltd_date_[10] = {0};  // changed since we are writing 8 + 1 chars below

      const std::string ExchMonthCode("FGHJKMNQUVXZ");
      const boost::gregorian::date_duration one_day_date_duration(1);

      int ltd_mm = ExchMonthCode.find(this_shc[2]);
      ltd_mm++;

      int ltd_yy = atoi(this_shc.substr(3).c_str());

      auto input_yyyy = _input_date_ / 10000;
      auto input_y = input_yyyy % 10;
      auto input_yyy = input_yyyy / 10;

      auto exp_year = input_yyy * 10 + ltd_yy;

      // the expiry is in next decade
      if (ltd_yy < input_y) {
        exp_year = (input_yyy + 1) * 10 + ltd_yy;
      }

      if (sprintf(_ltd_date_, "%04d%02d01", exp_year, ltd_mm) > 0) {
        std::stringstream ss;
        ss << _input_date_;

        boost::gregorian::date sdate_ =
            boost::gregorian::from_undelimited_string(ss.str());  // my compiler  doesnt know to_string ?
        boost::gregorian::date edate_ = boost::gregorian::from_undelimited_string(_ltd_date_);

        while (edate_.day_of_week() != boost::gregorian::Wednesday) {
          edate_ = edate_ + one_day_date_duration;
        }

        edate_ = edate_ + boost::gregorian::date_duration(12);  // 2 days before third wednesday
        // TODO : check if edate_ is exchange holiday
        return ((edate_ - sdate_).days());
      }
    }

    if (this_shc.substr(0, 2) == "Si" || this_shc.substr(0, 2) == "Eu" || this_shc.substr(0, 2) == "SR" ||
        this_shc.substr(0, 2) == "GZ" || this_shc.substr(0, 2) == "VB" || this_shc.substr(0, 2) == "LK") {
      char _ltd_date_[10] = {0};
      const std::string ExchMonthCode("FGHJKMNQUVXZ");

      int ltd_mm = ExchMonthCode.find(this_shc[2]);
      ltd_mm++;
      int ltd_yy = atoi(this_shc.substr(3).c_str());

      auto input_yyyy = _input_date_ / 10000;
      auto input_y = input_yyyy % 10;
      auto input_yyy = input_yyyy / 10;

      auto exp_year = input_yyy * 10 + ltd_yy;

      // the expiry is in next decade
      if (ltd_yy < input_y) {
        exp_year = (input_yyy + 1) * 10 + ltd_yy;
      }

      if (sprintf(_ltd_date_, "%04d%02d01", exp_year, ltd_mm) > 0) {
        std::stringstream ss;
        ss << _input_date_;

        boost::gregorian::date sdate_ = boost::gregorian::from_undelimited_string(ss.str());
        boost::gregorian::date edate_ = boost::gregorian::from_undelimited_string(_ltd_date_);

        return ((edate_ - sdate_).days());
      }
    }

    if (this_shc.substr(0, 3) == "I~~") {
      // ICE shortcode
      const std::string ExchMonthCode("FGHJKMNQUVXZ");
      int ltd_mm = ExchMonthCode.find(this_shc[6]);
      ltd_mm++;
      std::string ltd_yy = this_shc.substr(9, 2);

      std::stringstream t_mm_st;
      if (ltd_mm <= 9) {
        t_mm_st << 0;
      }
      t_mm_st << ltd_mm;

      std::string mm = t_mm_st.str();

      std::string exp_month_ = "20" + ltd_yy + mm + "01";
      boost::gregorian::date edate_ = boost::gregorian::from_undelimited_string(exp_month_);

      std::stringstream ss;
      ss << _input_date_;
      boost::gregorian::date sdate_ = boost::gregorian::from_undelimited_string(ss.str());

      const boost::gregorian::date_duration one_day_(1);

      while (edate_.day_of_week() != boost::gregorian::Wednesday) {
        edate_ = edate_ + one_day_;
      }

      edate_ = edate_ + boost::gregorian::date_duration(12);  // 2 days before third wednesday
      // TODO : check if edate_ is exchange holiday
      return (edate_ - sdate_).days();
    }

    if (this_shc.find("LFI") != std::string::npos) {
      std::string exp_month_ = "20" + this_shc.substr(5, 4) + "01";
      boost::gregorian::date edate_ = boost::gregorian::from_undelimited_string(exp_month_);

      std::stringstream ss;
      ss << _input_date_;
      boost::gregorian::date sdate_ = boost::gregorian::from_undelimited_string(ss.str());

      const boost::gregorian::date_duration one_day_(1);

      while (edate_.day_of_week() != boost::gregorian::Wednesday) {
        edate_ = edate_ + one_day_;
      }

      edate_ = edate_ + boost::gregorian::date_duration(12);  // 2 days before third wednesday
      // TODO : check if edate_ is exchange holiday
      return (edate_ - sdate_).days();
    }

    if (this_shc.substr(0, 3) == "L~~") {
      const std::string ExchMonthCode("FGHJKMNQUVXZ");
      int ltd_mm = ExchMonthCode.find(this_shc[6]);
      ltd_mm++;
      std::string ltd_yy = this_shc.substr(9, 2);

      std::stringstream t_mm_st;
      if (ltd_mm <= 9) {
        t_mm_st << 0;
      }
      t_mm_st << ltd_mm;

      std::string mm = t_mm_st.str();

      std::string exp_month_ = "20" + ltd_yy + mm + "01";

      boost::gregorian::date edate_ = boost::gregorian::from_undelimited_string(exp_month_);

      std::stringstream ss;
      ss << _input_date_;
      boost::gregorian::date sdate_ = boost::gregorian::from_undelimited_string(ss.str());

      const boost::gregorian::date_duration one_day_(1);

      while (edate_.day_of_week() != boost::gregorian::Wednesday) {
        edate_ = edate_ + one_day_;
      }

      edate_ = edate_ + boost::gregorian::date_duration(14);  // third wednesday
      // TODO : check if edate_ is exchange holiday
      return (edate_ - sdate_).days();
    }

    if (this_shc.substr(0, 3) == "LFL") {
      std::string exp_month_ = "20" + this_shc.substr(5, 4) + "01";
      boost::gregorian::date edate_ = boost::gregorian::from_undelimited_string(exp_month_);

      std::stringstream ss;
      ss << _input_date_;
      boost::gregorian::date sdate_ = boost::gregorian::from_undelimited_string(ss.str());

      const boost::gregorian::date_duration one_day_(1);

      while (edate_.day_of_week() != boost::gregorian::Wednesday) {
        edate_ = edate_ + one_day_;
      }

      edate_ = edate_ + boost::gregorian::date_duration(14);  // third wednesday
      // TODO : check if edate_ is exchange holiday
      return (edate_ - sdate_).days();
    }

    if (this_shc.substr(0, 3) == "FVS") {
      std::string exp_month_ = "20" + this_shc.substr(5, 4) + "01";
      boost::gregorian::date edate_ = boost::gregorian::from_undelimited_string(exp_month_);

      std::stringstream ss;
      ss << _input_date_;
      boost::gregorian::date sdate_ = boost::gregorian::from_undelimited_string(ss.str());

      const boost::gregorian::date_duration one_day_(1);
      const boost::gregorian::months one_month_(1);

      edate_ = edate_ + one_month_ - one_day_;  // Last day of same month

      while (edate_.day_of_week() != boost::gregorian::Friday) {
        edate_ = edate_ - one_day_;
      }

      edate_ = edate_ - boost::gregorian::date_duration(9);  // Wednesday preceding 2nd last Friday
      // TODO : check if edate_ is exchange holiday: If holiday, go to immediately preceding working day
      return (edate_ - sdate_).days();
    }

    if (this_shc.substr(0, 2) == "VX") {
      std::string exp_month_ = "20" + this_shc.substr(4, 4) + "01";
      boost::gregorian::date edate_ = boost::gregorian::from_undelimited_string(exp_month_);

      std::stringstream ss;
      ss << _input_date_;
      boost::gregorian::date sdate_ = boost::gregorian::from_undelimited_string(ss.str());

      const boost::gregorian::date_duration one_day_(1);
      const boost::gregorian::months one_month_(1);

      edate_ = edate_ + one_month_;  // First day of next month

      while (edate_.day_of_week() != boost::gregorian::Friday) {
        edate_ = edate_ + one_day_;
      }

      edate_ = edate_ - boost::gregorian::date_duration(
                            16);  // Wednesday which is 30 days prior to 3rd Friday of next month. +14 - 30 = -16
      // TODO : check if edate_ is exchange holiday: If holiday, go to preceding working Wednesday
      return (edate_ - sdate_).days();
    }

    if (this_shc.substr(0, 4) == "FGBL") {
      std::string exp_month_ = "20" + this_shc.substr(6, 4) + "01";
      boost::gregorian::date sdate_ = boost::gregorian::from_undelimited_string(exp_month_);
      std::stringstream ss;
      int maturity_month_ = (atoi(this_shc.substr(6, 4).c_str()) + 1000);
      ss << "20" << maturity_month_ << "01";
      boost::gregorian::date edate_ = boost::gregorian::from_undelimited_string(ss.str());
      // TODO : check if edate_ is exchange holiday
      return (edate_ - sdate_).days();
      //
    }

    if (this_shc.substr(0, 4) == "FGBS") {
      std::string exp_month_ = "20" + this_shc.substr(6, 4) + "01";
      boost::gregorian::date sdate_ = boost::gregorian::from_undelimited_string(exp_month_);
      std::stringstream ss;
      int maturity_month_ = (atoi(this_shc.substr(6, 4).c_str()) + 200);
      ss << "20" << maturity_month_ << "01";
      boost::gregorian::date edate_ = boost::gregorian::from_undelimited_string(ss.str());
      // TODO : check if edate_ is exchange holiday
      return (edate_ - sdate_).days();
    }
    if (this_shc.substr(0, 4) == "FGBM") {
      std::string exp_month_ = "20" + this_shc.substr(6, 4) + "01";
      boost::gregorian::date sdate_ = boost::gregorian::from_undelimited_string(exp_month_);
      std::stringstream ss;
      int maturity_month_ = (atoi(this_shc.substr(6, 4).c_str()) + 500);
      ss << "20" << maturity_month_ << "01";
      boost::gregorian::date edate_ = boost::gregorian::from_undelimited_string(ss.str());
      // TODO : check if edate_ is exchange holiday
      return (edate_ - sdate_).days();
      //                //            //
      //                      }
      //
    }

    if (this_shc.substr(0, 2) == "CL") {
      const std::string ExchMonthCode("FGHJKMNQUVXZ");
      int ltd_mm = ExchMonthCode.find(this_shc[2]);
      int ltd_yy = 2010 + atoi(this_shc.substr(3).c_str());

      // From exchange contract specs
      boost::gregorian::date d1(ltd_yy, ltd_mm, 25);
      std::string day_of_week_ = d1.day_of_week().as_long_string();

      const boost::gregorian::date_duration one_day_date_duration(1);
      const boost::gregorian::date_duration two_day_date_duration(2);
      const boost::gregorian::date_duration three_day_date_duration(3);

      if (day_of_week_ == "Sunday" || day_of_week_ == "Monday" || day_of_week_ == "Tuesday" ||
          day_of_week_ == "Wednesday") {
        // If any of the 3 days from today is holiday, skip them
        d1 -= two_day_date_duration;
      }

      if (day_of_week_ == "Saturday") d1 -= one_day_date_duration;

      d1 -= three_day_date_duration;

      std::stringstream ss;
      ss << _input_date_;

      boost::gregorian::date sdate_ = boost::gregorian::from_undelimited_string(ss.str());

      return ((d1 - sdate_).days());
    }

    if (this_shc.substr(0, 2) == "BR") {
      const std::string ExchMonthCode("FGHJKMNQUVXZ");
      int ltd_mm = ExchMonthCode.find(this_shc[2]);
      int ltd_yy = 2010 + atoi(this_shc.substr(3).c_str());

      if (ltd_mm == 12) {
        ltd_mm = 1;
        ltd_yy++;
      } else {
        ltd_mm++;
      }

      boost::gregorian::date d1(ltd_yy, ltd_mm, 1);
      const boost::gregorian::date_duration two_day_date_duration(2);
      d1 -= two_day_date_duration;
      std::stringstream ss;
      ss << _input_date_;

      boost::gregorian::date sdate_ = boost::gregorian::from_undelimited_string(ss.str());
      return ((d1 - sdate_).days());
    }
    return -1;
  }

  static inline void get_alphas(std::string _shc_, double _indep1_term_, double _indep2_term_, double _dep_term_,
                                double &_alpha1_, double &_alpha2_, int date_ = 20150203) {
    std::replace(_shc_.begin(), _shc_.end(), ' ', '~');

    _alpha1_ = 0.0;
    _alpha2_ = 0.0;

    // SANITY CHECK
    if (_indep2_term_ == 0 || _indep1_term_ == 0 || _dep_term_ == 0 || _indep2_term_ == _indep1_term_) {
      return;
    }

    if (_shc_.find("DI1") != std::string::npos) {
      _alpha1_ = ((_indep2_term_ - _dep_term_) / (_indep2_term_ - _indep1_term_) * (_indep1_term_ / _dep_term_));
      _alpha2_ = (-(_indep1_term_ - _dep_term_) / (_indep2_term_ - _indep1_term_) * (_indep2_term_ / _dep_term_));
      return;
    }

    if (_shc_.find("FVS") != std::string::npos) {
      _alpha1_ = ((_indep2_term_ - _dep_term_) / (_indep2_term_ - _indep1_term_) * (_indep1_term_ / _dep_term_));
      _alpha2_ = (-(_indep1_term_ - _dep_term_) / (_indep2_term_ - _indep1_term_) * (_indep2_term_ / _dep_term_));
      return;
    }

    if (_shc_.find("VX") != std::string::npos) {
      _alpha1_ = ((_indep2_term_ - _dep_term_) / (_indep2_term_ - _indep1_term_) * (_indep1_term_ / _dep_term_));
      _alpha2_ = (-(_indep1_term_ - _dep_term_) / (_indep2_term_ - _indep1_term_) * (_indep2_term_ / _dep_term_));
      return;
    }

    if (_shc_.find("LFI") != std::string::npos || _shc_.find("LFL") != std::string::npos ||
        _shc_.find("GE") != std::string::npos || _shc_.find("IR") != std::string::npos || _shc_.substr(0, 3) == "I~~" ||
        _shc_.substr(0, 3) == "L~~") {
      _alpha1_ = ((_indep2_term_ - _dep_term_) / (_indep2_term_ - _indep1_term_));
      _alpha2_ = (-(_indep1_term_ - _dep_term_) / (_indep2_term_ - _indep1_term_));
      return;
    }

    if (_shc_.find("FGBM") != std::string::npos) {
      if (_indep2_term_ < _indep1_term_) {
        _alpha1_ = ((_indep2_term_ - _dep_term_) / (_indep2_term_ - _indep1_term_)) *
                   GetDV01SpreadSizeRatio("FGBL_0", "FGBM_0", date_);
        _alpha2_ = (-(_indep1_term_ - _dep_term_) / (_indep2_term_ - _indep1_term_)) *
                   GetDV01SpreadSizeRatio("FGBS_0", "FGBM_0", date_);
      } else {
        _alpha1_ = ((_indep2_term_ - _dep_term_) / (_indep2_term_ - _indep1_term_)) *
                   GetDV01SpreadSizeRatio("FGBS_0", "FGBM_0", date_);
        _alpha2_ = (-(_indep1_term_ - _dep_term_) / (_indep2_term_ - _indep1_term_)) *
                   GetDV01SpreadSizeRatio("FGBL_0", "FGBM_0", date_);
      }

      return;
    }
  }

  static inline bool IsSpreadStrategy(std::string _strategy_name_) {
    return (_strategy_name_ == "PriceBasedSpreadTrading");
  }

  static inline double GetDV01SpreadSizeRatio(const std::string &shortcode_1_, const std::string &shortcode_2_,
                                              int date_, double price_1_, double price_2_) {
    double size_factor_;
    double leg1_dv01_ = HFSAT::CurveUtils::dv01(shortcode_1_, date_, price_1_);
    double leg2_dv01_ = HFSAT::CurveUtils::dv01(shortcode_2_, date_, price_2_);
    size_factor_ = leg2_dv01_ / leg1_dv01_;

    return size_factor_;
  }

  static inline double GetDV01SpreadSizeRatio(const std::string &shortcode_1_, const std::string &shortcode_2_,
                                              int date_) {
    double size_factor_;

    double price_1_ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(shortcode_1_)
                          ->price_from_type(kPriceTypeMktSizeWPrice);
    double price_2_ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(shortcode_2_)
                          ->price_from_type(kPriceTypeMktSizeWPrice);

    double leg1_dv01_ = HFSAT::CurveUtils::dv01(shortcode_1_, date_, price_1_);
    double leg2_dv01_ = HFSAT::CurveUtils::dv01(shortcode_2_, date_, price_2_);
    size_factor_ = leg2_dv01_ / leg1_dv01_;

    return size_factor_;
  }

  static std::string GetDI1SpreadSecname(const std::string &_secname_1_, const std::string &_secname_2_, int _date_) {
    // shortexpiry-longexpiry as secname
    if (HFSAT::SecurityDefinitions::GetDIReserves(_date_, _secname_1_) <
        HFSAT::SecurityDefinitions::GetDIReserves(_date_, _secname_2_)) {
      return (_secname_1_ + "-" + _secname_2_);
    } else {
      return (_secname_2_ + "-" + _secname_1_);
    }
  }

  static int GetExpiryNumber(const std::string &_secname_, int _YYYYMMDD_) {
    if (_secname_.find("DI1") == 0) {
      const std::string &DI1_months_ = "FGHJKMNQUVXZ";
      int exp_yy_ = atoi(_secname_.substr(4, 2).c_str());
      if (DI1_months_.find(_secname_[3]) == std::string::npos) {
        return -1;
      }
      int exp_mm_ = DI1_months_.find(_secname_[3]);
      exp_mm_++;

      int yy_ = (_YYYYMMDD_ / 10000) % 100;
      int mm_ = (_YYYYMMDD_ / 100) % 100;

      return (12 * (exp_yy_ - yy_) + exp_mm_ - mm_ - 1);  // first expiry is 0
    }
    return -1;
  }
};
}
#endif /* CURVE_UTILS_HPP_ */
