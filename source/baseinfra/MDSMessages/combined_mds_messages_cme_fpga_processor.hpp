// =====================================================================================
//
//       Filename:  combined_mds_messages_cme_source.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/30/2014 01:06:07 PM
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

#include <set>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/simple_security_name_indexer.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"

#include "baseinfra/FPGA/fpga_reader.hpp"

namespace HFSAT {

class CombinedMDSMessagesCMEFPGAProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  PriceLevelGlobalListener* p_price_level_global_listener_;  ///< Listeners of CME messages in LiveTrading of all types
  FPGAHalfBookGlobalListener* fpga_global_listener_;
  ExternalTimeListener* p_time_keeper_;
  FPGAReader* fpga_reader_;
  FPGAHalfBook fpga_half_book_[2];  // 0-> bid, 1->ask

  std::set<unsigned int> is_processing_quincy_source_for_this_security_;
  ProfilerTimeInfo time_info_;
  unsigned long cpu_freq_cycles;

 public:
  CombinedMDSMessagesCMEFPGAProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_time_keeper_(NULL),
        fpga_reader_(new FPGAReader("*", "pcie://pcie0")),
        is_processing_quincy_source_for_this_security_(),
        cpu_freq_cycles(HFSAT::RuntimeProfiler::GetUniqueInstance(HFSAT::ProfilerType::kTRADEINIT).GetCPUFreq())

  {}

  ~CombinedMDSMessagesCMEFPGAProcessor() {}

