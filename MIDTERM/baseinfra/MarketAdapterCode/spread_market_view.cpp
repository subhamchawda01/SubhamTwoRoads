/*
 * spread_market_view.cpp
 *
 *  Created on: 05-Jun-2014
 *      Author: archit
 */

#include "baseinfra/MarketAdapter/spread_market_view.hpp"

namespace HFSAT {

SpreadMarketView::SpreadMarketView(DebugLogger &_dbglogger_, const Watch &_watch_, const std::string &_spread_shc_)
    : dbglogger_(_dbglogger_),
      watch_(_watch_),
      spread_shc_(_spread_shc_),
      shortcode_vec_(),
      uts_vec_(),
      smv_vec_(),
      secid_to_idx_map_(),
      ready_vec_(),
      is_ready_(false),
      bestbid_price_(0.0),
      bestask_price_(0.0),
      bestbid_size_(0),
      bestask_size_(0),
      aggbid_price_(0.0),
      aggask_price_(0.0),
      last_bestbid_size_idx_(-1),
      last_bestask_size_idx_(-1),
      last_bestbid_price_(),
      last_bestask_price_(),
      mid_price_(0.0),
      mkt_price_(0.0) {
  std::ifstream t_spread_shc_file_;
  std::string t_spread_shc_filename_ = std::string(SPREADINFO_DIR) + "shortcode.txt";
  t_spread_shc_file_.open(t_spread_shc_filename_.c_str(), std::ifstream::in);
  if (t_spread_shc_file_.is_open()) {
    const int kReadLineBufferLen = 1024;
    char readline_buffer_[kReadLineBufferLen];
    bzero(readline_buffer_, kReadLineBufferLen);

    while (t_spread_shc_file_.good()) {
      bzero(readline_buffer_, kReadLineBufferLen);
      t_spread_shc_file_.getline(readline_buffer_, kReadLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kReadLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() >= 5 && strcmp(tokens_[0], _spread_shc_.c_str()) == 0) {
        //_spread_shc_ _shc1_ _size1_ _shc2_ _size2_ ...
        for (unsigned int i = 1; i <= (tokens_.size() - 1) / 2; i++) {
          std::string t_shc_ = tokens_[2 * i - 1];
          if (HFSAT::VectorUtils::UniqueVectorAdd(shortcode_vec_, t_shc_)) {
            DBGLOG_TIME_CLASS_FUNC_LINE << t_shc_ << DBGLOG_ENDL_FLUSH;
            uts_vec_.push_back(atoi(tokens_[2 * i]));

            secid_to_idx_map_[HFSAT::SecurityNameIndexer::GetUniqueInstance().GetIdFromString(t_shc_)] =
                smv_vec_.size();

            smv_vec_.push_back(
                HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance().GetSecurityMarketView(t_shc_));
            smv_vec_[smv_vec_.size() - 1]->subscribe_price_type(this, kPriceTypeMktSizeWPrice);
            ready_vec_.push_back(false);
            last_bestbid_price_.push_back(0.0);
            last_bestask_price_.push_back(0.0);
          }
        }
      }
    }
    t_spread_shc_file_.close();
    norm_factor_ = SumSize();
  } else {
    std::cerr << "can't open file spread_shortcode_file: " << t_spread_shc_filename_ << "\n";
    exit(1);
  }
}

void SpreadMarketView::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_) {
  // TODO:: Need to take care of cases where best level don't have enough sizes
  // we can also switch to demand driven variables, instead of updating on every update
  int t_update_idx_ = secid_to_idx_map_[_security_id_];

  if (!is_ready_) {
    ready_vec_[t_update_idx_] = true;
    is_ready_ = HFSAT::VectorUtils::CheckAllForValue(ready_vec_, true);
    if (is_ready_) {
      InitializeBestVars();
      NotifyListeners();
    }
    return;
  }

  bestbid_price_ +=
      uts_vec_[t_update_idx_] * (uts_vec_[t_update_idx_] > 0
                                     ? _market_update_info_.bestbid_price_ - last_bestbid_price_[t_update_idx_]
                                     : _market_update_info_.bestask_price_ - last_bestask_price_[t_update_idx_]);
  bestask_price_ +=
      uts_vec_[t_update_idx_] * (uts_vec_[t_update_idx_] > 0
                                     ? _market_update_info_.bestask_price_ - last_bestask_price_[t_update_idx_]
                                     : _market_update_info_.bestbid_price_ - last_bestbid_price_[t_update_idx_]);

  aggbid_price_ += uts_vec_[t_update_idx_] *
                   (_market_update_info_.bestbid_price_ -
                    last_bestbid_price_[t_update_idx_]);  // buy belly(uts>0) at best and sell rest aggressively
  aggask_price_ += uts_vec_[t_update_idx_] *
                   (_market_update_info_.bestask_price_ -
                    last_bestask_price_[t_update_idx_]);  // sell belly(uts>0) at best and buy rest aggressively

  if (t_update_idx_ != last_bestbid_size_idx_ && last_bestbid_size_idx_ >= 0) {
    double t_size_ =
        (uts_vec_[t_update_idx_] > 0 ? _market_update_info_.bestbid_size_ : _market_update_info_.bestask_size_) /
        double(abs(uts_vec_[t_update_idx_]));
    if (t_size_ < bestbid_size_) {
      bestbid_size_ = t_size_;
      last_bestbid_size_idx_ = t_update_idx_;
    }
  } else {
    bestbid_size_ =
        (uts_vec_[0] > 0 ? smv_vec_[0]->bestbid_size() : smv_vec_[0]->bestask_size()) / double(abs(uts_vec_[0]));
    last_bestbid_size_idx_ = 0;
    for (unsigned int i = 1; i < smv_vec_.size(); i++) {
      double t_size_ =
          (uts_vec_[i] > 0 ? smv_vec_[i]->bestbid_size() : smv_vec_[i]->bestask_size()) / double(abs(uts_vec_[i]));
      if (t_size_ < bestbid_size_) {
        bestbid_size_ = t_size_;
        last_bestbid_size_idx_ = i;
      }
    }
  }

  if (t_update_idx_ != last_bestask_size_idx_ && last_bestask_size_idx_ >= 0) {
    double t_size_ =
        (uts_vec_[t_update_idx_] > 0 ? _market_update_info_.bestask_size_ : _market_update_info_.bestbid_size_) /
        double(abs(uts_vec_[t_update_idx_]));
    if (t_size_ < bestask_size_) {
      bestask_size_ = t_size_;
      last_bestask_size_idx_ = t_update_idx_;
    }
  } else {
    bestask_size_ =
        (uts_vec_[0] > 0 ? smv_vec_[0]->bestask_size() : smv_vec_[0]->bestbid_size()) / double(abs(uts_vec_[0]));
    last_bestask_size_idx_ = 0;
    for (unsigned int i = 1; i < smv_vec_.size(); i++) {
      double t_size_ =
          (uts_vec_[i] > 0 ? smv_vec_[i]->bestask_size() : smv_vec_[i]->bestbid_size()) / double(abs(uts_vec_[i]));
      if (t_size_ < bestask_size_) {
        bestask_size_ = t_size_;
        last_bestask_size_idx_ = i;
      }
    }
  }

  last_bestbid_price_[t_update_idx_] = _market_update_info_.bestbid_price_;
  last_bestask_price_[t_update_idx_] = _market_update_info_.bestask_price_;

  ComputePrices();
  NotifyListeners();
  // ShowBook();
}

