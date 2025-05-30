#include "tradeengine/Indicator/BookDelta.hpp"

BookDelta::BookDelta(HFSAT::SecurityMarketView* smv, HFSAT::Watch& _watch_, HFSAT::DebugLogger& _dbglogger_,
                     double weight_, int num_levels, double decay_factor, double skew_factor, double average_spread,
                     double level_decay_factor)
    : BasePrice(smv, _watch_, _dbglogger_, weight_),
      num_levels_(num_levels),
      decay_factor_(decay_factor),
      skew_factor_(skew_factor),
      average_spread_(average_spread / 100),
      level_decay_factor_(level_decay_factor),
      bid_val_(0),
      ask_val_(0) {
  bid_pxsz_vec_.resize(num_levels_ + 1);
  ask_pxsz_vec_.resize(num_levels_ + 1);
  temp_store_.resize(num_levels_ + 1);
}

BookDelta::~BookDelta() {}

void BookDelta::OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {
  CalculateBookDelta();
}

void BookDelta::OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                             const HFSAT::MarketUpdateInfo& _market_update_info_) {
  CalculateBookDelta();
}

bool BookDelta::CalculateBookDelta() {
  base_bid_price_ = smv_->market_update_info().bestbid_price_;
  base_ask_price_ = smv_->market_update_info().bestask_price_;
  if (CalculateBidDelta() && CalculateAskDelta() && base_bid_price_ != kInvalidPrice &&
      base_ask_price_ != kInvalidPrice) {
    double skew = (bid_val_ - ask_val_) * average_spread_ * skew_factor_ * smv_->market_update_info().bestbid_price_;
    double mkt_px = (smv_->market_update_info().bestbid_price_ * smv_->market_update_info().bestask_size_ +
                     smv_->market_update_info().bestask_price_ * smv_->market_update_info().bestbid_size_) /
                    (smv_->market_update_info().bestbid_size_ + smv_->market_update_info().bestask_size_);
    // double mid_px = (smv_->market_update_info().bestbid_price_ + smv_->market_update_info().bestask_price_)*0.5;
    // if ((mkt_px < mid_px && skew < 0) || (mkt_px > mid_px && skew > 0)) {
    //	skew = 0;
    //}
    base_bid_price_ = mkt_px + skew;
    base_ask_price_ = base_bid_price_;
    /*dbglogger_ << watch_.tv() << " CalcBookDelta bidval:" << bid_val_
            << " askval:" << ask_val_ << " diff:" << bid_val_ - ask_val_
            << " skew " << skew << " mkt_px " << mkt_px
            << " basepx " << base_bid_price_ << DBGLOG_ENDL_FLUSH;*/
  } else {
    return false;
  }
  return true;
}

bool BookDelta::CalculateBidDelta() {
  int master_bid_level_count = 1;
  int prev_vec_level = 0;
  double num_bid_val = 0;
  double denom_bid_val = 0;
  double level_decay = 1;
  for (int level_count_ = 0; level_count_ < num_levels_; level_count_++) {
    HFSAT::MarketUpdateInfoLevelStruct* bid_info_ = smv_->bid_info(level_count_);

    if (bid_info_ == NULL) {
      // Hit bottom of book
      temp_store_[level_count_].price_ = 0;
      temp_store_[level_count_].price_int_ = 0;
      bid_pxsz_vec_ = temp_store_;
      return false;
    }

    double bid_price_ = bid_info_->limit_price_;
    int bid_int_price_ = bid_info_->limit_int_price_;
    int bid_size_ = bid_info_->limit_size_;

    while (master_bid_level_count <= num_levels_ && prev_vec_level < num_levels_ &&
           bid_pxsz_vec_[prev_vec_level].price_int_ != 0 && bid_int_price_ < bid_pxsz_vec_[prev_vec_level].price_int_) {
      num_bid_val += -1 * bid_pxsz_vec_[prev_vec_level].size_ / level_decay;
      denom_bid_val += bid_pxsz_vec_[prev_vec_level].size_ / level_decay;
      level_decay *= level_decay_factor_;
      master_bid_level_count++;
      prev_vec_level++;
    }
    if (master_bid_level_count <= num_levels_) {
      if (prev_vec_level < num_levels_ && bid_pxsz_vec_[prev_vec_level].price_int_ != 0 &&
          bid_int_price_ == bid_pxsz_vec_[prev_vec_level].price_int_) {
        num_bid_val += (bid_size_ - bid_pxsz_vec_[prev_vec_level].size_) / level_decay;
        denom_bid_val += (bid_size_ + bid_pxsz_vec_[prev_vec_level].size_) / (level_decay);
        level_decay *= level_decay_factor_;
        master_bid_level_count++;
        prev_vec_level++;
      } else if (bid_int_price_ > bid_pxsz_vec_[prev_vec_level].price_int_ || prev_vec_level >= num_levels_ ||
                 bid_pxsz_vec_[prev_vec_level].price_int_ == 0) {
        num_bid_val += bid_size_ / level_decay;
        denom_bid_val += bid_size_ / level_decay;
        level_decay *= level_decay_factor_;
        master_bid_level_count++;
      }
    }

    temp_store_[level_count_].price_ = bid_price_;
    temp_store_[level_count_].price_int_ = bid_int_price_;
    temp_store_[level_count_].size_ = bid_size_;
  }

  // TODO move to vector pointer
  bid_pxsz_vec_ = temp_store_;
  // bid_val_ = 0;
  if (master_bid_level_count > num_levels_) {
    // We are ready to output
    bid_val_ = (decay_factor_ * num_bid_val / denom_bid_val) + (1 - decay_factor_) * bid_val_;
    /*dbglogger_ << watch_.tv() << " numval " << num_bid_val
            << " denomval " << denom_bid_val << " currval "
            << num_bid_val/denom_bid_val << " bidval " << bid_val_ << DBGLOG_ENDL_FLUSH;*/
  } else {
    // error case; return best book price
    return false;
  }
  return true;
}

