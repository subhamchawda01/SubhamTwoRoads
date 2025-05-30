/**
    \file IndicatorsCode/synthetic_index.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/synthetic_index.hpp"
#include "dvctrade/Indicators/index_utils.hpp"

namespace HFSAT {

/**
 *
 * @param _shortcodes_affecting_this_indicator_
 * @param _ors_source_needed_vec_
 * @param r_tokens_
 */
void SyntheticIndex::CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                       std::vector<std::string> &_ors_source_needed_vec_,
                                       const std::vector<const char *> &r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  if (r_tokens_.size() > 5) {
    CollectConstituents(_shortcodes_affecting_this_indicator_, r_tokens_[3], atoi(r_tokens_[5]));
  } else if (r_tokens_.size() > 4) {
    CollectConstituents(_shortcodes_affecting_this_indicator_, r_tokens_[3]);
  }
}

/**
 *
 * @param _shortcodes_affecting_this_indicator_
 * @param shortcode_
 */
void SyntheticIndex::CollectConstituents(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                         std::string shortcode_, int _use_fut_) {
  if (shortcode_.compare("BR_IND_0") == 0) {
    CollectBovespaConstituents(_shortcodes_affecting_this_indicator_, shortcode_);
  } else if ((shortcode_.compare("NSE_NIFTY_FUT0") == 0) || (shortcode_.compare("NSE_BANKNIFTY_FUT0") == 0)) {
    CollectNSEIndicesConstituents(_shortcodes_affecting_this_indicator_, shortcode_, _use_fut_);
  }
}

void SyntheticIndex::CollectBovespaConstituents(std::vector<std::string> &shortcodes_affecting_this_indicator,
                                                std::string shortcode) {
  std::ifstream shortcode_list_file;
  int date_ = HFSAT::ExchangeSymbolManager::GetUniqueInstance().YYYYMMDD();  // hack
  std::string filename_ = GetBovespaIndexConstituentFileList(date_);

  shortcode_list_file.open(filename_.c_str(), std::ifstream::in);
  if (shortcode_list_file.is_open()) {
    while (shortcode_list_file.good()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      shortcode_list_file.getline(readline_buffer_, kL1AvgBufferLen);
      if (readline_buffer_[0] == '#') {
        continue;
      }
      // # shortcode_ theoretical_volume_
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() < 2) {
        continue;
      }
      std::string t_shc_ = tokens_[0];
      if (tokens_.size() > 4 && std::string(tokens_[1]).compare("Reductor") == 0) {
        continue;
      }
      VectorUtils::UniqueVectorAdd(shortcodes_affecting_this_indicator, t_shc_);
      bzero(readline_buffer_, kL1AvgBufferLen);
    }
  }
}

/**
 *
 * @param shortcodes_affecting_this_indicator
 * @param shortcode
 */
void SyntheticIndex::CollectNSEIndicesConstituents(std::vector<std::string> &shortcodes_affecting_this_indicator,
                                                   std::string shortcode, int use_fut_) {
  int tradingdate = HFSAT::ExchangeSymbolManager::GetUniqueInstance().YYYYMMDD();
  std::vector<std::string> cons_list;
  if (shortcode.compare("NSE_NIFTY_FUT0") == 0) {
    GetConstituentListInNIFTY(tradingdate, cons_list, "NIFTY");
  } else if (shortcode.compare("NSE_BANKNIFTY_FUT0") == 0) {
    GetConstituentListInNIFTY(tradingdate, cons_list, "BANKNIFTY");
  }

  if (use_fut_ == 1) {
    for (unsigned i = 0; i < cons_list.size(); i++) {
      cons_list[i] = cons_list[i] + "_FUT0";
    }
  }

  HFSAT::VectorUtils::UniqueVectorAdd(shortcodes_affecting_this_indicator, cons_list);
}

