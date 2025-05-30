/* =====================================================================================

   Filename:  ExecLogicCode/options_mean_reverting_trading.cpp

   Description:

   Version:  1.0
   Created:  Monday 30 May 2016 05:06:09  UTC
   Revision:  none
   Compiler:  g++

   Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011

   Address:  Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   Phone:  +91 80 4190 3551

   =====================================================================================
*/
#include <numeric>
#include "dvctrade/ExecLogic/options_mean_reverting_trading.hpp"
#define INVALID_MEAN_REVERTING_ORDER_PRICE -1

namespace HFSAT {

void OptionsMeanRevertingTrading::CollectSourceShortCodes(DebugLogger& _dbglogger_, std::string product_filename,
                                                          std::vector<std::string>& source_shortcode_vec_,
                                                          std::vector<std::string>& ors_source_needed_vec_) {
  std::ifstream products_file_ifstream_;
  products_file_ifstream_.open(product_filename);
  // format : FUT OPTIONSLOGIC STEP_INCREAMENT NO_OF_OPTIONS
  // NSE_PNB_FUT0 OTMInAOI 1 6

  if (products_file_ifstream_.is_open()) {
    const int kProductsFileBufflerLen = 1024;
    char readlinebuffer_[kProductsFileBufflerLen];
    bzero(readlinebuffer_, kProductsFileBufflerLen);
    while (products_file_ifstream_.good()) {
      bzero(readlinebuffer_, kProductsFileBufflerLen);
      products_file_ifstream_.getline(readlinebuffer_, kProductsFileBufflerLen);
      PerishableStringTokenizer st_(readlinebuffer_, kProductsFileBufflerLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() == 4) {
        // making sure size of fut_shortcode_vec_ and opt_object_vec_ is same !
        //      fut_shortcode_vec_.push_back(tokens_[0]);
        //      opt_shortcode_vec_.push_back(HFSAT::OptionsExecLogicUtils::GetOptionsShortcodesFromLogic(tokens_));
        VectorUtils::UniqueVectorAdd(source_shortcode_vec_, (std::string)tokens_[0]);
        VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string)tokens_[0]);

        std::vector<std::string> list_of_options_ = NSEExecLogicUtils::GetOptionsShortcodesFromLogic(tokens_);
        VectorUtils::UniqueVectorAdd(source_shortcode_vec_, list_of_options_);
        // VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, list_of_options_);
      }
      // non trading futures
      if (tokens_.size() == 1) {
        VectorUtils::UniqueVectorAdd(source_shortcode_vec_, (std::string)tokens_[0]);
        VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string)tokens_[0]);
      }
    }
  }
}

