/**
   \file IndicatorsCode/moore_penrise).cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/moore_penrose_fx_sf.hpp"

namespace HFSAT {

void MoorePenroseFxSf::CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                         std::vector<std::string> &_ors_source_needed_vec_,
                                         const std::vector<const char *> &r_tokens_) {
  std::ifstream moore_penrose_file;
  std::ostringstream filepath;
  filepath << "/spare/local/tradeinfo/MoorePenroseFxSf/" << (std::string)r_tokens_[5];
  moore_penrose_file.open(filepath.str(), std::ifstream::in);
  const int kL1AvgBufferLen = 1024;
  char readline_buffer_[kL1AvgBufferLen];
  if (moore_penrose_file.is_open()) {
    bzero(readline_buffer_, kL1AvgBufferLen);
    moore_penrose_file.getline(readline_buffer_, kL1AvgBufferLen);

    if (moore_penrose_file.good()) {
      bzero(readline_buffer_, kL1AvgBufferLen);
      moore_penrose_file.getline(readline_buffer_, kL1AvgBufferLen);
    }

    while (moore_penrose_file.good()) {
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if ((std::string(tokens_[0]).compare("SHORTCODES") == 0)) {
        VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)tokens_[1]);
        VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)tokens_[2]);
        VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)tokens_[3]);
      }
      bzero(readline_buffer_, kL1AvgBufferLen);
      moore_penrose_file.getline(readline_buffer_, kL1AvgBufferLen);
    }
    moore_penrose_file.close();

  } else {
    std::cerr << "can't open file : " << filepath.str() << "\n";
    exit(0);
  }
}

MoorePenroseFxSf *MoorePenroseFxSf::GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                      const std::vector<const char *> &r_tokens_,
                                                      PriceType_t _basepx_pxtype_) {
  ShortcodeSecurityMarketViewMap::StaticCheckValid(std::string(r_tokens_[3]));
  PriceType_t t_price_type_ = kPriceTypeMax;
  if (r_tokens_.size() < 5) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_, "Less args");
  } else if (r_tokens_.size() < 6) {
    t_price_type_ = StringToPriceType_t(r_tokens_[4]);
  } else {
    auto i = 0u;
    while (t_price_type_ == kPriceTypeMax && i < r_tokens_.size() && std::string(r_tokens_[i]).compare("#") != 0) {
      t_price_type_ = StringToPriceType_t(r_tokens_[i]);
      i++;
    }
    if (t_price_type_ == kPriceTypeMax) {
      t_price_type_ = _basepx_pxtype_;
    }
  }

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(std::string(r_tokens_[3]))),
                           std::string(r_tokens_[5]), t_price_type_);
}

MoorePenroseFxSf *MoorePenroseFxSf::GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                      SecurityMarketView &t_dep_market_view_,
                                                      std::string moore_penrose_filename, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_dep_market_view_.secname() << ' ' << ' ' << PriceType_t_To_String(_price_type_)
              << " " << moore_penrose_filename;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, MoorePenroseFxSf *> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new MoorePenroseFxSf(t_dbglogger_, r_watch_, concise_indicator_description_, t_dep_market_view_,
                             moore_penrose_filename, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

MoorePenroseFxSf::MoorePenroseFxSf(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                   const std::string &concise_indicator_description_,
                                   SecurityMarketView &t_dep_market_view_, std::string moore_penrose_filename,
                                   PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      price_type_(_price_type_),
      tod_smv_(NULL),
      tom_smv_(NULL),
      fut_smv_(NULL),
      tod_coeff_(0),
      tom_coeff_(0),
      fut_coeff_(0),
      last_recorded_tod_price_(0),
      last_recorded_tom_price_(0),
      last_recorded_fut_price_(0) {
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(r_watch_.YYYYMMDD());
  dte_ = HFSAT::CurveUtils::_get_term_(r_watch_.YYYYMMDD(),
                                       HFSAT::ExchangeSymbolManager::GetExchSymbol(fut_smv_->shortcode()));
  Initialize(moore_penrose_filename);
  if (!dep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }
  is_data_interrupted_vec_.assign(dep_market_view_.sec_name_indexer_.NumSecurityId(), false);
}

void MoorePenroseFxSf::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                                    const MarketUpdateInfo &_market_update_info_) {
  OnMarketUpdate(_security_id_, _market_update_info_);
}

void MoorePenroseFxSf::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_) {
  double new_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);

  if (!is_ready_) {
    is_ready_ = tod_smv_->is_ready() && tom_smv_->is_ready() && fut_smv_->is_ready();

    if (is_ready_) {
      ResetIndicatorValue();
      NotifyIndicatorListeners(indicator_value_);
    }

  } else if (!data_interrupted_) {
    if (_security_id_ == tod_smv_->security_id()) {
      indicator_value_ += tod_coeff_ * (new_price_ - last_recorded_tod_price_);
      last_recorded_tod_price_ = new_price_;
    } else if (_security_id_ == tom_smv_->security_id()) {
      indicator_value_ += tom_coeff_ * (new_price_ - last_recorded_tom_price_);
      last_recorded_tom_price_ = new_price_;
    } else if (_security_id_ == fut_smv_->security_id()) {
      indicator_value_ += fut_coeff_ * (new_price_ - last_recorded_fut_price_);
      last_recorded_fut_price_ = new_price_;
    }
    NotifyIndicatorListeners(indicator_value_);
  }
}

void MoorePenroseFxSf::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  is_data_interrupted_vec_[_security_id_] = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void MoorePenroseFxSf::OnMarketDataResumed(const unsigned int _security_id_) {
  is_data_interrupted_vec_[_security_id_] = false;
  if (!HFSAT::VectorUtils::LinearSearchValue(is_data_interrupted_vec_, true)) {
    data_interrupted_ = false;
    ResetIndicatorValue();
    NotifyIndicatorListeners(indicator_value_);
  }
}

void MoorePenroseFxSf::Initialize(std::string moore_penrose_filename) {
  std::ifstream moore_penrose_weights_infile_;
  std::ostringstream filepath;
  filepath << "/spare/local/tradeinfo/MoorePenroseFxSf/" << moore_penrose_filename;
  moore_penrose_weights_infile_.open(filepath.str(), std::ifstream::in);
  double tod_weight_ = 0.0;
  double tom_weight_ = 0.0;
  double fut_weight_ = 0.0;
  if (moore_penrose_weights_infile_.is_open()) {
    const int kL1AvgBufferLen = 1024;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);
    moore_penrose_weights_infile_.getline(readline_buffer_, kL1AvgBufferLen);
    while (moore_penrose_weights_infile_.good()) {
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (std::string("SHORTCODES").compare(tokens_[0]) == 0) {
        if (tokens_.size() > 3) {
          std::string tod_shc_ = std::string(tokens_[1]);
          tod_smv_ = (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(std::string(tod_shc_)));
          std::string tom_shc_ = std::string(tokens_[2]);
          tom_smv_ = (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(std::string(tom_shc_)));
          std::string fut_shc_ = std::string(tokens_[3]);
          fut_smv_ = (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(std::string(fut_shc_)));
        }
      }
      if (std::string("TOD").compare(tokens_[0]) == 0) {
        tod_weight_ = atof(tokens_[1]);
      } else if (std::string("TOM").compare(tokens_[0]) == 0) {
        tom_weight_ = atof(tokens_[1]);
      } else if (std::string("FUT").compare(tokens_[0]) == 0) {
        fut_weight_ = atof(tokens_[1]);
      }

      bzero(readline_buffer_, kL1AvgBufferLen);
      moore_penrose_weights_infile_.getline(readline_buffer_, kL1AvgBufferLen);
    }
    moore_penrose_weights_infile_.close();
  } else {
    std::cerr << "can't open moore_penrose file : " << filepath.str() << "\n";
    exit(0);
  }

  double norm_factor_ = fut_weight_ * fut_weight_ + (dte_ - 1) * (dte_ - 1) * tod_weight_ * tod_weight_ +
                        dte_ * dte_ * tom_weight_ * tom_weight_;

  if (fut_smv_->shortcode().compare(dep_market_view_.shortcode()) == 0) {
    norm_factor_ = norm_factor_ / fut_weight_;
    tod_coeff_ = -fut_weight_ * (dte_ - 1) / norm_factor_;
    tom_coeff_ = fut_weight_ * dte_ / norm_factor_;
    fut_coeff_ = -fut_weight_ / norm_factor_;
  } else if (tod_smv_->shortcode().compare(dep_market_view_.shortcode()) == 0) {
    norm_factor_ = norm_factor_ / tod_weight_;
    tod_coeff_ = -(dte_ - 1) * (dte_ - 1) * tod_weight_ / norm_factor_;
    tom_coeff_ = dte_ * (dte_ - 1) * tod_weight_ / norm_factor_;
    fut_coeff_ = -(dte_ - 1) * fut_weight_ / norm_factor_;
  } else if (tom_smv_->shortcode().compare(dep_market_view_.shortcode()) == 0) {
    norm_factor_ = norm_factor_ / tom_weight_;
    tod_coeff_ = dte_ * (dte_ - 1) * tom_weight_ / norm_factor_;
    tom_coeff_ = -dte_ * dte_ * tom_weight_ / norm_factor_;
    fut_coeff_ = dte_ * tom_weight_ / norm_factor_;
  }
}

void MoorePenroseFxSf::ResetIndicatorValue() { indicator_value_ = 0; }
}
