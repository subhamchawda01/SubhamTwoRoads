/*
 * homdcpf_logged_message_filesource.cpp
 *
 *  Created on: 11-Aug-2014
 *      Author: diwakar
 */

#include "baseinfra/LoggedSources/hkomd_logged_message_filesource.hpp"

namespace HFSAT {

void HKOMDCPFLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {
    do {
      size_t available_len = bulk_file_reader_.read(&next_event_, sizeof(HKOMD_MDS::HKOMDPFCommonStruct));
      if (available_len < sizeof(HKOMD_MDS::HKOMDPFCommonStruct)) {
        next_event_timestamp_ = ttime_t(time_t(0), 0);
        rw_hasevents_ = false;
        break;
      }
      next_event_timestamp_ = next_event_.time_;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    } while (next_event_timestamp_ < r_start_time);
  } else {
    rw_hasevents_ = false;
  }
}

void HKOMDCPFLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& _hasevents_) {
  if (bulk_file_reader_.is_open()) {
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKOMD_MDS::HKOMDPFCommonStruct));
    if (available_len_ < sizeof(HKOMD_MDS::HKOMDPFCommonStruct)) {
      next_event_timestamp_ = ttime_t(time_t(0), 0);
      _hasevents_ = false;
    } else {
      next_event_timestamp_ = next_event_.time_;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  } else {
    _hasevents_ = false;
  }
}

void HKOMDCPFLoggedMessageFileSource::ProcessAllEvents() {
  NotifyExternalDataListenerListener(security_id_);
  while (true) {
    m_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
    //      if( IsNormalTradeTime(security_id_, next_event_timestamp_))

    switch (next_event_.msg_) {
      case HKOMD_MDS::HKOMD_PF_TRADE:
        // filter msgs which don't have type as BLANK
        {
          if (!IsNormalTradeTime(security_id_, next_event_timestamp_)) break;
          TradeType_t buysell_ = (next_event_.data_.trade_.side_ == 2) ? kTradeTypeBuy : kTradeTypeSell;
          price_level_global_listener_hkomd_->OnTrade(security_id_, next_event_.data_.trade_.price_,
                                                      next_event_.data_.trade_.quantity_, buysell_);
        }
        break;
      case HKOMD_MDS::HKOMD_PF_DELTA: {
        if (!IsNormalTradeTime(security_id_, next_event_timestamp_)) break;
        bool intermediate_ = false;
        TradeType_t buysell_ = kTradeTypeBuy;
        if (next_event_.data_.delta_.side_ == 0) {
          buysell_ = kTradeTypeBuy;
        } else if (next_event_.data_.delta_.side_ == 1) {
          buysell_ = kTradeTypeSell;
        } else {
          return;
        }
        if (next_event_.data_.delta_.intermediate_ == 'Y' || (int)next_event_.data_.delta_.intermediate_ == 1) {
          intermediate_ = true;
        }
        switch (next_event_.data_.delta_.action_) {
          case 0:  // new level
          {
            price_level_global_listener_hkomd_->OnPriceLevelNew(
                security_id_, buysell_, next_event_.data_.delta_.level_, next_event_.data_.delta_.price_,
                next_event_.data_.delta_.quantity_, next_event_.data_.delta_.num_orders_, intermediate_);

          } break;
          case 1:  // change
          {
            price_level_global_listener_hkomd_->OnPriceLevelChange(
                security_id_, buysell_, next_event_.data_.delta_.level_, next_event_.data_.delta_.price_,
                next_event_.data_.delta_.quantity_, next_event_.data_.delta_.num_orders_, intermediate_);
          } break;
          case 2: {
            // price_level_global_listener_hkomd_ -> OnOrderLevelDelete( security_id_,
            price_level_global_listener_hkomd_->OnPriceLevelDelete(security_id_, buysell_,
                                                                   next_event_.data_.delta_.level_,
                                                                   next_event_.data_.delta_.price_, intermediate_);

          } break;
          case 8: {
            // std::cerr << " RESET MESSAGE in HKOMDCPF @" << next_event_timestamp_ << " SecName:" <<
            // HFSAT::SecurityNameIndexer::GetUniqueInstance ( ).GetSecurityNameFromId ( security_id_ ) <<  "\n";
          } break;
          case 74: {
            std::cerr << " CLEAR Message in HKOMDCPF \n";
          } break;
          default: { std::cerr << " action not recognised for hkomd event\n"; } break;
        }
      } break;
      default: {
        dbglogger_ << "Weird msgtype " << next_event_.msg_ << " in " << __func__ << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        exit(-1);
      } break;
    }

    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKOMD_MDS::HKOMDPFCommonStruct));
    if (available_len_ < sizeof(HKOMD_MDS::HKOMDPFCommonStruct)) {
      next_event_timestamp_ = ttime_t(time_t(0), 0);
      break;
    } else {
      next_event_timestamp_ = next_event_.time_;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  }
}

void HKOMDCPFLoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  NotifyExternalDataListenerListener(security_id_);
  while (next_event_timestamp_ <= _endtime_) {
    m_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
    // if( IsNormalTradeTime(security_id_, next_event_timestamp_))

    switch (next_event_.msg_) {
      case HKOMD_MDS::HKOMD_PF_TRADE:
        // filter msgs which don't have type as BLANK
        {
          if (!IsNormalTradeTime(security_id_, next_event_timestamp_)) break;
          TradeType_t buysell_ =
              (next_event_.data_.trade_.side_ == 2) ? kTradeTypeBuy : kTradeTypeSell;  // for sell it is 3
          price_level_global_listener_hkomd_->OnTrade(security_id_, next_event_.data_.trade_.price_,
                                                      next_event_.data_.trade_.quantity_, buysell_);
        }
        break;
      case HKOMD_MDS::HKOMD_PF_DELTA: {
        if (!IsNormalTradeTime(security_id_, next_event_timestamp_)) break;
        bool intermediate_ = false;
        TradeType_t buysell_ = kTradeTypeBuy;
        if (next_event_.data_.delta_.side_ == 0) {
          buysell_ = kTradeTypeBuy;
        } else if (next_event_.data_.delta_.side_ == 1) {
          buysell_ = kTradeTypeSell;
        } else {
          return;
        }
        if (next_event_.data_.delta_.intermediate_ == 'Y' || (int)next_event_.data_.delta_.intermediate_ == 1) {
          intermediate_ = true;
        }

        switch (next_event_.data_.delta_.action_) {
          case 0:  // New
          {
            //                    order_level_global_listener_hkomd_ -> OnOrderLevelNew ( security_id_,
            price_level_global_listener_hkomd_->OnPriceLevelNew(
                security_id_, buysell_, next_event_.data_.delta_.level_, next_event_.data_.delta_.price_,
                next_event_.data_.delta_.quantity_, next_event_.data_.delta_.num_orders_, intermediate_);

          } break;
          case 1:  // Change
          {
            price_level_global_listener_hkomd_->OnPriceLevelChange(
                security_id_, buysell_, next_event_.data_.delta_.level_, next_event_.data_.delta_.price_,
                next_event_.data_.delta_.quantity_, next_event_.data_.delta_.num_orders_, intermediate_);
          } break;
          case 2: {
            price_level_global_listener_hkomd_->OnPriceLevelDelete(security_id_, buysell_,
                                                                   next_event_.data_.delta_.level_,
                                                                   next_event_.data_.delta_.price_, intermediate_);

          } break;
          case 8: {
            // std::cerr << " RESET MESSAGE in HKOMDCPF @" << next_event_timestamp_ << " SecName:" <<
            // HFSAT::SecurityNameIndexer::GetUniqueInstance ( ).GetSecurityNameFromId ( security_id_ ) <<  "\n";
          } break;
          case 74: {
            std::cerr << " CLEAR Message in HKOMDCPF \n";
          } break;
          default: { std::cerr << " action not recognised for hkomd event\n"; } break;
        }
      } break;
      default: {
        dbglogger_ << "Weird msgtype " << next_event_.msg_ << " in " << __func__ << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        exit(-1);
      }
    }

    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKOMD_MDS::HKOMDPFCommonStruct));
    if (available_len_ < sizeof(HKOMD_MDS::HKOMDPFCommonStruct)) {
      next_event_timestamp_ = ttime_t(time_t(0), 0);
      break;
    } else {
      next_event_timestamp_ = next_event_.time_;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  }
}
}