void OptionsMeanRevertingTrading::CollectTradingShortCodes(DebugLogger& _dbglogger_, std::string product_filename,
                                                           std::vector<std::string>& _trading_product_vec_) {
  std::ifstream products_file_ifstream_;
  products_file_ifstream_.open(product_filename);
  // format : FUT OPTIONSLOGIC STEP_INCREAMENT NO_OF_OPTIONS
  // NSE_PNB_FUT0 OTMInAOI 1 6

  if (products_file_ifstream_.is_open()) {
    const int kProductsFileBufflerLen = 1024;
    char readlinebuffer_[kProductsFileBufflerLen];
    bzero(readlinebuffer_, kProductsFileBufflerLen);
    while (products_file_ifstream_.good()) {
      bzero(readlinebuffer_, kProductsFileBufflerLen);
      products_file_ifstream_.getline(readlinebuffer_, kProductsFileBufflerLen);
      PerishableStringTokenizer st_(readlinebuffer_, kProductsFileBufflerLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      // only options are sent ( for som )
      if (tokens_.size() == 4) {
        // making sure size of fut_shortcode_vec_ and opt_object_vec_ is same !
        //      fut_shortcode_vec_.push_back(tokens_[0]);
        //      opt_shortcode_vec_.push_back(HFSAT::OptionsExecLogicUtils::GetOptionsShortcodesFromLogic(tokens_));
        std::vector<std::string> list_of_options_ = NSEExecLogicUtils::GetOptionsShortcodesFromLogic(tokens_);
        VectorUtils::UniqueVectorAdd(_trading_product_vec_, list_of_options_);
      }
    }
  }
}

OptionsMeanRevertingTrading::OptionsMeanRevertingTrading(
    DebugLogger& _dbglogger_, const Watch& _watch_, std::string _products_filename,
    const std::vector<SecurityMarketView*> _dep_market_view_fut_vec_,
    const std::vector<SmartOrderManager*> _order_manager_fut_vec_, const std::string& _global_paramfilename_,
    const bool _livetrading_, MultBasePNL* _mult_base_pnl_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_)
    // we are passing options only for now as we intend to trade only options
    // here we are simply make sure to listen to executions

    : MultExecInterface(_dbglogger_, _watch_, _dep_market_view_fut_vec_, _order_manager_fut_vec_,
                        _global_paramfilename_, _livetrading_, _trading_start_utc_mfm_, _trading_end_utc_mfm_),
      mult_base_pnl_(_mult_base_pnl_) {
  mult_base_pnl_->AddListener(this);

  days_to_expiry_ = NSEExecLogicUtils::NumDaysToExpire(watch_.YYYYMMDD(), "STKOPT");
  if (days_to_expiry_ < global_param_set_->min_days_to_expiry_to_trade_) {
    DBGLOG_TIME_CLASS_FUNC_LINE << "min_day_to_expiry " << days_to_expiry_ << "\n";
    exit(0);
  }

  CollectFuturesOptionsShortCodes(_products_filename);
  num_trading_futures_ = trading_fut_shortcode_vec_.size();
  num_nontrading_futures_ = nontrading_fut_shortcode_vec_.size();
  num_futures_ = num_trading_futures_ + num_nontrading_futures_;
  // hack we are expecting size of options to be same !
  num_options_ = num_trading_futures_ * num_options_per_future_;

  // we are making sure out security ids are assigned as follows:
  // trading_fut n_options [0 -> n] ( 0 is also allowed )
  // trading_fut n_options [1(n+1) -> 2(n)+1]
  // trading_fut n_options [(k-1)*(n+1) -> k(n)+1]

  // non_trading_fut k(n+1) + 0
  // non_trading_fut k(n+1) + 1
  // non_trading_fut k(n+1) + m-1

  // k + m == total number of futures
  // security ids of futures, 0 * (n + 1), 1 * (n + 1) ..... k-1 * (n+1), k
  //

  // n -> no of options
  // k -> no of trading futures
  // m -> no of nontrading futures
  for (unsigned int t_fut_idx_ = 0; t_fut_idx_ < num_trading_futures_; t_fut_idx_++) {
    future_smv_vec_.push_back(
        ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(trading_fut_shortcode_vec_[t_fut_idx_]));
  }
  for (unsigned int t_fut_idx_ = 0; t_fut_idx_ < num_nontrading_futures_; t_fut_idx_++) {
    future_smv_vec_.push_back(
        ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(nontrading_fut_shortcode_vec_[t_fut_idx_]));
  }

  // future prod_id is the index in future_smv_vec_
  // option prod_id is the index in dep_market_view_vec_
  // we make security_id -> {future_prod_id, option_prod_id}
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  secid_2_prodid_.resize(sec_name_indexer_.NumSecurityId(), std::pair<int, int>(-1, -1));
  for (unsigned int t_fut_idx_ = 0, t_opt_idx_ = 0; t_fut_idx_ < num_futures_; t_fut_idx_++) {
    std::pair<int, int> fut_opt_id_(t_fut_idx_, -1);
    int t_security_id_ = future_smv_vec_[t_fut_idx_]->security_id();
    secid_2_prodid_[t_security_id_] = fut_opt_id_;
    for (unsigned int t_count_ = 0; t_count_ < num_options_per_future_ && t_opt_idx_ < num_options_;
         t_count_++, t_opt_idx_++) {
      fut_opt_id_ = std::make_pair(t_fut_idx_, t_opt_idx_);
      t_security_id_ = dep_market_view_vec_[t_opt_idx_]->security_id();
      secid_2_prodid_[t_security_id_] = fut_opt_id_;
    }
  }

  // initializing risk variables
  // initializing premium variables
  // initializing core strategy logic variables
  Initialize();

  // dont_trade_vec
  std::vector<int> t_dont_trade_secid_vec_;
  NSEExecLogicUtils::SetDontTradeForEarnings(watch_, dbglogger_, t_dont_trade_secid_vec_, future_smv_vec_);
  NSEExecLogicUtils::SetDontTradeForBan(watch_, dbglogger_, t_dont_trade_secid_vec_);
  NSEExecLogicUtils::SetDontTradeForCorAct(watch_, dbglogger_, t_dont_trade_secid_vec_);
  for (unsigned int i = 0; i < t_dont_trade_secid_vec_.size(); i++) {
    dont_trade_[secid_2_prodid_[t_dont_trade_secid_vec_[i]].first] = true;
    DBGLOG_TIME_CLASS_FUNC_LINE << "Trading in "
                                << future_smv_vec_[secid_2_prodid_[t_dont_trade_secid_vec_[i]].first]->shortcode()
                                << " disabled\n";
  }

  watch_.subscribe_OneMinutePeriod(this);
  DBGLOG_TIME_CLASS_FUNC_LINE << "Sucessfully created exec objects\n";
}

void OptionsMeanRevertingTrading::CollectFuturesOptionsShortCodes(std::string product_filename) {
  std::ifstream products_file_ifstream_;
  products_file_ifstream_.open(product_filename);
  // format : FUT OPTIONSLOGIC STEP_INCREAMENT NO_OF_OPTIONS
  // NSE_PNB_FUT0 OTMInAOI 1 6

  if (products_file_ifstream_.is_open()) {
    num_options_per_future_ = 0;

    const int kProductsFileBufflerLen = 1024;
    char readlinebuffer_[kProductsFileBufflerLen];
    bzero(readlinebuffer_, kProductsFileBufflerLen);
    while (products_file_ifstream_.good()) {
      bzero(readlinebuffer_, kProductsFileBufflerLen);
      products_file_ifstream_.getline(readlinebuffer_, kProductsFileBufflerLen);
      PerishableStringTokenizer st_(readlinebuffer_, kProductsFileBufflerLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() == 4) {
        // making sure size of fut_shortcode_vec_ and opt_object_vec_ is same !
        //      fut_shortcode_vec_.push_back(tokens_[0]);
        //      opt_shortcode_vec_.push_back(HFSAT::OptionsExecLogicUtils::GetOptionsShortcodesFromLogic(tokens_));
        std::vector<std::string> list_of_options_ = NSEExecLogicUtils::GetOptionsShortcodesFromLogic(tokens_);

        trading_fut_shortcode_vec_.push_back((std::string)tokens_[0]);
        if (num_options_per_future_ == 0) {
          VectorUtils::UniqueVectorAdd(opt_shortcode_vec_, list_of_options_);
          num_options_per_future_ = list_of_options_.size();
        } else if (num_options_per_future_ == list_of_options_.size()) {
          VectorUtils::UniqueVectorAdd(opt_shortcode_vec_, list_of_options_);
        } else {
          std::cerr << "expected no of options " << num_options_per_future_ << ", instead we got "
                    << list_of_options_.size() << " for " << tokens_[0] << "\n";
          exit(-1);
        }
      }
      if (tokens_.size() == 1) {
        if (std::find(trading_fut_shortcode_vec_.begin(), trading_fut_shortcode_vec_.end(), tokens_[0]) ==
            trading_fut_shortcode_vec_.end()) {
          nontrading_fut_shortcode_vec_.push_back((std::string)tokens_[0]);
        }
      }
    }
  }
}

void OptionsMeanRevertingTrading::Initialize() {
  // global risk one per exec

  global_min_pnl_ = 0;
  product_delta_adjusted_position_in_lots_.resize(num_futures_, 0.0);
  product_position_in_lots_.resize(num_options_);

  if (livetrading_) {
    fut_level_should_be_getting_flat_.resize(num_futures_, true);
    opt_level_should_be_getting_flat_.resize(num_options_, true);

    fut_level_should_be_getting_flat_aggressively_.resize(num_futures_, false);
  } else {
    fut_level_should_be_getting_flat_.resize(num_futures_, false);
    opt_level_should_be_getting_flat_.resize(num_options_, false);

    fut_level_should_be_getting_flat_aggressively_.resize(num_futures_, false);
  }

  // we first get best price to place in futures and while placing actual orders,
  // we look at options to get same delta exposure
  risk_adjusted_model_future_bid_price_vec_.resize(num_futures_, INVALID_MEAN_REVERTING_ORDER_PRICE);
  risk_adjusted_model_future_ask_price_vec_.resize(num_futures_, INVALID_MEAN_REVERTING_ORDER_PRICE);
  bid_int_price_to_place_at_.resize(num_options_, INVALID_MEAN_REVERTING_ORDER_PRICE);
  ask_int_price_to_place_at_.resize(num_options_, INVALID_MEAN_REVERTING_ORDER_PRICE);

  // rate of placing orders,
  seconds_at_last_buy_.resize(num_options_, 0);
  seconds_at_last_sell_.resize(num_options_, 0);

  is_ready_vec_.resize(num_options_, false);

  // subscribing only to price events + trade events
  if (num_futures_ != future_smv_vec_.size()) {
    std::cerr << "logical error future_smv_vec size is not as expected\n";
    exit(-1);
  }
  for (unsigned int t_fut_idx_ = 0; t_fut_idx_ < num_futures_; t_fut_idx_++) {
    future_smv_vec_[t_fut_idx_]->subscribe_price_type(this, kPriceTypeMidprice);
  }

  // we are susbscribing to options to improve executions
  for (unsigned int t_opt_idx_ = 0; t_opt_idx_ < num_options_; t_opt_idx_++) {
    dep_market_view_vec_[t_opt_idx_]->subscribe_price_type(this, kPriceTypeMidprice);
  }

  // now strategy core logic
  residuals_.resize(num_futures_);
  residual_sum_.resize(num_futures_);
  residual_sumsqr_.resize(num_futures_);
  stdev_residuals_.resize(num_futures_);
  inst_prices_.resize(num_futures_);
  port_prices_.resize(num_futures_);
  inst_betas_.resize(num_futures_);
  last_inst_prices_.resize(num_futures_);
  last_port_prices_.resize(num_futures_);
  for (unsigned int t_fut_idx_ = 0; t_fut_idx_ < num_futures_; t_fut_idx_++) {
    residuals_[t_fut_idx_].clear();
    inst_prices_[t_fut_idx_].clear();
    port_prices_[t_fut_idx_].clear();
  }
  msecs_at_last_vec_processing_ = 0;
  predictor_vec_.resize(num_futures_, std::vector<std::pair<int, double>>());
  dont_trade_.resize(num_futures_, false);

  ReadHistValues();
  DBGLOG_TIME_CLASS_FUNC_LINE << "HistValues Read\n";
  SetOptionObjects();
  DBGLOG_TIME_CLASS_FUNC_LINE << "OptionObjects Set\n";
  SetImpliedVolCalculator();
  DBGLOG_TIME_CLASS_FUNC_LINE << "ImpliedVolCalculator Set\n";
  ExpandRiskPremiumVarsAcrossFutures();
  DBGLOG_TIME_CLASS_FUNC_LINE << "RiskPremiumVarsAcrossFutures Set\n";
  ExpandRiskPremiumVarsAcrossOptions();
  DBGLOG_TIME_CLASS_FUNC_LINE << "RiskPremiumVarsAcrossOptions Set\n";
}

// copy of mean_reverting_trading !!
void OptionsMeanRevertingTrading::ReadHistValues() {
  char t_filename_[1024];
  //    sprintf(t_filename_,"/spare/local/tradeinfo/NSE_Files/MRHist/%s.%d",(global_param_set_->histfile_prefix_).c_str(),
  //    watch_.YYYYMMDD());
  sprintf(t_filename_, "%s%d", (global_param_set_->histfile_prefix_).c_str(), watch_.YYYYMMDD());
  //    std::cout << " File " << t_filename_ << '\n';
  std::ifstream histfile_;
  histfile_.open(t_filename_);
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  if (histfile_.is_open()) {
    const int kParamfileListBufflerLen = 51200;
    char readlinebuffer_[kParamfileListBufflerLen];
    bzero(readlinebuffer_, kParamfileListBufflerLen);
    int t_stk_id_ = 0;
    int t_prod_id_ = 0;
    while (histfile_.good()) {
      bzero(readlinebuffer_, kParamfileListBufflerLen);
      histfile_.getline(readlinebuffer_, kParamfileListBufflerLen);
      std::string this_line_ = std::string(readlinebuffer_);
      PerishableStringTokenizer st_(readlinebuffer_, kParamfileListBufflerLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() == 0) {
        continue;
      }
      if (strcmp(tokens_[0], "Port:") == 0) {
        // format of this line will be target_stock num_predictors pred_stock_1 weight_1 .... pred_stock_n weight_n
        t_stk_id_ = sec_name_indexer_.GetIdFromString(tokens_[1]);
        if (t_stk_id_ < 0) {
          // data for a stock which is not in our trading config  .. continue
          continue;
        } else {
          t_prod_id_ = secid_2_prodid_[t_stk_id_].first;
          unsigned int t_num_preds_ = atoi(tokens_[2]);
          if (tokens_.size() != 3 + 2 * t_num_preds_) {
            std::cerr << "Malformed first line of stock data in hist file .. exiting \n";
            exit(-1);
          }
          for (unsigned int t_ctr_ = 0; t_ctr_ < t_num_preds_; t_ctr_++) {
            int t_pred_id_ = sec_name_indexer_.GetIdFromString(tokens_[3 + 2 * t_ctr_]);
            double t_weight_ = atof(tokens_[4 + 2 * t_ctr_]);
            if (t_pred_id_ < 0) {
              std::cerr << " Predictor " << tokens_[3 + 2 * t_ctr_] << " not present in strat config .. exiting \n";
              exit(-1);
            }
            int t_pred_prod_id_ = secid_2_prodid_[t_pred_id_].first;
            std::pair<int, double> pred_entry_(t_prod_id_, t_weight_);
            // indep_secid -> {dep_secid, beta}
            predictor_vec_[t_pred_prod_id_].push_back(pred_entry_);
          }
        }
      } else if (strcmp(tokens_[0], "HIST_PRICES") == 0 &&
                 tokens_.size() == (2 * global_param_set_->hist_price_length_ + 1) && t_stk_id_ >= 0) {
        // format of line is stk_px{t-1} port_px{t-1} ... stk_px{0} port_px{0}
        // here we need to map t_stk_id_ <-> fut_index_
        for (unsigned int t_ctr_ = 0; t_ctr_ < global_param_set_->hist_price_length_; t_ctr_++) {
          inst_prices_[t_prod_id_].push_back(atof(tokens_[t_ctr_ * 2 + 1]));
          port_prices_[t_prod_id_].push_back(atof(tokens_[t_ctr_ * 2 + 2]));
        }
        inst_betas_[t_prod_id_] = std::inner_product(inst_prices_[t_prod_id_].begin(), inst_prices_[t_prod_id_].end(),
                                                     port_prices_[t_prod_id_].begin(), 0.0) /
                                  std::inner_product(port_prices_[t_prod_id_].begin(), port_prices_[t_prod_id_].end(),
                                                     port_prices_[t_prod_id_].begin(), 0.0);
        last_port_prices_[t_prod_id_] = atof(tokens_[2 * global_param_set_->hist_price_length_]);
        last_inst_prices_[t_prod_id_] = atof(tokens_[2 * global_param_set_->hist_price_length_ - 1]);
        DBGLOG_TIME_CLASS_FUNC_LINE << "Instrument: " << future_smv_vec_[t_prod_id_]->shortcode() << " beta "
                                    << inst_betas_[t_prod_id_] << " Pxs " << last_inst_prices_[t_prod_id_] << ':'
                                    << last_port_prices_[t_prod_id_] << '\n';
      } else if ((strcmp(tokens_[0], "HIST_ERROR") == 0) &&
                 (tokens_.size() == global_param_set_->hist_error_length_ + 1) && (t_prod_id_ >= 0)) {
        for (unsigned int t_ctr_ = 0; t_ctr_ < global_param_set_->hist_error_length_; t_ctr_++) {
          residuals_[t_prod_id_].push_back(atof(tokens_[t_ctr_ + 1]));
        }
        residual_sum_[t_prod_id_] = std::accumulate(residuals_[t_prod_id_].begin(), residuals_[t_prod_id_].end(), 0.0);
        residual_sumsqr_[t_prod_id_] = std::inner_product(residuals_[t_prod_id_].begin(), residuals_[t_prod_id_].end(),
                                                          residuals_[t_prod_id_].begin(), 0.0);
        stdev_residuals_[t_prod_id_] = sqrt(residual_sumsqr_[t_prod_id_] / global_param_set_->hist_error_length_ -
                                            residual_sum_[t_prod_id_] / global_param_set_->hist_error_length_ *
                                                residual_sum_[t_prod_id_] / global_param_set_->hist_error_length_);
        DBGLOG_TIME_CLASS_FUNC_LINE << "Instrument: " << future_smv_vec_[t_prod_id_]->shortcode() << " last error "
                                    << residuals_[t_prod_id_][global_param_set_->hist_error_length_ - 1] << " stdev "
                                    << stdev_residuals_[t_prod_id_] << '\n';
      } else if (strcmp(tokens_[0], "LAST_PRICE") == 0) {
        // format is LAST_PRICE INST1 PX1 .. INSTN PXN
        for (unsigned int t_ctr_ = 1; t_ctr_ < tokens_.size(); t_ctr_ = t_ctr_ + 2) {
          int t_sec_id_ = sec_name_indexer_.GetIdFromString(tokens_[t_ctr_]);
          if (t_sec_id_ >= 0) {
            int t_fut_idx_ = secid_2_prodid_[t_sec_id_].first;
            last_inst_prices_[t_fut_idx_] = atof(tokens_[t_ctr_ + 1]);
          }
        }
      } else if (strcmp(tokens_[0], "INST_VOLATILITY") == 0) {
        // format is INST_VOLATILITY INST1 VOL1 .. INSTN VOLN
        // values are normalized for banknifty vol being 1
        for (unsigned int t_ctr_ = 1; t_ctr_ < tokens_.size(); t_ctr_ = t_ctr_ + 2) {
          int t_sec_id_ = sec_name_indexer_.GetIdFromString(tokens_[t_ctr_]);
          if (t_sec_id_ >= 0) {
            // inst_return_vol_[t_sec_id_] = atof(tokens_[t_ctr_ + 1]);
          }
        }
      } else if (t_stk_id_ >= 0 && tokens_.size() > 0) {
        std::cerr << "Error - hist file format incorrect " << tokens_[0] << " " << tokens_.size() << '\n';
        exit(-1);
      }
    }
  }
  histfile_.close();

  // disable trading in products which are not specified in hist file
  for (unsigned int t_fut_idx_ = 0; t_fut_idx_ < num_trading_futures_; t_fut_idx_++) {
    if (residuals_[t_fut_idx_].size() != global_param_set_->hist_error_length_ ||
        inst_prices_[t_fut_idx_].size() != global_param_set_->hist_price_length_) {
      dont_trade_[t_fut_idx_] = true;
      DBGLOG_TIME_CLASS_FUNC_LINE << "Trading in " << future_smv_vec_[t_fut_idx_]->shortcode() << " disabled\n";
    }
  }
  for (unsigned int t_fut_idx_ = num_trading_futures_; t_fut_idx_ < num_futures_; t_fut_idx_++) {
    dont_trade_[t_fut_idx_] = true;
    DBGLOG_TIME_CLASS_FUNC_LINE << "Trading in " << future_smv_vec_[t_fut_idx_]->shortcode() << " disabled\n";
  }

  // validate portfolio constituents -- debug mode
  for (unsigned int t_fut_idx_ = 0; t_fut_idx_ < num_futures_; t_fut_idx_++) {
    DumpPortfolioConstituents(t_fut_idx_);
  }
}

// may be some sanity check here ?
void OptionsMeanRevertingTrading::DumpPortfolioConstituents(unsigned int prod_id) {
  DBGLOG_TIME_CLASS_FUNC_LINE << " Instrument:Px " << future_smv_vec_[prod_id]->shortcode() << ":"
                              << last_inst_prices_[prod_id] << " DumpPortConst \n";
  DBGLOG_TIME_CLASS_FUNC_LINE << " Last port px " << last_port_prices_[prod_id] << '\n';
  if (prod_id < num_trading_futures_ && (last_port_prices_[prod_id] / last_inst_prices_[prod_id] > 1.10 ||
                                         last_port_prices_[prod_id] / last_inst_prices_[prod_id] < 0.90)) {
    std::cerr << watch_.YYYYMMDD() << ": Last Port Price and Last Intrument Price are too far .. exiting\n";
    exit(-1);
  }
  double t_comp_px_ = 0;
  for (unsigned int t_ctr_ = 0; t_ctr_ < num_futures_; t_ctr_++) {
    std::vector<std::pair<int, double>>::iterator t_iter_;
    for (t_iter_ = predictor_vec_[t_ctr_].begin(); t_iter_ != predictor_vec_[t_ctr_].end(); t_iter_++) {
      if ((*t_iter_).first == (int)prod_id) {
        DBGLOG_TIME_CLASS_FUNC_LINE << "Port constituent " << future_smv_vec_[t_ctr_]->shortcode() << " weight "
                                    << (*t_iter_).second << " inst px " << last_inst_prices_[t_ctr_] << '\n';
        t_comp_px_ = t_comp_px_ + (*t_iter_).second * last_inst_prices_[t_ctr_];
      }
    }
  }
  // i am not sure why computed port prices will different from last_port_price but just in case
  DBGLOG_TIME_CLASS_FUNC_LINE << " Computed port px " << t_comp_px_ << "\n\n";
  if (prod_id < num_trading_futures_ &&
      (t_comp_px_ / last_inst_prices_[prod_id] > 1.10 || t_comp_px_ / last_inst_prices_[prod_id] < 0.90)) {
    std::cerr << "Computed Port Price and Instrument Last Price are too far .. exiting\n";
    exit(-1);
  }
}

void OptionsMeanRevertingTrading::SetOptionObjects() {
  HFSAT::NSESecurityDefinitions& t_nse_secdef_ = HFSAT::NSESecurityDefinitions::GetUniqueInstance(watch_.YYYYMMDD());

  for (unsigned int t_fut_idx_ = 0, t_opt_idx_ = 0; t_fut_idx_ < num_trading_futures_; t_fut_idx_++) {
    double t_fut_px_ = t_nse_secdef_.GetLastClose(future_smv_vec_[t_fut_idx_]->shortcode());
    for (unsigned int t_count_ = 0; t_count_ < num_options_per_future_ && t_opt_idx_ < num_options_;
         t_count_++, t_opt_idx_++) {
      // first make the option object
      HFSAT::OptionObject* t_opt_obj_ =
          HFSAT::OptionObject::GetUniqueInstance(dbglogger_, watch_, dep_market_view_vec_[t_opt_idx_]->shortcode());
      option_obj_vec_.push_back(t_opt_obj_);

      // initialize greeks ( specifically delta ), update every 15 minutes
      double t_opt_px_ = t_nse_secdef_.GetLastCloseForOptions(dep_market_view_vec_[t_opt_idx_]->shortcode());
      while (t_opt_px_ < 0) {
        DBGLOG_TIME_CLASS_FUNC_LINE << "no last close price for " << dep_market_view_vec_[t_opt_px_]->shortcode()
                                    << "\n";
        std::string shc_ =
            HFSAT::NSESecurityDefinitions::GetPrevOptionInCurrentSchema(dep_market_view_vec_[t_opt_idx_]->shortcode());
        t_opt_px_ = t_nse_secdef_.GetLastCloseForOptions(shc_);
      }
      t_opt_obj_->ComputeGreeks(t_fut_px_, t_opt_px_);
      option_delta_vec_.push_back(t_opt_obj_->greeks_.delta_);
      DBGLOG_TIME_CLASS_FUNC_LINE << dep_market_view_vec_[t_opt_idx_]->shortcode() << " FutPx: " << t_fut_px_
                                  << " OptPx: " << t_opt_px_ << " Delta: " << t_opt_obj_->greeks_.delta_
                                  << " Gamma: " << t_opt_obj_->greeks_.gamma_ << " Vega: " << t_opt_obj_->greeks_.vega_
                                  << DBGLOG_ENDL_FLUSH;
    }
  }
}

void OptionsMeanRevertingTrading::OnTimePeriodUpdate(const int _num_pages_to_add_) {
  bool t_reduce_max_lots_ = false;
  if (global_param_set_->staggered_getflat_msecs_vec_.size() > 0 &&
      watch_.msecs_from_midnight() >= global_param_set_->staggered_getflat_msecs_vec_[0]) {
    t_reduce_max_lots_ = true;
    global_param_set_->staggered_getflat_msecs_vec_.erase(global_param_set_->staggered_getflat_msecs_vec_.begin());
  }

  // whenever we recompute delta we also figure out which one to start and stop trading
  // using getflat due to liquidy
  static int counter_to_adjust_delta_ = 0;
  bool t_recompute_delta_ = false;
  if (counter_to_adjust_delta_ % global_param_set_->rate_in_minutes_to_recompute_delta_ == 0) {
    t_recompute_delta_ = true;
  }
  counter_to_adjust_delta_++;

  for (unsigned int t_fut_idx_ = 0, t_opt_idx_ = 0; t_fut_idx_ < num_trading_futures_; t_fut_idx_++) {
    double t_fut_px_ = future_smv_vec_[t_fut_idx_]->mid_price();
    for (unsigned int t_count_ = 0; t_count_ < num_options_per_future_ && t_opt_idx_ < num_options_;
         t_count_++, t_opt_idx_++) {
      if (t_recompute_delta_) {
        option_obj_vec_[t_opt_idx_]->ComputeGreeks(t_fut_px_, dep_market_view_vec_[t_opt_idx_]->mid_price());
        option_delta_vec_[t_opt_idx_] = option_obj_vec_[t_opt_idx_]->greeks_.delta_;
        DBGLOG_TIME_CLASS_FUNC_LINE << "Recomputed Delta " << option_delta_vec_[t_opt_idx_] << "\n";
        // trade only top x liquid call and put options
        // liquidity can be defined as abs(delta) ?
      }

      if (t_reduce_max_lots_) {
        option_maxlots_[t_opt_idx_] /= 2;
        DBGLOG_TIME_CLASS_FUNC_LINE << "Reset MaxLots " << option_maxlots_[t_opt_idx_] << "\n";
      }
    }
  }
}

void OptionsMeanRevertingTrading::SetImpliedVolCalculator() {
  for (unsigned int t_opt_idx_ = 0u; t_opt_idx_ < num_options_; t_opt_idx_++) {
    CommonIndicator* t_iv_indicator_;
    if (global_param_set_->iv_model_setting_ == 0) {
      t_iv_indicator_ = nullptr;
    } else if (global_param_set_->iv_model_setting_ == 1) {
      t_iv_indicator_ = ImpliedVolCalculator::GetUniqueInstance(dbglogger_, watch_, *dep_market_view_vec_[t_opt_idx_],
                                                                kPriceTypeMidprice);
    } else if (global_param_set_->iv_model_setting_ == 2) {
      t_iv_indicator_ =
          MovingAverageImpliedVol::GetUniqueInstance(dbglogger_, watch_, *dep_market_view_vec_[t_opt_idx_],
                                                     global_param_set_->iv_model_aux_token_1_, kPriceTypeMidprice);
    } else if (global_param_set_->iv_model_setting_ == 3) {
      t_iv_indicator_ = MovingAvgPriceImpliedVol::GetUniqueInstance(dbglogger_, watch_, *dep_market_view_vec_[t_opt_idx_], global_param_set_->iv_model_aux_token_1_, kPriceTypeMidprice);
    } else if (global_param_set_->iv_model_setting_ == 4) {
      // based on number of days to expiry
      // closer to expiry higher the effect of iv
      // also higher the effect of iv based on abs(fut-strike)
      // abs(fut_price - strike), num_days_to_expiry_
      if (days_to_expiry_ > global_param_set_->iv_model_aux_token_2_) {
	t_iv_indicator_ = MovingAverageImpliedVol::GetUniqueInstance(dbglogger_, watch_, *dep_market_view_vec_[t_opt_idx_], global_param_set_->iv_model_aux_token_1_, kPriceTypeMidprice);
      } else {
        t_iv_indicator_ = nullptr;
      }
    }
    implied_vol_vec_.push_back(t_iv_indicator_);
  }
}

void OptionsMeanRevertingTrading::ExpandRiskPremiumVarsAcrossFutures() {
  // set inst specific parameters
  inst_unitlots_.resize(num_futures_);
  inst_maxlots_.resize(num_futures_);
  inst_base_threshold_.resize(num_futures_);
  inst_increase_threshold_.resize(num_futures_);
  inst_decrease_threshold_.resize(num_futures_);

  inst_return_vol_.resize(num_futures_, 1.0);
  for (unsigned int t_fut_idx_ = 0; t_fut_idx_ < num_futures_; t_fut_idx_++) {
    if (global_param_set_->use_notional_scaling_) {
      // max introduces since last_inst_prices_ for an instrument is not necessarily present in histfile.
      double t_inst_notional_per_lot_ =
          std::max(100000.0, last_inst_prices_[t_fut_idx_] * (future_smv_vec_[t_fut_idx_]->min_order_size()));
      inst_unitlots_[t_fut_idx_] = std::max(1, (int)round(global_param_set_->notional_for_unit_lot_ *
                                                          global_param_set_->inst_uts_ / t_inst_notional_per_lot_));
      inst_maxlots_[t_fut_idx_] =
          std::max(1, (int)round(global_param_set_->notional_for_unit_lot_ * global_param_set_->inst_maxlots_ /
                                 (inst_return_vol_[t_fut_idx_] * t_inst_notional_per_lot_)));
      if (inst_maxlots_[t_fut_idx_] == 1) {
        inst_base_threshold_[t_fut_idx_] =
            global_param_set_->base_threshold_ +
            (global_param_set_->inst_maxlots_ - 1) * global_param_set_->increase_threshold_ / 2.0;
        inst_increase_threshold_[t_fut_idx_] = global_param_set_->increase_threshold_;
        inst_decrease_threshold_[t_fut_idx_] = global_param_set_->decrease_threshold_ +
                                               (inst_base_threshold_[t_fut_idx_] - global_param_set_->base_threshold_);
      } else {
        inst_base_threshold_[t_fut_idx_] = global_param_set_->base_threshold_;
        inst_increase_threshold_[t_fut_idx_] =
            global_param_set_->increase_threshold_ * global_param_set_->inst_maxlots_ / inst_maxlots_[t_fut_idx_];
        inst_decrease_threshold_[t_fut_idx_] =
            global_param_set_->decrease_threshold_ * global_param_set_->inst_maxlots_ / inst_maxlots_[t_fut_idx_];
      }
    } else {
      inst_unitlots_[t_fut_idx_] = global_param_set_->inst_uts_;
      inst_maxlots_[t_fut_idx_] = global_param_set_->inst_maxlots_;
      inst_base_threshold_[t_fut_idx_] = global_param_set_->base_threshold_;
      inst_increase_threshold_[t_fut_idx_] = global_param_set_->increase_threshold_;
      inst_decrease_threshold_[t_fut_idx_] = global_param_set_->decrease_threshold_;
    }
  }
}

void OptionsMeanRevertingTrading::ExpandRiskPremiumVarsAcrossOptions() {
  // delta bid-ask-spread 
  // liquidity cost_of_trade ( buy + sell )
  // assumption: delta caputures liquidity : bid_ask_spread captures execution_cost  

  option_unitlots_.resize(num_options_);
  option_maxlots_.resize(num_options_);
  for (unsigned int t_fut_idx_ = 0, t_opt_idx_ = 0; t_fut_idx_ < num_trading_futures_; t_fut_idx_++) {
    for (unsigned int t_count_ = 0; t_count_ < num_options_per_future_; t_count_++, t_opt_idx_++) {
      option_unitlots_[t_opt_idx_] = inst_unitlots_[t_fut_idx_];
      option_maxlots_[t_opt_idx_] = global_param_set_->opt_maxlots_ * fabs(option_delta_vec_[t_opt_idx_]);
      CommonIndicator* t_bas_indicator_ = MovingAvgBidAskSpread::GetUniqueInstance(dbglogger_, watch_, dep_market_view_vec_[t_opt_idx_], 300.0);
      bid_ask_spread_vec_.push_back(t_bas_indicator_);
      DBGLOG_TIME_CLASS_FUNC_LINE << dep_market_view_vec_[t_opt_idx_]->shortcode() << " IL " << option_unitlots_[t_opt_idx_]
				  << " IM " << option_maxlots_[t_opt_idx_] << " BT " << inst_base_threshold_[t_fut_idx_]
				  << " IT " << inst_increase_threshold_[t_fut_idx_] << " DT "
				  << inst_decrease_threshold_[t_fut_idx_] << " SF " << global_param_set_->opt_bid_ask_spread_factor_ << '\n';
    }
  }
}

// Supported just because of inheritance - not used
int OptionsMeanRevertingTrading::my_global_position() const {
  double pos_ = 0;
  for (unsigned int t_fut_idx_ = 0; t_fut_idx_ < num_trading_futures_; t_fut_idx_++) {
    pos_ += product_delta_adjusted_position_in_lots_[t_fut_idx_];
  }
  return (pos_);
}

// Supports StartTrading/StopTrading/DumpPositions
void OptionsMeanRevertingTrading::OnControlUpdate(const ControlMessage& _control_message_, const char* symbol,
                                                  const int trader_id) {
  ControlMessageCode_t code = _control_message_.message_code_;
  if (code == kControlMessageCodeGetflat) {
    if (strlen(_control_message_.strval_1_) == 0) {
      GetAllFlat();
    } else {
      for (unsigned int t_fut_idx_ = 0; t_fut_idx_ < num_futures_; t_fut_idx_++) {
        if (strcmp(_control_message_.strval_1_, future_smv_vec_[t_fut_idx_]->shortcode().c_str()) == 0) {
          GetFlatTradingLogic(secid_2_prodid_[t_fut_idx_].first);
          break;
        }
      }
    }
  } else if (code == kControlMessageCodeStartTrading) {
    if (strlen(_control_message_.strval_1_) == 0) {
      for (unsigned int t_fut_idx_ = 0; t_fut_idx_ < num_futures_; t_fut_idx_++) {
        fut_level_should_be_getting_flat_[t_fut_idx_] = false;
      }
    } else {
      for (unsigned int t_fut_idx_ = 0; t_fut_idx_ < num_futures_; t_fut_idx_++) {
        if (strcmp(_control_message_.strval_1_, future_smv_vec_[t_fut_idx_]->shortcode().c_str()) == 0) {
          fut_level_should_be_getting_flat_[secid_2_prodid_[t_fut_idx_].first] = false;
          break;
        }
      }
    }
  } else if (code == kControlMessageCodeDumpPositions) {
    for (unsigned int t_fut_idx_ = 0, t_opt_idx_ = 0; t_fut_idx_ < num_trading_futures_; t_fut_idx_++) {
      DBGLOG_TIME_CLASS_FUNC_LINE << trading_fut_shortcode_vec_[t_fut_idx_] << " Pos "
                                  << product_delta_adjusted_position_in_lots_[t_fut_idx_] << '\n';
      for (unsigned int t_count_ = 0; t_count_ < num_options_per_future_; t_count_++, t_opt_idx_++) {
        DBGLOG_TIME_CLASS_FUNC_LINE << opt_shortcode_vec_[t_opt_idx_] << " Pos "
                                    << product_position_in_lots_[t_opt_idx_] << '\n';
      }
    }
    DBGLOG_DUMP;
  }
}

void OptionsMeanRevertingTrading::GetAllFlat() {
  for (unsigned int t_fut_idx_ = 0; t_fut_idx_ < num_futures_; t_fut_idx_++) {
    GetFlatTradingLogic(t_fut_idx_);
  }
}

// we are not getting delta flat but flat per option !
void OptionsMeanRevertingTrading::GetFlatTradingLogic(int t_fut_idx_) {
  for (unsigned int t_count_ = 0, t_opt_idx_ = t_fut_idx_ * num_options_per_future_; t_count_ < num_options_per_future_;
       t_count_++, t_opt_idx_++) {
    int thisproduct_positions_ = product_position_in_lots_[t_opt_idx_];
    if (thisproduct_positions_ == 0) {
      order_manager_vec_[t_opt_idx_]->CancelAllOrders();
    } else if (thisproduct_positions_ > 0) {
      order_manager_vec_[t_opt_idx_]->CancelAllBidOrders();
    } else if (thisproduct_positions_ < 0) {
      order_manager_vec_[t_opt_idx_]->CancelAllAskOrders();
    }
  }
  fut_level_should_be_getting_flat_[t_fut_idx_] = true;
}

// here we only set signal before calling trading logic
// OnMarketUpdate -> UpdatePortPxForId -> SetBestPrices -> TradingLogic ( risk_checks + exec_control_checks ) ->
// PlaceAndCancelOrders -> PlaceSingle[Buy/Sell]Order
// we should only get call future smvs
void OptionsMeanRevertingTrading::OnMarketUpdate(const unsigned int _security_id_,
                                                 const MarketUpdateInfo& _market_update_info_) {
  double t_diff_ = 0;
  int t_fut_idx_ = secid_2_prodid_[_security_id_].first;
  int t_opt_idx_ = secid_2_prodid_[_security_id_].second;

  // only for future
  if (t_opt_idx_ == -1) {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << future_smv_vec_[t_fut_idx_]->shortcode() << "\n";
    }
    if (future_smv_vec_[t_fut_idx_]->bestbid_price() > last_inst_prices_[t_fut_idx_] &&
        future_smv_vec_[t_fut_idx_]->bestbid_size() > 0) {
      t_diff_ = future_smv_vec_[t_fut_idx_]->bestbid_price() - last_inst_prices_[t_fut_idx_];
    }
    if (future_smv_vec_[t_fut_idx_]->bestask_price() < last_inst_prices_[t_fut_idx_] &&
        future_smv_vec_[t_fut_idx_]->bestask_size() > 0 && future_smv_vec_[t_fut_idx_]->bestask_price() > 0) {
      t_diff_ = future_smv_vec_[t_fut_idx_]->bestask_price() - last_inst_prices_[t_fut_idx_];
    }

    if (fabs(t_diff_) > 1e-5) {
      UpdatePortPxForId(t_fut_idx_, t_diff_);
      TradingLogic();
    }
    double current_unrealized_pnl_ = 0;
    for (unsigned int t_opt_idx_ = 0; t_opt_idx_ < num_options_; t_opt_idx_++) {
      current_unrealized_pnl_ += order_manager_vec_[t_opt_idx_]->base_pnl().total_pnl();
    }
    if (current_unrealized_pnl_ < global_min_pnl_) {
      global_min_pnl_ = current_unrealized_pnl_;
    }
  } else {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << dep_market_view_vec_[t_opt_idx_]->shortcode() << "\n";
    }
    SetBestPrices(t_fut_idx_, t_opt_idx_);
    TradingLogic(t_fut_idx_, t_opt_idx_);
  }
}
// all l1 listeners will get ontradeprint
// here we set signal and threshold before calling trading logic
// OnTradePrint -> UpdatePortPxForId -> SetBestPrices -> UpdateVectorsAndRecomputeBeta -> TradingLogic ( risk_checks +
// exec_control_checks ) -> PlaceAndCancelOrders -> PlaceSingle[Buy/Sell]Order
void OptionsMeanRevertingTrading::OnTradePrint(const unsigned int _security_id_,
                                               const TradePrintInfo& _trade_print_info_,
                                               const MarketUpdateInfo& _market_update_info_) {
  int t_fut_idx_ = secid_2_prodid_[_security_id_].first;
  int t_opt_idx_ = secid_2_prodid_[_security_id_].second;

  // if it is options trade, dont care
  if (t_opt_idx_ >= 0) {
    return;
  }

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << future_smv_vec_[t_fut_idx_]->shortcode() << "\n";
  }

  if (fabs(_trade_print_info_.trade_price_ - last_inst_prices_[t_fut_idx_] > 1e-5)) {
    UpdatePortPxForId(t_fut_idx_, _trade_print_info_.trade_price_ - last_inst_prices_[t_fut_idx_]);
  }

  // update  vectors and recompute betas
  if (watch_.msecs_from_midnight() - msecs_at_last_vec_processing_ >= 60000) {
    for (unsigned int t_fut_idx_ = 0; t_fut_idx_ < num_futures_; t_fut_idx_++) {
      if (dont_trade_[t_fut_idx_] == false) {
        inst_prices_[t_fut_idx_].push_back(last_inst_prices_[t_fut_idx_]);
        port_prices_[t_fut_idx_].push_back(last_port_prices_[t_fut_idx_]);
        inst_prices_[t_fut_idx_].erase(inst_prices_[t_fut_idx_].begin());
        port_prices_[t_fut_idx_].erase(port_prices_[t_fut_idx_].begin());
        double t_new_beta_ = std::inner_product(inst_prices_[t_fut_idx_].begin(), inst_prices_[t_fut_idx_].end(),
                                                port_prices_[t_fut_idx_].begin(), 0.0) /
                             std::inner_product(port_prices_[t_fut_idx_].begin(), port_prices_[t_fut_idx_].end(),
                                                port_prices_[t_fut_idx_].begin(), 0.0);
        //	std::cout << " Beta_change_for " << dep_market_view_vec_[t_fut_idx_]->shortcode() << " is " << (
        // t_new_beta_/inst_betas_[t_fut_idx_] - 1.0 )*100 << " Beta " << t_new_beta_ << '\n';
        inst_betas_[t_fut_idx_] = t_new_beta_;

        double t_del_residual_ = residuals_[t_fut_idx_][0];
        residual_sum_[t_fut_idx_] -= t_del_residual_;
        residual_sumsqr_[t_fut_idx_] -= (t_del_residual_ * t_del_residual_);
        double t_add_residual_ =
            last_inst_prices_[t_fut_idx_] - inst_betas_[t_fut_idx_] * last_port_prices_[t_fut_idx_];
        residual_sum_[t_fut_idx_] += t_add_residual_;
        residual_sumsqr_[t_fut_idx_] += (t_add_residual_ * t_add_residual_);
        residuals_[t_fut_idx_].push_back(t_add_residual_);
        residuals_[t_fut_idx_].erase(residuals_[t_fut_idx_].begin());
        stdev_residuals_[t_fut_idx_] = sqrt(residual_sumsqr_[t_fut_idx_] / global_param_set_->hist_error_length_ -
                                            residual_sum_[t_fut_idx_] / global_param_set_->hist_error_length_ *
                                                residual_sum_[t_fut_idx_] / global_param_set_->hist_error_length_);
        //        if( t_fut_idx_ == 0 )
        //                std::cout << " Stock " << dep_market_view_vec_[t_fut_idx_+1]->shortcode() << " beta " <<
        //                inst_betas_[t_fut_idx_] << " stdev_residual " << stdev_residuals_[t_fut_idx_] <<
        //                " Add_Res " << t_add_residual_ << " Del_Resid " << t_del_residual_ <<
        //		" Inst Px " << last_inst_prices_[t_fut_idx_] << " Port Px " << last_port_prices_[t_fut_idx_] <<
        //'\n';
      }
    }
    msecs_at_last_vec_processing_ = watch_.msecs_from_midnight();
  }

  // getflat due to strat max loss computation
  // TODO: need to implement pnl_per_future ?
  double t_total_pnl_ = 0.0;
  for (unsigned int t_opt_idx_ = 0; t_opt_idx_ < num_options_; t_opt_idx_++) {
    t_total_pnl_ += order_manager_vec_[t_opt_idx_]->base_pnl().total_pnl();
  }

  if (t_total_pnl_ < -1.0 * global_param_set_->strat_max_loss_) {
    for (unsigned int t_fut_idx_ = 0; t_fut_idx_ < num_futures_; t_fut_idx_++) {
      fut_level_should_be_getting_flat_[t_fut_idx_] = true;
    }
  }

  /*  DBGLOG_TIME_CLASS_FUNC_LINE << watch_.msecs_from_midnight() << " Sec " <<
    dep_market_view_vec_[t_prod_id_]->shortcode() << " Mid Px "
              <<
    (dep_market_view_vec_[t_prod_id_]->bestbid_price()+dep_market_view_vec_[t_prod_id_]->bestask_price())*0.5
              << " BIP " << 0.05*bid_int_price_to_place_at_[t_prod_id_] << " AIP " <<
    0.05*ask_int_price_to_place_at_[t_prod_id_]
              << " Port_Px " << last_port_prices_[t_prod_id_] << " Beta " << inst_betas_[t_prod_id_]
              << " Stdev " << stdev_residuals_[t_prod_id_] << " Pos " << product_position_in_lots_[t_prod_id_]
              << " Dont_Trade " << ( dont_trade_[t_prod_id_]?'Y':'N') << " GetFlat " << (
    should_be_getting_flat_[t_prod_id_]?'Y':'N') << '\n';
    DBGLOG_DUMP;  */
  TradingLogic();
}

