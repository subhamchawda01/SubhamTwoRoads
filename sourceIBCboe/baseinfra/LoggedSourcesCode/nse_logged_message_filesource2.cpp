/**
   \file MDSMessagesCode/nse_logged_message_filesource.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <algorithm>
#include "baseinfra/LoggedSources/nse_logged_message_filesource2.hpp"
// A historical file for NSE option can contain many symbols - necessary support has been added for that.

namespace HFSAT {

NSELoggedMessageFileSource2::NSELoggedMessageFileSource2(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool use_todays_data_)
    : ExternalDataListener(),
      dbglogger_(t_dbglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      curr_date_(t_preevent_YYYYMMDD_),
      p_order_global_listener_nse_(NULL),
      p_order_level_listener_sim_(NULL),
      p_time_keeper_(NULL),
      bulk_file_reader_(),
      next_event_(),
      generic_msg_(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      skip_intermediate_message_(false),
      actual_data_skip_(false),
      price_multiplier(100.00),
      last_seen_delta_type_(HFSAT::kTradeTypeNoInfo),
      nse_daily_token_symbol_handler_(HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(t_preevent_YYYYMMDD_)),
      nse_refdata_loader_(HFSAT::Utils::NSERefDataLoader::GetUniqueInstance(t_preevent_YYYYMMDD_)){
  

  orderadd_msg_count = 0;
  ordermodify_msg_count = 0;
  ordercancel_msg_count = 0;
  trade_msg_count = 0;
  trade_execution_count = 0;
  total_events = 0;
  
  InitalizeRefData();
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;
  strcpy(datasource_symbol_, NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol_).c_str());

  //std::cout << "DATA SOURCE " << datasource_symbol_ << std::endl;
  std::string t_nse_filename_ =
      NSELoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
  //std::cout << "FILENAME: " << t_nse_filename_ << HFSAT::kTLocNSE << " " << trading_location_file_read_ << std::endl ;
  // should be irrelevant currently

  int added_delay = 0;
  // added delays is used by the robustness check script to variate line delay by a constant number
  added_delay = TradingLocationUtils::GetAddedDelay(trading_location_file_read_, r_trading_location_);
  delay_usecs_to_add_ = added_delay;

  //std::cout <<"ADDED DELAY : " << delay_usecs_to_add_ << std::endl ;

  if (trading_location_file_read_ != r_trading_location_) {
    std::cout << " Trying to Get deley between : " << trading_location_file_read_ << " " << r_trading_location_ << std::endl ;
    delay_usecs_to_add_ +=
        TradingLocationUtils::GetMSecsBetweenTradingLocations(trading_location_file_read_, r_trading_location_);
    std::cout << " DELAY : " << delay_usecs_to_add_ << std::endl ;
  }
  if (delay_usecs_to_add_ != 0) need_to_add_delay_usecs_ = true;

  if (TradingLocationUtils::GetUsecsForFasterDataForShortcodeAtLocation(
          sec_name_indexer_.GetShortcodeFromId(security_id_), r_trading_location_, t_preevent_YYYYMMDD_, false) !=
      0) {  // Experiment with the possibility of using faster data for this product at this location.
    need_to_add_delay_usecs_ = true;
    delay_usecs_to_add_ -= TradingLocationUtils::GetUsecsForFasterDataForShortcodeAtLocation(
        sec_name_indexer_.GetShortcodeFromId(security_id_), r_trading_location_, t_preevent_YYYYMMDD_, false);
  }

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(t_nse_filename_);
  } else {
    bulk_file_reader_.open(t_nse_filename_);
    std::cerr << "For NSE symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_nse_filename_ << std::endl;
  }
#ifdef IS_PROFILING_ENABLED
// HFSAT::CpucycleProfiler::SetUniqueInstance(10);
/*HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(1, "OnOrderAdd");
HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(2, "OnOrderModify");
HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(3, "OnOrderDelete");
HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(4, "OnTrade");
HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(5, "PredictionLogic");
HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(6, "MapOperations");
HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(7, "SMVLogic");
HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(8, "AllocLogic");
HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(9, "HashIndexCalculation");*/

#endif
}

