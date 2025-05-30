/**
   \file MDSMessagesCode/bse_logged_message_filesource2.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <algorithm>
#include "baseinfra/LoggedSources/bse_logged_message_filesource2.hpp"
 // A historical file for BSE option can contain many symbols - necessary support has been added for that.

namespace HFSAT {

  BSELoggedMessageFileSource2::BSELoggedMessageFileSource2(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool use_todays_data_)
    : ExternalDataListener(),
    dbglogger_(t_dbglogger_),
    sec_name_indexer_(t_sec_name_indexer_),
    security_id_(t_security_id_),
    exchange_symbol_(t_exchange_symbol_),
    p_order_global_listener_bse_(NULL),
    p_order_level_listener_sim_(NULL),
    p_time_keeper_(NULL),
    bulk_file_reader_(),
    generic_msg_(),
    next_event_(),
    trading_location_file_read_(r_trading_location_),
    delay_usecs_to_add_(0),
    seq_num_(0),
    need_to_add_delay_usecs_(false),
    skip_intermediate_message_(false),
    last_seen_delta_type_(HFSAT::kTradeTypeNoInfo),
    bse_daily_token_symbol_handler_(HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(t_preevent_YYYYMMDD_)),
    bse_refdata_loader_(HFSAT::Utils::BSERefDataLoader::GetUniqueInstance(t_preevent_YYYYMMDD_)){ 


    InitalizeRefData();
    next_event_timestamp_.tv_sec = 0;
    next_event_timestamp_.tv_usec = 0;
    strcpy(datasource_symbol_, BSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol_).c_str());

    std::string t_bse_filename_ =
      BSELoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
      
    // std::cout << "--- SEQ NO --- " << seq_num_ << std::endl;
    // std::cout << "FILENAME: " << t_bse_filename_ << " " << HFSAT::kTLocBSE << " " << trading_location_file_read_ << " " << r_trading_location_ << std::endl ;

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
      bulk_file_reader_.open(t_bse_filename_);
    }
    else {
      bulk_file_reader_.open(t_bse_filename_);
      std::cerr << "For BSE symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
        << trading_location_file_read_ << " returned filename = " << t_bse_filename_ << std::endl;
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

  void BSELoggedMessageFileSource2::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
    if (bulk_file_reader_.is_open()) {
      if (next_event_timestamp_.tv_sec == 0 && next_event_timestamp_.tv_usec == 0) {
        // read the next_event_
        // set next_event_timestamp_
        size_t available_len_;
        do {
          available_len_ = bulk_file_reader_.read(&generic_msg_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
          next_event_ = generic_msg_.generic_data_.bse_data_;
        } while (available_len_ == sizeof(HFSAT::MDS_MSG::GenericMDSMessage) && false);

        if (available_len_ <
          sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) { /* not enough data to fulfill this request to read a struct */
          next_event_timestamp_ =
            ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
          rw_hasevents_ = false;
        }
        else {
          next_event_timestamp_ = next_event_.source_time;
          if (need_to_add_delay_usecs_) {
            next_event_timestamp_.addusecs(delay_usecs_to_add_);
          }
        }
      }
    }
    else {
      rw_hasevents_ = false;
    }
    SetTimeToSkipUntilFirstEvent(r_start_time_);
  }

  void BSELoggedMessageFileSource2::ComputeEarliestDataTimestamp(bool& _hasevents_) {

    //std::cout << "BSELoggedMessageFileSource2::ComputeEarliestDataTimestamp" << std::endl;
    if (bulk_file_reader_.is_open()) {
      // read the next_event_
      // set next_event_timestamp_
      size_t available_len_;
      do {
        //std::cout << "do available_len_: " << available_len_
        //          << " " << next_event_.getContract() << ", " << datasource_symbol_ << std::endl;
        available_len_ = bulk_file_reader_.read(&generic_msg_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
        next_event_ = generic_msg_.generic_data_.bse_data_;
        //std::cout << "after do available_len_: " << available_len_
        //          << " " << next_event_.getContract() << ", " << datasource_symbol_ << std::endl;

      } while (available_len_ == sizeof(HFSAT::MDS_MSG::GenericMDSMessage) &&
        strcmp(next_event_.getContract(), datasource_symbol_) != 0);

      //std::cout << "available_len_: " << available_len_ << std::endl;
      if (available_len_ <
        sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) { /* not enough data to fulfill this request to read a struct */
        next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
        _hasevents_ = false;
      }
      else {
        next_event_timestamp_ = next_event_.source_time;
        if (need_to_add_delay_usecs_) {
          next_event_timestamp_.addusecs(delay_usecs_to_add_);
        }
      }
    }
    else {

      //std::cout << "ELSE" << std::endl;
      _hasevents_ = false;
    }
  }

  inline void BSELoggedMessageFileSource2::_ProcessThisMsg() {
    //std::cout << "BSELoggedMessageFileSource2::_ProcessThisMsg" << std::endl;
    NotifyExternalDataListenerListener(security_id_);
    p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
    //std::cout << "BSE EVENT " << next_event_.ToString() << std::endl;

    if (p_order_global_listener_bse_) {
      if (next_event_.order_.msg_seq_num_ <= seq_num_) {
        dbglogger_ << "DUPLICATE PACKETS " << exchange_symbol_ << " Expected seq_num : " << seq_num_ + 1 << " Curr_seq_num : " << next_event_.order_.msg_seq_num_ << "\n";
        std::cout << "Skipping Duplicate packets: Expected seq_num : " << seq_num_ + 1
          << " Curr_seq_num : " << next_event_.order_.msg_seq_num_
          << std::endl;
        return;
      }
      seq_num_ = next_event_.order_.msg_seq_num_;
      //std::cout << "p_order_global_listener_bse_ present: "  << std::endl;
      //p_order_global_listener_bse_->OnPriceLevelUpdate(&next_event_, sizeof(next_event_), HFSAT::MDS_MSG::BSE);

  //


      switch (next_event_.order_.action_) {
      case '0': {
        // std::cout << "ORDER_ADD SEQ_NO: " << next_event_.order_.msg_seq_num_ << std::endl;

        // Order New
        // Update watch
        TradeType_t t_buysell_ = next_event_.order_.side == 'B' ? kTradeTypeBuy : kTradeTypeSell;
        p_time_keeper_->OnTimeReceived(next_event_.source_time);

        p_order_global_listener_bse_->OnOrderAdd(security_id_, t_buysell_, next_event_.order_.priority_ts,
          next_event_.order_.price, next_event_.order_.size,
          next_event_.order_.intermediate_);

      } break;
      case '1': {
//        std::cout << "ORDER_MODIFY: or ORDER_MODIFY_SAME_PRIORITY: SEQ_NO: " << next_event_.order_.msg_seq_num_ << std::endl;
        //ORDER_MODIFY:
        //ORDER_MODIFY_SAME_PRIORITY:

        TradeType_t t_buysell_ = next_event_.order_.side == 'B' ? kTradeTypeBuy : kTradeTypeSell;
        p_time_keeper_->OnTimeReceived(next_event_.source_time);
        /*
                p_order_global_listener_bse_->OnOrderModify(
                        security_id_, t_buysell_, next_event_.order_.price, next_event_.order_.size,
                        next_event_.order_.prev_price, next_event_.order_.prev_size);
        */
        p_order_global_listener_bse_->OnOrderModify(

          security_id_, t_buysell_, next_event_.order_.prev_priority_ts, next_event_.order_.priority_ts,
          next_event_.order_.price, next_event_.order_.size,
          next_event_.order_.prev_price, next_event_.order_.prev_size);


      } break;

      case '2': {

//        std::cout << "ORDER_DELETE: SEQ_NO: " << next_event_.order_.msg_seq_num_ << std::endl;
        //ORDER_DELETE:

        TradeType_t t_buysell_ = next_event_.order_.side == 'B' ? kTradeTypeBuy : kTradeTypeSell;
        p_time_keeper_->OnTimeReceived(next_event_.source_time);

        p_order_global_listener_bse_->OnOrderDelete(security_id_, t_buysell_, next_event_.order_.priority_ts,
          next_event_.order_.price, //(int)next_event_.order_.size, 
          true, false);


      } break;

      case '3': {

//        std::cout << "ORDER_MASS_DELETE: SEQ_NO: " << next_event_.order_.msg_seq_num_ << std::endl;
        //ORDER_MASS_DELETE:

              //TradeType_t t_buysell_ = next_event_.order_.side == 'B' ? kTradeTypeBuy : kTradeTypeSell;

        p_time_keeper_->OnTimeReceived(next_event_.source_time);
        p_order_global_listener_bse_->OnOrderMassDelete(security_id_);
      } break;

      case '4': {

//        std::cout << "PARTIAL_EXECUTION: SEQ_NO: " << next_event_.order_.msg_seq_num_ << std::endl;
        //PARTIAL_EXECUTION:
        TradeType_t t_buysell_ = next_event_.order_.side == 'B' ? kTradeTypeBuy : kTradeTypeSell;
        p_time_keeper_->OnTimeReceived(next_event_.source_time);
        /*
          p_order_global_listener_bse_->OnPartialOrderExecution(
                        security_id_, t_buysell_, next_event_.order_.price, next_event_.order_.size);
        */
        p_order_global_listener_bse_->OnTrade(security_id_, t_buysell_, next_event_.order_.priority_ts,
          next_event_.order_.price, (int32_t)next_event_.order_.size);


      } break;

      case '5': {

//        std::cout << "FULL_EXECUTION: SEQ_NO: " << next_event_.order_.msg_seq_num_ << std::endl;
        //FULL_EXECUTION:


        TradeType_t t_buysell_ = next_event_.order_.side == 'B' ? kTradeTypeBuy : kTradeTypeSell;
        p_time_keeper_->OnTimeReceived(next_event_.source_time);
        /*
          p_order_global_listener_bse_->OnFullOrderExecution(
                        security_id_, t_buysell_, next_event_.order_.price, next_event_.order_.size);
        */
        p_order_global_listener_bse_->OnTrade(security_id_, t_buysell_, next_event_.order_.priority_ts,
          next_event_.order_.price, (int32_t)next_event_.order_.size);
      } break;

      case '6': {

//        std::cout << "EXECUTION_SUMMARY SEQ_NO: " << next_event_.order_.msg_seq_num_ << std::endl;
        //EXECUTION_SUMMARY:

//        TradeType_t t_buysell_ = next_event_.order_.side == 'B' ? kTradeTypeBuy : kTradeTypeSell;
//        p_time_keeper_->OnTimeReceived(next_event_.source_time);
//        p_order_global_listener_bse_->OnExecutionSummary(security_id_, t_buysell_, next_event_.order_.price,
//          next_event_.order_.size - (int32_t)next_event_.order_.prev_price);


      } break;

      case '8': {
        p_order_global_listener_bse_->OnIndexPrice(security_id_,(double)next_event_.order_.price);
      }break; 
      case '9': {
          int32_t token = next_event_.token_;
          int32_t mod_security_id = segment_to_token_secid_map_[BSE_FO_SEGMENT_MARKING][token];
          if (mod_security_id < 0) return;
          p_order_global_listener_bse_->OnOpenInterestUpdate(mod_security_id,(double)next_event_.order_.size);
          }break;
      default: {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "UNEXPECTED TYPE OF MESSAGE RECEVIED : " << (int32_t)(next_event_.order_.action_)

          << DBGLOG_ENDL_NOFLUSH;

        DBGLOG_DUMP;
      } break;
      }
    }
    //

  }

  void BSELoggedMessageFileSource2::ProcessAllEvents() {
    //std::cout << "BSELoggedMessageFileSource2::ProcessAllEvents" << std::endl;

    while (true) {
      _ProcessThisMsg();


      // read the next_event_
      // set next_event_timestamp_
      size_t available_len_;
      do {
        available_len_ = bulk_file_reader_.read(&generic_msg_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
        next_event_ = generic_msg_.generic_data_.bse_data_;
      } while (available_len_ == sizeof(HFSAT::MDS_MSG::GenericMDSMessage) &&
        strcmp(next_event_.getContract(), datasource_symbol_) != 0);

      if (available_len_ <
        sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) { /* not enough data to fulfill this request to read a struct */
        next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
        break;
      }
      else {
        next_event_timestamp_ = next_event_.source_time;
        if (need_to_add_delay_usecs_) {
          next_event_timestamp_.addusecs(delay_usecs_to_add_);
        }
      }
    }
  }


  void BSELoggedMessageFileSource2::ProcessEventsTill(const ttime_t _endtime_) {
    // assumes next_event_timestamp_
    // assumes mdu_type_
//    std::cout << "next_event_timestamp_<= _endtime_ :: " << next_event_timestamp_ << " <= " << _endtime_ << std::endl;

    while (next_event_timestamp_ <= _endtime_) {
      _ProcessThisMsg();

      size_t available_len_;
      do {
        available_len_ = bulk_file_reader_.read(&generic_msg_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
        next_event_ = generic_msg_.generic_data_.bse_data_;
      } while (available_len_ == sizeof(HFSAT::MDS_MSG::GenericMDSMessage) && false);


      if (available_len_ <

        sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) { /* not enough data to fulfill this request to read a struct */

//        std::cout << "Less length: " << available_len_ << std::endl;
        next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
        break;
      }
      else {
        next_event_timestamp_ = next_event_.source_time;
        if (need_to_add_delay_usecs_) {
          next_event_timestamp_.addusecs(delay_usecs_to_add_);
        }
      }
    }
  }
}
