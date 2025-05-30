/**
    \file IndicatorsCode/trade_book_adjusted_price.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/trade_book_adjusted_price.hpp"
#include "dvctrade/ModelMath/model_creator.hpp"

// DEFAULT::
// INDICATOR 1.00 MultMktComplexPriceTopOff DEP 4 0.4
// INDICATOR 1.00 DiffEDAvgTPxBasepx DEP 1

namespace HFSAT {

void TradeBookAdjustedPrice::CollectShortCodes(std::vector<std::string>& t_shortcodes_affecting_this_indicator_,
                                               std::vector<std::string>& t_ors_source_needed_vec_,
                                               const std::vector<const char*>& t_tokens_) {
  VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator_, (std::string)t_tokens_[3]);
}

TradeBookAdjustedPrice* TradeBookAdjustedPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                                                  const std::vector<const char*>& t_tokens_,
                                                                  PriceType_t t_basepx_pxtype_) {
  PriceType_t t_price_type_ = kPriceTypeMax;

  if (t_tokens_.size() < 7) {
    if (t_tokens_.size() < 6) {
      ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                  "INDICATOR _this_weight_ _indicator_string_ _dep_shortcode_ _ratio1_ _ratio2_ _price_type_");
    } else {
      t_dbglogger_ << "INDICATOR _this_weight_ _indicator_string_ _dep_shortcode_ _ratio1_ _ratio2_ _price_type_"
                   << "\n";
      t_dbglogger_.CheckToFlushBuffer();
      t_price_type_ = t_basepx_pxtype_;
    }
  } else {
    if (std::string(t_tokens_[6]).compare("#") == 0) {
      t_price_type_ = t_basepx_pxtype_;
    } else {
      t_price_type_ = StringToPriceType_t(t_tokens_[6]);
    }
  }

  return GetUniqueInstance(t_dbglogger_, t_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(t_tokens_[3])),
                           atof(t_tokens_[4]), atof(t_tokens_[5]), t_price_type_);
}

TradeBookAdjustedPrice* TradeBookAdjustedPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                                                  const SecurityMarketView& t_indep_market_view_,
                                                                  double t_book_weight_, double t_trade_weight_,
                                                                  PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_indep_market_view_.shortcode() << ' ' << t_book_weight_ << ' ' << t_trade_weight_
              << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  // if you are reading from the file price_type doesnt not matter
  static std::map<std::string, TradeBookAdjustedPrice*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new TradeBookAdjustedPrice(t_dbglogger_, t_watch_, concise_indicator_description_, t_indep_market_view_,
                                   t_book_weight_, t_trade_weight_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TradeBookAdjustedPrice::TradeBookAdjustedPrice(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                               const std::string& t_concise_indicator_description_,
                                               const SecurityMarketView& t_indep_market_view_, double t_book_weight_,
                                               double t_trade_weight_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, t_watch_, t_concise_indicator_description_),
      indep_market_view_(t_indep_market_view_),
      price_type_(_price_type_),
      book_weight_(t_book_weight_),
      trade_weight_(t_trade_weight_) {
  std::ifstream t_composite_infile_;
  int this_YYYYMMDD_ = DateTime::CalcPrevDay(watch_.YYYYMMDD());
  std::string t_composite_infilename_ = "";
  std::string book_indicator_ = "INDICATOR 1.00 MultMktComplexPriceTopOff " +
                                std::string(indep_market_view_.shortcode()) + " 4 0.4 " +
                                PriceType_t_To_FullString(_price_type_);
  std::string trade_indicator_ = "INDICATOR 1.00 DiffEDAvgTPxBasepx " + std::string(indep_market_view_.shortcode()) +
                                 " 1 " + PriceType_t_To_FullString(_price_type_);
  for (unsigned int ii = 0; ii < 40; ii++) {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << std::string(BASETRADEINFODIR) << "CompositePrice/composite_price_info_" << this_YYYYMMDD_ << ".txt";
    t_composite_infilename_ = t_temp_oss_.str();

    if (FileUtils::exists(t_composite_infilename_)) {
      break;
    } else {
      this_YYYYMMDD_ = DateTime::CalcPrevDay(this_YYYYMMDD_);
    }
  }

  if (!FileUtils::exists(t_composite_infilename_)) {  // All attempts failed.
    t_composite_infilename_ = std::string(BASETRADEINFODIR) + "CompositePrice/composite_price_info_DEFAULT.txt";
  }
  t_composite_infile_.open(t_composite_infilename_.c_str(), std::ifstream::in);
  t_dbglogger_ << "retrieving sub indicators from: " << t_composite_infilename_.c_str() << "\n";
  t_dbglogger_.CheckToFlushBuffer();

  if (t_composite_infile_.is_open()) {
    const int iBufferLength = 1024;
    char readline_buffer_[iBufferLength];
    bzero(readline_buffer_, iBufferLength);
    while (t_composite_infile_.good()) {
      bzero(readline_buffer_, iBufferLength);
      t_composite_infile_.getline(readline_buffer_, iBufferLength);
      std::string sIndicatorString_ = readline_buffer_;
      PerishableStringTokenizer st_(readline_buffer_, iBufferLength);
      const std::vector<const char*>& sub_tokens_ = st_.GetTokens();
      if (sub_tokens_.size() > 3 && strcmp(sub_tokens_[0], "INDICATOR") == 0 &&
          strcmp(sub_tokens_[3], indep_market_view_.shortcode().c_str()) == 0) {
        if (sIndicatorString_.find("Mult") != std::string::npos) {
          book_indicator_ = sIndicatorString_;
        } else {
          trade_indicator_ = sIndicatorString_;
        }
      }
    }
    t_composite_infile_.close();
  }

  t_dbglogger_ << "sub indicators : " << book_indicator_ << "\n" << trade_indicator_ << "\n";
  t_dbglogger_.CheckToFlushBuffer();

  const int book_ind_string_length_ = book_indicator_.length() + 1;
  char book_ind_char_buffer_[book_ind_string_length_];
  bzero(book_ind_char_buffer_, book_ind_string_length_);
  strcpy(book_ind_char_buffer_, book_indicator_.c_str());
  PerishableStringTokenizer bst_(book_ind_char_buffer_, book_ind_string_length_);
  const std::vector<const char*>& bsub_tokens_ = bst_.GetTokens();
  CommonIndicator* bt_this_indicator_ =
      ModelCreator::GetIndicatorFromTokens(t_dbglogger_, t_watch_, bsub_tokens_, price_type_);
  bt_this_indicator_->add_indicator_listener(sub_indicator_vec_.size(), this, book_weight_);
  sub_indicator_vec_.push_back(bt_this_indicator_);

  const int trade_ind_string_length_ = trade_indicator_.length() + 1;
  char trade_ind_char_buffer_[trade_ind_string_length_];
  bzero(trade_ind_char_buffer_, trade_ind_string_length_);
  strcpy(trade_ind_char_buffer_, trade_indicator_.c_str());
  PerishableStringTokenizer tst_(trade_ind_char_buffer_, trade_ind_string_length_);
  const std::vector<const char*>& tsub_tokens_ = tst_.GetTokens();
  CommonIndicator* tt_this_indicator_ =
      ModelCreator::GetIndicatorFromTokens(t_dbglogger_, t_watch_, tsub_tokens_, price_type_);
  tt_this_indicator_->add_indicator_listener(sub_indicator_vec_.size(), this, trade_weight_);
  sub_indicator_vec_.push_back(tt_this_indicator_);

  InitializeValues(sub_indicator_vec_.size());
}

void TradeBookAdjustedPrice::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    is_ready_vec_[_indicator_index_] = true;
    is_ready_ = AreAllReady();
  } else if (!data_interrupted_) {
    if (sub_indicator_vec_[_indicator_index_]->IsDataInterrupted()) {
      indicator_value_ -= prev_value_vec_[_indicator_index_];
      prev_value_vec_[_indicator_index_] = 0;
    } else {
      indicator_value_ += (_new_value_ - prev_value_vec_[_indicator_index_]);
      prev_value_vec_[_indicator_index_] = _new_value_;
    }
    NotifyIndicatorListeners(indicator_value_);
  }
}

void TradeBookAdjustedPrice::InitializeValues(unsigned int _num_indicators_) {
  is_ready_vec_.resize(_num_indicators_);
  prev_value_vec_.resize(_num_indicators_);

  VectorUtils::FillInValue(is_ready_vec_, false);
  VectorUtils::FillInValue(prev_value_vec_, 0.0);
}

bool TradeBookAdjustedPrice::AreAllReady() { return VectorUtils::CheckAllForValue(is_ready_vec_, true); }
void TradeBookAdjustedPrice::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                     const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void TradeBookAdjustedPrice::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues(sub_indicator_vec_.size());
  data_interrupted_ = false;
}
}
