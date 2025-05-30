// =====================================================================================
// 
//       Filename:  MarketAnalyzer.hpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  09/05/2019 08:25:54 AM
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


#pragma once

namespace tradeengine { namespace indicator {

  struct DataMatrix{

    uint64_t total_size_;
    double min_px_;
    double max_px_;
    double avg_px_;
    double avg_px2_;
    double avg_px_trades_;
    double volume_weighted_avg_px_;
    double volume_weighted_avg_px2_;
    double min_trade_sz_;
    double max_trade_sz_;
    double avg_trade_sz_;
    double max_spread_;
    double min_spread_;
    double avg_spread_;
    double min_px_increment_;
    double price_stdev_;
    int total_volume_;
    int total_trades_;
    int total_buy_volume_;
    int total_sell_volume_;
    int total_count_;

    void OnMarketUpdate(HFSAT::MarketUpdateInfo const & market_update_info){
      if (this_smv_.bestask_size() > 0 && this_smv_.bestbid_size() > 0) {
        total_size_ += (this_smv_.bestask_size() + this_smv_.bestbid_size()) / 2;

        avg_px_ = (avg_px_ * total_count_ + this_smv_.mid_price()) / (total_count_ + 1);
        avg_px2_ = (avg_px2_ * total_count_ + this_smv_.mid_price()*this_smv_.mid_price()) / (total_count_ + 1);

        total_count_++;

        if (this_smv_.mid_price() > max_px_) {
          max_px_ = this_smv_.mid_price();
        }

        if (this_smv_.mid_price() < min_px_) {
          min_px_ = this_smv_.mid_price();
        }

        double spread = this_smv_.bestask_price() - this_smv_.bestbid_price();

        if (spread > max_spread_) max_spread_ = spread;

        if (spread < min_spread_) min_spread_ = spread;

        avg_spread_ = (avg_spread_ * (total_count_ - 1) + spread) / total_count_;
        min_px_increment_ = this_smv_.min_price_increment();
      }
    }

    void OnTradePrint(HFSAT::TradePrintInfo const & trade_print_info, HFSAT::MarketUpdateInfo const & market_update_info){

      if (this_smv_.bestask_size() > 0 && this_smv_.bestbid_size() > 0) {
        avg_px_trades_ = (avg_px_trades_ * total_trades_ + _trade_print_info_.trade_price_) / (total_trades_ + 1);
        avg_trade_sz_ = (avg_trade_sz_ * total_trades_ + _trade_print_info_.size_traded_) / (total_trades_ + 1);
        total_trades_ += 1;
        if (_trade_print_info_.buysell_ == HFSAT::kTradeTypeBuy) {
          total_buy_volume_ += _trade_print_info_.size_traded_;
        } else if (_trade_print_info_.buysell_ == HFSAT::kTradeTypeSell) {
          total_sell_volume_ += _trade_print_info_.size_traded_;
        }

        if (_trade_print_info_.size_traded_ > max_trade_sz_) {
          max_trade_sz_ = _trade_print_info_.size_traded_;
        } else if (_trade_print_info_.size_traded_ < min_trade_sz_) {
          min_trade_sz_ = _trade_print_info_.size_traded_;
        }

        volume_weighted_avg_px_ = (volume_weighted_avg_px_ * total_volume_ +
                                   _trade_print_info_.trade_price_ * _trade_print_info_.size_traded_) /
                                  (total_volume_ + _trade_print_info_.size_traded_);
        volume_weighted_avg_px2_ =
            (volume_weighted_avg_px2_ * total_volume_ +
             (_trade_print_info_.trade_price_ * _trade_print_info_.trade_price_ * _trade_print_info_.size_traded_)) /
            (total_volume_ + _trade_print_info_.size_traded_);
        total_volume_ += _trade_print_info_.size_traded_;
      }

    }

  };

  class MarketAnalyzer{

    private :

      //maximum number of securities in a strat - only dependent 
      DataMatrix market_data_matrix_[512];
      DataMatrix our_data_matrix_[512];

      MarketAnalyzer(){}
      MarketAnalyzer(MarketAnalyzer const & disabled_copy_constructor) = delete;

    public :

      static MarketAnalyzer & GetUniqueInstance(){
        MarketAnalyzer unique_instance;
        return unique_instance;
      }

      void OnMarketUpdate(uint32_t const & sec_id, HFSAT::MarketUpdateInfo const & market_update_info){
        market_data_matrix_[sec_id].OnMarketUpdate(market_update_info);
      }

      void OnTradePrint(uint32_t const & sec_id, HFSAT::TradePrintInfo const & trade_print_info, HFSAT::MarketUpdateInfo const & market_update_info){
        market_data_matrix_[sec_id].OnTradePrint(trade_print_info,market_update_info);
      }

  };

}}