// we get this per option !
// OnExec -> SetBestPrices > TradingLogic ( risk_checks + exec_control_checks ) -> PlaceAndCancelOrders ->
// PlaceSingle[Buy/Sell]Order
void OptionsMeanRevertingTrading::OnExec(const int _new_position_, const int _exec_quantity_,
                                         const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                                         const int _security_id_) {
  int t_fut_idx_ = secid_2_prodid_[_security_id_].first;
  int t_opt_idx_ = secid_2_prodid_[_security_id_].second;
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << dep_market_view_vec_[t_opt_idx_]->shortcode() << "\n";
  }

  product_position_in_lots_[t_opt_idx_] = _new_position_ / dep_market_view_vec_[t_opt_idx_]->min_order_size();

  // we are choosing to recompute for all, so we can incorporate small delta change as well
  double t_delta_adjusted_position_ = 0.0;
  for (unsigned int t_count_ = 0, t_opt_idx_ = t_fut_idx_ * num_options_per_future_; t_count_ < num_options_per_future_;
       t_count_++, t_opt_idx_++) {
    t_delta_adjusted_position_ += product_position_in_lots_[t_opt_idx_] * option_delta_vec_[t_opt_idx_];
  }
  product_delta_adjusted_position_in_lots_[t_fut_idx_] = t_delta_adjusted_position_;

  if (_buysell_ == HFSAT::kTradeTypeBuy) {
    seconds_at_last_buy_[t_opt_idx_] = watch_.msecs_from_midnight() / 1000;
  } else {
    seconds_at_last_sell_[t_opt_idx_] = watch_.msecs_from_midnight() / 1000;
  }
  // given risk has changed need to reevalute the orders in the book
  // we should never have t_fut_idx_ of nontrading futures !
  SetBestPrices(t_fut_idx_);
  TradingLogic();
}

