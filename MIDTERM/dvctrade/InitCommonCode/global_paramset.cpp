// =====================================================================================
//
//       Filename:  global_paramset.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Monday 30 May 2016 05:04:13  UTC
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================
#include <vector>
#include <stdlib.h>
#include <string.h>
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvctrade/InitCommon/global_paramset.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

namespace HFSAT {

GlobalParamSet::GlobalParamSet(const std::string &_paramfilename_, const int r_tradingdate_)
    : tradingdate_(r_tradingdate_),
      strat_max_loss_(3000),
      product_maxloss_(1000),
      returns_duration_(600),
      open_trade_loss_(1000),
      inst_uts_(0),
      inst_maxlots_(0),
      inst_worstcaselots_(0),
      portfolio_maxlots_(0),
      inst_mur_vec_(),
      cooloff_secs_(0),
      base_threshold_(0),
      place_keep_diff_(0),
      change_threshold_(0),
      increase_threshold_(0),
      decrease_threshold_(0),
      increase_threshold_mur_(0),
      decrease_threshold_mur_(0),
      use_book_indicator_(false),
      use_trade_indicator_(false),
      histfile_prefix_("hist_file_mr_"),
      hist_price_length_(60),
      hist_error_length_(360),
      time_hysterisis_factor_(0.0),
      notional_for_unit_lot_(500000.0),
      use_notional_scaling_(false),
      use_notional_n2d_scaling_(false),
      portprice_type_(0),
      custom_scaling_vec_(),
      use_custom_scaling_(false),
      use_dv01_ratios_(false),
      base_threshold_vec_(),
  
      use_abnormal_volume_getflat_(false),
      abnormal_share_set_threshold_(0.05),
      severity_to_getflat_on_(1.00),

      min_bid_int_price_to_trade_(1),
      min_days_to_expiry_to_trade_(1),
      staggered_getflat_msecs_vec_(),
      aggressive_getflat_msecs_(99999999),
      rate_in_minutes_to_recompute_delta_(10),
      iv_model_setting_(1),
      iv_model_aux_token_1_(30),
      iv_model_aux_token_2_(10),
      aggressive_(false),
      max_int_spread_to_cross_(1),
      agg_cooloff_secs_(1),
      opt_bid_ask_spread_factor_(0.0),
      place_supporting_orders_(false),

      read_strat_max_loss_(false),
      read_product_maxloss_(false),
      read_returns_duration_(false),
      read_open_trade_loss_(false),
      read_inst_uts_(false),
      read_inst_maxlots_(false),
      read_portfolio_maxlots_(false),
      read_inst_mur_vec_(false),
      read_cooloff_secs_(false),
      read_base_threshold_(false),
      read_change_threshold_(false),
      read_increase_threshold_(false),
      read_decrease_threshold_(false),
      read_increase_threshold_mur_(false),
      read_decrease_threshold_mur_(false),
      read_use_book_indicator_(false),
      read_use_trade_indicator_(false),
      read_histfile_prefix_(false),
      read_hist_prices_length_(false),
      read_hist_error_length_(false),
      read_time_hysterisis_factor_(false),
      read_notional_for_unit_lot_(false),
      read_use_notional_scaling_(false),
      read_use_notional_n2d_scaling_(false),

      read_use_abnormal_volume_getflat_(false),
      read_abnormal_share_set_threshold_(false),
      read_severity_to_getflat_on_(false),
      read_base_threshold_vec_(false),