bool BookDelta::CalculateAskDelta() {
  int master_ask_level_count = 1;
  int prev_vec_level = 0;
  double num_ask_val = 0;
  double denom_ask_val = 0;
  double level_decay = 1;
  for (int level_count_ = 0; level_count_ < num_levels_; level_count_++) {
    HFSAT::MarketUpdateInfoLevelStruct* ask_info_ = smv_->ask_info(level_count_);

    if (ask_info_ == NULL) {
      // Hit bottom of book
      temp_store_[level_count_].price_ = 0;
      temp_store_[level_count_].price_int_ = 0;
      ask_pxsz_vec_ = temp_store_;
      return false;
    }

    double ask_price_ = ask_info_->limit_price_;
    int ask_int_price_ = ask_info_->limit_int_price_;
    int ask_size_ = ask_info_->limit_size_;

    while (master_ask_level_count <= num_levels_ && prev_vec_level < num_levels_ &&
           ask_pxsz_vec_[prev_vec_level].price_int_ != 0 && ask_int_price_ > ask_pxsz_vec_[prev_vec_level].price_int_) {
      num_ask_val += -1 * ask_pxsz_vec_[prev_vec_level].size_ / level_decay;
      denom_ask_val += ask_pxsz_vec_[prev_vec_level].size_ / level_decay;
      level_decay *= level_decay_factor_;
      master_ask_level_count++;
      prev_vec_level++;
    }
    if (master_ask_level_count <= num_levels_) {
      if (prev_vec_level < num_levels_ && ask_pxsz_vec_[prev_vec_level].price_int_ != 0 &&
          ask_int_price_ == ask_pxsz_vec_[prev_vec_level].price_int_) {
        num_ask_val += (ask_size_ - ask_pxsz_vec_[prev_vec_level].size_) / level_decay;
        denom_ask_val += (ask_size_ + ask_pxsz_vec_[prev_vec_level].size_) / (level_decay);
        level_decay *= level_decay_factor_;
        master_ask_level_count++;
        prev_vec_level++;
      } else if (ask_int_price_ < ask_pxsz_vec_[prev_vec_level].price_int_ || prev_vec_level >= num_levels_ ||
                 ask_pxsz_vec_[prev_vec_level].price_int_ == 0) {
        num_ask_val += ask_size_ / level_decay;
        denom_ask_val += ask_size_ / level_decay;
        level_decay *= level_decay_factor_;
        master_ask_level_count++;
      }
    }

    temp_store_[level_count_].price_ = ask_price_;
    temp_store_[level_count_].price_int_ = ask_int_price_;
    temp_store_[level_count_].size_ = ask_size_;
  }

  // TODO move to vector pointer
  ask_pxsz_vec_ = temp_store_;
  // ask_val_ = 0;
  if (master_ask_level_count > num_levels_) {
    // We are ready to output
    ask_val_ = (decay_factor_ * num_ask_val / denom_ask_val) + (1 - decay_factor_) * ask_val_;
    /*dbglogger_ << watch_.tv() << " numval " << num_ask_val
            << " denomval " << denom_ask_val << " currval "
            << num_ask_val/denom_ask_val << " askval " << ask_val_ << DBGLOG_ENDL_FLUSH;*/
  } else {
    // error case
    return false;
  }
  return true;
}