void OptionsMeanRevertingTrading::UpdatePortPxForId(int _fut_idx_, double px_diff_) {
  DBGLOG_TIME_CLASS_FUNC_LINE << _fut_idx_ << "\n";
  last_inst_prices_[_fut_idx_] = last_inst_prices_[_fut_idx_] + px_diff_;
  std::vector<std::pair<int, double>>::iterator t_iter_;
  for (t_iter_ = predictor_vec_[_fut_idx_].begin(); t_iter_ != predictor_vec_[_fut_idx_].end(); t_iter_++) {
    int t_port_idx_ = (*t_iter_).first;
    double t_port_wt_ = (*t_iter_).second;
    last_port_prices_[t_port_idx_] = last_port_prices_[t_port_idx_] + t_port_wt_ * px_diff_;
    // target prices might have changed for this .. so recompute best prices to place at
    if (!dont_trade_[t_port_idx_]) {
      SetBestPrices(t_port_idx_);
    } else {
      DBGLOG_TIME_CLASS_FUNC_LINE << t_port_idx_ << "trading disabled\n";
    }
  }
}

// we are only applying lower bounds here,
// upper bound are applied by TradingLogic function
void OptionsMeanRevertingTrading::SetBestPrices(int _fut_idx_, int _opt_idx_) {
  // first we get required strength of signal needed given current risk
  // we only do this for fut_price_changes_
  if (_opt_idx_ == -1) {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << trading_fut_shortcode_vec_[_fut_idx_] << "\n";
    }
    double t_buy_k_ = inst_base_threshold_[_fut_idx_];
    double t_sell_k_ = inst_base_threshold_[_fut_idx_];
    if (product_delta_adjusted_position_in_lots_[_fut_idx_] > 0) {
      t_buy_k_ = t_buy_k_ +
                 inst_increase_threshold_[_fut_idx_] * product_delta_adjusted_position_in_lots_[_fut_idx_] /
                     inst_unitlots_[_fut_idx_];
      t_sell_k_ = t_sell_k_ -
                  inst_decrease_threshold_[_fut_idx_] * product_delta_adjusted_position_in_lots_[_fut_idx_] /
                      inst_unitlots_[_fut_idx_];

      t_sell_k_ = std::max(0.0, t_sell_k_ -
                                    global_param_set_->time_hysterisis_factor_ *
                                        (watch_.msecs_from_midnight() / 1000.0 - seconds_at_last_buy_[_fut_idx_]));
    } else if (product_delta_adjusted_position_in_lots_[_fut_idx_] < 0) {
      t_buy_k_ = t_buy_k_ +
                 inst_decrease_threshold_[_fut_idx_] * product_delta_adjusted_position_in_lots_[_fut_idx_] /
                     inst_unitlots_[_fut_idx_];
      t_sell_k_ = t_sell_k_ -
                  inst_increase_threshold_[_fut_idx_] * product_delta_adjusted_position_in_lots_[_fut_idx_] /
                      inst_unitlots_[_fut_idx_];

      t_buy_k_ = std::max(0.0, t_buy_k_ -
                                   global_param_set_->time_hysterisis_factor_ *
                                       (watch_.msecs_from_midnight() / 1000.0 - seconds_at_last_sell_[_fut_idx_]));
    }

    // second we need set risk_adjusted_model_future_price
    risk_adjusted_model_future_bid_price_vec_[_fut_idx_] =
        last_port_prices_[_fut_idx_] * inst_betas_[_fut_idx_] - t_buy_k_ * stdev_residuals_[_fut_idx_];
    risk_adjusted_model_future_ask_price_vec_[_fut_idx_] =
        last_port_prices_[_fut_idx_] * inst_betas_[_fut_idx_] + t_sell_k_ * stdev_residuals_[_fut_idx_];
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << (int)floor(risk_adjusted_model_future_bid_price_vec_[_fut_idx_] /
                                                future_smv_vec_[_fut_idx_]->min_price_increment()) << " "
                                  << (int)ceil(risk_adjusted_model_future_ask_price_vec_[_fut_idx_] /
                                               future_smv_vec_[_fut_idx_]->min_price_increment()) << " Port_Px "
                                  << last_port_prices_[_fut_idx_] << " Beta " << inst_betas_[_fut_idx_] << " Thres "
                                  << t_buy_k_ << " Stdev " << stdev_residuals_[_fut_idx_]
                                  << " DelPos:" << product_delta_adjusted_position_in_lots_[_fut_idx_] << " Mkt "
                                  << future_smv_vec_[_fut_idx_]->bestbid_int_price() << " "
                                  << future_smv_vec_[_fut_idx_]->bestask_int_price() << "\n";
    }
  }

  // third we need to infer model option prices
  // kMethodBlackScholes:
  // also as the first iteration we are inferring both bid and ask seperately, as options tend to be more
  // illiquid we dont want bid_ask_spread of option to be a factor, for now
  // but we could better here, so we made this virtual function

  unsigned int t_opt_start_idx_ = std::max(_fut_idx_ * (int)num_options_per_future_, _opt_idx_);
  unsigned int t_counter_end_idx_ = (_opt_idx_ != -1 ? 1 : num_options_per_future_);

  for (unsigned int t_count_ = 0, t_opt_idx_ = t_opt_start_idx_; t_count_ < t_counter_end_idx_;
       t_count_++, t_opt_idx_++) {
    if (!is_ready_vec_[t_opt_idx_]) {
      if (implied_vol_vec_[t_opt_idx_] == nullptr) {
        if (dep_market_view_vec_[t_opt_idx_]->is_ready_complex2(0) ||
            (dep_market_view_vec_[t_opt_idx_]->is_ready() &&
             dep_market_view_vec_[t_opt_idx_]->spread_increments() <= 4)) {
          is_ready_vec_[t_opt_idx_] = true;
        } else {
          continue;
        }
      } else if (implied_vol_vec_[t_opt_idx_]->IsIndicatorReady()) {
        is_ready_vec_[t_opt_idx_] = true;
      } else {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << opt_shortcode_vec_[t_opt_idx_] << " " << t_opt_idx_
                                      << " either dep or implied vol indicator not ready\n";
        }
        continue;
      }
    }

    bool t_temp_var_;
    double t_spread_factor_in_ticks_ = global_param_set_->opt_bid_ask_spread_factor_ * bid_ask_spread_vec_[t_opt_idx_]->indicator_value(t_temp_var_);
    double t_spread_factor_ = t_spread_factor_in_ticks_ * 0.05;

    if (option_obj_vec_[t_opt_idx_]->is_call() == 1) {  // call
      if (implied_vol_vec_[t_opt_idx_] == nullptr) {
        // if risk_adjusted_model_fut_prices are around mkt_fut_prices we place on options directly
        // logic 1, some delta based threshold would be helpful !! one direction to work
        // call bid
        if (global_param_set_->aggressive_ &&
            risk_adjusted_model_future_bid_price_vec_[_fut_idx_] >= future_smv_vec_[_fut_idx_]->bestask_price() + t_spread_factor_) {
          bid_int_price_to_place_at_[t_opt_idx_] = dep_market_view_vec_[t_opt_idx_]->bestask_int_price();
        } else if (risk_adjusted_model_future_bid_price_vec_[_fut_idx_] >=
                   future_smv_vec_[_fut_idx_]->bestbid_price() + t_spread_factor_) {
          bid_int_price_to_place_at_[t_opt_idx_] = dep_market_view_vec_[t_opt_idx_]->bestbid_int_price();
        } else {
          bid_int_price_to_place_at_[t_opt_idx_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
        }
        // call-ask
        if (global_param_set_->aggressive_ &&
            risk_adjusted_model_future_ask_price_vec_[_fut_idx_] <= future_smv_vec_[_fut_idx_]->bestbid_price() - t_spread_factor_) {
          ask_int_price_to_place_at_[t_opt_idx_] = dep_market_view_vec_[t_opt_idx_]->bestbid_int_price();
        } else if (risk_adjusted_model_future_ask_price_vec_[_fut_idx_] <=
                   future_smv_vec_[_fut_idx_]->bestask_price() - t_spread_factor_) {
          ask_int_price_to_place_at_[t_opt_idx_] = dep_market_view_vec_[t_opt_idx_]->bestask_int_price();
        } else {
          ask_int_price_to_place_at_[t_opt_idx_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
        }
      } else {
        // call-bid
        bid_int_price_to_place_at_[t_opt_idx_] =
            (int)floor((option_obj_vec_[t_opt_idx_]->ModelImpliedPrice(
                           risk_adjusted_model_future_bid_price_vec_[_fut_idx_],
                           implied_vol_vec_[t_opt_idx_]->indicator_value(t_temp_var_))) /
                       dep_market_view_vec_[t_opt_idx_]->min_price_increment() - t_spread_factor_in_ticks_);
        // call-ask
        ask_int_price_to_place_at_[t_opt_idx_] =
            (int)ceil((option_obj_vec_[t_opt_idx_]->ModelImpliedPrice(
                          risk_adjusted_model_future_ask_price_vec_[_fut_idx_],
                          implied_vol_vec_[t_opt_idx_]->indicator_value(t_temp_var_))) /
                      dep_market_view_vec_[t_opt_idx_]->min_price_increment() + t_spread_factor_in_ticks_);
      }
    } else {
      if (implied_vol_vec_[t_opt_idx_] == nullptr) {
        // put ask
        if (risk_adjusted_model_future_bid_price_vec_[_fut_idx_] >= future_smv_vec_[_fut_idx_]->bestask_price() + t_spread_factor_) {
          ask_int_price_to_place_at_[t_opt_idx_] = dep_market_view_vec_[t_opt_idx_]->bestbid_int_price();
        } else if (risk_adjusted_model_future_bid_price_vec_[_fut_idx_] >=
                   future_smv_vec_[_fut_idx_]->bestbid_price() + t_spread_factor_) {
          ask_int_price_to_place_at_[t_opt_idx_] = dep_market_view_vec_[t_opt_idx_]->bestask_int_price();
        } else {
          ask_int_price_to_place_at_[t_opt_idx_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
        }
        // put-bid
        if (risk_adjusted_model_future_bid_price_vec_[_fut_idx_] <= future_smv_vec_[_fut_idx_]->bestbid_price() - t_spread_factor_) {
          bid_int_price_to_place_at_[t_opt_idx_] = dep_market_view_vec_[t_opt_idx_]->bestask_int_price();
        } else if (risk_adjusted_model_future_ask_price_vec_[_fut_idx_] <=
                   future_smv_vec_[_fut_idx_]->bestask_price() - t_spread_factor_) {
          bid_int_price_to_place_at_[t_opt_idx_] = dep_market_view_vec_[t_opt_idx_]->bestbid_int_price();
        } else {
          bid_int_price_to_place_at_[t_opt_idx_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
        }
      } else {
        // put ask
        ask_int_price_to_place_at_[t_opt_idx_] =
            (int)ceil((option_obj_vec_[t_opt_idx_]->ModelImpliedPrice(
                          risk_adjusted_model_future_bid_price_vec_[_fut_idx_],
                          implied_vol_vec_[t_opt_idx_]->indicator_value(t_temp_var_))) /
                      dep_market_view_vec_[t_opt_idx_]->min_price_increment() + t_spread_factor_in_ticks_);

        bid_int_price_to_place_at_[t_opt_idx_] =
            (int)floor((option_obj_vec_[t_opt_idx_]->ModelImpliedPrice(
                           risk_adjusted_model_future_ask_price_vec_[_fut_idx_],
                           implied_vol_vec_[t_opt_idx_]->indicator_value(t_temp_var_))) /
                       dep_market_view_vec_[t_opt_idx_]->min_price_increment() - t_spread_factor_in_ticks_);
      }
    }

    if (bid_int_price_to_place_at_[t_opt_idx_] < dep_market_view_vec_[t_opt_idx_]->bestbid_int_price() - 10) {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << t_opt_idx_ << " disabling because it is 2 ticks less than best_bidprice "
                                    << bid_int_price_to_place_at_[t_opt_idx_] << " "
                                    << dep_market_view_vec_[t_opt_idx_]->bestbid_int_price() << "\n";
      }
      bid_int_price_to_place_at_[t_opt_idx_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
    }
    if (ask_int_price_to_place_at_[t_opt_idx_] > dep_market_view_vec_[t_opt_idx_]->bestask_int_price() + 10) {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << t_opt_idx_ << " disabling because it is 2 ticks more than best_askprice "
                                    << ask_int_price_to_place_at_[t_opt_idx_] << " "
                                    << dep_market_view_vec_[t_opt_idx_]->bestask_int_price() << "\n";
      }
      ask_int_price_to_place_at_[t_opt_idx_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
    }

    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << t_opt_idx_ << " our_setbestprice_quote " << bid_int_price_to_place_at_[t_opt_idx_]
                                  << " " << ask_int_price_to_place_at_[t_opt_idx_] << " mkt_quote "
                                  << dep_market_view_vec_[t_opt_idx_]->bestbid_int_price() << " "
                                  << dep_market_view_vec_[t_opt_idx_]->bestask_int_price() << "\n";
    }
  }
}

// on each exec-change_in_risk / marketupdate-change_in_signal / ontradeprint-change_in_premium
// we look at all products, however we set use various checks to avoid sending orders to each product
// more smart checks will prevent them further, for now we take what all are put in mrt
void OptionsMeanRevertingTrading::TradingLogic(int _fut_idx_, int _opt_idx_) {
  // we have three variables
  // fut_starting_point , how many futures
  // opt_starting_point, how many options?
  // case 1: one opt
  // case 2: all options of a fut
  // case 3: all options and all futures

  // for case 1: fut_idx and opt_idx are specified
  // fut_init_value = fut_idx
  // opt_init_value = opt_idx
  // fut_count_ = 1
  // opt_count_ = 1

  // for case 2: fut_idx is specified
  // fut_init_value = fut_idx
  // opt_init_value = fut_idx * num_options_per_future_
  // fut_count = 1
  // opt_count = num_options_per_future_

  // for case 3:
  // fut_init_value = 0
  // opt_init_value = 0
  // fut_count = num_trading_futures_
  // opt_count = num_options_per_future_

  unsigned int t_fut_start_idx_ = std::max(0, _fut_idx_);
  unsigned int t_opt_start_idx_ = std::max((int)t_fut_start_idx_ * (int)num_options_per_future_, _opt_idx_);

  // fut_idx will always be less than num_trading_futures
  // hence t_fut_end_idx_ lees than equal to num_trading_futures
  unsigned int t_fut_end_idx_ = (_fut_idx_ != -1 ? (t_fut_start_idx_ + 1) : num_trading_futures_);
  // counter always start with 0
  unsigned int t_counter_end_idx_ = (_opt_idx_ != -1 ? 1 : num_options_per_future_);

  // strategy level checks:
  // trading hours check
  if (watch_.msecs_from_midnight() < trading_start_utc_mfm_) {
    return;
  }

  if (watch_.msecs_from_midnight() > global_param_set_->aggressive_getflat_msecs_) {
    for (unsigned int t_fut_idx_ = 0; t_fut_idx_ < num_trading_futures_; t_fut_idx_++) {
      fut_level_should_be_getting_flat_aggressively_[t_fut_idx_] = true;
      fut_level_should_be_getting_flat_[t_fut_idx_] = true;
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << "getting flat aggressively due to close\n";
      }
    }
  } else if (watch_.msecs_from_midnight() > trading_end_utc_mfm_) {
    for (unsigned int t_fut_idx_ = 0; t_fut_idx_ < num_trading_futures_; t_fut_idx_++) {
      fut_level_should_be_getting_flat_[t_fut_idx_] = true;
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << "getflat due to close\n";
      }
    }
  }
  // global max pos checks
  int t_gpos_ = my_global_position();
  bool disallow_long_trades_ = (t_gpos_ > global_param_set_->portfolio_maxlots_ ? true : false);
  bool disallow_short_trades_ = (-1 * t_gpos_ > global_param_set_->portfolio_maxlots_ ? true : false);
  // global max loss already applied in ontradeprint
  if (disallow_short_trades_) {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << "disallow_short_trades is set\n";
    }
  }
  if (disallow_long_trades_) {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << "disallow_long_trades is set\n";
    }
  }

  // future - level
  for (unsigned int t_fut_idx_ = t_fut_start_idx_, t_opt_idx_ = t_opt_start_idx_; t_fut_idx_ < t_fut_end_idx_;
       t_fut_idx_++) {
    if (!fut_level_should_be_getting_flat_[t_fut_idx_]) {
      int t_pnl_per_future_ = 0;
      for (unsigned int t_count_ = 0; t_count_ < t_counter_end_idx_; t_count_++, t_opt_idx_++) {
        t_pnl_per_future_ += order_manager_vec_[t_opt_idx_]->base_pnl().total_pnl();
        if (!opt_level_should_be_getting_flat_[t_opt_idx_] && is_ready_vec_[t_opt_idx_] &&
            dep_market_view_vec_[t_opt_idx_]->bestbid_int_price() < global_param_set_->min_bid_int_price_to_trade_) {
          opt_level_should_be_getting_flat_[t_opt_idx_] = true;
          DBGLOG_TIME_CLASS_FUNC_LINE << "opt_level_getflat is set for bestbid_int_price check "
                                      << dep_market_view_vec_[t_opt_idx_]->bestbid_int_price() << "\n";
        }
      }
      if (t_pnl_per_future_ < -1 * global_param_set_->product_maxloss_) {
        fut_level_should_be_getting_flat_[t_fut_idx_] = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << "fut_level_getflat is set for max_loss hit\n";
        }
      }
    }
  }
  // option - level
  // TODO

  for (unsigned int t_fut_idx_ = t_fut_start_idx_, t_opt_idx_ = t_opt_start_idx_; t_fut_idx_ < t_fut_end_idx_;
       t_fut_idx_++) {
    for (unsigned int t_count_ = 0; t_count_ < t_counter_end_idx_; t_count_++, t_opt_idx_++) {
      if (option_obj_vec_[t_opt_idx_]->is_call() == 1) {
        // call-bid
        if (disallow_long_trades_ || dont_trade_[t_fut_idx_] ||
            product_delta_adjusted_position_in_lots_[t_fut_idx_] >= inst_maxlots_[t_fut_idx_] ||
            product_position_in_lots_[t_opt_idx_] >= option_maxlots_[t_opt_idx_] ||
            (watch_.msecs_from_midnight() / 1000 - seconds_at_last_buy_[t_opt_idx_] <
                 global_param_set_->cooloff_secs_ &&
             product_delta_adjusted_position_in_lots_[t_fut_idx_] >= 0) ||
            bid_int_price_to_place_at_[t_opt_idx_] == INVALID_MEAN_REVERTING_ORDER_PRICE) {
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC_LINE << (int)floor(risk_adjusted_model_future_bid_price_vec_[t_fut_idx_] /
                                                      future_smv_vec_[t_fut_idx_]->min_price_increment()) << " "
                                        << (int)ceil(risk_adjusted_model_future_ask_price_vec_[t_fut_idx_] /
                                                     future_smv_vec_[t_fut_idx_]->min_price_increment()) << " "
                                        << opt_shortcode_vec_[t_opt_idx_] << " bid-place disabled "
                                        << bid_int_price_to_place_at_[t_opt_idx_] << "\n";
          }
          bid_int_price_to_place_at_[t_opt_idx_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
        } else if (global_param_set_->aggressive_ &&
                   dep_market_view_vec_[t_opt_idx_]->spread_increments() <=
                       global_param_set_->max_int_spread_to_cross_ &&
                   watch_.msecs_from_midnight() / 1000 - seconds_at_last_buy_[t_opt_idx_] >=
                       global_param_set_->agg_cooloff_secs_) {
          bid_int_price_to_place_at_[t_opt_idx_] =
              std::min(dep_market_view_vec_[t_opt_idx_]->bestask_int_price(), bid_int_price_to_place_at_[t_opt_idx_]);
        } else {
          bid_int_price_to_place_at_[t_opt_idx_] =
              std::min(dep_market_view_vec_[t_opt_idx_]->bestbid_int_price(), bid_int_price_to_place_at_[t_opt_idx_]);
        }
        // call-ask
        if (disallow_short_trades_ || dont_trade_[t_fut_idx_] ||
            product_delta_adjusted_position_in_lots_[t_fut_idx_] <= -inst_maxlots_[t_fut_idx_] ||
            product_position_in_lots_[t_opt_idx_] <= -option_maxlots_[t_opt_idx_] ||
            (watch_.msecs_from_midnight() / 1000 - seconds_at_last_sell_[t_opt_idx_] <
                 global_param_set_->cooloff_secs_ &&
             product_delta_adjusted_position_in_lots_[t_fut_idx_] <= 0) ||
            ask_int_price_to_place_at_[t_opt_idx_] == INVALID_MEAN_REVERTING_ORDER_PRICE) {
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC_LINE << (int)floor(risk_adjusted_model_future_bid_price_vec_[t_fut_idx_] /
                                                      future_smv_vec_[t_fut_idx_]->min_price_increment()) << " "
                                        << (int)ceil(risk_adjusted_model_future_ask_price_vec_[t_fut_idx_] /
                                                     future_smv_vec_[t_fut_idx_]->min_price_increment()) << " "
                                        << opt_shortcode_vec_[t_opt_idx_] << " ask-place disabled "
                                        << ask_int_price_to_place_at_[t_opt_idx_] << "\n";
          }
          ask_int_price_to_place_at_[t_opt_idx_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
        } else if (global_param_set_->aggressive_ &&
                   dep_market_view_vec_[t_opt_idx_]->spread_increments() <=
                       global_param_set_->max_int_spread_to_cross_ &&
                   watch_.msecs_from_midnight() / 1000 - seconds_at_last_sell_[t_opt_idx_] >=
                       global_param_set_->agg_cooloff_secs_) {
          ask_int_price_to_place_at_[t_opt_idx_] =
              std::max(dep_market_view_vec_[t_opt_idx_]->bestbid_int_price(), ask_int_price_to_place_at_[t_opt_idx_]);
        } else {
          ask_int_price_to_place_at_[t_opt_idx_] =
              std::max(dep_market_view_vec_[t_opt_idx_]->bestask_int_price(), ask_int_price_to_place_at_[t_opt_idx_]);
        }
      } else {
        // put-ask
        if (disallow_long_trades_ || dont_trade_[t_fut_idx_] ||
            product_delta_adjusted_position_in_lots_[t_fut_idx_] >= inst_maxlots_[t_fut_idx_] ||
            product_position_in_lots_[t_opt_idx_] <= -option_maxlots_[t_opt_idx_] ||
            (watch_.msecs_from_midnight() / 1000 - seconds_at_last_sell_[t_opt_idx_] <
                 global_param_set_->cooloff_secs_ &&
             product_delta_adjusted_position_in_lots_[t_fut_idx_] >= 0) ||
            ask_int_price_to_place_at_[t_opt_idx_] == INVALID_MEAN_REVERTING_ORDER_PRICE) {
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC_LINE << (int)floor(risk_adjusted_model_future_bid_price_vec_[t_fut_idx_] /
                                                      future_smv_vec_[t_fut_idx_]->min_price_increment()) << " "
                                        << (int)ceil(risk_adjusted_model_future_ask_price_vec_[t_fut_idx_] /
                                                     future_smv_vec_[t_fut_idx_]->min_price_increment()) << " "
                                        << opt_shortcode_vec_[t_opt_idx_] << " ask-place disabled "
                                        << ask_int_price_to_place_at_[t_opt_idx_] << "\n";
          }
          ask_int_price_to_place_at_[t_opt_idx_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
        } else if (global_param_set_->aggressive_ &&
                   dep_market_view_vec_[t_opt_idx_]->spread_increments() <=
                       global_param_set_->max_int_spread_to_cross_ &&
                   watch_.msecs_from_midnight() / 1000 - seconds_at_last_sell_[t_opt_idx_] >=
                       global_param_set_->agg_cooloff_secs_) {
          ask_int_price_to_place_at_[t_opt_idx_] =
              std::max(dep_market_view_vec_[t_opt_idx_]->bestbid_int_price(), ask_int_price_to_place_at_[t_opt_idx_]);
        } else {
          ask_int_price_to_place_at_[t_opt_idx_] =
              std::max(dep_market_view_vec_[t_opt_idx_]->bestask_int_price(), ask_int_price_to_place_at_[t_opt_idx_]);
        }
        // put-bid
        if (disallow_short_trades_ || dont_trade_[t_fut_idx_] ||
            product_delta_adjusted_position_in_lots_[t_fut_idx_] <= -inst_maxlots_[t_fut_idx_] ||
            product_position_in_lots_[t_opt_idx_] >= option_maxlots_[t_opt_idx_] ||
            (watch_.msecs_from_midnight() / 1000 - seconds_at_last_buy_[t_opt_idx_] <
                 global_param_set_->cooloff_secs_ &&
             product_delta_adjusted_position_in_lots_[t_fut_idx_] <= 0) ||
            bid_int_price_to_place_at_[t_opt_idx_] == INVALID_MEAN_REVERTING_ORDER_PRICE) {
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC_LINE << (int)floor(risk_adjusted_model_future_bid_price_vec_[t_fut_idx_] /
                                                      future_smv_vec_[t_fut_idx_]->min_price_increment()) << " "
                                        << (int)ceil(risk_adjusted_model_future_ask_price_vec_[t_fut_idx_] /
                                                     future_smv_vec_[t_fut_idx_]->min_price_increment()) << " "
                                        << opt_shortcode_vec_[t_opt_idx_] << " bid-place disabled "
                                        << bid_int_price_to_place_at_[t_opt_idx_] << "\n";
          }
          bid_int_price_to_place_at_[t_opt_idx_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
        } else if (global_param_set_->aggressive_ &&
                   dep_market_view_vec_[t_opt_idx_]->spread_increments() <=
                       global_param_set_->max_int_spread_to_cross_ &&
                   watch_.msecs_from_midnight() / 1000 - seconds_at_last_buy_[t_opt_idx_] >=
                       global_param_set_->agg_cooloff_secs_) {
          bid_int_price_to_place_at_[t_opt_idx_] =
              std::min(dep_market_view_vec_[t_opt_idx_]->bestask_int_price(), bid_int_price_to_place_at_[t_opt_idx_]);
        } else {
          bid_int_price_to_place_at_[t_opt_idx_] =
              std::min(dep_market_view_vec_[t_opt_idx_]->bestbid_int_price(), bid_int_price_to_place_at_[t_opt_idx_]);
        }
      }

      // all getflat cases
      if (fut_level_should_be_getting_flat_[t_fut_idx_] || opt_level_should_be_getting_flat_[t_opt_idx_] ||
          abs(product_position_in_lots_[t_opt_idx_]) > option_maxlots_[t_opt_idx_]) {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          if (fut_level_should_be_getting_flat_[t_fut_idx_]) {
            DBGLOG_TIME_CLASS_FUNC_LINE << trading_fut_shortcode_vec_[t_fut_idx_]
                                        << " fut level getflat is set to true \n";
          } else if (opt_level_should_be_getting_flat_[t_opt_idx_]) {
            DBGLOG_TIME_CLASS_FUNC_LINE << opt_shortcode_vec_[t_opt_idx_] << " opt level getflat is set to true \n";
          } else {
            DBGLOG_TIME_CLASS_FUNC_LINE << opt_shortcode_vec_[t_opt_idx_] << " getting flat due to option maxlots \n";
          }
        }

        if (product_position_in_lots_[t_opt_idx_] == 0) {
          bid_int_price_to_place_at_[t_opt_idx_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
          ask_int_price_to_place_at_[t_opt_idx_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
        } else if (product_position_in_lots_[t_opt_idx_] > 0) {
          bid_int_price_to_place_at_[t_opt_idx_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
          if (fut_level_should_be_getting_flat_aggressively_[t_fut_idx_] &&
              (watch_.msecs_from_midnight() / 1000 - seconds_at_last_sell_[t_opt_idx_] > 1)) {
            ask_int_price_to_place_at_[t_opt_idx_] = dep_market_view_vec_[t_opt_idx_]->bestbid_int_price();
          } else {
            ask_int_price_to_place_at_[t_opt_idx_] = dep_market_view_vec_[t_opt_idx_]->bestask_int_price();
          }
        } else {
          ask_int_price_to_place_at_[t_opt_idx_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
          if (fut_level_should_be_getting_flat_aggressively_[t_fut_idx_] &&
              (watch_.msecs_from_midnight() / 1000 - seconds_at_last_buy_[t_opt_idx_] > 1)) {
            bid_int_price_to_place_at_[t_opt_idx_] = dep_market_view_vec_[t_opt_idx_]->bestask_int_price();
          } else {
            bid_int_price_to_place_at_[t_opt_idx_] = dep_market_view_vec_[t_opt_idx_]->bestbid_int_price();
          }
        }
      }
    }
  }
  PlaceAndCancelOrders(_fut_idx_, _opt_idx_);
}

void OptionsMeanRevertingTrading::PlaceAndCancelOrders(int _fut_idx_, int _opt_idx_) {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << _fut_idx_ << " " << _opt_idx_ << "\n";
  }
  unsigned int t_fut_start_idx_ = std::max(0, _fut_idx_);
  unsigned int t_opt_start_idx_ = std::max((int)t_fut_start_idx_ * (int)num_options_per_future_, _opt_idx_);
  unsigned int t_fut_end_idx_ = (_fut_idx_ != -1 ? (t_fut_start_idx_ + 1) : num_trading_futures_);
  unsigned int t_counter_end_idx_ = (_opt_idx_ != -1 ? 1 : num_options_per_future_);
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << t_fut_start_idx_ << " " << t_opt_start_idx_ << " " << t_fut_end_idx_ << " "
                                << t_counter_end_idx_ << "\n";
  }

  // iterate sequentially for all products
  for (unsigned int t_fut_idx_ = t_fut_start_idx_, t_opt_idx_ = t_opt_start_idx_; t_fut_idx_ < t_fut_end_idx_;
       t_fut_idx_++) {
    for (unsigned int t_count_ = 0; t_count_ < t_counter_end_idx_; t_count_++, t_opt_idx_++) {
      // buy order handling
      if (bid_int_price_to_place_at_[t_opt_idx_] == INVALID_MEAN_REVERTING_ORDER_PRICE) {
        order_manager_vec_[t_opt_idx_]->CancelAllBidOrders();
      } else {
        PlaceSingleBuyOrder(t_opt_idx_, bid_int_price_to_place_at_[t_opt_idx_]);
      }
      // ask order handling
      if (ask_int_price_to_place_at_[t_opt_idx_] == INVALID_MEAN_REVERTING_ORDER_PRICE) {
        order_manager_vec_[t_opt_idx_]->CancelAllAskOrders();
      } else {
        PlaceSingleSellOrder(t_opt_idx_, ask_int_price_to_place_at_[t_opt_idx_]);
      }
    }
  }
}

