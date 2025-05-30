// =====================================================================================
//
//       Filename:  combined_mds_messages_shm_logger.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/30/2014 12:39:20 PM
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <stdint.h>

#include <iostream>
#include <fstream>

#include <set>

#include "baseinfra/MDSMessages/combined_mds_messages_shm_base.hpp"
#include "dvccode/Utils/load_low_bandwidth_code_mapping.hpp"

#include "dvccode/Utils/runtime_profiler.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/CDef/common_security_definition_structs.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"

namespace HFSAT {
namespace MDSMessages {

class CombinedMDSMessagesShmLogger : public HFSAT::MDSMessages::CombinedMDSMessagesShmBase {
 private:
  HFSAT::FastMdConsumerMode_t processing_mode_;

  HFSAT::DebugLogger &dbglogger_;

  // this is meant to be a local copy to track queue position, compare with volatile shared memory counter
  int index_;

  HFSAT::MDS_MSG::GenericMDSMessage generic_mds_message_;
  uint32_t generic_mds_message_pkt_size_;

  std::string mdslog_file_name_;
  HFSAT::BulkFileWriter *mds_bulk_writer_;

  // These Are Specific Handling WHich We Need to Convert the Low Bandwidth Structs Sent Over Multicast to The Original
  // Common Struct Form For the Compatibility Reasons
  std::map<uint8_t, std::string> eurex_product_code_to_shortcode_;
  std::map<std::string, std::string> eurex_shortcode_to_exchange_symbol_;

  std::map<uint8_t, std::string> cme_product_code_to_shortcode_;
  std::map<std::string, std::string> cme_shortcode_to_exchange_symbol_;

  EUREX_MDS::EUREXCommonStruct eurex_cstr_;
  CME_MDS::CMECommonStruct cme_cstr_;
  std::map<std::string, std::string> valid_exchage_symbol_to_shortcode;

 public:
  CombinedMDSMessagesShmLogger(HFSAT::DebugLogger &_dbglogger_, HFSAT::FastMdConsumerMode_t _mode_,
                               const std::string &_mdslog_file_name_)
      : CombinedMDSMessagesShmBase(),
        processing_mode_(_mode_),
        dbglogger_(_dbglogger_),
        index_(-1),
        generic_mds_message_(),
        generic_mds_message_pkt_size_(sizeof(HFSAT::MDS_MSG::GenericMDSMessage)),
        mdslog_file_name_(_mdslog_file_name_) {
    std::string file_name = mdslog_file_name_;  // Plus some processing to file_name if needed
    mds_bulk_writer_ = new HFSAT::BulkFileWriter(file_name.c_str(), 32 * 1024,
                                                 std::ofstream::binary | std::ofstream::app | std::ofstream::ate);
    HFSAT::Utils::LowBWCodeMappingLoader::LoadMappingForGivenFile(
        DEF_LS_PRODUCTCODE_SHORTCODE_, eurex_product_code_to_shortcode_, eurex_shortcode_to_exchange_symbol_);
    HFSAT::Utils::LowBWCodeMappingLoader::LoadMappingForGivenFile(
        DEF_CME_LS_PRODUCTCODE_SHORTCODE_, cme_product_code_to_shortcode_, cme_shortcode_to_exchange_symbol_);
    InitializeExchSymbolToShortCodeMap();
  }

  ~CombinedMDSMessagesShmLogger() {
    mds_bulk_writer_->Close();
    delete mds_bulk_writer_;
  }

  inline void InitializeExchSymbolToShortCodeMap() {
    int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
    const HFSAT::ShortcodeContractSpecificationMap &contract_specification_map =
        HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).contract_specification_map_;
    for (auto it = contract_specification_map.begin(); it != contract_specification_map.end(); it++) {
      std::string short_code = it->first;
      std::string exchange_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(short_code);
      if (!exchange_symbol.empty())
        valid_exchage_symbol_to_shortcode.insert(std::pair<std::string, std::string>(exchange_symbol, short_code));
    }
  }

