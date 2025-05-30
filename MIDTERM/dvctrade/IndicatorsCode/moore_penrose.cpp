/**
   \file IndicatorsCode/diff_price_type.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/moore_penrose.hpp"

std::map<std::string, std::vector<std::string>> HFSAT::MoorePenrose::spreads_short_code_map;
namespace HFSAT {

void MoorePenrose::CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                     std::vector<std::string> &_ors_source_needed_vec_,
                                     const std::vector<const char *> &r_tokens_) {
  std::ifstream moore_penrose_file;
  std::ostringstream filepath;
  filepath << "/spare/local/tradeinfo/MoorePenrose/" << (std::string)r_tokens_[3];
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
      VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)tokens_[0]);
      bzero(readline_buffer_, kL1AvgBufferLen);
      moore_penrose_file.getline(readline_buffer_, kL1AvgBufferLen);
    }
    moore_penrose_file.close();

  } else {
    std::cerr << "can't ope file : " << filepath.str() << "\n";
    exit(0);
  }
}

MoorePenrose *MoorePenrose::GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                              const std::vector<const char *> &r_tokens_, PriceType_t _basepx_pxtype_) {
  ShortcodeSecurityMarketViewMap::StaticCheckValid(std::string(r_tokens_[4]));
  PriceType_t t_price_type_ = kPriceTypeMax;
  if (r_tokens_.size() < 5) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_, "Less args");
  } else if (r_tokens_.size() < 6) {
    t_price_type_ = _basepx_pxtype_;
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
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(std::string(r_tokens_[4]))),
                           std::string(r_tokens_[3]), t_price_type_);
}

MoorePenrose *MoorePenrose::GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                              SecurityMarketView &t_indep_market_view_,
                                              std::string moore_penrose_filename, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_indep_market_view_.secname() << ' ' << ' ' << PriceType_t_To_String(_price_type_)
              << " " << moore_penrose_filename;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, MoorePenrose *> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new MoorePenrose(t_dbglogger_, r_watch_, concise_indicator_description_, t_indep_market_view_,
                         moore_penrose_filename, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

MoorePenrose::MoorePenrose(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                           const std::string &concise_indicator_description_, SecurityMarketView &t_indep_market_view_,
                           std::string moore_penrose_filename, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(t_indep_market_view_),
      price_type_(_price_type_) {
  Get_spread_shortcode_mapping();
  secid_weight_map = new double[indep_market_view_.sec_name_indexer_.NumSecurityId()];
  secid_last_recorded_prices = new double[indep_market_view_.sec_name_indexer_.NumSecurityId()];
  secid_smv_map = new SecurityMarketView *[indep_market_view_.sec_name_indexer_.NumSecurityId()];
  for (auto i = 0u; i < indep_market_view_.sec_name_indexer_.NumSecurityId(); i++) {
    secid_smv_map[i] = NULL;
  }
  Initialize(moore_penrose_filename);
  if (!indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }

  is_data_interrupted_vec_.assign(indep_market_view_.sec_name_indexer_.NumSecurityId(), false);
}

void MoorePenrose::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                                const MarketUpdateInfo &_market_update_info_) {
  OnMarketUpdate(_security_id_, _market_update_info_);
}

void MoorePenrose::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_) {
  double newprice = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  if (!is_ready_) {
    is_ready_ = true;
    for (auto i = 0u; i < indep_market_view_.sec_name_indexer_.NumSecurityId(); i++) {
      if (secid_smv_map[i] != NULL) is_ready_ = is_ready_ && secid_smv_map[i]->is_ready();
    }

    secid_last_recorded_prices[_security_id_] = newprice;
    if (is_ready_) {
      ResetIndicatorValue();
      NotifyIndicatorListeners(indicator_value_);
    }
  } else if (!data_interrupted_) {
    indicator_value_ += secid_weight_map[_security_id_] * (newprice - secid_last_recorded_prices[_security_id_]);
    secid_last_recorded_prices[_security_id_] = newprice;
    NotifyIndicatorListeners(indicator_value_);
  } else {
    secid_last_recorded_prices[_security_id_] = newprice;
  }
}

void MoorePenrose::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (secid_smv_map[_security_id_] != NULL) {
    data_interrupted_ = true;
    is_data_interrupted_vec_[_security_id_] = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void MoorePenrose::OnMarketDataResumed(const unsigned int _security_id_) {
  if (secid_smv_map[_security_id_] != NULL) {
    is_data_interrupted_vec_[_security_id_] = false;
    if (!HFSAT::VectorUtils::LinearSearchValue(is_data_interrupted_vec_, true)) {
      data_interrupted_ = false;
      ResetIndicatorValue();
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void MoorePenrose::Initialize(std::string moore_penrose_filename) {
  std::ifstream moore_penrose_coeff;
  std::ostringstream filepath;
  filepath << "/spare/local/tradeinfo/MoorePenrose/" << moore_penrose_filename;
  moore_penrose_coeff.open(filepath.str(), std::ifstream::in);
  if (moore_penrose_coeff.is_open()) {
    const int kL1AvgBufferLen = 1024;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);
    moore_penrose_coeff.getline(readline_buffer_, kL1AvgBufferLen);
    std::vector<std::string> spread_tokens_;

    if (moore_penrose_coeff.good()) {
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      // const std::vector <const char *> & tokens_ = st_.GetTokens();
      for (auto i = 0u; i < st_.GetTokens().size(); i++) {
        std::string token = st_.GetTokens()[i];
        std::string temp = "";
        // std::size_t length = (token).copy(& temp[0], token.size());
        spread_tokens_.push_back(token);
      }
      bzero(readline_buffer_, kL1AvgBufferLen);
      moore_penrose_coeff.getline(readline_buffer_, kL1AvgBufferLen);
    }
    while (moore_penrose_coeff.good()) {
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (indep_market_view_.shortcode().compare(tokens_[0]) == 0) {
        for (unsigned int i = 1; i < tokens_.size(); i++) {
          std::string spread_shrtcode = spread_tokens_[i];
          std::vector<std::string> related_shrtcodes = spreads_short_code_map[spread_shrtcode];
          for (unsigned int j = 0; j < related_shrtcodes.size(); j++) {
            SecurityMarketView *smv = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(related_shrtcodes[j]);
            if (smv != NULL) {
              unsigned int sec_id = smv->security_id();
              if (secid_smv_map[sec_id] == NULL) {
                if (!smv->subscribe_price_type(this, price_type_)) {
                  PriceType_t t_error_price_type_ = price_type_;
                  std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                            << t_error_price_type_ << std::endl;
                }
                secid_smv_map[sec_id] = smv;
                secid_weight_map[sec_id] = 0.0;
              }
              secid_weight_map[sec_id] += std::pow(-1, (j % 2)) * atof(tokens_[i]);
              secid_last_recorded_prices[sec_id] = 0;
              //			    std::cout << secid_weight_map[sec_id] << " " << spread_shrtcode << " " <<
              // related_shrtcodes[j] << " \n";
            }
          }
        }
      }
      moore_penrose_coeff.getline(readline_buffer_, kL1AvgBufferLen);
    }

    moore_penrose_coeff.close();
  } else {
    std::cerr << "can't open moore_penrose file : " << filepath.str() << "\n";
    exit(0);
  }
}

std::vector<std::string> MoorePenrose::Get_shrtcodes_from_spread(std::string spread_shrtcode) {
  if (spread_shrtcode.compare("SP_VX0_VX1") == 0) {
    return {"VX_0", "VX_1", "SP_VX0_VX1"};
  } else if (spread_shrtcode.compare("SP_VX0_VX2") == 0) {
    return {"VX_0", "VX_2", "SP_VX0_VX2"};
  } else if (spread_shrtcode.compare("SP_VX0_VX3") == 0) {
    return {"VX_0", "VX_3", "SP_VX0_VX3"};
  } else if (spread_shrtcode.compare("SP_VX1_VX2") == 0) {
    return {"VX_1", "VX_2", "SP_VX1_VX2"};
  } else if (spread_shrtcode.compare("SP_VX1_VX3") == 0) {
    return {"VX_1", "VX_3", "SP_VX1_VX3"};
  } else if (spread_shrtcode.compare("SP_VX2_VX3") == 0) {
    return {"VX_2", "VX_3", "SP_VX2_VX3"};
  } else {
    return {};
  }
}

void MoorePenrose::Get_spread_shortcode_mapping() {
  std::ifstream spread_short_code_file;
  std::ostringstream filepath;
  filepath << "/spare/local/tradeinfo/MoorePenrose/SpreadShortCodeMap";
  spread_short_code_file.open(filepath.str(), std::ifstream::in);
  if (spread_short_code_file.is_open()) {
    const int kL1AvgBufferLen = 1024;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);
    spread_short_code_file.getline(readline_buffer_, kL1AvgBufferLen);
    std::vector<std::string> spread_tokens_;

    while (spread_short_code_file.good()) {
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() == 4) {
        spreads_short_code_map[std::string(tokens_[0])] = {tokens_[1], tokens_[2], tokens_[3]};
      }
      bzero(readline_buffer_, kL1AvgBufferLen);
      spread_short_code_file.getline(readline_buffer_, kL1AvgBufferLen);
    }
  } else {
    std::cerr << "can't open spreadshortcodemap file for moorepenrose : " << filepath.str() << "\n";
  }
  spread_short_code_file.close();
}

void MoorePenrose::ResetIndicatorValue() {
  indicator_value_ = 0;
  for (auto i = 0u; i < indep_market_view_.sec_name_indexer_.NumSecurityId(); i++) {
    if (secid_smv_map[i] != NULL) {
      unsigned int t_secid_ = secid_smv_map[i]->security_id();
      indicator_value_ += secid_weight_map[t_secid_] * secid_last_recorded_prices[t_secid_];
    }
  }
}
}