void OptionsMeanRevertingTrading::PlaceSingleBuyOrder(unsigned int t_opt_idx_, int int_order_px_) {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << dep_market_view_vec_[t_opt_idx_]->shortcode() << ' ' << int_order_px_ << " Mkt "
                                << dep_market_view_vec_[t_opt_idx_]->bestbid_int_price() << " --- "
                                << dep_market_view_vec_[t_opt_idx_]->bestask_int_price() << '\n';
  }

  HFSAT::SmartOrderManager* t_om_ = order_manager_vec_[t_opt_idx_];
  int t_order_vec_top_bid_t_opt_idx__ = t_om_->GetOrderVecTopBidIndex();
  int t_order_vec_bottom_bid_t_opt_idx__ = t_om_->GetOrderVecBottomBidIndex();
  int t_existing_int_price_ = -1;

  if (t_order_vec_top_bid_t_opt_idx__ != t_order_vec_bottom_bid_t_opt_idx__) {
    DBGLOG_TIME_CLASS_FUNC << "More than one bid orders for " << t_opt_idx_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    std::cerr << "More than one bid orders for " << t_opt_idx_ << "\n";
    exit(-1);
  }

  if (t_om_->GetUnSequencedBids().size() > 1) {
    DBGLOG_TIME_CLASS_FUNC << " More than one unsequenced order for " << t_opt_idx_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    std::cerr << " More than one unsequenced order for " << t_opt_idx_ << "\n";
    exit(-1);
  }

  if (t_om_->GetUnSequencedBids().size() > 0) {
    return;                                            // don't do anything if unseq bid order is live
  } else if (t_order_vec_top_bid_t_opt_idx__ != -1) {  // -1 denotes no active orders in ordermanager
    t_existing_int_price_ = t_om_->GetBidIntPrice(t_order_vec_top_bid_t_opt_idx__);
  }

  int t_order_size_to_place_ =
      std::max(0, std::min(option_unitlots_[t_opt_idx_],
                           option_maxlots_[t_opt_idx_] - product_position_in_lots_[t_opt_idx_])) *
      dep_market_view_vec_[t_opt_idx_]->min_order_size();
  if (t_order_size_to_place_ == 0) {
    DBGLOG_TIME_CLASS_FUNC << opt_shortcode_vec_[t_opt_idx_] << " " << option_unitlots_[t_opt_idx_] << " "
                           << option_maxlots_[t_opt_idx_] << " " << product_position_in_lots_[t_opt_idx_]
                           << " Order Size Place request for size 0 " << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    std::cerr << opt_shortcode_vec_[t_opt_idx_] << " " << option_unitlots_[t_opt_idx_] << " "
              << option_maxlots_[t_opt_idx_] << " " << product_position_in_lots_[t_opt_idx_]
              << " Order Size Place request for size 0 \n";
    exit(-1);
  }

  HFSAT::BaseOrder* t_order_ = NULL;
  if (t_existing_int_price_ != -1) {
    t_order_ = t_om_->GetBottomBidOrderAtIntPx(t_existing_int_price_);
  }

  // if there is no order active place an order
  if (t_existing_int_price_ == -1) {
    DBGLOG_TIME_CLASS_FUNC << " Send Buy Order for " << opt_shortcode_vec_[t_opt_idx_] << ' ' << t_order_size_to_place_
                           << " @ " << int_order_px_ << " best bid "
                           << dep_market_view_vec_[t_opt_idx_]->bestbid_int_price() << " Pos "
                           << product_position_in_lots_[t_opt_idx_] << DBGLOG_ENDL_FLUSH;
    t_om_->SendTradeIntPx(int_order_px_, t_order_size_to_place_, HFSAT::kTradeTypeBuy, 'B', HFSAT::kOrderDay);
  }
  // else if we need to modify price
  else if (t_existing_int_price_ != int_order_px_) {
    //      DBGLOG_TIME_CLASS_FUNC << " Modify Buy Order Price for " << opt_shortcode_vec_[t_opt_idx_] << " Old_Px " <<
    //      t_existing_int_price_ << " New_Px " << int_order_px_ << DBGLOG_ENDL_FLUSH;
    t_om_->ModifyOrderAndLog(t_order_, dep_market_view_vec_[t_opt_idx_]->GetDoublePx(int_order_px_), int_order_px_,
                             t_order_size_to_place_);
  }
}