  inline void SetPriceLevelGlobalListener(PriceLevelGlobalListener* p_new_listener_) {
    p_price_level_global_listener_ = p_new_listener_;
  }
  inline void SetFPGAGlobalListener(FPGAHalfBookGlobalListener* listener_) { fpga_global_listener_ = listener_; }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }
  inline void ExcludeSecurityFromProcessing(unsigned int _security_id_) {
    is_processing_quincy_source_for_this_security_.insert(_security_id_);
  }

  inline void ProcessCMEFPGAEvents(bool _keep_in_loop_ = true) {
    int result, security_id_, level_, size_, num_ords_, msg_type_;
    bool intermediate_;
    double price_;
    TradeType_t _buysell_;
    bool new_update_bid_ = false, new_update_ask_ = false;

    while (true) {
      // Make FPGA API call and process messages
      result = fpga_reader_->readMessage();
      if (result <= 0) {  // No data
        break;
      }

      security_id_ = fpga_reader_->getSecurityId();

      char* sec_name = NULL;
      if ((security_id_ <= 0) ||
          ((sec_name = fpga_reader_->getSecurityName(security_id_)) == NULL))  // Check if ref for security id available
      {
        // Security not found..not sure why.
        fpga_reader_->releaseMessage();  // Deallocate message
        continue;
      }

      HFSAT::RuntimeProfiler::GetUniqueInstance(HFSAT::ProfilerType::kTRADEINIT).Start(time_info_);
      time_info_ = HFSAT::RuntimeProfiler::GetUniqueInstance(HFSAT::ProfilerType::kTRADEINIT).GetTimeInfo();

      // Artificial timestamp for combined writer and SHM1 in this case
      // Setting artificial total time diff for CSW = 0.98 usec
      time_info_.cshmw_start_time = time_info_.tradeinit_start_time - cpu_freq_cycles;
      // Setting artificial total time diff for SHM1 = 0.02 usec
      time_info_.cshmw_end_time = time_info_.tradeinit_start_time - 0.02 * cpu_freq_cycles;

      HFSAT::RuntimeProfiler::GetUniqueInstance(HFSAT::ProfilerType::kTRADEINIT).Start(time_info_);

      security_id_ = sec_name_indexer_.GetIdFromSecname(sec_name);
      if (security_id_ < 0) {
        // Security not found..not sure why.
        fpga_reader_->releaseMessage();  // Deallocate message
        continue;
      }
      msg_type_ = fpga_reader_->getMessageType();

      if (msg_type_ == FPGA_BOOK_LEVEL_UPDATE) {
        // Resetting values
        new_update_bid_ = new_update_ask_ = false;
        for (int ctr = 0; ctr < 5; ctr++) {
          fpga_half_book_[0].num_orders_[ctr] = 0;
          fpga_half_book_[0].sizes_[ctr] = -1;
          fpga_half_book_[0].prices_[ctr] = 0;
          fpga_half_book_[1].num_orders_[ctr] = 0;
          fpga_half_book_[1].sizes_[ctr] = -1;
          fpga_half_book_[1].prices_[ctr] = 0;
        }

        while (!fpga_reader_->parsedMessageFully()) {
          if (fpga_reader_->parseCMEBookLevelUpdate(_buysell_, level_, price_, size_, num_ords_, intermediate_)) {
            if (level_ >= 5) continue;
            if (_buysell_ == kTradeTypeBuy) {
              new_update_bid_ = true;
              fpga_half_book_[0].num_orders_[level_] = num_ords_;
              fpga_half_book_[0].sizes_[level_] = size_;
              fpga_half_book_[0].prices_[level_] = price_;
            } else {
              new_update_ask_ = true;
              fpga_half_book_[1].num_orders_[level_] = num_ords_;
              fpga_half_book_[1].sizes_[level_] = size_;
              fpga_half_book_[1].prices_[level_] = price_;
            }
          }
        }
        bool is_intermediate = new_update_bid_ && new_update_ask_;
        if (new_update_bid_ || new_update_ask_) {
          p_time_keeper_->OnTimeReceived(HFSAT::GetTimeOfDay());
          if (new_update_bid_) {
            fpga_global_listener_->OnHalfBookChange(security_id_, kTradeTypeBuy, &(fpga_half_book_[0]),
                                                    is_intermediate);
          }
          if (new_update_ask_) {
            fpga_global_listener_->OnHalfBookChange(security_id_, kTradeTypeSell, &(fpga_half_book_[1]), false);
          }
        }
      } else if (msg_type_ == FPGA_EXECUTION_SUMMARY) {
        if (fpga_reader_->parseCMEExecutionSummary(price_, size_, _buysell_)) {
          p_time_keeper_->OnTimeReceived(HFSAT::GetTimeOfDay());
          fpga_global_listener_->OnTrade(security_id_, price_, size_, _buysell_);
        }
      }
      fpga_reader_->releaseMessage();  // Deallocate message
      if (!_keep_in_loop_) break;
    }
  }

  /*inline void ProcessCMEEvent(CME_MDS::CMECommonStruct* next_event_) {
    switch (next_event_->msg_) {
      case CME_MDS::CME_DELTA: {
        register int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->data_.cme_dels_.contract_);

        // Check Security - Process Only Required Ones
        if (security_id < 0) return;

        p_time_keeper_->OnTimeReceived(next_event_->time_);

        if (next_event_->data_.cme_dels_.level_ > 0) {
          TradeType_t _buysell_ = TradeType_t(next_event_->data_.cme_dels_.type_ - '0');

          switch (next_event_->data_.cme_dels_.action_) {
            case '0': {
              p_price_level_global_listener_->OnPriceLevelNew(
                  security_id, _buysell_, next_event_->data_.cme_dels_.level_, next_event_->data_.cme_dels_.price_,
                  next_event_->data_.cme_dels_.size_, next_event_->data_.cme_dels_.num_ords_,
                  next_event_->data_.cme_dels_.intermediate_);
            } break;
            case '1': {
              p_price_level_global_listener_->OnPriceLevelChange(
                  security_id, _buysell_, next_event_->data_.cme_dels_.level_, next_event_->data_.cme_dels_.price_,
                  next_event_->data_.cme_dels_.size_, next_event_->data_.cme_dels_.num_ords_,
                  next_event_->data_.cme_dels_.intermediate_);
            } break;
            case '2': {
              p_price_level_global_listener_->OnPriceLevelDelete(
                  security_id, _buysell_, next_event_->data_.cme_dels_.level_, next_event_->data_.cme_dels_.price_,
                  next_event_->data_.cme_dels_.intermediate_);
            } break;
            default: {
              std::cerr << " Weird message type in CMEShmDataSource::ProcessAllEvents CME_DELTA : " << next_event_->msg_
                        << "\n";
            } break;
          }
        }

      } break;

      case CME_MDS::CME_TRADE: {
        register int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->data_.cme_trds_.contract_);

        // Check Security - Process Only Required Ones
        if (security_id < 0) return;

        p_time_keeper_->OnTimeReceived(next_event_->time_);

        TradeType_t _buysell_ =
            ((next_event_->data_.cme_trds_.agg_side_ == 1)
                 ? (kTradeTypeBuy)
                 : ((next_event_->data_.cme_trds_.agg_side_ == 2) ? kTradeTypeSell : kTradeTypeNoInfo));
        p_price_level_global_listener_->OnTrade(security_id, next_event_->data_.cme_trds_.trd_px_,
                                                next_event_->data_.cme_trds_.trd_qty_, _buysell_);

      } break;

      default: {
        std::cerr << " Weird message type in CMEShmSource::ProcessAllEvents " << next_event_->msg_ << "\n";

      } break;
    }
  }*/
};
}