void SpreadMarketView::InitializeBestVars() {
  bestbid_price_ = 0.0;
  bestask_price_ = 0.0;
  aggbid_price_ = 0.0;
  aggask_price_ = 0.0;
  for (auto i = 0u; i < smv_vec_.size(); i++) {
    bestbid_price_ += uts_vec_[i] * (uts_vec_[i] > 0 ? smv_vec_[i]->bestbid_price() : smv_vec_[i]->bestask_price());
    bestask_price_ += uts_vec_[i] * (uts_vec_[i] > 0 ? smv_vec_[i]->bestask_price() : smv_vec_[i]->bestbid_price());
    aggbid_price_ += uts_vec_[i] * smv_vec_[i]->bestbid_price();  // buy belly(uts>0) at best and sell rest aggressively
    aggask_price_ += uts_vec_[i] * smv_vec_[i]->bestask_price();  // sell belly(uts>0) at best and buy rest aggressively
    last_bestbid_price_[i] = smv_vec_[i]->bestbid_price();
    last_bestask_price_[i] = smv_vec_[i]->bestask_price();
  }

  bestbid_size_ =
      (uts_vec_[0] > 0 ? smv_vec_[0]->bestbid_size() : smv_vec_[0]->bestask_size()) / double(abs(uts_vec_[0]));
  last_bestbid_size_idx_ = 0;
  for (unsigned int i = 1; i < smv_vec_.size(); i++) {
    double t_size_ =
        (uts_vec_[i] > 0 ? smv_vec_[i]->bestbid_size() : smv_vec_[i]->bestask_size()) / double(abs(uts_vec_[i]));
    if (t_size_ < bestbid_size_) {
      bestbid_size_ = t_size_;
      last_bestbid_size_idx_ = i;
    }
  }

  bestask_size_ =
      (uts_vec_[0] > 0 ? smv_vec_[0]->bestask_size() : smv_vec_[0]->bestbid_size()) / double(abs(uts_vec_[0]));
  last_bestask_size_idx_ = 0;
  for (unsigned int i = 1; i < smv_vec_.size(); i++) {
    double t_size_ =
        (uts_vec_[i] > 0 ? smv_vec_[i]->bestask_size() : smv_vec_[i]->bestbid_size()) / double(abs(uts_vec_[i]));
    if (t_size_ < bestask_size_) {
      bestask_size_ = t_size_;
      last_bestask_size_idx_ = i;
    }
  }
  ComputePrices();
  // ShowBook();
}