      paramfilename_(_paramfilename_) {
  LoadGolbalParamSet();
}

void GlobalParamSet::LoadGolbalParamSet() {
  std::ifstream paramfile_;
  paramfile_.open(paramfilename_.c_str(), std::ifstream::in);
  if (paramfile_.is_open()) {
    const int kParamFileLineBufferLen = 1024;
    char readline_buffer_[kParamFileLineBufferLen];
    bzero(readline_buffer_, kParamFileLineBufferLen);

    while (paramfile_.good()) {
      bzero(readline_buffer_, kParamFileLineBufferLen);
      paramfile_.getline(readline_buffer_, kParamFileLineBufferLen);
      std::string param_line(readline_buffer_);
      PerishableStringTokenizer st_(readline_buffer_, kParamFileLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() < 1) continue;

      // look at the second token and depending on string fill in the appropriate variable from the third token
      // example :
      // PARAMVALUE WORST_CASE_POSITION 60 # comments ...
      // PARAMVALUE MAX_POSITION 30 # comments ...
      // PARAMVALUE UNIT_TRADE_SIZE 5 # comments ...
      if ((strcmp(tokens_[0], "PARAMVALUE") == 0) && (tokens_.size() >= 3)) {
        if (strcmp(tokens_[1], "STRAT_MAX_LOSS") == 0) {
          strat_max_loss_ = atof(tokens_[2]);
          read_strat_max_loss_ = true;
        } else if (strcmp(tokens_[1], "PRODUCT_MAXLOSS") == 0) {
          product_maxloss_ = atof(tokens_[2]);
          read_product_maxloss_ = true;
        } else if (strcmp(tokens_[1], "RETURNS_DURATION") == 0) {
          returns_duration_ = atoi(tokens_[2]);
          read_returns_duration_ = true;
        } else if (strcmp(tokens_[1], "OPEN_TRADE_LOSS") == 0) {
          open_trade_loss_ = atof(tokens_[2]);
          read_open_trade_loss_ = true;
        } else if (strcmp(tokens_[1], "INST_UTS") == 0) {
          inst_uts_ = atoi(tokens_[2]);
          read_inst_uts_ = true;
        } else if (strcmp(tokens_[1], "INST_MAXLOTS") == 0) {
          inst_maxlots_ = atoi(tokens_[2]);
          read_inst_maxlots_ = true;
        } else if (strcmp(tokens_[1], "INST_WORSTCASELOTS") == 0) {
          inst_worstcaselots_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "PORTFOLIO_MAXLOTS") == 0) {
          portfolio_maxlots_ = atoi(tokens_[2]);
          read_portfolio_maxlots_ = true;
        } else if (strcmp(tokens_[1], "INST_MUR_VEC") == 0) {
          std::vector<char *> values_vec_;
          PerishableStringTokenizer::NonConstStringTokenizer((char *)tokens_[2], ",", values_vec_);
          for (unsigned i = 0; i < values_vec_.size(); i++) {
            inst_mur_vec_.push_back(atoi(values_vec_[i]));
          }
          read_inst_mur_vec_ = true;
        } else if (strcmp(tokens_[1], "COOLOFF_SECS") == 0) {
          cooloff_secs_ = atoi(tokens_[2]);
          read_cooloff_secs_ = true;
        } else if (strcmp(tokens_[1], "BASE_THRESHOLD") == 0) {
          base_threshold_ = atof(tokens_[2]);
          read_base_threshold_ = true;
        } else if (strcmp(tokens_[1], "PLACE_KEEP_DIFF") == 0) {
          place_keep_diff_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "BASE_THRESHOLD_VEC") == 0) {
          std::vector<char *> values_vec_;
          PerishableStringTokenizer::NonConstStringTokenizer((char *)tokens_[2], ",", values_vec_);
          for (unsigned i = 0; i < values_vec_.size(); i++) {
            base_threshold_vec_.push_back(atof(values_vec_[i]));
          }
          read_base_threshold_vec_ = true;
        } else if (strcmp(tokens_[1], "CHANGE_THRESHOLD") == 0) {
          change_threshold_ = atof(tokens_[2]);
          read_change_threshold_ = true;
        } else if (strcmp(tokens_[1], "INCREASE_THRESHOLD") == 0) {
          increase_threshold_ = atof(tokens_[2]);
          read_increase_threshold_ = true;
        } else if (strcmp(tokens_[1], "DECREASE_THRESHOLD") == 0) {
          decrease_threshold_ = atof(tokens_[2]);
          read_decrease_threshold_ = true;
        } else if (strcmp(tokens_[1], "INCREASE_THRESHOLD_MUR") == 0) {
          increase_threshold_mur_ = atof(tokens_[2]);
          read_increase_threshold_mur_ = true;
        } else if (strcmp(tokens_[1], "DECREASE_THRESHOLD_MUR") == 0) {
          decrease_threshold_mur_ = atof(tokens_[2]);
          read_decrease_threshold_mur_ = true;
        } else if (strcmp(tokens_[1], "USE_BOOK_INDICATOR") == 0) {
          use_book_indicator_ = (atoi(tokens_[2]) > 0);
          read_use_book_indicator_ = true;
        } else if (strcmp(tokens_[1], "USE_TRADE_INDICATOR") == 0) {
          use_trade_indicator_ = (atoi(tokens_[2]) > 0);
          read_use_trade_indicator_ = true;
        } else if (strcmp(tokens_[1], "HISTFILE_PREFIX") == 0) {
          histfile_prefix_ = tokens_[2];
          read_histfile_prefix_ = true;
        } else if (strcmp(tokens_[1], "HIST_PRICES_LENGTH") == 0) {
          hist_price_length_ = atoi(tokens_[2]);
          read_hist_prices_length_ = true;
        } else if (strcmp(tokens_[1], "HIST_ERROR_LENGTH") == 0) {
          hist_error_length_ = atoi(tokens_[2]);
          read_hist_error_length_ = true;
        } else if (strcmp(tokens_[1], "TIME_HYSTERISIS_FACTOR") == 0) {
          time_hysterisis_factor_ = atof(tokens_[2]);
          read_time_hysterisis_factor_ = true;
        } else if (strcmp(tokens_[1], "NOTIONAL_FOR_UNIT_LOT") == 0) {
          notional_for_unit_lot_ = atof(tokens_[2]);
          read_notional_for_unit_lot_ = true;
        } else if (strcmp(tokens_[1], "USE_NOTIONAL_SCALING") == 0) {
          use_notional_scaling_ = (atoi(tokens_[2]) > 0);
          read_use_notional_scaling_ = true;
        } else if (strcmp(tokens_[1], "USE_NOTIONAL_N2D_SCALING") == 0) {
          use_notional_n2d_scaling_ = (atoi(tokens_[2]) > 0);
          read_use_notional_n2d_scaling_ = true;
        } else if (strcmp(tokens_[1], "PRTPRICE_TYPE") == 0) {
          portprice_type_ = atoi(tokens_[2]);  // 0:lasttradeprice, 1:midprice, 2:mktwprice
        } else if (strcmp(tokens_[1], "USE_CUSTOM_SCALING") == 0) {
          std::vector<char *> values_vec_;
          PerishableStringTokenizer::NonConstStringTokenizer((char *)tokens_[2], ",", values_vec_);
          for (unsigned i = 0; i < values_vec_.size(); i++) {
            custom_scaling_vec_.push_back(atof(values_vec_[i]));
          }
          use_custom_scaling_ = true;
	} else if (strcmp(tokens_[1], "USE_DV01_RATIOS") == 0) {
	  use_dv01_ratios_ = true;
        } else if (strcmp(tokens_[1], "ABNORMAL_SHARE_SET_THRESHOLD") == 0) {
          abnormal_share_set_threshold_ = atof(tokens_[2]);
          read_abnormal_share_set_threshold_ = true;
        } else if (strcmp(tokens_[1], "USE_ABNORMAL_VOLUME_GETFLAT") == 0) {
          use_abnormal_volume_getflat_ = (atoi(tokens_[2]) > 0);
          read_use_abnormal_volume_getflat_ = true;
        } else if (strcmp(tokens_[1], "OPT_MAXLOTS") == 0) {
          opt_maxlots_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "MIN_BID_INT_PRICE_TO_TRADE") == 0) {
          min_bid_int_price_to_trade_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "STAGGERED_GETFLAT_TIME") == 0) {
          for (unsigned i = 2; i < tokens_.size(); i++) {
            staggered_getflat_msecs_vec_.push_back(
                HFSAT::DateTime::GetUTCSecondsFromMidnightFromTZHHMM(tradingdate_, atoi(tokens_[i] + 4), tokens_[i]) *
                1000);
          }
        } else if (strcmp(tokens_[1], "AGGRESSIVE_GETFLAT_MSECS") == 0) {
          aggressive_getflat_msecs_ =
              HFSAT::DateTime::GetUTCSecondsFromMidnightFromTZHHMM(tradingdate_, atoi(tokens_[2] + 4), tokens_[2]) *
              1000;
        } else if (strcmp(tokens_[1], "RATE_IN_MINUTES_TO_RECOMPUTE_DELTA") == 0) {
          rate_in_minutes_to_recompute_delta_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "IV_MODEL_SETTING") == 0) {
          iv_model_setting_ = atoi(tokens_[2]);
          if (tokens_.size() >= 4) {
            iv_model_aux_token_1_ = atoi(tokens_[3]);
          }
          if (tokens_.size() >= 5) {
            iv_model_aux_token_2_ = atoi(tokens_[4]);
          }
        } else if (strcmp(tokens_[1], "MIN_DAYS_TO_EXPIRY_TO_TRADE") == 0) {
          min_days_to_expiry_to_trade_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "AGGRESSIVE") == 0) {
          aggressive_ = (atoi(tokens_[2]) > 0);
        } else if (strcmp(tokens_[1], "MAX_INT_SPREAD_TO_CROSS") == 0) {
          max_int_spread_to_cross_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "AGG_COOLOFF_SECS") == 0) {
          agg_cooloff_secs_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "OPT_BID_ASK_SPREAD_FACTOR") == 0) {
          opt_bid_ask_spread_factor_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "PLACE_SUPPORTING_ORDERS") == 0) {
          place_supporting_orders_ = (bool)atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "SEVERITY_TO_GETFLAT_ON") == 0) {
          severity_to_getflat_on_ = atof(tokens_[2]);
          read_severity_to_getflat_on_ = true;
        }
      }
    }
  }
  ReconcileParams();
}

void GlobalParamSet::ReconcileParams() {
  if (read_increase_threshold_mur_ && !read_increase_threshold_) {
    increase_threshold_ = increase_threshold_mur_ / inst_maxlots_;
  }
  if (read_decrease_threshold_mur_ && !read_decrease_threshold_) {
    decrease_threshold_ = decrease_threshold_mur_ / inst_maxlots_;
  }
}
}
