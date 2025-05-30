/**
   \file MDSMessagesCode/ibkr_logged_message_filesource.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <algorithm>
#include <cstring>
#include "baseinfra/LoggedSources/ibkr_l1_logged_message_filesource.hpp"

namespace HFSAT {

IBKRL1LoggedMessageFileSource::IBKRL1LoggedMessageFileSource(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool use_todays_data_)
    : ExternalDataListener(),
      dbglogger_(t_dbglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      listener_(nullptr),
      p_time_keeper_(NULL),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      skip_intermediate_message_(false),
      last_seen_delta_type_(HFSAT::kTradeTypeNoInfo),
      event_counter(0){
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  std::string t_ibkr_filename_ =
      IBKRLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);

// std::cout << " GOT FILE : " << t_ibkr_filename_ << std::endl;
  // should be irrelevant currently

  int added_delay = 0;
  // added delays is used by the robustness check script to variate line delay by a constant number
  added_delay = TradingLocationUtils::GetAddedDelay(trading_location_file_read_, r_trading_location_);
  delay_usecs_to_add_ = added_delay;

  if (trading_location_file_read_ != r_trading_location_) {
    delay_usecs_to_add_ +=
        TradingLocationUtils::GetMSecsBetweenTradingLocations(trading_location_file_read_, r_trading_location_) * 1000;
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
    bulk_file_reader_.open(t_ibkr_filename_);
  } else {
    bulk_file_reader_.open(t_ibkr_filename_);
//    std::cerr << "For IBKR symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
//              << trading_location_file_read_ << " returned filename = " << t_ibkr_filename_ << std::endl;
  }
}

void IBKRL1LoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {
    if (next_event_timestamp_.tv_sec == 0 && next_event_timestamp_.tv_usec == 0) {
      // read the next_event_
      // set next_event_timestamp_IBKR1
      size_t available_len_;
      do {
        available_len_ = bulk_file_reader_.read(&next_event_, sizeof(IBL1UpdateTick));
//std::cout << next_event_.ToString() << std::endl;
      } while (available_len_ == sizeof(IBL1UpdateTick) && strcmp(CBOESecurityDefinitions::ConvertDataSourceNametoExchSymbol(next_event_.symbol).c_str(), exchange_symbol_) != 0);

      if (available_len_ < sizeof(IBL1UpdateTick)) { /* not enough data to fulfill this request to read a struct */
        next_event_timestamp_ =
            ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
        rw_hasevents_ = false;
      } else {
        next_event_timestamp_ = next_event_.time;
        if (need_to_add_delay_usecs_) {
          next_event_timestamp_.addusecs(delay_usecs_to_add_);
        }
      }
    }
  } else {
    rw_hasevents_ = false;
  }

  SetTimeToSkipUntilFirstEvent(r_start_time_);
}