SyntheticIndex *SyntheticIndex::GetUniqueInstance(DebugLogger &t_dbglogger, const Watch &r_watch,
                                                  const std::vector<const char *> &r_tokens,
                                                  PriceType_t basepx_pxtype) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _price_type_
  if (r_tokens.size() < 5) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger,
                "   FutureToSoptPriceing Incorrect Syntax. Correct syntax would b  INDICATOR _this_weight_ "
                "_indicator_string_ _dep_market_view_  _price_type_ ");
  }

  if (r_tokens.size() == 5) {
    return GetUniqueInstance(t_dbglogger, r_watch,
                             (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens[3])),
                             StringToPriceType_t(r_tokens[4]));
  } else {
    return GetUniqueInstance(t_dbglogger, r_watch,
                             (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens[3])),
                             StringToPriceType_t(r_tokens[4]), atoi(r_tokens[5]));
  }
}

/**
 *
 * @param t_dbglogger
 * @param r_watch
 * @param dep_market_view
 * @param price_type
 * @return
 */
SyntheticIndex *SyntheticIndex::GetUniqueInstance(DebugLogger &t_dbglogger, const Watch &r_watch,
                                                  SecurityMarketView *dep_market_view, PriceType_t price_type,
                                                  int _use_fut_) {
  std::ostringstream t_temp_oss_;

  t_temp_oss_ << VarName() << ' ' << dep_market_view->secname() << ' ' << PriceType_t_To_String(price_type)
              << _use_fut_;

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SyntheticIndex *> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new SyntheticIndex(
        t_dbglogger, r_watch, concise_indicator_description_, dep_market_view, price_type, _use_fut_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

/**
 *
 * @param t_dbglogger
 * @param r_watch
 * @param concise_indicator_description_
 * @param dep_market_view
 * @param price_type
 */
SyntheticIndex::SyntheticIndex(DebugLogger &t_dbglogger, const Watch &r_watch,
                               const std::string &concise_indicator_description_, SecurityMarketView *dep_market_view,
                               PriceType_t price_type, int _use_fut_)
    : CommonIndicator(t_dbglogger, r_watch, concise_indicator_description_),
      dep_market_view_(dep_market_view),
      sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
      price_type_(price_type),
      index_divisor_(1),
      index_multiplier_(1),
      synthetic_index_(0),
      tradingdate_(r_watch.YYYYMMDD()),
      current_dep_price_(0),
      current_indep_price_(0),
      base_index_value_(1),
      base_mkt_capital_(1),
      indicator_start_mfm_(0),
      dep_interrupted_(false),
      indep_interrupted_(false),
      is_ready_vec_(),
      dep_ready_(false),
      use_fut_(_use_fut_) {
  // Rescale teh vector to sec_name_indexer size

  indep_market_view_vec_.resize(sec_name_indexer_.NumSecurityId(), NULL);
  indep_interrupted_vec_.resize(sec_name_indexer_.NumSecurityId(), false);
  last_constituent_price_.resize(sec_name_indexer_.NumSecurityId(), 0.0);
  constituent_theoretical_volume_.resize(sec_name_indexer_.NumSecurityId(), 0.0);
  constituent_price_from_file_.resize(sec_name_indexer_.NumSecurityId(), 0.0);
  constituent_weight_.resize(sec_name_indexer_.NumSecurityId(), 0);

  is_illiquid_prod_.resize(sec_name_indexer_.NumSecurityId(), false);
  is_ready_vec_.resize(sec_name_indexer_.NumSecurityId(), false);

  if (dep_market_view_->shortcode().compare("BR_IND_0") == 0) {
    index_divisor_ = 0.00017401674;
    index_multiplier_ = 1.0 / index_divisor_;
    LoadBovespaConstituents();
    index_type_ = 1;

  } else if ((dep_market_view_->shortcode().compare("NSE_NIFTY_FUT0") == 0) ||
             dep_market_view_->shortcode().compare("NSE_BANKNIFTY_FUT0") == 0) {
    LoadNSEConstituents();
    index_type_ = 2;
  }

  for (unsigned i = 0; i < indep_market_view_vec_.size(); i++) {
    if ((indep_market_view_vec_[i] != NULL) && !indep_market_view_vec_[i]->subscribe_price_type(this, price_type_)) {
      PriceType_t t_error_price_type_ = price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
    }
  }
}

void SyntheticIndex::LoadBovespaConstituents() {
  std::ifstream shortcode_list_file;
  std::string filename_ = GetBovespaIndexConstituentFileList(watch_.YYYYMMDD());
  DBGLOG_TIME_CLASS_FUNC_LINE << " Constituent list file picked: " << filename_ << DBGLOG_ENDL_FLUSH;
  shortcode_list_file.open(filename_.c_str(), std::ifstream::in);
  if (shortcode_list_file.is_open()) {
    while (shortcode_list_file.good()) {
      // # Name _theoritical_quantity_ percentage_contribution_in_index_
      // _theoretical_quantity_is and reductor both are divided by factor of 10^11
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      shortcode_list_file.getline(readline_buffer_, kL1AvgBufferLen);
      if (readline_buffer_[0] == '#') {
        continue;
      }
      // # shortcode_ theoretical_volume_
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() < 2) {
        continue;
      }
      std::string t_shc_ = tokens_[0];
      if (tokens_.size() > 4 && std::string(tokens_[1]).compare("Reductor") == 0) {
        index_divisor_ = atof(tokens_[3]);
        index_multiplier_ = 1.0 / index_divisor_;
        continue;
      }
      int sec_id_ = sec_name_indexer_.GetIdFromString(t_shc_);
      if (sec_id_ < 0) {
        ExitVerbose(kExitErrorCodeGeneral, (" Shortcode " + t_shc_ + " not added to secname indexer.").c_str());
      }
      indep_market_view_vec_[sec_id_] = (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(tokens_[0]));
      constituent_theoretical_volume_[sec_id_] = atof(tokens_[1]);
      bzero(readline_buffer_, kL1AvgBufferLen);
    }
  }

  std::ifstream price_file_;
  std::stringstream st_;
  st_ << BASETRADEINFODIR << "/IndexInfo/closing_price_" << DateTime::CalcPrevWeekDay(watch_.YYYYMMDD()) << ".txt";
  filename_ = st_.str();
  int size_ = filename_.size();
  if (FileUtils::ExistsWithSize(filename_, size_)) {
    price_file_.open(filename_.c_str(), std::ifstream::in);
  } else {
    st_.clear();
    st_ << BASETRADEINFODIR << "/IndexInfo/closing_price_" << watch_.YYYYMMDD() << ".txt";
    filename_ = st_.str();
    price_file_.open(filename_.c_str(), std::ifstream::in);
  }
  if (price_file_.is_open()) {
    const int kPriceFileBufferLen = 1024;
    char readline_buffer_[kPriceFileBufferLen];
    while (price_file_.good()) {
      bzero(readline_buffer_, kPriceFileBufferLen);
      price_file_.getline(readline_buffer_, kPriceFileBufferLen);
      PerishableStringTokenizer strtok_(readline_buffer_, kPriceFileBufferLen);
      const std::vector<const char *> &tokens_ = strtok_.GetTokens();
      if (tokens_.size() < 3) {
        continue;
      }
      std::string shc_ = tokens_[0];
      int sec_id_ = sec_name_indexer_.GetIdFromString(shc_);
      if (sec_id_ < 0) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " Shortcode " << shc_ << " not added to secname indexer." << DBGLOG_ENDL_FLUSH;
        continue;
      }
      constituent_price_from_file_[sec_id_] = atof(tokens_[1]);
    }
  }

  std::ifstream illiquid_prod_;
  std::string illiquid_prod_list_ = std::string(BASETRADEINFODIR) + std::string("/IndexInfo/illiquid_prod_list.txt");
  illiquid_prod_.open(illiquid_prod_list_.c_str(), std::ifstream::in);
  if (illiquid_prod_.is_open()) {
    const int kPriceFileBufferLen = 1024;
    char readline_buffer_[kPriceFileBufferLen];
    while (illiquid_prod_.good()) {
      bzero(readline_buffer_, kPriceFileBufferLen);
      illiquid_prod_.getline(readline_buffer_, kPriceFileBufferLen);
      PerishableStringTokenizer strtok_(readline_buffer_, kPriceFileBufferLen);
      const std::vector<const char *> &tokens_ = strtok_.GetTokens();
      if (tokens_.size() < 1) {
        continue;
      }
      std::string shc_ = tokens_[0];
      int sec_id_ = sec_name_indexer_.GetIdFromString(shc_);
      if (sec_id_ < 0) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " Shortcode " << shc_ << " not added to secname indexer." << DBGLOG_ENDL_FLUSH;
        continue;
      }
      is_illiquid_prod_[sec_id_] = true;
    }
  }
}

