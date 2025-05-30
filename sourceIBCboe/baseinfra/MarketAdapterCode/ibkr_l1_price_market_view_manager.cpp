/**
   \file MarketAdapterCode/eurex_price_level_market_view_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   Phone: +91 80 4190 3551
*/

#include "dvccode/CDef/security_definitions.hpp"
#include "baseinfra/MarketAdapter/ibkr_l1_price_market_view_manager.hpp"

namespace HFSAT {

IBKRL1PriceMarketViewManager::IBKRL1PriceMarketViewManager(DebugLogger& t_dbglogger, const Watch& t_watch,
                                                   const SecurityNameIndexer& t_sec_name_indexer,
                                                   const std::vector<SecurityMarketView*>& t_security_market_view_map)
    : BaseMarketViewManager(t_dbglogger, t_watch, t_sec_name_indexer, t_security_market_view_map) {}

void IBKRL1PriceMarketViewManager::Ib_Tick_Update(const int32_t t_security_id, const TradeType_t t_buysell,
                                          double price, const int32_t new_size, const bool is_intermediate_message) {
  if((strncmp(sec_name_indexer_.GetSecurityNameFromId(t_security_id), "CBOE_IDX", 8) == 0)){
    return;
  }
  SecurityMarketView& this_smv = *(security_market_view_map_[t_security_id]);
  if(new_size==0)price=kInvalidPrice;
  // DBGLOG_CLASS_FUNC_LINE_INFO << "Ib_Tick_Update Buy/Sell Side : " << t_buysell
  //                                      << DBGLOG_ENDL_NOFLUSH;
  //         DBGLOG_DUMP;
  // std::cout<<"Current Best Price and Sizes "<<this_smv.bestbid_size()<<" "<<this_smv.bestbid_price()<<" "<<this_smv.bestask_size()<<" "<<this_smv.bestask_price()<<"\n";
  // std::cout<<"Bid Levels Size:"<<this_smv.bidlevels_size()<<" Ask Levels Size:"<<this_smv.asklevels_size()<<"\n";
  // std::cout<<"Is Bid Book Empty? "<<this_smv.IsBidBookEmpty()<<", is ask book empty? "<<this_smv.IsAskBookEmpty()<<"\n";
  // std::cout<<"Buy/Sell?"<<t_buysell<<" Price:"<<price<<" Size:"<<new_size<<"\n";
  const int current_bid_price=this_smv.bestbid_int_price();
  const int current_ask_price=this_smv.bestask_int_price();
  switch (t_buysell) {
    case kTradeTypeBuy: {
      // std::cout<<"Level Info Struct:Int Price="<<this_smv.GetIntPx(price)<<"Price="<<price<<"\n";
      MarketUpdateInfoLevelStruct new_level_(0, this_smv.GetIntPx(price), price, new_size, 1, watch_.tv());
      if(new_size==0){
        new_level_.limit_int_price_=kInvalidIntPrice;
      }
      this_smv.ReplaceTopBid(new_level_);
      // std::cout<<"Current Best Price and Sizes after replacing top bid"<<this_smv.bid_price(0)<<" "<<this_smv.bid_size(0)<<" "<<this_smv.ask_price(0)<<" "<<this_smv.ask_size(0)<<"\n";
      // std::cout<<"Current Best Price and Sizes after replacing top bid :"<<this_smv.bid_level_info(0)->limit_price_<<" "<<this_smv.bid_level_info(0)->limit_size_<<"\n";
      // std::cout<<"Current Best Price and Sizes after replacing top bid :"<<this_smv.bid_price(0)<<" "<<this_smv.bid_int_price(0)<<" "<<this_smv.bid_size(0)<<"\n";
      this_smv.SetBestLevelBidVariablesOnQuote();

      if (!is_intermediate_message) {
        // Uncross

        this_smv.UpdateL1Prices();

        const int new_bid_price=this_smv.bestbid_int_price();
        const int new_ask_price=this_smv.bestask_int_price();
        if(new_bid_price==new_ask_price){
          MarketUpdateInfoLevelStruct new_level_(0, kInvalidIntPrice, kInvalidPrice, kInvalidSize, 0, watch_.tv());
          this_smv.ReplaceTopAsk(new_level_);
          this_smv.SetBestLevelAskVariablesOnQuote();
          if(this_smv.is_ready()){
            this_smv.NotifyL1PriceListeners();
          }
          this_smv.UpdateL1Prices();
          break;
        }
        // std::cout<<"Prices updated will notify\n";
        // Notify PL Indicatore listneres
        if (this_smv.pl_change_listeners_present_) {
          // DBGLOG_CLASS_FUNC_LINE_INFO << "Will notify pl change listeners"
          //                              << DBGLOG_ENDL_NOFLUSH;
          // DBGLOG_DUMP;
          this_smv.NotifyOnPLChangeListeners(t_security_id, this_smv.market_update_info_, kTradeTypeBuy, 0,
                                             new_level_.limit_int_price_, new_level_.limit_int_price_level_, 0,
                                             new_size, 0, 1, is_intermediate_message, 'N');
        }
        if (!this_smv.market_update_info_.bidlevels_.empty() && !this_smv.market_update_info_.asklevels_.empty()) {
          // DBGLOG_CLASS_FUNC_LINE_INFO << "Will notify l1 size listeners"
          //                              << DBGLOG_ENDL_NOFLUSH;
          // DBGLOG_DUMP;
          if(current_bid_price != this_smv.bestbid_int_price() || current_ask_price != this_smv.bestask_int_price())
            this_smv.NotifyL1PriceListeners();
          else this_smv.NotifyL1SizeListeners();
        }
        // this_smv.OnL1SizeUpdate ( );
        this_smv.l1_changed_since_last_ = false;
        // DBGLOG_CLASS_FUNC_LINE_INFO << "Will notify on ready listeners"
        //                                << DBGLOG_ENDL_NOFLUSH;
        //   DBGLOG_DUMP;
        this_smv.NotifyOnReadyListeners();
      } else {
        // Notify PL Indicatore listneres
        if (this_smv.pl_change_listeners_present_) {
          // DBGLOG_CLASS_FUNC_LINE_INFO << "Will notify on pl change listeners"
          //                              << DBGLOG_ENDL_NOFLUSH;
          // DBGLOG_DUMP;
          this_smv.NotifyOnPLChangeListeners(t_security_id, this_smv.market_update_info_, kTradeTypeBuy, 0,
                                             new_level_.limit_int_price_, new_level_.limit_int_price_level_, 0,
                                             new_size, 0, 1, is_intermediate_message, 'N');
        }
        this_smv.l1_changed_since_last_ = true;
      }

    } break;
    case kTradeTypeSell: {
      // since most new levels will be at top .. so special case for this ...
      // using O(1) PushFront not O(DEF_MARKET_DEPTH) PushAt
      // also needed to set bestlevel variables
      // std::cout<<"Level Info Struct:Int Price="<<this_smv.GetIntPx(price)<<"Price="<<price<<"\n";
      MarketUpdateInfoLevelStruct new_level_(0, this_smv.GetIntPx(price), price, new_size, 1, watch_.tv());
      // std::cout<<"Level Info Struct:Int Price="<<this_smv.GetIntPx(price)<<"Price="<<price<<"\n";
      if(new_size==0){
        new_level_.limit_int_price_=kInvalidIntPrice;
      }
      this_smv.ReplaceTopAsk(new_level_);
      // std::cout<<"Current Best Price and Sizes after replacing top ask"<<this_smv.bid_price(0)<<" "<<this_smv.bid_size(0)<<" "<<this_smv.ask_price(0)<<" "<<this_smv.ask_size(0)<<"\n";
      // // std::cout<<"Current Best Price and Sizes after replacing top ask"<<this_smv.ask_level_info(0)->limit_price_<<" "<<this_smv.ask_level_info(0)->limit_size_<<"\n";
      // std::cout<<"Current Best Price and Sizes after replacing top ask :"<<this_smv.ask_level_info(0)->limit_price_<<" "<<this_smv.ask_level_info(0)->limit_size_<<"\n";
      // std::cout<<"Current Best Price and Sizes after replacing top ask :"<<this_smv.ask_price(0)<<" "<<this_smv.ask_int_price(0)<<" "<<this_smv.ask_size(0)<<"\n";
      
      this_smv.SetBestLevelAskVariablesOnQuote();

      if (!is_intermediate_message) {
        // Uncross

        this_smv.UpdateL1Prices();

        const int new_bid_price=this_smv.bestbid_int_price();
        const int new_ask_price=this_smv.bestask_int_price();
        if(new_bid_price==new_ask_price){
          MarketUpdateInfoLevelStruct new_level_(0, kInvalidIntPrice, kInvalidPrice, kInvalidSize, 0, watch_.tv());
          this_smv.ReplaceTopBid(new_level_);
          this_smv.SetBestLevelBidVariablesOnQuote();
          if(this_smv.is_ready()){
            this_smv.NotifyL1PriceListeners();
          }
          this_smv.UpdateL1Prices();
          break;
        }
        if (this_smv.pl_change_listeners_present_) {
          // DBGLOG_CLASS_FUNC_LINE_INFO << "Will notify pl change listeners"
          //                              << DBGLOG_ENDL_NOFLUSH;
          // DBGLOG_DUMP;
          this_smv.NotifyOnPLChangeListeners(t_security_id, this_smv.market_update_info_, kTradeTypeSell, 0,
                                             new_level_.limit_int_price_, new_level_.limit_int_price_level_, 0,
                                             new_size, 0, 1, is_intermediate_message, 'N');
          // dbglogger_ << "NOTIF..L1SIZEUPDATE NEW SELL "<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
        }

        if (!this_smv.market_update_info_.bidlevels_.empty() && !this_smv.market_update_info_.asklevels_.empty()) {
          // DBGLOG_CLASS_FUNC_LINE_INFO << "Will notify l1 size listeners"
          //                              << DBGLOG_ENDL_NOFLUSH;
          // DBGLOG_DUMP;
          if(current_bid_price != this_smv.bestbid_int_price() || current_ask_price != this_smv.bestask_int_price())
            this_smv.NotifyL1PriceListeners();
          else this_smv.NotifyL1SizeListeners();
        }
        // this_smv.OnL1SizeUpdate( );
        this_smv.l1_changed_since_last_ = false;
        // DBGLOG_CLASS_FUNC_LINE_INFO << "Will notify on ready listeners"
        //                                << DBGLOG_ENDL_NOFLUSH;
        //   DBGLOG_DUMP;
        this_smv.NotifyOnReadyListeners();
      } else  // Non-intermediate msg
      {
        if (this_smv.pl_change_listeners_present_) {
          // DBGLOG_CLASS_FUNC_LINE_INFO << "Will notify pl change listeners"
          //                              << DBGLOG_ENDL_NOFLUSH;
          // DBGLOG_DUMP;
          this_smv.NotifyOnPLChangeListeners(t_security_id, this_smv.market_update_info_, kTradeTypeSell, 0,
                                             new_level_.limit_int_price_, new_level_.limit_int_price_level_, 0,
                                             new_size, 0, 1, is_intermediate_message, 'N');
        }
        this_smv.l1_changed_since_last_ = true;
      }
    } break;
    default: { break; }
  }
  // std::cout<<"After Update Current Best Price and Sizes "<<this_smv.bestbid_size()<<" "<<this_smv.bestbid_price()<<" "<<this_smv.bestask_size()<<" "<<this_smv.bestask_price()<<"\n";

}
void IBKRL1PriceMarketViewManager::Ib_Tick_Size_Only_Update(const int32_t t_security_id, const TradeType_t t_buysell,
                                          const int32_t new_size, const bool is_intermediate_message) {
  if((strncmp(sec_name_indexer_.GetSecurityNameFromId(t_security_id), "CBOE_IDX", 8) == 0)){
    return;
  }
  SecurityMarketView& this_smv = *(security_market_view_map_[t_security_id]);
  switch (t_buysell) {
    case kTradeTypeBuy: {
        const int current_size=this_smv.bestbid_size();
        if((current_size!=new_size)&&this_smv.bestbid_price()>0)  //If the size is already same no need to update/notify 
            IBKRL1PriceMarketViewManager::Ib_Tick_Update(t_security_id,t_buysell,this_smv.bestbid_price(),new_size,is_intermediate_message);
    } break;
    case kTradeTypeSell: {
        const int current_size=this_smv.bestask_size();
        if((current_size!=new_size)&&(this_smv.bestask_price()>0))  //If the size is already same no need to update/notify 
            IBKRL1PriceMarketViewManager::Ib_Tick_Update(t_security_id,t_buysell,this_smv.bestask_price(),new_size,is_intermediate_message);
    } break;
    default: { break; }
  }
}
inline void IBKRL1PriceMarketViewManager::Ib_Trade(const int32_t t_security_id_, const double trade_price_, const int32_t trade_size_,
                                            const TradeType_t t_buysell) {
//   DBGLOG_CLASS_FUNC_LINE_INFO << "Ib_Trade : " << trade_price_ << " , " << trade_size_ << "Side:" << t_buysell << DBGLOG_ENDL_NOFLUSH;
//           DBGLOG_DUMP;

  // DBGLOG_CLASS_FUNC_LINE_INFO << " IB_Trade : " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << DBGLOG_ENDL_NOFLUSH;
  if((strncmp(sec_name_indexer_.GetSecurityNameFromId(t_security_id_), "CBOE_IDX", 8) == 0)){
    IBKRL1PriceMarketViewManager::OnIndexPrice(t_security_id_,trade_price_);
    return;
  }
  security_market_view_map_[t_security_id_]->OnTradeIBKR(trade_price_, trade_size_, t_buysell);
}

void IBKRL1PriceMarketViewManager::OnIndexPrice(const unsigned int t_security_id_, const double t_spot_price_){
  //std::cout << "IndexedNSEMarketViewManager2::OnIndexPrice = " << t_spot_price_ << std::endl;
  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  // std::cout << "trd , spt :: " << smv_.trade_print_info_.trade_price_ << " , " << t_spot_price_ << std::endl;
  // std::cout <<"SC: " << smv_.secname() <<" ID: " << smv_.security_id() << " EXCH: " << smv_.exch_source() << std::endl;
    // std::cout << "IF LOOP " << std::endl;
  smv_.market_update_info_.mid_price_ = t_spot_price_;
  smv_.market_update_info_.bestask_price_ = t_spot_price_;
  smv_.market_update_info_.bestask_int_price_ = smv_.GetIntPx(smv_.market_update_info_.bestask_price_);
  smv_.market_update_info_.bestbid_price_ = t_spot_price_;
  smv_.market_update_info_.bestbid_int_price_ = smv_.GetIntPx(smv_.market_update_info_.bestbid_price_);
  smv_.trade_print_info_.trade_price_ = t_spot_price_;
  smv_.trade_print_info_.size_traded_ = 1;
  smv_.is_ready_ = true;
  smv_.NotifySpotIndexListeners();
}
}
