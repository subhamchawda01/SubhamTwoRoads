/**
   \file MDSMessageCode/ntp_logged_message_filesource.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "dvccode/CDef/mds_messages.hpp"
#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filesource.hpp"

namespace HFSAT {

NTPLoggedMessageFileSource::NTPLoggedMessageFileSource(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, TradingLocation_t r_trading_location_,
    bool use_order_depth_book_, bool is_bmf_equity_, bool t_use_fake_faster_data_)
    : ExternalDataListener(),
      dbglogger_(t_dbglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      p_price_level_global_listener_(NULL),
      p_order_level_global_listener_(NULL),
      p_price_level_order_book_global_listener_(NULL),
      p_ntp_price_level_global_listener_(NULL),
      p_order_level_listener_sim_(NULL),
      p_time_keeper_(NULL),
      trade_time_manager_(HFSAT::TradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_preevent_YYYYMMDD_)),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      use_order_book_(use_order_depth_book_ || is_bmf_equity_),
      event_queue_(),
      events_left_(0),
      next_non_intermediate_time_(0, 0),
      use_fake_faster_data_(t_use_fake_faster_data_) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  // find the filename
  std::string t_ntp_filename_ = NTPLoggedMessageFileNamer::GetName(
      t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_, use_order_depth_book_, is_bmf_equity_);

  int added_delay = 0;
  // added delays is used by the robustness check script to variate line delay by a constant number
  added_delay = TradingLocationUtils::GetAddedDelay(trading_location_file_read_, r_trading_location_);
  delay_usecs_to_add_ = added_delay;

  if (trading_location_file_read_ != r_trading_location_) {
    delay_usecs_to_add_ +=
        TradingLocationUtils::GetMSecsBetweenTradingLocations(trading_location_file_read_, r_trading_location_) * 1000;
  }
  if (delay_usecs_to_add_ != 0) need_to_add_delay_usecs_ = true;

  ComputeFirstNonIntermediateTime(t_ntp_filename_);
  if (TradingLocationUtils::GetUsecsForFasterDataForShortcodeAtLocation(
          sec_name_indexer_.GetShortcodeFromId(security_id_), r_trading_location_, t_preevent_YYYYMMDD_,
          use_fake_faster_data_) !=
      0) {  // Experiment with the possibility of using faster data for this product at this location.
    need_to_add_delay_usecs_ = true;
    delay_usecs_to_add_ -= TradingLocationUtils::GetUsecsForFasterDataForShortcodeAtLocation(
        sec_name_indexer_.GetShortcodeFromId(security_id_), r_trading_location_, t_preevent_YYYYMMDD_,
        use_fake_faster_data_);
  }

  DBGLOG_CLASS_FUNC << "For NTP symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                    << " trading_location " << trading_location_file_read_ << " DUsecs " << delay_usecs_to_add_
                    << " @use " << r_trading_location_ << " returned filename = " << t_ntp_filename_
                    << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(t_ntp_filename_);
  } else {
    std::cerr << "For NTP symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_ntp_filename_ << std::endl;
  }
  // bulk_file_reader_.open ( t_cme_filename_ ) ;
}

void NTPLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_
    // to check if next_event_timestamp_
    // if greater than r_endtime_
    do {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NTP_MDS::NTPCommonStruct));
      if (available_len_ < sizeof(NTP_MDS::NTPCommonStruct)) {  // data not found in file
        next_event_timestamp_ =
            ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
        rw_hasevents_ = false;
        break;
      }
      next_event_timestamp_ =
          next_event_.time_;  // need more work here to optimize, or preferably send ttime_t from MDS or ORS

      if (first_non_intermediate_time_ >= r_start_time_) {
        next_event_timestamp_ = first_non_intermediate_time_;
        return;
      }

      if (need_to_add_delay_usecs_) {
        switch (next_event_.msg_) {
          case NTP_MDS::NTP_DELTA: {
            if (!next_event_.data_.ntp_dels_.intermediate_) {
              next_event_timestamp_.addusecs(delay_usecs_to_add_);
            }
          } break;
          default:  // in order messages we assign timestamp to intermediate messages , so not having the check for that
          {
            next_event_timestamp_.addusecs(delay_usecs_to_add_);
          } break;
        }
      }
    } while (next_event_timestamp_ < r_start_time_ && !use_order_book_);

  } else {  // data file not open
    rw_hasevents_ = false;
  }
  /// disable callback
  if (use_order_book_) {
    SetTimeToSkipUntilFirstEvent(r_start_time_);
  }
}

void NTPLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& _hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NTP_MDS::NTPCommonStruct));
    if (available_len_ <
        sizeof(NTP_MDS::NTPCommonStruct)) {           /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      _hasevents_ = false;
    } else {
      next_event_timestamp_ = next_event_.time_;  // TODO : Converting from timeval to ttime_t. Need more work here to
                                                  // optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        switch (next_event_.msg_) {
          case NTP_MDS::NTP_DELTA: {
            if (!next_event_.data_.ntp_dels_.intermediate_) {
              next_event_timestamp_.addusecs(delay_usecs_to_add_);
            }
          } break;
          default:  // in order messages we assign timestamp to intermediate messages , so not having the check for that
          {
            next_event_timestamp_.addusecs(delay_usecs_to_add_);
          } break;
        }
      }
    }
  } else {
    _hasevents_ = false;
  }
}

inline bool NTPLoggedMessageFileSource::_SetNextTimeStamp() {
  if (events_left_ <= 0) {
    events_left_ = 0;
    bool found_non_intermediate_event_ = false;

    while (!found_non_intermediate_event_) {
      if (bulk_file_reader_.read(&next_event_, sizeof(NTP_MDS::NTPCommonStruct)) < sizeof(NTP_MDS::NTPCommonStruct)) {
        next_event_timestamp_ = ttime_t(0, 0);
        return false;
      }

      if (next_event_.time_.tv_sec != 0) {
        next_non_intermediate_time_ = next_event_.time_;
        found_non_intermediate_event_ = true;
      }

      event_queue_.push_back(next_event_);
      events_left_++;
    }
  }

  if (events_left_ > 0) {
    next_event_ = event_queue_[0];
    next_event_timestamp_ = next_non_intermediate_time_;

    if (need_to_add_delay_usecs_) {
      next_event_timestamp_.addusecs(delay_usecs_to_add_);
    }

    events_left_--;
    event_queue_.erase(event_queue_.begin());

    return true;
  }

  return true;
}

inline MktStatus_t NTPLoggedMessageFileSource::GetMarketStatus(NTP_MDS::NTPCommonStruct& _next_event_) {
  switch (_next_event_.msg_) {
    case NTP_MDS::NTP_DELTA: {
      switch (_next_event_.data_.ntp_dels_.flags[1]) {
        case 2: {
          return kMktTradingStatusPause;
        } break;
        case 4: {
          return kMktTradingStatusClosed;
        } break;
        case 17: {
          return kMktTradingStatusOpen;
        } break;
        case 18: {
          return kMktTradingStatusForbidden;
        } break;
        case 20: {
          return kMktTradingStatusUnknown;
        } break;
        case 21: {
          return kMktTradingStatusReserved;
        } break;
        case 101: {
          return kMktTradingStatuFinalClosingCall;
        } break;
        default: { return kMktTradingStatusOpen; } break;
      }
    } break;
    case NTP_MDS::NTP_TRADE: {
      switch (_next_event_.data_.ntp_trds_.flags_[1]) {
        case 2: {
          return kMktTradingStatusPause;
        } break;
        case 4: {
          return kMktTradingStatusClosed;
        } break;
        case 17: {
          return kMktTradingStatusOpen;
        } break;
        case 18: {
          return kMktTradingStatusForbidden;
        } break;
        case 20: {
          return kMktTradingStatusUnknown;
        } break;
        case 21: {
          return kMktTradingStatusReserved;
        } break;
        case 101: {
          return kMktTradingStatuFinalClosingCall;
        } break;
        default: { return kMktTradingStatusOpen; } break;
      }
    } break;
    default: { return kMktTradingStatusOpen; }
  }
  return kMktTradingStatusOpen;
}
inline void NTPLoggedMessageFileSource::_ProcessThisMsg() {
  NotifyExternalDataListenerListener(security_id_);
  if (!IsNormalTradeTime(security_id_, next_event_timestamp_)) return;

  if (p_ntp_price_level_global_listener_) {
    p_ntp_price_level_global_listener_->OnPriceLevelUpdate(&next_event_, sizeof(next_event_), HFSAT::MDS_MSG::NTP);

    switch (next_event_.msg_) {
      case NTP_MDS::NTP_DELTA: {
        if (next_event_.data_.ntp_dels_.level_ > 0) {
          // Only Update the Watch if the incoming message is of type non-intermediate
          if (next_event_.time_.tv_sec != 0) {
            p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
          }

          MktStatus_t this_status_ = GetMarketStatus(next_event_);
          p_ntp_price_level_global_listener_->OnMarketStatusUpdate(security_id_, this_status_);

          if (this_status_ != kMktTradingStatusOpen) {
            return;
          }
          TradeType_t _buysell_ = TradeType_t(next_event_.data_.ntp_dels_.type_ - '0');

          switch (next_event_.data_.ntp_dels_.action_) {
            case 0: {
              p_ntp_price_level_global_listener_->OnPriceLevelNew(
                  security_id_, _buysell_, next_event_.data_.ntp_dels_.level_, next_event_.data_.ntp_dels_.price_,
                  next_event_.data_.ntp_dels_.size_, next_event_.data_.ntp_dels_.num_ords_,
                  next_event_.data_.ntp_dels_.intermediate_);
            } break;
            case 1: {
              p_ntp_price_level_global_listener_->OnPriceLevelChange(
                  security_id_, _buysell_, next_event_.data_.ntp_dels_.level_, next_event_.data_.ntp_dels_.price_,
                  next_event_.data_.ntp_dels_.size_, next_event_.data_.ntp_dels_.num_ords_,
                  next_event_.data_.ntp_dels_.intermediate_);
            } break;
            case 2: {
              p_ntp_price_level_global_listener_->OnPriceLevelDelete(
                  security_id_, _buysell_, next_event_.data_.ntp_dels_.level_, next_event_.data_.ntp_dels_.price_,
                  next_event_.data_.ntp_dels_.intermediate_);
            } break;
            case 3: {  // one side of the book is to be cleared entirely
              p_ntp_price_level_global_listener_->OnPriceLevelDeleteThru(security_id_, _buysell_,
                                                                         next_event_.data_.ntp_dels_.intermediate_);
              break;
            }
            case 4: {  // needs price ?
              p_ntp_price_level_global_listener_->OnPriceLevelDeleteFrom(
                  security_id_, _buysell_, next_event_.data_.ntp_dels_.level_, next_event_.data_.ntp_dels_.price_,
                  next_event_.data_.ntp_dels_.intermediate_);
              break;
            }
            default: {
              fprintf(stderr, "Weird message type in NTPLoggedMessageFileSource::ProcessAllEvents NTP_DELTA\n");
            } break;
          }
        }
      } break;
      case NTP_MDS::NTP_TRADE: {
        p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

        MktStatus_t this_status_ = GetMarketStatus(next_event_);
        p_ntp_price_level_global_listener_->OnMarketStatusUpdate(security_id_, this_status_);
        if (this_status_ != kMktTradingStatusOpen) {
          return;
        }

        if (next_event_.data_.ntp_trds_.flags_[0] != 'X') {
          p_ntp_price_level_global_listener_->OnTrade(security_id_, next_event_.data_.ntp_trds_.trd_px_,
                                                      next_event_.data_.ntp_trds_.trd_qty_);
        }
      } break;
      case NTP_MDS::NTP_STATUS: {
        break;
      }
      case NTP_MDS::NTP_OPENPRICE: {
        break;
      }
      case NTP_MDS::NTP_IMBALANCE: {
        break;
      }
      default: {
        fprintf(stderr, "Weird message type in NTPLoggedMessageFileSource::ProcessAllEvents NTP_TRADE\n");
        break;
      }
    }
  }

  else {
    switch (next_event_.msg_) {
      case NTP_MDS::NTP_DELTA: {
        if (next_event_.data_.ntp_dels_.level_ > 0) {  // ignoring level 0 events right now

          // Only Update the Watch if the incoming message is of type non-intermediate
          if (next_event_.time_.tv_sec != 0) {
            p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
          }

          MktStatus_t this_status_ = GetMarketStatus(next_event_);
          if (p_price_level_global_listener_)
            p_price_level_global_listener_->OnMarketStatusUpdate(security_id_, this_status_);

          TradeType_t _buysell_ = TradeType_t(next_event_.data_.ntp_dels_.type_ - '0');

          switch (next_event_.data_.ntp_dels_.action_) {
            case 0: {
              p_price_level_global_listener_->OnPriceLevelNew(
                  security_id_, _buysell_, next_event_.data_.ntp_dels_.level_, next_event_.data_.ntp_dels_.price_,
                  next_event_.data_.ntp_dels_.size_, next_event_.data_.ntp_dels_.num_ords_,
                  next_event_.data_.ntp_dels_.intermediate_);
            } break;
            case 1: {
              p_price_level_global_listener_->OnPriceLevelChange(
                  security_id_, _buysell_, next_event_.data_.ntp_dels_.level_, next_event_.data_.ntp_dels_.price_,
                  next_event_.data_.ntp_dels_.size_, next_event_.data_.ntp_dels_.num_ords_,
                  next_event_.data_.ntp_dels_.intermediate_);
            } break;
            case 2: {
              p_price_level_global_listener_->OnPriceLevelDelete(
                  security_id_, _buysell_, next_event_.data_.ntp_dels_.level_, next_event_.data_.ntp_dels_.price_,
                  next_event_.data_.ntp_dels_.intermediate_);
            } break;
            case 3: {  // one side of the book is to be cleared entirely
              p_price_level_global_listener_->OnPriceLevelDeleteThrough(security_id_, _buysell_, DEF_MARKET_DEPTH,
                                                                        next_event_.data_.ntp_dels_.intermediate_);
            } break;
            case 4: {
              p_price_level_global_listener_->OnPriceLevelDeleteFrom(security_id_, _buysell_,
                                                                     next_event_.data_.ntp_dels_.level_,
                                                                     next_event_.data_.ntp_dels_.intermediate_);
            } break;
            default: {
              fprintf(stderr, "Weird message type in NTPLoggedMessageFileSource::ProcessAllEvents NTP_DELTA\n");
            } break;
          }
        }
      } break;

      case NTP_MDS::NTP_TRADE: {
        p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

        MktStatus_t this_status_ = GetMarketStatus(next_event_);
        if (p_price_level_global_listener_)
          p_price_level_global_listener_->OnMarketStatusUpdate(security_id_, this_status_);

        if (p_price_level_global_listener_) {
          if (next_event_.data_.ntp_trds_.flags_[0] != 'X') {
            p_price_level_global_listener_->OnTrade(security_id_, next_event_.data_.ntp_trds_.trd_px_,
                                                    next_event_.data_.ntp_trds_.trd_qty_, kTradeTypeNoInfo);
          }
        }
        if (p_order_level_global_listener_) {
          p_order_level_global_listener_->OnTrade(security_id_, next_event_.data_.ntp_trds_.trd_px_,
                                                  next_event_.data_.ntp_trds_.trd_qty_, kTradeTypeNoInfo);
        }
        if (p_price_level_order_book_global_listener_) {
          p_price_level_order_book_global_listener_->OnTrade(security_id_, next_event_.data_.ntp_trds_.trd_px_,
                                                             next_event_.data_.ntp_trds_.trd_qty_, kTradeTypeNoInfo);
        }

        if (p_order_level_listener_sim_) {
          p_order_level_listener_sim_->OnTrade(security_id_, next_event_.data_.ntp_trds_.trd_px_,
                                               next_event_.data_.ntp_trds_.trd_qty_, kTradeTypeNoInfo);
        }
      } break;

      case NTP_MDS::NTP_ORDER: {
        p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
        int security_id_ = sec_name_indexer_.GetIdFromSecname(next_event_.data_.ntp_ordr_.contract_);
        if (security_id_ < 0) break;

        // order-book reset msg
        if (next_event_.data_.ntp_ordr_.type_ == 'J') {
          if (p_price_level_order_book_global_listener_) {
            p_price_level_order_book_global_listener_->resetBook(security_id_);
          } else if (p_order_level_global_listener_) {
            p_order_level_global_listener_->resetBook(security_id_);
          }

          if (p_order_level_listener_sim_) {
            p_order_level_listener_sim_->ResetBook(security_id_);
          }
          break;
        }

        if (next_event_.data_.ntp_ordr_.level_ > 0) {  // ignoring level 0 events right now

          TradeType_t _buysell_ = TradeType_t(next_event_.data_.ntp_ordr_.type_ - '0');

          switch (next_event_.data_.ntp_ordr_.action_) {
            case 0: {
              if (p_price_level_order_book_global_listener_) {
                p_price_level_order_book_global_listener_->OnOrderLevelNew(
                    security_id_, next_event_.data_.ntp_ordr_.order_id_, _buysell_, next_event_.data_.ntp_ordr_.level_,
                    next_event_.data_.ntp_ordr_.price_, next_event_.data_.ntp_ordr_.size_, 0, false);
              } else if (p_order_level_global_listener_) {
                p_order_level_global_listener_->OnOrderLevelNew(
                    security_id_, next_event_.data_.ntp_ordr_.order_id_, _buysell_, next_event_.data_.ntp_ordr_.level_,
                    next_event_.data_.ntp_ordr_.price_, next_event_.data_.ntp_ordr_.size_, 0,
                    next_event_.data_.ntp_ordr_.intermediate_);
              }

              if (p_order_level_listener_sim_) {
                p_order_level_listener_sim_->OnOrderAdd(
                    security_id_, _buysell_, next_event_.data_.ntp_ordr_.level_, next_event_.data_.ntp_ordr_.order_id_,
                    next_event_.data_.ntp_ordr_.price_, next_event_.data_.ntp_ordr_.size_);
              }
            } break;
            case 1: {
              if (p_price_level_order_book_global_listener_) {
                p_price_level_order_book_global_listener_->OnOrderLevelChange(
                    security_id_, next_event_.data_.ntp_ordr_.order_id_, _buysell_, next_event_.data_.ntp_ordr_.level_,
                    next_event_.data_.ntp_ordr_.price_, next_event_.data_.ntp_ordr_.size_, 0, false);
              } else if (p_order_level_global_listener_) {
                p_order_level_global_listener_->OnOrderLevelChange(
                    security_id_, next_event_.data_.ntp_ordr_.order_id_, _buysell_, next_event_.data_.ntp_ordr_.level_,
                    next_event_.data_.ntp_ordr_.price_, next_event_.data_.ntp_ordr_.size_, 0,
                    next_event_.data_.ntp_ordr_.intermediate_);
              }

              if (p_order_level_listener_sim_) {
                p_order_level_listener_sim_->OnOrderModify(
                    security_id_, _buysell_, next_event_.data_.ntp_ordr_.level_, next_event_.data_.ntp_ordr_.order_id_,
                    next_event_.data_.ntp_ordr_.price_, next_event_.data_.ntp_ordr_.size_, 0, 0, 0);
              }
            } break;
            case 2: {
              if (p_price_level_order_book_global_listener_) {
                p_price_level_order_book_global_listener_->OnOrderLevelDelete(
                    security_id_, next_event_.data_.ntp_ordr_.order_id_, _buysell_, next_event_.data_.ntp_ordr_.level_,
                    next_event_.data_.ntp_ordr_.price_, next_event_.data_.ntp_ordr_.intermediate_);
              } else if (p_order_level_global_listener_) {
                p_order_level_global_listener_->OnOrderLevelDelete(security_id_, next_event_.data_.ntp_ordr_.order_id_,
                                                                   _buysell_, next_event_.data_.ntp_ordr_.level_,
                                                                   next_event_.data_.ntp_ordr_.price_, false);
              }

              if (p_order_level_listener_sim_) {
                p_order_level_listener_sim_->OnOrderDelete(
                    security_id_, _buysell_, next_event_.data_.ntp_ordr_.level_, next_event_.data_.ntp_ordr_.order_id_,
                    next_event_.data_.ntp_ordr_.price_, next_event_.data_.ntp_ordr_.size_);
              }
            } break;
            case 3: {
              // entire side of the book is to be cleared
              if (p_price_level_order_book_global_listener_) {
                p_price_level_order_book_global_listener_->OnOrderLevelDeleteThrough(security_id_, _buysell_,
                                                                                     MAX_BMF_ORDER_BOOK_DEPTH, false);
              } else if (p_order_level_global_listener_) {
                p_order_level_global_listener_->OnOrderLevelDeleteThrough(security_id_, _buysell_,
                                                                          MAX_BMF_ORDER_BOOK_DEPTH, false);
              }
            } break;
            case 4: {
              if (p_price_level_order_book_global_listener_) {
                p_price_level_order_book_global_listener_->OnOrderLevelDeleteFrom(
                    security_id_, _buysell_, next_event_.data_.ntp_ordr_.level_, false);
              } else if (p_order_level_global_listener_) {
                p_order_level_global_listener_->OnOrderLevelDeleteFrom(security_id_, _buysell_,
                                                                       next_event_.data_.ntp_ordr_.level_, false);
              }
            } break;
            default: {
              fprintf(stderr, "Weird message type in NTPLoggedSource::ProcessAllEvents NTP_ORDER %d \n",
                      (int)next_event_.msg_);
            } break;
          }
        }

      } break;

      default: {
        fprintf(stderr, "Weird message type in NTPLoggedMessageFileSource::ProcessAllEvents NTP_TRADE\n");
      } break;
    }
  }
}

void NTPLoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void NTPLoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void NTPLoggedMessageFileSource::ComputeFirstNonIntermediateTime(std::string logged_filesource_name_) {
  BulkFileReader this_filesource_;
  this_filesource_.open(logged_filesource_name_);
  if (this_filesource_.is_open()) {
    while (true) {
      size_t available_len_ = this_filesource_.read(&next_event_, sizeof(NTP_MDS::NTPCommonStruct));
      if (available_len_ < sizeof(NTP_MDS::NTPCommonStruct)) {
        break;
      } else {
        if (next_event_.time_.tv_sec == 0) {
          continue;
        } else {
          first_non_intermediate_time_ = next_event_.time_;
          break;
        }
      }
    }
    this_filesource_.close();
  }
}
}