void SpreadMarketView::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                                    const MarketUpdateInfo &_market_update_info_) {
  OnMarketUpdate(_security_id_, _market_update_info_);
}

void SpreadMarketView::ComputePrices() {
  mid_price_ = (bestbid_price_ + bestask_price_) / 2;
  if (bestbid_size_ + bestask_size_ > 0.0) {
    mkt_price_ = (bestbid_size_ * bestask_price_ + bestask_size_ * bestbid_price_) / (bestbid_size_ + bestask_size_);
  } else {
    mkt_price_ = mid_price_;
  }
}

int SpreadMarketView::SumSize() const {
  int t_size_ = 0;
  for (auto i = 0u; i < uts_vec_.size(); i++) {
    t_size_ += abs(uts_vec_[i]);
  }
  return t_size_;
}

bool SpreadMarketView::IsBellyShc(const std::string _shortcode_) const {
  for (auto i = 0u; i < shortcode_vec_.size(); i++) {
    if (shortcode_vec_[i] == _shortcode_) {
      return uts_vec_[i] > 0;
    }
  }
  return false;
}

double SpreadMarketView::price_from_type(const std::string pricetype_) const {
  if (pricetype_ == "Mkt") {
    return mkt_price_ / norm_factor_;
  } else if (pricetype_ == "Mid") {
    return mid_price_ / norm_factor_;
  } else if (pricetype_ == "AggLegBid") {
    return aggbid_price_ / norm_factor_;
  } else if (pricetype_ == "AggLegAsk") {
    return aggask_price_ / norm_factor_;
  } else {
    return mkt_price_ / norm_factor_;
  }
}

double SpreadMarketView::price_from_type(const PriceType_t t_price_type_) const {
  switch (t_price_type_) {
    case (kPriceTypeMktSizeWPrice):
      return mkt_price_ / norm_factor_;
      break;
    case (kPriceTypeMidprice):
      return mid_price_ / norm_factor_;
      break;
    case (kPriceTypeSpreadAggLegBidPrice):
      return aggbid_price_ / norm_factor_;
      break;
    case (kPriceTypeSpreadAggLegAskPrice):
      return aggask_price_ / norm_factor_;
      break;
    default:
      return mkt_price_ / norm_factor_;
  }
}

bool SpreadMarketView::SubscribeSpreadMarketView(SpreadMarketViewListener *_listener_) const {
  if (_listener_) {
    return HFSAT::VectorUtils::UniqueVectorAdd(listeners_, _listener_);
  } else {
    return false;
  }
}

void SpreadMarketView::ShowBook() const {
  DBGLOG_TIME_CLASS_FUNC_LINE << "SpreadMArketView " << shortcode() << " "
                              << "BS: " << bestbid_size_ << " "
                              << "BP: " << bestbid_price_ << " "
                              << "AP: " << bestask_price_ << " "
                              << "AS: " << bestask_size_ << " "
                              << "ALBP: " << aggbid_price_ << " "
                              << "ALAP: " << aggask_price_ << " "
                              << "Mkt: " << mkt_price_ << " "
                              << "Mid: " << mid_price_ << " "
                              << "NF: " << norm_factor_ << " " << DBGLOG_ENDL_FLUSH;
}

void SpreadMarketView::PrintBook() const {
  std::cout << watch_.tv().ToString() << " SpreadMArketView " << shortcode() << " "
            << "BS: " << bestbid_size_ << " "
            << "BP: " << bestbid_price_ << " "
            << "AP: " << bestask_price_ << " "
            << "AS: " << bestask_size_ << " "
            << "ALBP: " << aggbid_price_ << " "
            << "ALAP: " << aggask_price_ << " "
            << "Mkt: " << mkt_price_ << " "
            << "Mid: " << mid_price_ << " "
            << "NF: " << norm_factor_ << " " << std::endl;
}

void SpreadMarketView::NotifyListeners() {
  for (auto i = 0u; i < listeners_.size(); i++) {
    listeners_[i]->OnSpreadMarketViewUpdate(*this);
  }
}
} /* namespace HFSAT */