void OptionsMeanRevertingTrading::PlaceSingleSellOrder(unsigned int t_opt_idx_, int int_order_px_) {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << dep_market_view_vec_[t_opt_idx_]->shortcode() << ' ' << int_order_px_ << " Mkt "
                                << dep_market_view_vec_[t_opt_idx_]->bestbid_int_price() << " --- "
                                << dep_market_view_vec_[t_opt_idx_]->bestask_int_price() << '\n';
  }
  //    std::cout << watch_.msecs_from_midnight() << "PSSO " << dep_market_view_vec_[t_opt_idx_]->shortcode() << ' ' <<
  //    int_order_px_ << " Mkt " << dep_market_view_vec_[t_opt_idx_]->bestbid_int_price() << " --- " <<
  //    dep_market_view_vec_[t_opt_idx_]->bestask_int_price() << '\n';
  HFSAT::SmartOrderManager* t_om_ = order_manager_vec_[t_opt_idx_];
  int t_order_vec_top_ask_t_opt_idx__ = t_om_->GetOrderVecTopAskIndex();
  int t_order_vec_bottom_ask_t_opt_idx__ = t_om_->GetOrderVecBottomAskIndex();
  int t_existing_int_price_ = -1;

  if (t_order_vec_top_ask_t_opt_idx__ != t_order_vec_bottom_ask_t_opt_idx__) {
    DBGLOG_TIME_CLASS_FUNC << "More than one ask orders for " << opt_shortcode_vec_[t_opt_idx_] << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    std::cerr << "More than one ask orders for " << opt_shortcode_vec_[t_opt_idx_] << "\n";
    exit(-1);
  }

  if (t_om_->GetUnSequencedAsks().size() > 1) {
    DBGLOG_TIME_CLASS_FUNC << " More than one unsequenced sell order for " << opt_shortcode_vec_[t_opt_idx_]
                           << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    std::cerr << " More than one unsequenced sell order for " << opt_shortcode_vec_[t_opt_idx_] << DBGLOG_ENDL_FLUSH;
    exit(-1);
  }

  if (t_om_->GetUnSequencedAsks().size() > 0) {
    return;                                            // don't do anything if unseq bid order is live
  } else if (t_order_vec_top_ask_t_opt_idx__ != -1) {  // -1 denotes no active orders in ordermanager
    t_existing_int_price_ = t_om_->GetAskIntPrice(t_order_vec_top_ask_t_opt_idx__);
  }

  int t_order_size_to_place_ =
      std::max(0, std::min(option_unitlots_[t_opt_idx_],
                           option_maxlots_[t_opt_idx_] + product_position_in_lots_[t_opt_idx_])) *
      dep_market_view_vec_[t_opt_idx_]->min_order_size();
  if (t_order_size_to_place_ == 0) {
    DBGLOG_TIME_CLASS_FUNC << opt_shortcode_vec_[t_opt_idx_] << " " << option_unitlots_[t_opt_idx_] << " "
                           << option_maxlots_[t_opt_idx_] << " " << product_position_in_lots_[t_opt_idx_]
                           << " Order Size Place request for size 0 " << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    std::cerr << opt_shortcode_vec_[t_opt_idx_] << " " << option_unitlots_[t_opt_idx_] << " "
              << option_maxlots_[t_opt_idx_] << " " << product_position_in_lots_[t_opt_idx_]
              << " Order Size Place request for size 0 \n";
    exit(-1);
  }

  HFSAT::BaseOrder* t_order_ = NULL;
  if (t_existing_int_price_ != -1) {
    t_order_ = t_om_->GetBottomAskOrderAtIntPx(t_existing_int_price_);
  }

  // if there is no order active place an order
  if (t_existing_int_price_ == -1) {
    DBGLOG_TIME_CLASS_FUNC << " Send Sell Order for " << opt_shortcode_vec_[t_opt_idx_] << ' ' << t_order_size_to_place_
                           << " @ " << int_order_px_ << " best ask "
                           << dep_market_view_vec_[t_opt_idx_]->bestask_int_price() << " Pos"
                           << product_position_in_lots_[t_opt_idx_] << DBGLOG_ENDL_FLUSH;
    t_om_->SendTradeIntPx(int_order_px_, t_order_size_to_place_, HFSAT::kTradeTypeSell, 'B', HFSAT::kOrderDay);
  }
  // else if we need to modify price
  else if (t_existing_int_price_ != int_order_px_) {
    //      DBGLOG_TIME_CLASS_FUNC << " Modify Sell Order Price for " << opt_shortcode_vec_[t_opt_idx_] << " Old_Px " <<
    //      t_existing_int_price_ << " New_Px " << int_order_px_ << DBGLOG_ENDL_FLUSH;
    t_om_->ModifyOrderAndLog(t_order_, dep_market_view_vec_[t_opt_idx_]->GetDoublePx(int_order_px_), int_order_px_,
                             t_order_size_to_place_);
  }
}