void SyntheticIndex::LoadNSEConstituents() {
  std::vector<std::string> constituent_list;
  std::vector<double> const_theo_vol;
  std::vector<double> const_investible_weight;
  if (dep_market_view_->shortcode().compare("NSE_NIFTY_FUT0") == 0) {
    GetConstituentListInNIFTY(tradingdate_, constituent_list, "NIFTY");
    GetConstituentEquityVolNIFTY(tradingdate_, const_theo_vol, "NIFTY");
    base_index_value_ = 1000.0;
    base_mkt_capital_ = 2.06 * pow(10, 12);
    index_divisor_ = GetIndexFactorNIFTY(tradingdate_, "NIFTY");
    index_multiplier_ = 1.0 / index_divisor_;
  } else {
    GetConstituentListInNIFTY(tradingdate_, constituent_list, "BANKNIFTY");
    GetConstituentEquityVolNIFTY(tradingdate_, const_theo_vol, "BANKNIFTY");
    base_index_value_ = 1000.0;
    base_mkt_capital_ = 0.8 * pow(10, 12);
    index_divisor_ = GetIndexFactorNIFTY(tradingdate_, "BANKNIFTY");
    index_multiplier_ = 1.0 / index_divisor_;
  }

  for (unsigned i = 0; i < constituent_list.size(); i++) {
    if (use_fut_ == 1) {
      constituent_list[i] = constituent_list[i] + "_FUT0";
    }
    int sec_id = sec_name_indexer_.GetIdFromString(constituent_list[i]);
    if (sec_id < 0) {
      ExitVerbose(kExitErrorCodeGeneral,
                  (" Shortcode " + constituent_list[i] + " not added to secname indexer.").c_str());
    }
    indep_market_view_vec_[sec_id] = (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(constituent_list[i]));
    constituent_theoretical_volume_[sec_id] = const_theo_vol[i];
    // constituent_weight_[sec_id] = const_investible_weight[i];
    DBGLOG_TIME_CLASS_FUNC_LINE << " shc: " << constituent_list[i]
                                << " theo_vol: " << constituent_theoretical_volume_[sec_id]
                                //  << constituent_weight_[sec_id]
                                << DBGLOG_ENDL_FLUSH;
  }
}

