/*
 * spread_utils.cpp
 *
 *  Created on: Mar 5, 2015
 *      Author: archit
 */

#include "baseinfra/TradeUtils/spread_utils.hpp"

namespace HFSAT {

bool SpreadUtils::GetLegTradesFromSpreadTrade(const char *_spd_secname_, double _spd_trd_px_,
                                              unsigned int _spd_trd_qty_, TradeType_t _spd_buysell_, int _YYYYMMDD_,
                                              DebugLogger &dbglogger_, std::vector<TradeInfoStruct> &retval_) {
  char t_spd_secname_copy_[32];  // assuming 32 chars is enough to accomodate spdname
  strcpy(t_spd_secname_copy_, _spd_secname_);
  std::vector<char *> tokens_;
  PerishableStringTokenizer::NonConstStringTokenizer(t_spd_secname_copy_, "-", tokens_);

  if (tokens_.size() == 2) {
    char *secname_long_maturity_leg_ = tokens_[1];
    char *secname_short_maturity_leg_ = tokens_[0];

    SecurityNameIndexer &sec_name_indexer_ = SecurityNameIndexer::GetUniqueInstance();

    int security_id_long_maturity_leg_ = sec_name_indexer_.GetIdFromSecname(secname_long_maturity_leg_);
    int security_id_short_maturity_leg_ = sec_name_indexer_.GetIdFromSecname(secname_short_maturity_leg_);

    if (security_id_long_maturity_leg_ >= 0 && security_id_short_maturity_leg_ >= 0) {
      std::string shortcode_long_maturity_leg_ = sec_name_indexer_.GetShortcodeFromId(security_id_long_maturity_leg_);
      std::string shortcode_short_maturity_leg_ = sec_name_indexer_.GetShortcodeFromId(security_id_short_maturity_leg_);

      if (GetLegTradesFromSpreadTrade(
              ShortcodeSecurityMarketViewMap::GetUniqueInstance().GetSecurityMarketView(shortcode_long_maturity_leg_),
              ShortcodeSecurityMarketViewMap::GetUniqueInstance().GetSecurityMarketView(shortcode_short_maturity_leg_),
              _spd_trd_px_, _spd_trd_qty_, _spd_buysell_, _YYYYMMDD_, dbglogger_, retval_)) {
        retval_[0].security_id_ = security_id_long_maturity_leg_;
        retval_[1].security_id_ = security_id_short_maturity_leg_;
        strcpy(retval_[0].contract_, secname_long_maturity_leg_);
        strcpy(retval_[1].contract_, secname_short_maturity_leg_);
        return true;
      }
    }
  }
  return false;
}

// for efficiency - this function leaves security_id and contract fields blank because calling function already has smv
bool SpreadUtils::GetLegTradesFromSpreadTrade(const SecurityMarketView *_smv_long_maturity_leg_,
                                              const SecurityMarketView *_smv_short_maturity_leg_, double _spd_trd_px_,
                                              unsigned int _spd_trd_qty_, TradeType_t _spd_buysell_, int _YYYYMMDD_,
                                              DebugLogger &dbglogger_, std::vector<TradeInfoStruct> &retval_) {
  if (_smv_long_maturity_leg_ == NULL || _smv_short_maturity_leg_ == NULL) {
    return false;
  }

  TradeInfoStruct long_maturity_trade_strcut_;
  TradeInfoStruct short_maturity_trade_strcut_;

  double min_price_increment_to_use_ =
      std::min(_smv_long_maturity_leg_->min_price_increment(), _smv_short_maturity_leg_->min_price_increment());

  long_maturity_trade_strcut_.trd_qty_ = _spd_trd_qty_;
  double spread_size_ratio_ = HFSAT::CurveUtils::GetDV01SpreadSizeRatio(
      _smv_short_maturity_leg_->shortcode(), _smv_long_maturity_leg_->shortcode(), _YYYYMMDD_,
      _smv_short_maturity_leg_->mkt_size_weighted_price(), _smv_long_maturity_leg_->mkt_size_weighted_price());
  short_maturity_trade_strcut_.trd_qty_ =
      HFSAT::MathUtils::RoundOff(spread_size_ratio_ * _spd_trd_qty_, _smv_short_maturity_leg_->min_order_size());

  if (_spd_buysell_ == kTradeTypeBuy) {
    double bid_px_long_maturity_leg_ = _smv_long_maturity_leg_->bestbid_price();
    double ask_px_short_maturity_leg_ = _smv_short_maturity_leg_->bestask_price();
    double spread_bid_px_as_per_smv_ = bid_px_long_maturity_leg_ - ask_px_short_maturity_leg_;

    if (!(HFSAT::MathUtils::DblPxCompare(spread_bid_px_as_per_smv_, _spd_trd_px_, min_price_increment_to_use_ / 2.0))) {
      dbglogger_
          << "SpreadUtils::GetLegTradesFromSpreadTrade: Spread Mkt Price differ from trade price, spd_trade_px_: "
          << _spd_trd_px_ << " bid_px_long_maturity_leg_: " << bid_px_long_maturity_leg_
          << " ask_px_short_maturity_leg_: " << ask_px_short_maturity_leg_
          << " spread_bid_px_as_per_smv_: " << spread_bid_px_as_per_smv_ << DBGLOG_ENDL_FLUSH;

      ask_px_short_maturity_leg_ = bid_px_long_maturity_leg_ - _spd_trd_px_;

      dbglogger_ << " Resetting ask_px_short_maturity_leg_: " << ask_px_short_maturity_leg_ << DBGLOG_ENDL_FLUSH;
    }
    long_maturity_trade_strcut_.trd_px_ = bid_px_long_maturity_leg_;
    short_maturity_trade_strcut_.trd_px_ = ask_px_short_maturity_leg_;
    long_maturity_trade_strcut_.buysell_ = kTradeTypeBuy;
    short_maturity_trade_strcut_.buysell_ = kTradeTypeSell;
    retval_.push_back(long_maturity_trade_strcut_);
    retval_.push_back(short_maturity_trade_strcut_);
    return true;
  } else if (_spd_buysell_ == kTradeTypeSell) {
    double ask_px_long_maturity_leg_ = _smv_long_maturity_leg_->bestask_price();
    double bid_px_short_maturity_leg_ = _smv_short_maturity_leg_->bestbid_price();
    double spread_ask_px_as_per_smv_ = ask_px_long_maturity_leg_ - bid_px_short_maturity_leg_;

    if (!(HFSAT::MathUtils::DblPxCompare(spread_ask_px_as_per_smv_, _spd_trd_px_, min_price_increment_to_use_ / 2.0))) {
      dbglogger_
          << "SpreadUtils::GetLegTradesFromSpreadTrade: Spread Mkt Price differ from trade price, spd_trade_px_: "
          << _spd_trd_px_ << " ask_px_long_maturity_leg_: " << ask_px_long_maturity_leg_
          << " bid_px_short_maturity_leg_: " << bid_px_short_maturity_leg_
          << " spread_bid_px_as_per_smv_: " << spread_ask_px_as_per_smv_ << DBGLOG_ENDL_FLUSH;

      bid_px_short_maturity_leg_ = ask_px_long_maturity_leg_ - _spd_trd_px_;

      dbglogger_ << " Resetting bid_px_short_maturity_leg_: " << bid_px_short_maturity_leg_ << DBGLOG_ENDL_FLUSH;
    }
    long_maturity_trade_strcut_.trd_px_ = ask_px_long_maturity_leg_;
    short_maturity_trade_strcut_.trd_px_ = bid_px_short_maturity_leg_;
    long_maturity_trade_strcut_.buysell_ = kTradeTypeSell;
    short_maturity_trade_strcut_.buysell_ = kTradeTypeBuy;
    retval_.push_back(long_maturity_trade_strcut_);
    retval_.push_back(short_maturity_trade_strcut_);
    return true;
  }

  return false;
}

// for efficiency - this function leaves security_id and contract fields blank because calling function already has smv
bool SpreadUtils::GetLegTradesFromFlyTrade(const std::vector<const SecurityMarketView *> &_p_smv_vec_,
                                           const std::vector<double> &_size_factor_vec_,
                                           const std::vector<double> &_price_factor_vec_, double _fly_trd_px_,
                                           unsigned int _fly_trd_qty_, TradeType_t _fly_buysell_,
                                           DebugLogger &dbglogger_, std::vector<TradeInfoStruct> &retval_) {
  if (_p_smv_vec_.size() < 1 || _p_smv_vec_.size() != _size_factor_vec_.size() ||
      _p_smv_vec_.size() != _price_factor_vec_.size() ||
      HFSAT::VectorUtils::LinearSearchValue(_p_smv_vec_, (const SecurityMarketView *)(NULL))) {
    // can have sanity checks on _size_fact & _price_fact as well, ignoring for now
    return false;
  }

  double min_price_increment_to_use_ = _p_smv_vec_[0]->min_price_increment() * fabs(_price_factor_vec_[0]);
  for (unsigned int i = 1; i < _p_smv_vec_.size(); i++) {
    min_price_increment_to_use_ =
        std::min(min_price_increment_to_use_, _p_smv_vec_[i]->min_price_increment() * fabs(_price_factor_vec_[i]));
  }

  retval_.clear();
  double exp_fly_price_ = 0;
  for (auto i = 0u; i < _p_smv_vec_.size(); i++) {
    retval_.emplace_back(TradeInfoStruct());

    retval_[i].trd_qty_ =
        HFSAT::MathUtils::RoundOff(_size_factor_vec_[i] * _fly_trd_qty_, _p_smv_vec_[i]->min_order_size());
    retval_[i].buysell_ = _price_factor_vec_[i] > 0.0 ? _fly_buysell_ : TradeType_t(1 - _fly_buysell_);

    // assuming best price exec, logging when this assumption fails
    retval_[i].trd_px_ =
        retval_[i].buysell_ == kTradeTypeBuy ? _p_smv_vec_[i]->bestbid_price() : _p_smv_vec_[i]->bestask_price();
    exp_fly_price_ += retval_[i].trd_px_ * _price_factor_vec_[i];
  }

  if (!(HFSAT::MathUtils::DblPxCompare(exp_fly_price_, _fly_trd_px_, min_price_increment_to_use_ / 2.0))) {
    dbglogger_ << "SpreadUtils::GetLegTradesFromFlyTrade: exp_fly_price_( << " << exp_fly_price_
               << " ) != fly_trade_px_( " << _fly_trd_px_ << " ). Using Market Prices\n." << DBGLOG_ENDL_FLUSH;
  }

  return true;
}

void SpreadUtils::GetSpreadSecname(const std::vector<const SecurityMarketView *> &_p_smv_vec_,
                                   std::string &_spd_secname_) {
  if (_p_smv_vec_.empty()) {
    _spd_secname_ = "";
    return;
  }

  if (strncmp(_p_smv_vec_[0]->secname(), "DI1", 3) == 0) {
    // this naming is specific to DI1*, reverse order
    _spd_secname_ = std::string(_p_smv_vec_[_p_smv_vec_.size() - 1]->secname());
    for (int i = _p_smv_vec_.size() - 2; i >= 0; i--) {
      _spd_secname_ += "-";
      _spd_secname_ += std::string(_p_smv_vec_[i]->secname());
    }
  } else {
    _spd_secname_ = std::string(_p_smv_vec_[0]->secname());
    for (unsigned int i = 1; i < _p_smv_vec_.size(); i++) {
      _spd_secname_ += "-";
      _spd_secname_ += std::string(_p_smv_vec_[i]->secname());
    }
  }
}

} /* namespace HFSAT */