void IBKRL1LoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& _hasevents_) {


  if (bulk_file_reader_.is_open()) {
    DBGLOG_CLASS_FUNC_LINE_INFO << "Bulk file is open" <<DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;
    // read the next_event_
    // set next_event_timestamp_
    size_t available_len_;
    do {
      available_len_ = bulk_file_reader_.read(&next_event_, sizeof(IBL1UpdateTick));
      DBGLOG_CLASS_FUNC_LINE_INFO << available_len_ <<" "<< next_event_.symbol << " "<<CBOESecurityDefinitions::ConvertDataSourceNametoExchSymbol(next_event_.symbol)<<" "<<exchange_symbol_ <<DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
    } while (available_len_ == sizeof(IBL1UpdateTick) && strcmp(CBOESecurityDefinitions::ConvertDataSourceNametoExchSymbol(next_event_.symbol).c_str(), exchange_symbol_) != 0);
    DBGLOG_CLASS_FUNC_LINE_INFO << available_len_ <<DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
    if (available_len_ < sizeof(IBL1UpdateTick)) { /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      _hasevents_ = false;
    } else {
      next_event_timestamp_ = next_event_.time;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  } else {
    DBGLOG_CLASS_FUNC_LINE_INFO << "Bulk file is not open" <<DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;
    _hasevents_ = false;
  }

std::cout << "ComputeEarliestDataTimestamp " << _hasevents_ << std::endl;
}

void IBKRL1LoggedMessageFileSource::_ProcessThisMsg() {
//  std::cout<<"Will notify external data\n";
  NotifyExternalDataListenerListener(security_id_);
//  std::cout<<"Will update timer :"<< security_id_<<"\n";
  p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
//  std::cout<<"Will send to notify\n";
  // std::cout<<" SYMBOL: " << exchange_symbol_ << " " << next_event_.ToString()<<"\n";
      // DBGLOG_DUMP;

  if(listener_){
    // std::cout<<"Listener is not empty\n";
    // std::cout << "Address of listener_: " << listener_ << "\n";
    switch (next_event_.ib_update_type) {
      case IBUpdateType::IB_TICK_UPDATE: {
        // DBGLOG_CLASS_FUNC_LINE_INFO << "Side : " << next_event_.side
        //                                << DBGLOG_ENDL_NOFLUSH;
        //   DBGLOG_DUMP;
        // std::cout<<"IB_TICK_UPDATE\n";

        if ('A' == next_event_.side) {         // Ask update
          
        event_counter++;
          listener_->Ib_Tick_Update(
              security_id_, HFSAT::TradeType_t::kTradeTypeSell,
              next_event_.ask_price,
              next_event_.ask_size,false);
        } else if ('B' == next_event_.side) {  // Bid update
        event_counter++;
          // std::cout<<"Bid Side\n";
          // std::cout<<security_id_<<" "<<HFSAT::TradeType_t::kTradeTypeBuy<<" "<<next_event_.bid_price<<" "<<next_event_.bid_size<<"\n";
          listener_->Ib_Tick_Update(
              security_id_, HFSAT::TradeType_t::kTradeTypeBuy,
              next_event_.bid_price,
              next_event_.bid_size,false);
          // HFSAT::CpucycleProfiler::GetUniqueInstance().End(2);
        } else {
          DBGLOG_CLASS_FUNC_LINE_ERROR << "UNEXPECTED TICK UPDATE RECEVIED IN DATA : " << next_event_.side
                                      << DBGLOG_ENDL_NOFLUSH;
          DBGLOG_DUMP;
        }

      } break;

      case IBUpdateType::IB_TICK_SIZE_ONLY_UPDATE: {

        // DBGLOG_CLASS_FUNC_LINE_INFO << "Side : " << next_event_.side
        //                                << DBGLOG_ENDL_NOFLUSH;
        //   DBGLOG_DUMP;
        if ('S' == next_event_.side) {         // Ask size only update

        event_counter++;
          listener_->Ib_Tick_Size_Only_Update(
              security_id_, HFSAT::TradeType_t::kTradeTypeSell,
              next_event_.ask_size,false);
        } else if ('B' == next_event_.side) {  // Bid size only update

        event_counter++;
          listener_->Ib_Tick_Size_Only_Update(
              security_id_, HFSAT::TradeType_t::kTradeTypeBuy,
              next_event_.bid_size,false);
          // HFSAT::CpucycleProfiler::GetUniqueInstance().End(2);
        } else {
          DBGLOG_CLASS_FUNC_LINE_ERROR << "UNEXPECTED TICK SIZE ONLY UPDATE RECEVIED IN DATA : " << next_event_.side
                                      << DBGLOG_ENDL_NOFLUSH;
          DBGLOG_DUMP;
        }

      } break;

      case IBUpdateType::IB_TRADE: {
        // DBGLOG_CLASS_FUNC_LINE_INFO << "Ib_Update_Type :" << next_event_.ib_update_type
        //                                << DBGLOG_ENDL_NOFLUSH;
        //   DBGLOG_DUMP;

        /*int32_t bid_size_remaining = 0;
        int32_t ask_size_remaining = 0;*/

        // HFSAT::CpucycleProfiler::GetUniqueInstance().Start(4);
        listener_->Ib_Trade(
            security_id_,next_event_.trade_price,next_event_.trade_size,TradeType_t::kTradeTypeNoInfo);
        // HFSAT::CpucycleProfiler::GetUniqueInstance().End(4);
      } break;

      case IBUpdateType::IB_GREEKS_UPDATE: {
      } break;

      case IBUpdateType::IB_INFO_UPDATE: {
      } break;
      default: {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "UNEXPECTED TYPE OF MESSAGE RECEVIED : " << (int32_t)(next_event_.ib_update_type)
                                    << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
      } break;
    }
  }
  
  // if (listener_) {
  //   switch (next_event_.ib_update_type) {
  //     case IBUpdateType::IB_TICK_UPDATE:
  //       listener_->IB_TICK_UPDATE(security_id_, next_event_);
  //       break;

  //     case IBUpdateType::IB_TICK_SIZE_ONLY_UPDATE:
  //       listener_->IB_TICK_SIZE_ONLY_UPDATE(security_id_, next_event_);
  //       break;

  //     case IBUpdateType::IB_TRADE:
  //       listener_->IB_TICK_SIZE_ONLY_UPDATE(security_id_, next_event_);
  //       break;

  //     default:
  //       dbglogger_ << "IBKRL1LoggedMessageFileSource::ProcessThisMsg : Error. Invalid Msge Type\n";
  //       break;
  //   }
  // }
}

void IBKRL1LoggedMessageFileSource::ProcessAllEvents() {

// std::cout << " PROCESS ALL EVENTS  :" << std::endl;

  while (true) {
    _ProcessThisMsg();
    size_t available_len_;
    do {
      available_len_ = bulk_file_reader_.read(&next_event_, sizeof(IBL1UpdateTick));
    } while (available_len_ == sizeof(IBL1UpdateTick) && strcmp(CBOESecurityDefinitions::ConvertDataSourceNametoExchSymbol(next_event_.symbol).c_str(), exchange_symbol_) != 0);

    if (available_len_ < sizeof(IBL1UpdateTick)) { /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      break;
    } else {
      next_event_timestamp_ = next_event_.time;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  }
}

void IBKRL1LoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {

  // std::cout << "shortcode : "<< exchange_symbol_ << " " << _endtime_ << std::endl;

  // assumes next_event_timestamp_
  // assumes mdu_type_
  // std::cout<<"Start and end time"<<next_event_timestamp_<<" "<<_endtime_<<"\n";
  while (next_event_timestamp_ <= _endtime_) {
    // std::cout<<"Current and end time"<<next_event_timestamp_<<" "<<_endtime_<<"\n"; 
    _ProcessThisMsg();
    // std::cout<<"Message processing done\n";
    // read the next_event_
    // set next_event_timestamp_
    size_t available_len_;
    do {
      available_len_ = bulk_file_reader_.read(&next_event_, sizeof(IBL1UpdateTick));
      // std::cout << "ProcessEventsTill CONDITIon : " << available_len_ << " " << strcmp(CBOESecurityDefinitions::ConvertDataSourceNametoExchSymbol(next_event_.symbol).c_str(), exchange_symbol_) << std::endl;
    } while (available_len_ == sizeof(IBL1UpdateTick) && strcmp(CBOESecurityDefinitions::ConvertDataSourceNametoExchSymbol(next_event_.symbol).c_str(), exchange_symbol_) != 0);

    if (available_len_ < sizeof(IBL1UpdateTick)) { /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      break;
    } else {
      next_event_timestamp_ = next_event_.time;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
    // std::cout<<"Current and end time after one loop:"<<next_event_timestamp_<<" "<<_endtime_<<"\n"; 
  }

  // std::cout << "Total Events : " << event_counter << std::endl;
}

void IBKRL1LoggedMessageFileSource::SetTimeToSkipUntilFirstEvent(const ttime_t start_time) {
  return;
  if (listener_) {
    listener_->SetTimeToSkipUntilFirstEvent(start_time);
  }
}

void IBKRL1LoggedMessageFileSource::SetOrderGlobalListenerIBKR(OrderGlobalListenerIBKR* listener) { listener_ = listener; }
}