void SyntheticIndex::WhyNotReady() {
  if (!is_ready_) {
    if (!dep_ready_) {
      if (dep_market_view_->is_ready_complex(2)) {
        dep_ready_ = true;
      } else {
        DBGLOG_TIME_CLASS << dep_market_view_->secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
    }

    for (unsigned i = 0; i < indep_market_view_vec_.size(); i++) {
      if ((indep_market_view_vec_[i] != NULL) && !(is_ready_vec_[i])) {
        if (indep_market_view_vec_[i]->is_ready() &&
            (!(indep_market_view_vec_[i]->IsBidBookEmpty() || indep_market_view_vec_[i]->IsAskBookEmpty()))) {
          is_ready_vec_[i] = true;
        } else {
          DBGLOG_TIME_CLASS << indep_market_view_vec_[i]->secname() << " is_ready() = false " << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
        }
      }
    }
  }
}

/**
 *
 * @param security_id
 * @param cr_market_update_info
 */
void SyntheticIndex::OnMarketUpdate(const unsigned int security_id, const MarketUpdateInfo &cr_market_update_info) {
  // the fllowing has been added since above we could be potentially be accessing the price when the
  // SMV is not ready

  //  if (dep_market_view_->IsBidBookEmpty() || dep_market_view_->IsAskBookEmpty()) {
  //    return;
  //  }
  if (indep_market_view_vec_[security_id] == NULL) {
    return;
  }
  if (!is_ready_) {
    if (indicator_start_mfm_ == 0) {
      DBGLOG_TIME_CLASS << "setting indicatorstart: " << DBGLOG_ENDL_FLUSH;
      indicator_start_mfm_ = watch_.msecs_from_midnight();
    }
    if (!dep_ready_) {
      if (dep_market_view_->is_ready_complex(2)) {
        dep_ready_ = true;
      }
    }

    if (dep_ready_) {
      is_ready_ = true;
      for (unsigned i = 0; i < indep_market_view_vec_.size(); i++) {
        if ((indep_market_view_vec_[i] != NULL)) {
          if (!(is_ready_vec_[i])) {
            if (indep_market_view_vec_[i]->is_ready() &&
                (!(indep_market_view_vec_[i]->IsBidBookEmpty() || indep_market_view_vec_[i]->IsAskBookEmpty()))) {
              is_ready_vec_[i] = true;
            }

            if (!is_ready_vec_[i] && (watch_.msecs_from_midnight() > indicator_start_mfm_ + 10 * 60 * 1000) &&
                is_illiquid_prod_[i]) {
              // There hasn't bee data for this instruement for last 30 minutes. using the closing price and getting
              // ready on it
              DBGLOG_TIME_CLASS << "making indicator ready: shc_" << indep_market_view_vec_[i]->shortcode()
                                << "startmfm: " << indicator_start_mfm_ << DBGLOG_ENDL_FLUSH;
              is_ready_vec_[i] = true;
            }
          }
          is_ready_ = (is_ready_ && is_ready_vec_[i]) && (last_constituent_price_[i] != 0);
        }
      }
      //     if (is_ready_) {
      //       InitializeValues();
      //     }
    }
  }

  double current_price = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info);
  if (current_price <= 0) {
    current_price = constituent_price_from_file_[security_id];
  }
  if (index_type_ == 1) {
    ComputeBovespaIndex(security_id, current_price);
  } else if (index_type_ == 2) {
    ComputeNSEIndices(security_id, current_price);
  }

  if (is_ready_) {
    NotifyIndicatorListeners(indicator_value_);
  }
}

