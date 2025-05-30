// =====================================================================================
//
//       Filename:  generic_data_converter.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/05/2014 09:07:45 AM
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

#include <iostream>
#include <stdlib.h>
#include <map>
#include <set>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/bmf_common_message_defines.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/sgx_mds_defines.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

#include "dvccode/CommonTradeUtils/live_struct_sim_struct.hpp"

int main(int argc, char** argv) {
  if (argc != 7) {
    std::cerr << " USAGE: EXEC <exch> <YYYYMMDD> <in_file_path> MDS_DATA_PATH ORS_DATA_PATH ORS_EXCH " << std::endl;
    exit(0);
  }

  std::string exch_source_ = argv[1];
  HFSAT::MDS_MSG::MDSMessageExchType exch_source_type_ = HFSAT::MDS_MSG::INVALID;

  if (exch_source_ == "CME")
    exch_source_type_ = HFSAT::MDS_MSG::CME;
  else if (exch_source_ == "EUREX")
    exch_source_type_ = HFSAT::MDS_MSG::EUREX;
  else if (exch_source_ == "EOBIPriceFeed")
    exch_source_type_ = HFSAT::MDS_MSG::EUREX;
  else if (exch_source_ == "LIFFE")
    exch_source_type_ = HFSAT::MDS_MSG::LIFFE;
  else if (exch_source_ == "CHIX_L1")
    exch_source_type_ = HFSAT::MDS_MSG::CHIX_L1;
  else if (exch_source_ == "ORS")
    exch_source_type_ = HFSAT::MDS_MSG::ORS_REPLY;
  else if (exch_source_ == "CONTROL")
    exch_source_type_ = HFSAT::MDS_MSG::CONTROL;
  else if (exch_source_ == "RTS_PF")
    exch_source_type_ = HFSAT::MDS_MSG::RTS;
  else if (exch_source_ == "MICEX")
    exch_source_type_ = HFSAT::MDS_MSG::MICEX;
  else if (exch_source_ == "NTP")
    exch_source_type_ = HFSAT::MDS_MSG::NTP;
  else if (exch_source_ == "PUMA")
    exch_source_type_ = HFSAT::MDS_MSG::BMF_EQ;
  else if (exch_source_ == "OSEPriceFeed")
    exch_source_type_ = HFSAT::MDS_MSG::OSE_CF;
  else if (exch_source_ == "OSEOrderFeed")
    exch_source_type_ = HFSAT::MDS_MSG::OSE_CF;
  else if (exch_source_ == "HKEX")
    exch_source_type_ = HFSAT::MDS_MSG::HKEX;
  else if (exch_source_ == "CSM")
    exch_source_type_ = HFSAT::MDS_MSG::CSM;
  else if (exch_source_ == "OSE_L1")
    exch_source_type_ = HFSAT::MDS_MSG::OSE_L1;
  else if (exch_source_ == "TMX")
    exch_source_type_ = HFSAT::MDS_MSG::TMX;
  else if (exch_source_ == "ICE")
    exch_source_type_ = HFSAT::MDS_MSG::ICE;
  else if (exch_source_ == "EBS")
    exch_source_type_ = HFSAT::MDS_MSG::EBS;
  else if (exch_source_ == "HKOMD")
    exch_source_type_ = HFSAT::MDS_MSG::HKOMD;
  else if (exch_source_ == "HKOMDPF")
    exch_source_type_ = HFSAT::MDS_MSG::HKOMDPF;
  else if (exch_source_ == "HKOMDCPF")
    exch_source_type_ = HFSAT::MDS_MSG::HKOMDPF;
  else if (exch_source_ == "AFLASH")
    exch_source_type_ = HFSAT::MDS_MSG::AFLASH;
  else if (exch_source_ == "ASXPF")
    exch_source_type_ = HFSAT::MDS_MSG::ASX;
  else if (exch_source_ == "OSEPF")
    exch_source_type_ = HFSAT::MDS_MSG::OSE_ITCH_PF;
  else if (exch_source_ == "TMX_OBF_PF")
    exch_source_type_ = HFSAT::MDS_MSG::TMX_OBF;
  else if (exch_source_ == "SGX") {
    exch_source_type_ = HFSAT::MDS_MSG::SGX;
  } else if (exch_source_ == "NSEL1") {
    exch_source_type_ = HFSAT::MDS_MSG::NSE_L1;
  } else
    exch_source_type_ = HFSAT::MDS_MSG::INVALID;

  if (HFSAT::MDS_MSG::INVALID == exch_source_type_) {
    exit(-1);
  }

  int YYYYMMDD = atoi(argv[2]);

  HFSAT::BulkFileReader* bulk_file_reader_ = new HFSAT::BulkFileReader();
  bulk_file_reader_->open(argv[3]);

  std::string mds_data_path_ = argv[4];
  std::string ors_data_path_ = argv[5];

  std::string ors_exch_source = argv[6];

  std::map<std::string, HFSAT::FastPriceConvertor*> fast_price_convertor_map_;
  HFSAT::FastPriceConvertor* this_fast_price_convertor_;

  HFSAT::BulkFileWriter* bulk_file_writer_ = NULL;

  HFSAT::TradingLocation_t curr_location_ = HFSAT::TradingLocationUtils::GetTradingLocationFromHostname();

  // FR2 having direct EOBI PF
  if (curr_location_ == HFSAT::kTLocFR2) {
    if (exch_source_type_ == HFSAT::MDS_MSG::EUREX) {
      exch_source_type_ = HFSAT::MDS_MSG::EOBI_PF;
    }
  }

  // Mos to have LIFFE as live
  if (curr_location_ == HFSAT::kTLocM1 || curr_location_ == HFSAT::kTLocBMF) {
    if (exch_source_type_ == HFSAT::MDS_MSG::LIFFE) {
      exch_source_type_ = HFSAT::MDS_MSG::LIFFE_LS;
    }
  }

  if (curr_location_ == HFSAT::kTLocBMF || curr_location_ == HFSAT::kTLocM1 || curr_location_ == HFSAT::kTLocJPY ||
      curr_location_ == HFSAT::kTLocTMX) {
    if (exch_source_type_ == HFSAT::MDS_MSG::ICE) {
      exch_source_type_ = HFSAT::MDS_MSG::ICE_LS;
    }
  }

  CME_MDS::CMECommonStruct cme_event_;
  EUREX_MDS::EUREXCommonStruct eurex_event_;
  LIFFE_MDS::LIFFECommonStruct liffe_event_;
  HFSAT::BATSCHI_PL_MDS::BatsChiPLCommonStruct chix_l1_event_;
  HFSAT::GenericORSReplyStruct ors_reply_event_;
  HFSAT::GenericControlRequestStruct control_struct_;
  RTS_MDS::RTSCommonStruct rts_event_;
  MICEX_MDS::MICEXCommonStruct micex_event_;
  NTP_MDS::NTPCommonStruct ntp_event_;
  NTP_MDS::NTPCommonStruct bmf_eq_event_;
  OSE_MDS::OSEPriceFeedCommonStruct ose_pf_event_;
  HKEX_MDS::HKEXCommonStruct hkex_event_;
  CSM_MDS::CSMCommonStruct csm_event_;
  OSE_MDS::OSEPLCommonStruct ose_l1_event_;
  OSE_MDS::OSECombinedCommonStruct ose_cf_event_;
  OSE_MDS::OSECommonStruct ose_event_;
  TMX_MDS::TMXCommonStruct tmx_event_;
  ICE_MDS::ICECommonStructLive ice_event_;
  HKOMD_MDS::HKOMDPFCommonStruct hkomd_pf_event_;
  AFLASH_MDS::AFlashCommonStructLive aflash_event_;
  ASX_MDS::ASXPFCommonStruct asx_event_;
  OSE_ITCH_MDS::OSEPFCommonStruct ose_itch_event_;
  SGX_MDS::SGXPFCommonStruct sgx_event_;
  TMX_OBF_MDS::TMXPFCommonStruct tmx_obf_event_;
  HFSAT::GenericL1DataStruct nse_l1_event_;

  if (bulk_file_reader_->is_open()) {
    HFSAT::MDS_MSG::GenericMDSMessage next_event_;

    while (true) {
      size_t available_length_ = bulk_file_reader_->read(&next_event_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));

      if (available_length_ < sizeof(next_event_)) break;

      // Never expect mixed type of data
      if (next_event_.mds_msg_exch_ != exch_source_type_) continue;

      if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::CME) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&cme_event_, &(next_event_.generic_data_.cme_data_), sizeof(CME_MDS::CMECommonStruct));
        bulk_file_writer_->Write(&cme_event_, sizeof(cme_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::LIFFE ||
                 next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::LIFFE_LS) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          std::string amr_code_ = next_event_.getContract();
          std::replace(amr_code_.begin(), amr_code_.end(), ' ', '~');
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << amr_code_ << "_" << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&(liffe_event_), &(next_event_.generic_data_.liffe_data_), sizeof(LIFFE_MDS::LIFFECommonStruct));
        bulk_file_writer_->Write(&liffe_event_, sizeof(liffe_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::NTP) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&(ntp_event_), &(next_event_.generic_data_.ntp_data_), sizeof(NTP_MDS::NTPCommonStruct));
        bulk_file_writer_->Write(&ntp_event_, sizeof(ntp_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::BMF_EQ) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&(bmf_eq_event_), &(next_event_.generic_data_.bmf_eq_data_), sizeof(NTP_MDS::NTPCommonStruct));
        bulk_file_writer_->Write(&bmf_eq_event_, sizeof(bmf_eq_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::AFLASH) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&aflash_event_, &next_event_.generic_data_.aflash_data_, sizeof(aflash_event_));

        bulk_file_writer_->Write(&aflash_event_, sizeof(aflash_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::ASX) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&(asx_event_), &(next_event_.generic_data_.asx_data_), sizeof(ASX_MDS::ASXPFCommonStruct));
        bulk_file_writer_->Write(&asx_event_, sizeof(asx_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::SGX) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&(sgx_event_), &(next_event_.generic_data_.sgx_data_), sizeof(SGX_MDS::SGXPFCommonStruct));
        bulk_file_writer_->Write(&sgx_event_, sizeof(sgx_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::OSE_ITCH_PF) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&(ose_itch_event_), &(next_event_.generic_data_.ose_itch_pf_data_),
               sizeof(OSE_ITCH_MDS::OSEPFCommonStruct));
        bulk_file_writer_->Write(&ose_itch_event_, sizeof(ose_itch_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::TMX_OBF) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&(tmx_obf_event_), &(next_event_.generic_data_.tmx_obf_data_), sizeof(TMX_OBF_MDS::TMXPFCommonStruct));
        bulk_file_writer_->Write(&tmx_obf_event_, sizeof(tmx_obf_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::NSE_L1) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&(nse_l1_event_), &(next_event_.generic_data_.nse_l1_data_), sizeof(HFSAT::GenericL1DataStruct));
        bulk_file_writer_->Write(&nse_l1_event_, sizeof(nse_l1_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::EUREX ||
                 next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::EOBI_PF) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&(eurex_event_), &(next_event_.generic_data_.eurex_data_), sizeof(EUREX_MDS::EUREXCommonStruct));
        bulk_file_writer_->Write(&eurex_event_, sizeof(eurex_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::CHIX_L1) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&(chix_l1_event_), &(next_event_.generic_data_.chix_l1_data_),
               sizeof(HFSAT::BATSCHI_PL_MDS::BatsChiPLCommonStruct));
        bulk_file_writer_->Write(&chix_l1_event_, sizeof(chix_l1_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::ORS_REPLY) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          std::string amr_code_ = next_event_.generic_data_.ors_reply_data_.symbol_;
          std::replace(amr_code_.begin(), amr_code_.end(), ' ', '~');
          file_name_with_path_ << ors_data_path_ << "/" << ors_exch_source << "/" << amr_code_ << "_" << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        HFSAT::Utils::ConvertORSLiveStructToSimStruct(next_event_.generic_data_.ors_reply_data_, ors_reply_event_);

        // We cannot do memcpy of the whole struct as the struct types are different (Ordering of struct member
        // variables).

        bulk_file_writer_->Write(&ors_reply_event_, sizeof(ors_reply_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::RTS) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&rts_event_, &(next_event_.generic_data_.rts_data_), sizeof(RTS_MDS::RTSCommonStruct));
        bulk_file_writer_->Write(&rts_event_, sizeof(rts_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::MICEX) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&micex_event_, &(next_event_.generic_data_.micex_data_), sizeof(MICEX_MDS::MICEXCommonStruct));
        bulk_file_writer_->Write(&micex_event_, sizeof(micex_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (HFSAT::MDS_MSG::OSE_PF == next_event_.mds_msg_exch_) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&(ose_pf_event_), &(next_event_.generic_data_.ose_pf_data_), sizeof(OSE_MDS::OSEPriceFeedCommonStruct));
        bulk_file_writer_->Write(&ose_pf_event_, sizeof(ose_pf_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (HFSAT::MDS_MSG::OSE_CF == next_event_.mds_msg_exch_) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&(ose_cf_event_), &(next_event_.generic_data_.ose_cf_data_), sizeof(OSE_MDS::OSECombinedCommonStruct));

        if ("OSEOrderFeed" == exch_source_) {
          memset((void*)&(ose_event_), 0, sizeof(OSE_MDS::OSECommonStruct));
          ose_event_.time_ = ose_cf_event_.time_;

          if (OSE_MDS::FEED_TRADE == ose_cf_event_.feed_msg_type_) {
            ose_event_.msg_ = OSE_MDS::OSE_TRADE;
            memcpy((void*)ose_event_.data_.ose_trds_.contract_, (void*)ose_cf_event_.contract_,
                   OSE_NEW_MDS_CONTRACT_TEXT_SIZE);
            ose_event_.data_.ose_trds_.trd_qty_ = ose_cf_event_.agg_size_;
            ose_event_.data_.ose_trds_.trd_px_ = ose_cf_event_.price_;
            ose_event_.data_.ose_trds_.seq_num = ose_cf_event_.message_sequence_;

            bulk_file_writer_->Write(&ose_event_, sizeof(ose_event_));
            bulk_file_writer_->CheckToFlushBuffer();

          } else if (OSE_MDS::FEED_TRADE_ORDER == ose_cf_event_.feed_msg_type_) {
            ose_event_.msg_ = OSE_MDS::OSE_TRADE_DELTA;
            memcpy((void*)ose_event_.data_.ose_dels_.contract_, (void*)ose_cf_event_.contract_,
                   OSE_NEW_MDS_CONTRACT_TEXT_SIZE);
            ose_event_.data_.ose_dels_.level_ = ose_cf_event_.order_feed_level_;
            ose_event_.data_.ose_dels_.qty_diff_ = ose_cf_event_.agg_size_;
            ose_event_.data_.ose_dels_.price_ = ose_cf_event_.price_;
            ose_event_.data_.ose_dels_.type_ = ose_cf_event_.type_;
            ose_event_.data_.ose_dels_.action_ = ose_cf_event_.action_;
            ose_event_.data_.ose_dels_.order_num = ose_cf_event_.exch_order_seq_;
            ose_event_.data_.ose_dels_.seq_num = ose_cf_event_.message_sequence_;

            bulk_file_writer_->Write(&ose_event_, sizeof(ose_event_));
            bulk_file_writer_->CheckToFlushBuffer();

          } else {
            ose_event_.msg_ = OSE_MDS::OSE_DELTA;
            memcpy((void*)ose_event_.data_.ose_dels_.contract_, (void*)ose_cf_event_.contract_,
                   OSE_NEW_MDS_CONTRACT_TEXT_SIZE);
            ose_event_.data_.ose_dels_.level_ = ose_cf_event_.order_feed_level_;
            ose_event_.data_.ose_dels_.qty_diff_ = ose_cf_event_.qty_diff_;
            ose_event_.data_.ose_dels_.price_ = ose_cf_event_.price_;
            ose_event_.data_.ose_dels_.type_ = ose_cf_event_.type_;
            ose_event_.data_.ose_dels_.action_ = ose_cf_event_.action_;
            ose_event_.data_.ose_dels_.order_num = ose_cf_event_.exch_order_seq_;
            ose_event_.data_.ose_dels_.seq_num = ose_cf_event_.message_sequence_;

            bulk_file_writer_->Write(&ose_event_, sizeof(ose_event_));
            bulk_file_writer_->CheckToFlushBuffer();
          }

        } else {
          if (false == ose_cf_event_.is_pricefeed_) continue;

          memset((void*)&(ose_pf_event_), 0, sizeof(OSE_MDS::OSEPriceFeedCommonStruct));
          ose_pf_event_.time_ = ose_cf_event_.time_;
          memcpy((void*)ose_pf_event_.contract_, (void*)ose_cf_event_.contract_, OSE_NEW_MDS_CONTRACT_TEXT_SIZE);
          ose_pf_event_.exch_order_seq_ = ose_cf_event_.exch_order_seq_;
          ose_pf_event_.price = ose_cf_event_.price_;
          ose_pf_event_.size = ose_cf_event_.agg_size_;
          ose_pf_event_.order_count_ = ose_cf_event_.order_count_;
          ose_pf_event_.type_ = (OSE_MDS::FEED_TRADE == ose_cf_event_.feed_msg_type_) ? 2 : ose_cf_event_.type_ - 1;
          ose_pf_event_.price_level_ = ose_cf_event_.price_feed_level_;
          ose_pf_event_.price_feed_msg_type_ = OSE_MDS::OSEPriceFeedMsgType(
              OSE_MDS::FEED_INVALID == ose_cf_event_.feed_msg_type_ ? OSE_MDS::PRICEFEED_INVALID
                                                                    : (int)ose_cf_event_.feed_msg_type_ - 1);

          bulk_file_writer_->Write(&ose_pf_event_, sizeof(ose_pf_event_));
          bulk_file_writer_->CheckToFlushBuffer();
        }

      } else if (HFSAT::MDS_MSG::OSE_L1 == next_event_.mds_msg_exch_) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&(ose_l1_event_), &(next_event_.generic_data_.ose_l1_data_), sizeof(OSE_MDS::OSEPLCommonStruct));
        bulk_file_writer_->Write(&ose_l1_event_, sizeof(ose_l1_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (HFSAT::MDS_MSG::HKEX == next_event_.mds_msg_exch_) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&(hkex_event_), &(next_event_.generic_data_.hkex_data_), sizeof(HKEX_MDS::HKEXCommonStruct));
        bulk_file_writer_->Write(&hkex_event_, sizeof(hkex_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (HFSAT::MDS_MSG::CSM == next_event_.mds_msg_exch_) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&csm_event_, &(next_event_.generic_data_.csm_data_), sizeof(CSM_MDS::CSMCommonStruct));
        bulk_file_writer_->Write(&csm_event_, sizeof(csm_event_));
        bulk_file_writer_->CheckToFlushBuffer();

      } else if (HFSAT::MDS_MSG::TMX == next_event_.mds_msg_exch_) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        if (fast_price_convertor_map_.empty()) {
          HFSAT::SecurityDefinitions& security_definitions_ =
              HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());
          HFSAT::ShortcodeContractSpecificationMap this_contract_specification_map_ =
              security_definitions_.contract_specification_map_;
          HFSAT::ShortcodeContractSpecificationMapCIter_t itr_ = this_contract_specification_map_.begin();

          HFSAT::ExchangeSymbolManager::SetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());

          for (itr_ = this_contract_specification_map_.begin(); itr_ != this_contract_specification_map_.end();
               itr_++) {
            std::string shortcode_ = (itr_->first);
            HFSAT::ContractSpecification contract_spec_ = (itr_->second);

            if (contract_spec_.exch_source_ != HFSAT::kExchSourceTMX) {
              continue;
            }

            std::string this_exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
            fast_price_convertor_map_[this_exch_symbol_] =
                new HFSAT::FastPriceConvertor(contract_spec_.min_price_increment_);
          }
        }

        if (fast_price_convertor_map_.find(next_event_.generic_data_.tmx_data_.contract_) ==
            fast_price_convertor_map_.end()) {
          continue;
        }
        this_fast_price_convertor_ = fast_price_convertor_map_[next_event_.generic_data_.tmx_data_.contract_];

        memset(&tmx_event_, 0, sizeof(tmx_event_));
        tmx_event_.time_ = next_event_.time_;

        switch (next_event_.generic_data_.tmx_data_.type_) {
          case 1: {
            tmx_event_.msg_ = TMX_MDS::TMX_TRADE;
            memcpy(tmx_event_.data_.tmx_trds_.contract_, next_event_.generic_data_.tmx_data_.contract_, 14);
            tmx_event_.data_.tmx_trds_.trd_px_ =
                this_fast_price_convertor_->GetDoublePx(next_event_.generic_data_.tmx_data_.bid_int_pxs_[0]);
            tmx_event_.data_.tmx_trds_.trd_qty_ = next_event_.generic_data_.tmx_data_.bid_szs_[0];
            tmx_event_.data_.tmx_trds_.type_ = next_event_.generic_data_.tmx_data_.num_levels_;
          } break;

          case 2: {
            tmx_event_.msg_ = TMX_MDS::TMX_QUOTE;
            memcpy(tmx_event_.data_.tmx_qts_.contract_, next_event_.generic_data_.tmx_data_.contract_, 14);
            tmx_event_.data_.tmx_qts_.bid_px_ =
                this_fast_price_convertor_->GetDoublePx(next_event_.generic_data_.tmx_data_.bid_int_pxs_[0]);
            tmx_event_.data_.tmx_qts_.bid_sz_ = next_event_.generic_data_.tmx_data_.bid_szs_[0];
            tmx_event_.data_.tmx_qts_.ask_px_ =
                this_fast_price_convertor_->GetDoublePx(next_event_.generic_data_.tmx_data_.ask_int_pxs_[0]);
            tmx_event_.data_.tmx_qts_.ask_sz_ = next_event_.generic_data_.tmx_data_.ask_szs_[0];
            tmx_event_.data_.tmx_qts_.status_ = next_event_.generic_data_.tmx_data_.num_levels_;
          } break;

          case 3:
          case 4:
          case 5:
          case 6: {
            tmx_event_.msg_ = TMX_MDS::TMX_BOOK;
            memcpy(tmx_event_.data_.tmx_books_.contract_, next_event_.generic_data_.tmx_data_.contract_, 14);
            tmx_event_.data_.tmx_books_.num_levels_ = next_event_.generic_data_.tmx_data_.num_levels_;

            for (int i = 0; i < 5; i++) {
              tmx_event_.data_.tmx_books_.bid_pxs_[i] =
                  this_fast_price_convertor_->GetDoublePx(next_event_.generic_data_.tmx_data_.bid_int_pxs_[i]);
              tmx_event_.data_.tmx_books_.bid_szs_[i] = next_event_.generic_data_.tmx_data_.bid_szs_[i];
              tmx_event_.data_.tmx_books_.num_bid_ords_[i] = next_event_.generic_data_.tmx_data_.num_bid_ords_[i];

              tmx_event_.data_.tmx_books_.ask_pxs_[i] =
                  this_fast_price_convertor_->GetDoublePx(next_event_.generic_data_.tmx_data_.ask_int_pxs_[i]);
              tmx_event_.data_.tmx_books_.ask_szs_[i] = next_event_.generic_data_.tmx_data_.ask_szs_[i];
              tmx_event_.data_.tmx_books_.num_ask_ords_[i] = next_event_.generic_data_.tmx_data_.num_ask_ords_[i];
            }

            if (next_event_.generic_data_.tmx_data_.type_ == 3) {
              tmx_event_.data_.tmx_books_.status_ = 'T';
            } else if (next_event_.generic_data_.tmx_data_.type_ == 4) {
              tmx_event_.data_.tmx_books_.status_ = 'Y';
            } else if (next_event_.generic_data_.tmx_data_.type_ == 5) {
              tmx_event_.data_.tmx_books_.status_ = 'O';
            } else if (next_event_.generic_data_.tmx_data_.type_ == 6) {
              tmx_event_.data_.tmx_books_.status_ = 'E';  // place holder for all other types of status
            }
          } break;

          default:  // Invalid
          {
          } break;
        }

        bulk_file_writer_->Write(&tmx_event_, sizeof(tmx_event_));
        bulk_file_writer_->CheckToFlushBuffer();
      }

      else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::ICE ||
               next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::ICE_LS) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          std::string amr_code_ = next_event_.getContract();
          std::replace(amr_code_.begin(), amr_code_.end(), ' ', '~');
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << amr_code_ << "_" << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&ice_event_, &next_event_.generic_data_.ice_data_, sizeof(ice_event_));

        bulk_file_writer_->Write(&ice_event_, sizeof(ice_event_));
        bulk_file_writer_->CheckToFlushBuffer();
      } else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::HKOMDPF) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          std::string amr_code_ = next_event_.getContract();
          std::replace(amr_code_.begin(), amr_code_.end(), '/', '~');
          std::replace(amr_code_.begin(), amr_code_.end(), ' ', '~');
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << amr_code_ << "_" << YYYYMMDD;
          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&hkomd_pf_event_, &next_event_.generic_data_.hkomd_pf_data_, sizeof(HKOMD_MDS::HKOMDPFCommonStruct));
        bulk_file_writer_->Write(&hkomd_pf_event_, sizeof(hkomd_pf_event_));
        bulk_file_writer_->CheckToFlushBuffer();
      } else if (next_event_.mds_msg_exch_ == HFSAT::MDS_MSG::CONTROL) {
        if (bulk_file_writer_ == NULL) {
          std::ostringstream file_name_with_path_;
          file_name_with_path_ << mds_data_path_ << "/" << exch_source_ << "/" << next_event_.getContract() << "_"
                               << YYYYMMDD;

          bulk_file_writer_ = new HFSAT::BulkFileWriter();
          bulk_file_writer_->Open((file_name_with_path_.str()).c_str(), std::ofstream::binary | std::ofstream::out);
        }

        memcpy(&control_struct_, &(next_event_.generic_data_.control_data_),
               sizeof(HFSAT::GenericControlRequestStruct));
        bulk_file_writer_->Write(&control_struct_, sizeof(control_struct_));
        bulk_file_writer_->CheckToFlushBuffer();
      }
    }
  }

  bulk_file_reader_->close();

  if (bulk_file_writer_) bulk_file_writer_->Close();

  return 0;
}