  inline void RunLiveShmSource(bool _keep_in_loop_ = true) {
    while (true) {
      // has to be volatile, waiting on shared memory segment queue position
      volatile int queue_position_ = *((int *)(generic_mds_shm_struct_ + GENERIC_MDS_QUEUE_SIZE));

      // events are available only if the queue position at source is higher by 1, circular queue, will lag behind by 1
      // packet at start
      if (-1 == index_) {
        index_ = queue_position_;
      }

      if (index_ == queue_position_) {
        // THis is kind of hack to work with Retail
        if (!_keep_in_loop_) break;

        continue;
      }

      index_ = (index_ + 1) & (GENERIC_MDS_QUEUE_SIZE - 1);

      // memcpy is only done as safegaurd from writer writing the same segment
      memcpy(&generic_mds_message_, (HFSAT::MDS_MSG::GenericMDSMessage *)(generic_mds_shm_struct_ + index_),
             generic_mds_message_pkt_size_);

      const char *exchange_symbol = nullptr;
      HFSAT::ExchSource_t exchange_source = generic_mds_message_.GetExchangeSourceFromGenericStruct();
      switch (generic_mds_message_.mds_msg_exch_) {
        case HFSAT::MDS_MSG::CME_LS: {
          HFSAT::Utils::LowBWCodeMappingLoader::GetCMEStructFromLowBWStruct(
              cme_product_code_to_shortcode_, cme_shortcode_to_exchange_symbol_, cme_cstr_,
              generic_mds_message_.generic_data_.cme_ls_data_);
          exchange_symbol = cme_cstr_.getContract();

        } break;
        case HFSAT::MDS_MSG::EUREX_LS: {
          HFSAT::Utils::LowBWCodeMappingLoader::GetEurexStructFromLowBWStruct(
              eurex_product_code_to_shortcode_, eurex_shortcode_to_exchange_symbol_, eurex_cstr_,
              generic_mds_message_.generic_data_.eurex_ls_data_);
          exchange_symbol = eurex_cstr_.getContract();

        } break;
        case HFSAT::MDS_MSG::EOBI_LS: {
          HFSAT::Utils::LowBWCodeMappingLoader::GetEurexStructFromLowBWStruct(
              eurex_product_code_to_shortcode_, eurex_shortcode_to_exchange_symbol_, eurex_cstr_,
              generic_mds_message_.generic_data_.eobi_ls_data_);
          exchange_symbol = eurex_cstr_.getContract();

        } break;
        default: { exchange_symbol = generic_mds_message_.getContract(); } break;
      }
      if (!exchange_symbol || !HasValidExchangeSymbol(exchange_source, exchange_symbol)) continue;

      switch (processing_mode_) {
        case HFSAT::kComShmConsumer: {
          // first log the exchange type followed by exchange specific data
          switch (generic_mds_message_.mds_msg_exch_) {
            case HFSAT::MDS_MSG::CME: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.cme_data_),
                                      sizeof(generic_mds_message_.generic_data_.cme_data_));

            } break;

            case HFSAT::MDS_MSG::LIFFE:
            case HFSAT::MDS_MSG::LIFFE_LS: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.liffe_data_),
                                      sizeof(generic_mds_message_.generic_data_.liffe_data_));

            } break;