/**
 *
 * @param sec_id
 * @param current_price
 */
void SyntheticIndex::ComputeBovespaIndex(const unsigned int sec_id, const double current_price) {
  synthetic_index_ +=
      (current_price - last_constituent_price_[sec_id]) * constituent_theoretical_volume_[sec_id] * index_multiplier_;
  last_constituent_price_[sec_id] = current_price;
  indicator_value_ = synthetic_index_;
}

/**
 *
 * @param sec_id
 * @param current_price
 */
void SyntheticIndex::ComputeNSEIndices(const unsigned int sec_id, const double current_price) {
  synthetic_index_ +=
      (current_price - last_constituent_price_[sec_id]) * constituent_theoretical_volume_[sec_id] * index_multiplier_;
  //  std::cout << "Synthetic Index :" << synthetic_index_ << "last_price: " << last_constituent_price_[sec_id] <<
  //  "current_price: " <<  current_price << "Sec ID :" << sec_id << std::endl;
  last_constituent_price_[sec_id] = current_price;
  indicator_value_ = synthetic_index_;
}

void SyntheticIndex::InitializeValues() { indicator_value_ = 0; }

/**
 *
 * @param _security_id_
 * @param msecs_since_last_receive_
 */
void SyntheticIndex::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  // On Market data interrupted, we would want to have the last value instead of reinitializing
  // In case of auction, since we stop the messages from file/live source, we dont need to do anything here

  for (unsigned i = 0; i < indep_market_view_vec_.size(); i++) {
    if ((indep_market_view_vec_[i] != NULL) && indep_market_view_vec_[i]->security_id() == _security_id_) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " Data Interrupted for sec: " << indep_market_view_vec_[i]->shortcode()
                                  << DBGLOG_ENDL_FLUSH;
      indep_interrupted_vec_[i] = true;
      indep_interrupted_ = true;
    }
  }
}

void SyntheticIndex::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    for (unsigned i = 0; i < indep_market_view_vec_.size(); i++) {
      if ((indep_market_view_vec_[i] != NULL) && indep_market_view_vec_[i]->security_id() == _security_id_) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " Data Resumed for sec: " << indep_market_view_vec_[i]->shortcode()
                                    << DBGLOG_ENDL_FLUSH;
        indep_interrupted_vec_[i] = false;
      }
      indep_interrupted_ = (indep_interrupted_ || indep_interrupted_vec_[i]);
    }
  }
}
}