void NSELoggedMessageFileSource2::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {

  if(false == actual_data_skip_){

    if (bulk_file_reader_.is_open()) {
      if (next_event_timestamp_.tv_sec == 0 && next_event_timestamp_.tv_usec == 0) {
        // read the next_event_
        // set next_event_timestamp_
        size_t available_len_;
        do {
          //++total_events;
          available_len_ = bulk_file_reader_.read(&generic_msg_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
          next_event_ = generic_msg_.generic_data_.nse_data_;

/*
          if(next_event_.msg_type != NSE_MDS::MsgType::kNSETradeExecutionRange && next_event_.msg_type != NSE_MDS::MsgType::kNSESpotIndexUpdate){

                switch (next_event_.activity_type) {

                    case 'N': {
                        if (token_to_history_tracker.find(next_event_.token) == token_to_history_tracker.end()) {
                          token_to_history_tracker[next_event_.token] = new HistoryTracker();
                        }
                        // Update History
                        std::map<uint64_t, Data>& old_data_info = token_to_history_tracker[next_event_.token]->order_id_data_;
                        Data data;
                        data.price = (next_event_.data).nse_order.order_price;
                        data.size = (next_event_.data).nse_order.order_qty;
                        old_data_info[(next_event_.data).nse_order.order_id] = data;
                    }

                    case 'M': {

                        if (token_to_history_tracker.end() == token_to_history_tracker.find(next_event_.token)) {
                          // std::cerr << "Couldn't Find OrderInfo to modify For token. Considering as new order " << (next_event_->token)
                          // << " @ : " << next_event_->source_time.tv_sec << std::endl;
                          token_to_history_tracker[next_event_.token] = new HistoryTracker();
                        }

                        std::map<uint64_t, Data>& old_data_info = token_to_history_tracker[next_event_.token]->order_id_data_;

                        if (old_data_info.find((next_event_.data).nse_order.order_id) == old_data_info.end()) {
                          //std::cout << "IGNORE FOR THIS CASE " << std::endl;
                        }else{
                          //std::cout << "IGNORE FOR THIS CASE " << std::endl;
                          old_data_info[(next_event_.data).nse_order.order_id].price =
                                (next_event_.data).nse_order.order_price / price_multiplier;
                          old_data_info[(next_event_.data).nse_order.order_id].size = (next_event_.data).nse_order.order_qty;
                        }
                    }

                    case 'X': {

                        if (token_to_history_tracker.find(next_event_.token) == token_to_history_tracker.end()) {
                          token_to_history_tracker[next_event_.token] = new HistoryTracker();
                        }
                        // Update History
                        std::map<uint64_t, Data>& old_data_info = token_to_history_tracker[next_event_.token]->order_id_data_;

                        if (old_data_info.find((next_event_.data).nse_order.order_id) == old_data_info.end()) {
                          //std::cout << "IGNORE FOR THIS CASE " << std::endl;
                        }
                        old_data_info.erase((next_event_.data).nse_order.order_id);
                    }

                }
          }
*/
        } while (available_len_ == sizeof(HFSAT::MDS_MSG::GenericMDSMessage) &&
                 strcmp(next_event_.getContract(curr_date_), datasource_symbol_) != 0);

        if (available_len_ <
            sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) { /* not enough data to fulfill this request to read a struct */
          next_event_timestamp_ =
              ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
          rw_hasevents_ = false;
        } else {
          next_event_timestamp_ = next_event_.source_time;
          if (need_to_add_delay_usecs_) {
            next_event_timestamp_.addusecs(delay_usecs_to_add_);
          }
        }
      }
    } else {
      rw_hasevents_ = false;
    }

  }else{

    if (bulk_file_reader_.is_open()) {
      if (next_event_timestamp_.tv_sec == 0 && next_event_timestamp_.tv_usec == 0) {
        // read the next_event_
        // set next_event_timestamp_
        size_t available_len_;
        do {
          available_len_ = bulk_file_reader_.read(&generic_msg_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
          next_event_ = generic_msg_.generic_data_.nse_data_;
          next_event_timestamp_ = next_event_.source_time;

          if(strcmp(next_event_.getContract(curr_date_), datasource_symbol_) != 0)continue;

        } while (available_len_ == sizeof(HFSAT::MDS_MSG::GenericMDSMessage) &&
                 next_event_timestamp_ < r_start_time_);

        if (available_len_ <
            sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) { /* not enough data to fulfill this request to read a struct */
          next_event_timestamp_ =
              ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
          rw_hasevents_ = false;
        } else {
          next_event_timestamp_ = next_event_.source_time;
          if (need_to_add_delay_usecs_) {
            next_event_timestamp_.addusecs(delay_usecs_to_add_);
          }
        }
      }
    } else {
      rw_hasevents_ = false;
    }

  }

  SetTimeToSkipUntilFirstEvent(r_start_time_);
}

void NSELoggedMessageFileSource2::ComputeEarliestDataTimestamp(bool& _hasevents_) {

  //std::cout << "Inside NSELoggedMessageFileSource2::ComputeEarliestDataTimestamp" << std::endl;
  if (bulk_file_reader_.is_open()) {
    // read the next_event_
    // set next_event_timestamp_
    size_t available_len_;
    do {
      available_len_ = bulk_file_reader_.read(&generic_msg_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
      next_event_ = generic_msg_.generic_data_.nse_data_;
    } while (available_len_ == sizeof(HFSAT::MDS_MSG::GenericMDSMessage) &&
             strcmp(next_event_.getContract(curr_date_), datasource_symbol_) != 0);

    if (available_len_ <
        sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) { /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      _hasevents_ = false;
    } else {
      next_event_timestamp_ = next_event_.source_time;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  } else {
    _hasevents_ = false;
  }
}

inline void NSELoggedMessageFileSource2::_ProcessThisMsg() {

  //++total_events;

  /*
  std::cout << orderadd_msg_count << " " << ordermodify_msg_count << " " << ordercancel_msg_count << " " << trade_msg_count << " "
            << trade_execution_count << " " << total_events << std::endl;
  */
   if( (next_event_.msg_type == NSE_MDS::MsgType::kNSEOrderDelta) || 
        (next_event_.msg_type == NSE_MDS::MsgType::kNSEOrderSpreadDelta) 
        ){
                
                switch (next_event_.activity_type) {

                    case 'N': {
                        
                        //std::cout << "NEW ORDER" << std::endl;
                        if (token_to_history_tracker.find(next_event_.token) == token_to_history_tracker.end()) {
                          token_to_history_tracker[next_event_.token] = new HistoryTracker();
                        }

                        // Update History
                        std::map<uint64_t, Data>& old_data_info = token_to_history_tracker[next_event_.token]->order_id_data_;
                        Data data;
                        data.price = (next_event_.data).nse_order.order_price;
                        data.size = (next_event_.data).nse_order.order_qty;
                        old_data_info[(next_event_.data).nse_order.order_id] = data;
                    }break;

                    case 'M': {

                        //std::cout << "MODIFY ORDER" << std::endl;
                        if (token_to_history_tracker.end() == token_to_history_tracker.find(next_event_.token)) {
                          /*
                           std::cerr << "Couldn't Find OrderInfo to modify For token. Considering as new order " << (next_event_->token)
                           << " @ : " << next_event_->source_time.tv_sec << std::endl;
                          */
                          token_to_history_tracker[next_event_.token] = new HistoryTracker();
                        }

                        std::map<uint64_t, Data>& old_data_info = token_to_history_tracker[next_event_.token]->order_id_data_;

                        if (old_data_info.find((next_event_.data).nse_order.order_id) == old_data_info.end()) {
                          //Change the price & the msg type to create new order
                          
                          /*
                          std::cout << "Couldn't Find OrderId For Modification. Considering as new order : "
                          << (next_event_.data).nse_order.order_id << " @ : " << next_event_.source_time.tv_sec
                          << std::endl;
                          */
                          //std::cout << "Prev order type " << next_event_.activity_type << std::endl;
                          next_event_.activity_type = 'N';
                          Data data;
                          data.price = (next_event_.data).nse_order.order_price / price_multiplier;
                          data.size = (next_event_.data).nse_order.order_qty;
                          old_data_info[(next_event_.data).nse_order.order_id] = data;
                          //std::cout << "New order type " << next_event_.activity_type << std::endl;
                        }else{
                          //std::cout << "IGNORE FOR THIS CASE ONLY UPDATE THE MAP VALUES" << std::endl;
      
                          old_data_info[(next_event_.data).nse_order.order_id].price =
                                (next_event_.data).nse_order.order_price / price_multiplier;
                          old_data_info[(next_event_.data).nse_order.order_id].size = (next_event_.data).nse_order.order_qty;
                        }
                    }break;

                    case 'X': {

                        if (token_to_history_tracker.end() == token_to_history_tracker.find(next_event_.token)) {
                            return;
                        }

                        //std::cout << "CANCEL ORDER" << std::endl;
                        std::map<uint64_t, Data>& old_data_info = token_to_history_tracker[next_event_.token]->order_id_data_;

                        if (old_data_info.find((next_event_.data).nse_order.order_id) == old_data_info.end()) {
                          
                          /*
                          std::cout << "Couldn't Find OrderId For Deletion : " << (next_event_.data).nse_order.order_id << " @ : " << next_event_.source_time.tv_sec
                          << std::endl;
                          */
                          return;
                        }
                        old_data_info.erase((next_event_.data).nse_order.order_id);
                    } break;

                    default: {
                      //std::cerr << "UNEXPECTED ACTIVITY TYPE RECEVIED IN DATA FOR ORDER ADD/MODIFY/CANCEL: " << next_event_.activity_type << std::endl;
                        return;  // ignore this packet
                    }
                }
          }else if (  (next_event_.msg_type != NSE_MDS::MsgType::kNSETrade) && 
                      (next_event_.msg_type != NSE_MDS::MsgType::kNSESpreadTrade) &&
                      (next_event_.msg_type != NSE_MDS::MsgType::kNSETradeExecutionRange) && 
                      (next_event_.msg_type != NSE_MDS::MsgType::kNSESpotIndexUpdate) &&
                      (next_event_.msg_type != NSE_MDS::MsgType::kNSEOpenInterestTick)){

                  //std::cerr << "UNEXPECTED ACTIVITY TYPE RECEVIED IN DATA FOR TRADE EXECUTION RANGE : " << next_event_.activity_type << std::endl;
                  return;  // ignore this packet   
          }

  NotifyExternalDataListenerListener(security_id_);
  p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);


  if (p_order_global_listener_nse_) {
    p_order_global_listener_nse_->OnPriceLevelUpdate(&next_event_, sizeof(next_event_), HFSAT::MDS_MSG::NSE);

    switch (next_event_.msg_type) {
      case NSE_MDS::MsgType::kNSEOrderDelta: {

        /*
         This field was never assigned while converting the data
         if (next_event_.data.nse_dotex_order_delta.spread_comb_type == '2' ||
            next_event_.data.nse_dotex_order_delta.spread_comb_type == '3')
          return;
        */

        TradeType_t t_buysell_ = next_event_.data.nse_order.buysell;

        switch (next_event_.activity_type) {
          case 'N': {
            ++orderadd_msg_count;
            if (p_order_level_listener_sim_) {
              p_order_level_listener_sim_->OnOrderAdd(security_id_, t_buysell_, (next_event_.data).nse_order.order_id,
                                                      (next_event_.data).nse_order.order_price / price_multiplier,
                                                      (next_event_.data).nse_order.order_qty, false);
            }
            p_order_global_listener_nse_->OnOrderAdd(security_id_, t_buysell_, (next_event_.data).nse_order.order_id,
                                                      (next_event_.data).nse_order.order_price / price_multiplier,
                                                      (next_event_.data).nse_order.order_qty, false);
            last_seen_delta_type_ = t_buysell_;
#ifdef IS_PROFILING_ENABLED

#endif
          } break;
          case 'M': {
            // max is used to take care of iceberg order induced issues
            ++ordermodify_msg_count;
            //std::cout << "ORDER MODIFY " << std::endl;
            if (p_order_level_listener_sim_) {
              p_order_level_listener_sim_->OnOrderModify(security_id_, t_buysell_, (next_event_.data).nse_order.order_id,
                                                      (next_event_.data).nse_order.order_price / price_multiplier,
                                                      (next_event_.data).nse_order.order_qty);
            }
            p_order_global_listener_nse_->OnOrderModify(security_id_, t_buysell_, (next_event_.data).nse_order.order_id,
                                                      (next_event_.data).nse_order.order_price / price_multiplier,
                                                      (next_event_.data).nse_order.order_qty);
          } break;
          case 'X': {
            // max is used to take care of iceberg order induced issues
            ++ordercancel_msg_count;
            if (p_order_level_listener_sim_) {
              p_order_level_listener_sim_->OnOrderDelete(security_id_, t_buysell_, (next_event_.data).nse_order.order_id,
                                                         (next_event_.data).nse_order.order_price / price_multiplier, true,
                                                         false);
            }

            p_order_global_listener_nse_->OnOrderDelete(security_id_, t_buysell_, (next_event_.data).nse_order.order_id,
                                                        (next_event_.data).nse_order.order_price / price_multiplier, true,
                                                        false);
            break;
          }
          default: {
            fprintf(stderr, "Weird message type in NSELoggedMessageFileSource2::ProcessAllEvents %d \n",
                    next_event_.activity_type);
            break;
          }
        }
        break;
      }
      case NSE_MDS::MsgType::kNSETrade: {
        ++trade_msg_count;
        if ((int64_t)next_event_.data.nse_trade.buy_order_id == -1 ||
            (int64_t)next_event_.data.nse_trade.sell_order_id== -1) {
          if (p_order_level_listener_sim_) {
            p_order_level_listener_sim_->OnHiddenTrade(security_id_, next_event_.data.nse_trade.trade_price / price_multiplier,
                                                       next_event_.data.nse_trade.trade_qty,
                                                       next_event_.data.nse_trade.buy_order_id,
                                                       next_event_.data.nse_trade.sell_order_id);
          }

          p_order_global_listener_nse_->OnHiddenTrade(security_id_, next_event_.data.nse_trade.trade_price / price_multiplier,
                                                       next_event_.data.nse_trade.trade_qty,
                                                       next_event_.data.nse_trade.buy_order_id,
                                                       next_event_.data.nse_trade.sell_order_id);

        } else {
          if (p_order_level_listener_sim_) {
            p_order_level_listener_sim_->OnTrade(security_id_, next_event_.data.nse_trade.trade_price / price_multiplier,
                                                       next_event_.data.nse_trade.trade_qty,
                                                       next_event_.data.nse_trade.buy_order_id,
                                                       next_event_.data.nse_trade.sell_order_id);
          }
          p_order_global_listener_nse_->OnTrade(security_id_, next_event_.data.nse_trade.trade_price / price_multiplier,
                                                       next_event_.data.nse_trade.trade_qty,
                                                       next_event_.data.nse_trade.buy_order_id,
                                                       next_event_.data.nse_trade.sell_order_id);
        }

      } break;
      case NSE_MDS::MsgType::kNSETradeExecutionRange: {
        ++trade_execution_count;
        p_order_global_listener_nse_->OnTradeExecRange(
            security_id_,
            next_event_.data.nse_trade_range.low_exec_band / price_multiplier,
            next_event_.data.nse_trade_range.high_exec_band / price_multiplier);
      } break;
      case NSE_MDS::MsgType::kNSESpotIndexUpdate: {
	    //std::cout << "NSE_MDS::MsgType::kNSESpotIndexUpdate: " << (double)next_event_.data.nse_spot_index.IndexValue << std::endl;
/*   
        if (p_order_level_listener_sim_) {
	  p_order_level_listener_sim_->OnIndexPrice(security_id_, (double)next_event_.data.nse_spot_index.IndexValue * 0.01);
        }
*/
	p_order_global_listener_nse_->OnIndexPrice(security_id_, (double)next_event_.data.nse_spot_index.IndexValue * 0.01);
      } break;
      case NSE_MDS::MsgType::kNSEOpenInterestTick :{
        //std::cout << "No Support For Open Interest IN Sim" << std::endl;
        //std::cout << "HURRAY RECEIVING UPDATES NOW\n";

        int32_t token = next_event_.data.nse_oi_data.Token;
        //std::cout << "Token " << token << std::endl;
        int32_t mod_security_id = segment_to_token_secid_map_[NSE_FO_SEGMENT_MARKING][token];
        if (mod_security_id < 0) return;

        /*
        else if(mod_security_id>0){
          std::cout << "SECID IS GREATER THAN ZERO FOR TOKEN " <<  token << " " << mod_security_id << std::endl;
        }
        */
        //std::cout << "UPDATED SECID " << mod_security_id  << " OI " << (double)next_event_.data.nse_oi_data.OpenInterest * 0.01 << std::endl; 
        
        p_order_global_listener_nse_->OnOpenInterestUpdate(mod_security_id, (double)next_event_.data.nse_oi_data.OpenInterest * 0.01);
      } break;
      default: { break; }
    }
  }
}

void NSELoggedMessageFileSource2::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    // read the next_event_
    // set next_event_timestamp_
    size_t available_len_;
    do {
      available_len_ = bulk_file_reader_.read(&generic_msg_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
      next_event_ = generic_msg_.generic_data_.nse_data_;
    } while (available_len_ == sizeof(HFSAT::MDS_MSG::GenericMDSMessage) &&
             strcmp(next_event_.getContract(curr_date_), datasource_symbol_) != 0);

    if (available_len_ <
        sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) { /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      break;
    } else {
      next_event_timestamp_ = next_event_.source_time;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  }
}

void NSELoggedMessageFileSource2::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  //std::cout << "NSELoggedMessageFileSource2::ProcessEventsTill" << std::endl;
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    // read the next_event_
    // set next_event_timestamp_
    size_t available_len_;
    do {

      available_len_ = bulk_file_reader_.read(&generic_msg_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
      next_event_ = generic_msg_.generic_data_.nse_data_;
      // std::cout << "Curr date " << curr_date_ << std::endl;
      // std::cout << "Token " << next_event_.token << " Segment " << next_event_.segment_type << std::endl;
      // std::cout << next_event_.getContract(curr_date_) << " " <<  datasource_symbol_ << std::endl;
    } while (available_len_ == sizeof(HFSAT::MDS_MSG::GenericMDSMessage) &&
             strcmp(next_event_.getContract(curr_date_), datasource_symbol_) != 0);

    if (available_len_ <
        sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) { /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      break;
    } else {
      next_event_timestamp_ = next_event_.source_time;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  }
}
}