            case HFSAT::MDS_MSG::EUREX_LS: {
              eurex_cstr_.time_ = generic_mds_message_.time_;

              generic_mds_message_.mds_msg_exch_ = HFSAT::MDS_MSG::EOBI_PF;
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(eurex_cstr_), sizeof(eurex_cstr_));

            } break;

            case HFSAT::MDS_MSG::EOBI_LS: {
              eurex_cstr_.time_ = generic_mds_message_.time_;

              generic_mds_message_.mds_msg_exch_ = HFSAT::MDS_MSG::EOBI_PF;
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(eurex_cstr_), sizeof(eurex_cstr_));

            } break;

            case HFSAT::MDS_MSG::CONTROL: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.control_data_),
                                      sizeof(generic_mds_message_.generic_data_.control_data_));

            } break;

            case HFSAT::MDS_MSG::ORS_REPLY: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.ors_reply_data_),
                                      sizeof(generic_mds_message_.generic_data_.ors_reply_data_));

            } break;

            case HFSAT::MDS_MSG::RTS: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.rts_data_),
                                      sizeof(generic_mds_message_.generic_data_.rts_data_));

            } break;

            // Same Handling for MICEX_CR and MICEX_EQ
            case HFSAT::MDS_MSG::MICEX: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.micex_data_),
                                      sizeof(generic_mds_message_.generic_data_.micex_data_));

            } break;

            case HFSAT::MDS_MSG::CME_LS: {
              cme_cstr_.time_ = generic_mds_message_.time_;

              generic_mds_message_.mds_msg_exch_ = HFSAT::MDS_MSG::CME;
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(cme_cstr_), sizeof(cme_cstr_));

            } break;

            case HFSAT::MDS_MSG::NTP:
            case HFSAT::MDS_MSG::NTP_LS: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.ntp_data_),
                                      sizeof(generic_mds_message_.generic_data_.ntp_data_));

            } break;
            case HFSAT::MDS_MSG::BMF_EQ: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.bmf_eq_data_),
                                      sizeof(generic_mds_message_.generic_data_.bmf_eq_data_));

            } break;
            case HFSAT::MDS_MSG::EOBI_PF: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.eobi_pf_data_),
                                      sizeof(generic_mds_message_.generic_data_.eobi_pf_data_));

            } break;

            case HFSAT::MDS_MSG::OSE_PF: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.ose_pf_data_),
                                      sizeof(generic_mds_message_.generic_data_.ose_pf_data_));

            } break;

            case HFSAT::MDS_MSG::OSE_CF: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.ose_cf_data_),
                                      sizeof(generic_mds_message_.generic_data_.ose_cf_data_));

            } break;

            case HFSAT::MDS_MSG::CSM: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.csm_data_),
                                      sizeof(generic_mds_message_.generic_data_.csm_data_));

            } break;

            case HFSAT::MDS_MSG::OSE_L1: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.ose_l1_data_),
                                      sizeof(generic_mds_message_.generic_data_.ose_l1_data_));

            } break;

            case HFSAT::MDS_MSG::TMX:
            case HFSAT::MDS_MSG::TMX_LS: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.tmx_data_),
                                      sizeof(generic_mds_message_.generic_data_.tmx_data_));
            } break;

            case HFSAT::MDS_MSG::TMX_OBF: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.tmx_obf_data_),
                                      sizeof(generic_mds_message_.generic_data_.tmx_obf_data_));
            } break;

            case HFSAT::MDS_MSG::ICE:
            case HFSAT::MDS_MSG::ICE_LS: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.ice_data_),
                                      sizeof(generic_mds_message_.generic_data_.ice_data_));
            } break;

            case HFSAT::MDS_MSG::ASX: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.asx_data_),
                                      sizeof(generic_mds_message_.generic_data_.asx_data_));
            } break;

            case HFSAT::MDS_MSG::SGX: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.sgx_data_),
                                      sizeof(generic_mds_message_.generic_data_.sgx_data_));
            } break;
            case HFSAT::MDS_MSG::OSE_ITCH_PF: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.ose_itch_pf_data_),
                                      sizeof(generic_mds_message_.generic_data_.ose_itch_pf_data_));
            } break;
            case HFSAT::MDS_MSG::HKOMDPF: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.hkomd_pf_data_),
                                      sizeof(generic_mds_message_.generic_data_.hkomd_pf_data_));
            } break;
            case HFSAT::MDS_MSG::AFLASH: {
              dbglogger_ << "Received Aflash message.. " << generic_mds_message_.mds_msg_exch_ << "\n";
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.aflash_data_),
                                      sizeof(generic_mds_message_.generic_data_.aflash_data_));
            } break;
            case HFSAT::MDS_MSG::RETAIL: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.retail_data_),
                                      sizeof(generic_mds_message_.generic_data_.retail_data_));
            } break;
            case HFSAT::MDS_MSG::NSE: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.nse_data_),
                                      sizeof(generic_mds_message_.generic_data_.nse_data_));
            } break;
            case HFSAT::MDS_MSG::NSE_L1: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.nse_l1_data_),
                                      sizeof(generic_mds_message_.generic_data_.nse_l1_data_));
            } break;
            case HFSAT::MDS_MSG::EOBI_OF: {
              mds_bulk_writer_->Write(&(generic_mds_message_.mds_msg_exch_),
                                      sizeof(generic_mds_message_.mds_msg_exch_));
              mds_bulk_writer_->Write(&(generic_mds_message_.generic_data_.eobi_of_data_),
                                      sizeof(generic_mds_message_.generic_data_.eobi_of_data_));
            } break;
            default: {
              std::cerr << " UNKNOWN PROCESSOR, SHOULD NEVER REACH HERE "
                        << "\n";

            } break;
          }

        } break;

        default: { std::cerr << " Mode Not Handled Yet : " << processing_mode_ << "\n"; } break;
      }
      mds_bulk_writer_->CheckToFlushBuffer();
    }
  }

 private:
  inline bool HasValidExchangeSymbol(HFSAT::ExchSource_t exch_source, std::string exchange_symbol) {
    auto it = valid_exchage_symbol_to_shortcode.find(exchange_symbol);

    if (it == valid_exchage_symbol_to_shortcode.end()) {
      return false;
    }

    std::string shortcode = it->second;
    // In case of NTP data, the exchange source defined in the SecDef is BMF, but generic_struct returns NTP
    // so the override the Exchange source with NTP
    if (exch_source == HFSAT::kExchSourceNTP) exch_source = HFSAT::kExchSourceBMF;

    // Skip data logging in case exchange source of shortcode is different than the one defined in security
    // definitions.
    return exch_source == HFSAT::SecurityDefinitions::GetContractExchSource(shortcode);
  }
};
}
}
