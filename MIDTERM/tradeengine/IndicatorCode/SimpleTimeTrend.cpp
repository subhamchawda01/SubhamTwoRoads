#include "tradeengine/Indicator/SimpleTimeTrend.hpp"

SimpleTimeTrend::SimpleTimeTrend(HFSAT::SecurityMarketView* smv, HFSAT::Watch& _watch_, HFSAT::DebugLogger& _dbglogger_,
                                 double weight_, double skew_factor, int64_t look_back_time,
                                 int64_t stable_px_look_back_time, int min_num_events)
    : BasePrice(smv, _watch_, _dbglogger_, weight_),
      skew_factor_(skew_factor),
      look_back_time_(look_back_time),
      stable_px_look_back_time_(stable_px_look_back_time),
      min_num_events_(min_num_events),
      current_time_usecs_(0),
      sum_px_(0),
      sum_size_(0),
      stable_sum_px_(0),
      stable_sum_size_(0),
      trend_avg_px_(0) {}

SimpleTimeTrend::~SimpleTimeTrend() {}

void SimpleTimeTrend::OnMarketUpdate(const unsigned int _security_id_,
                                     const HFSAT::MarketUpdateInfo& _market_update_info_) {
  current_time_usecs_ = watch_.usecs_from_midnight();
  base_bid_price_ = _market_update_info_.bestbid_price_;
  base_ask_price_ = _market_update_info_.bestask_price_;
  if (base_bid_price_ == kInvalidPrice || base_ask_price_ == kInvalidPrice) {
    return;
  }
  pruneTradeVector();
  trend_avg_px_ = (sum_size_ > 0) ? sum_px_ / sum_size_ : 0;

  double mid_px = (_market_update_info_.bestbid_price_ + _market_update_info_.bestask_price_) * 0.5;
  stable_px_info_.push_back(TradeTimeInfo(mid_px, 1, current_time_usecs_));
  stable_sum_px_ += mid_px;
  stable_sum_size_ += 1;
  double stable_avg_px = stable_sum_px_ / stable_sum_size_;

  /*dbglogger_ << watch_.tv() << " ST OnMarketUpdate sumpx: " << sum_px_
          << " sumsz: " << sum_size_ << " trend_avg_px: " << trend_avg_px_
          << " stable_sum_px: " << stable_sum_px_
          << " stable_sum_sz: " << stable_sum_size_
          << " stable_avg_px: " << stable_avg_px << DBGLOG_ENDL_FLUSH;*/

  double trend_skew = 0;
  if (trend_avg_px_ && trade_info_.size() >= min_num_events_) {
    trend_skew = (trend_avg_px_ - stable_avg_px) * skew_factor_;
  }

  double mkt_px = (_market_update_info_.bestbid_price_ * _market_update_info_.bestask_size_ +
                   _market_update_info_.bestask_price_ * _market_update_info_.bestbid_size_) /
                  (_market_update_info_.bestbid_size_ + _market_update_info_.bestask_size_);

  /*dbglogger_ << " ST skew: " << trend_skew << " mkt_px: " << mkt_px
          << " trend_diff: " << trend_avg_px_ - stable_avg_px << DBGLOG_ENDL_FLUSH;*/

  base_bid_price_ = mkt_px + trend_skew;
  base_ask_price_ = base_bid_price_;
}