void OptionsMeanRevertingTrading::ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_) {
  int total_pnl_ = 0;
  int total_volume_ = 0;
  for (unsigned int t_fut_idx_ = 0, t_opt_idx_ = 0; t_fut_idx_ < num_trading_futures_; t_fut_idx_++) {
    int total_pnl_per_future_ = 0;
    int total_volume_per_future_ = 0;
    for (unsigned int t_count_ = 0; t_count_ < num_options_per_future_; t_count_++, t_opt_idx_++) {
      if (order_manager_vec_[t_opt_idx_]->trade_volume() > 0) {
        std::cout << "Opt: " << watch_.YYYYMMDD() << ' ' << opt_shortcode_vec_[t_opt_idx_] << " "
                  << order_manager_vec_[t_opt_idx_]->base_pnl().ReportConservativeTotalPNL(true) << " "
                  << order_manager_vec_[t_opt_idx_]->trade_volume() / dep_market_view_vec_[t_opt_idx_]->min_order_size()
                  << " " << product_position_in_lots_[t_opt_idx_] << " OrderManager Count "
                  << (order_manager_vec_[t_opt_idx_]->SendOrderCount() +
                      order_manager_vec_[t_opt_idx_]->CxlOrderCount() +
                      order_manager_vec_[t_opt_idx_]->ModifyOrderCount()) << "\n";

        total_pnl_per_future_ += order_manager_vec_[t_opt_idx_]->base_pnl().ReportConservativeTotalPNL(true);
        total_volume_per_future_ +=
            order_manager_vec_[t_opt_idx_]->trade_volume() / (dep_market_view_vec_[t_opt_idx_]->min_order_size());
      }
    }
    std::cout << "Fut: " << watch_.YYYYMMDD() << " " << trading_fut_shortcode_vec_[t_fut_idx_] << " "
              << total_pnl_per_future_ << " " << total_volume_per_future_ << " "
              << product_delta_adjusted_position_in_lots_[t_fut_idx_] << "\n";
    total_pnl_ += total_pnl_per_future_;
    total_volume_ += total_volume_per_future_;
  }
  std::cout << "All: " << watch_.YYYYMMDD() << " OMRT " << total_pnl_ << " " << total_volume_ << " " << global_min_pnl_
            << " " << my_global_position() << "\n";
}
}