void SimpleTimeTrend::OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                                   const HFSAT::MarketUpdateInfo& _market_update_info_) {
  current_time_usecs_ = watch_.usecs_from_midnight();
  base_bid_price_ = _market_update_info_.bestbid_price_;
  base_ask_price_ = _market_update_info_.bestask_price_;
  if (base_bid_price_ == kInvalidPrice || base_ask_price_ == kInvalidPrice) {
    return;
  }
  trade_info_.push_back(
      TradeTimeInfo(_trade_print_info_.trade_price_, _trade_print_info_.size_traded_, current_time_usecs_));
  sum_px_ += _trade_print_info_.trade_price_ * _trade_print_info_.size_traded_;
  sum_size_ += _trade_print_info_.size_traded_;

  pruneTradeVector();

  trend_avg_px_ = (sum_size_ > 0) ? sum_px_ / sum_size_ : 0;

  double mid_px = (_market_update_info_.bestbid_price_ + _market_update_info_.bestask_price_) * 0.5;
  stable_px_info_.push_back(TradeTimeInfo(mid_px, 1, current_time_usecs_));
  stable_sum_px_ += mid_px;
  stable_sum_size_ += 1;
  double stable_avg_px = stable_sum_px_ / stable_sum_size_;

  /*dbglogger_ << watch_.tv() << " Trade px: " << _trade_print_info_.trade_price_ << " sz: "
          << _trade_print_info_.size_traded_
          << " sumpx: " << sum_px_
          << " sumsz: " << sum_size_
          << " trend_avg_px: " << trend_avg_px_
          << " stable_sum_px: " << stable_sum_px_
          << " stable_sum_sz: " << stable_sum_size_
          << " stable_avg_px: " << stable_avg_px << DBGLOG_ENDL_FLUSH;*/

  double trend_skew = 0;
  if (trend_avg_px_ && trade_info_.size() >= min_num_events_) {
    trend_skew = (trend_avg_px_ - stable_avg_px) * skew_factor_;
  }
  double mkt_px = (_market_update_info_.bestbid_price_ * _market_update_info_.bestask_size_ +
                   _market_update_info_.bestask_price_ * _market_update_info_.bestbid_size_) /
                  (_market_update_info_.bestbid_size_ + _market_update_info_.bestask_size_);

  /*dbglogger_ << " ST skew: " << trend_skew << " mkt_px: " << mkt_px
          << " trend_diff: " << trend_avg_px_ - stable_avg_px << DBGLOG_ENDL_FLUSH;*/

  base_bid_price_ = mkt_px + trend_skew;
  base_ask_price_ = base_bid_price_;
}

bool SimpleTimeTrend::pruneTradeVector() {
  while (trade_info_.size() > 0 && current_time_usecs_ - trade_info_.front().time_ > look_back_time_) {
    sum_px_ -= trade_info_.front().price_ * trade_info_.front().trade_size_;
    sum_size_ -= trade_info_.front().trade_size_;
    /*dbglogger_ << watch_.tv() << " Prune px: " << trade_info_.front().price_
                    << " sz: " << trade_info_.front().trade_size_ << DBGLOG_ENDL_FLUSH;*/
    trade_info_.pop_front();
  }

  while (stable_px_info_.size() > 0 &&
         current_time_usecs_ - stable_px_info_.front().time_ > stable_px_look_back_time_) {
    stable_sum_px_ -= stable_px_info_.front().price_ * stable_px_info_.front().trade_size_;
    stable_sum_size_ -= stable_px_info_.front().trade_size_;
    /*dbglogger_ << watch_.tv() << " Prune px: " << stable_px_info_.front().price_
                    << " sz: " << stable_px_info_.front().trade_size_
                    << DBGLOG_ENDL_FLUSH;*/
    stable_px_info_.pop_front();
  }
  return true;
}

/*bool SimpleTimeTrend::CalculateVWAPPrice() {
        double total_bid_size = 0;
        double total_bid_value = 0;
        double total_ask_size = 0;
        double total_ask_value = 0;

        for(int level_count_=0; level_count_<vwap_levels_; level_count_++) {
                HFSAT::MarketUpdateInfoLevelStruct* bid_info_ = smv_->bid_info(level_count_);
                HFSAT::MarketUpdateInfoLevelStruct* ask_info_ = smv_->ask_info(level_count_);

                if((ask_info_ == NULL) || (bid_info_ == NULL)) {
                        // Hit bottom of book
                        base_bid_price_ = smv_->market_update_info().bestbid_price_;
                        base_ask_price_ = smv_->market_update_info().bestask_price_;
                        return false;
                }

                double _bid_price_ = bid_info_->limit_price_;
                double _ask_price_ = ask_info_->limit_price_;

                int bid_size_ = bid_info_->limit_size_;
                int ask_size_ = ask_info_->limit_size_;

                total_bid_size += bid_size_;
                total_bid_value += (bid_size_*_bid_price_);
                total_ask_size += ask_size_;
                total_ask_value += (ask_size_*_ask_price_);
        }

        double avg_bid_px = total_bid_value/total_bid_size;
        double avg_ask_px = total_ask_value/total_ask_size;
        base_bid_price_ = (avg_bid_px*total_ask_size + avg_ask_px*total_bid_size)/(total_bid_size + total_ask_size);
        base_ask_price_ = base_bid_price_;
        return true;
}
*/
