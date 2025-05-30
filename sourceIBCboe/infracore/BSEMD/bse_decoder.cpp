/**
    \file EobiD/eobi_decoder.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

 */

#include "infracore/BSEMD/bse_decoder.hpp"
#include "infracore/BSEMD/bse_md_processor.hpp"

namespace HFSAT {

EobiDecoder::EobiDecoder() {
  body_len_ = 0;
  template_id_ = 0;
}

void EobiDecoder::Decode(char *bytes, uint32_t len) {
  static BSE_MD_Processor::BSEMDProcessor &eobi_md_processor_ = BSE_MD_Processor::BSEMDProcessor::GetInstance();
  body_len_ = 0;

  // Process the packet header
  eobi_md_processor_.current_mkt_seg_id_ = *(int32_t *)(bytes + 12);

  while (len >= body_len_) {
    bytes += body_len_;
    len -= body_len_;

    if (len < EOBI_MSG_HEADER_SIZE) {
      break;
    }

    body_len_ = *(uint16_t *)(bytes + 0);     // Read unsigned int of size 2 bytes at offset 0 for body length
    template_id_ = *(uint16_t *)(bytes + 2);  // Read unsigned int of size 2 bytes at offset 2 for template id

    if (eobi_md_processor_.GetLiveSource()->IsLocalData()) {
//      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(2);
    }
    DecodeTemplate(bytes);
    if (eobi_md_processor_.GetLiveSource()->IsLocalData()) {
//      HFSAT::CpucycleProfiler::GetUniqueInstance().End(2);
    }
  }
}

// When in doubt about the semantics or the offsets,
// please refer to eurex_enhanced_order_book_interfaces pdf from eurex website
// http://www.eurexchange.com/exchange-en/technology/t7/system-documentation/
void EobiDecoder::DecodeTemplate(char *bytes) {
#if IS_LOGGING_ENABLE
  std::cout << "EobiDecoder::DecodeTemplate START" << std::endl;
#endif
  static BSE_MD_Processor::BSEMDProcessor &eobi_md_processor_ = BSE_MD_Processor::BSEMDProcessor::GetInstance();
  switch (template_id_) {
    case ORDER_ADD: {
#if IS_LOGGING_ENABLE
      std::cout << "ORDER_ADD:" << std::endl;
#endif
      eobi_md_processor_.o_security_id_ = *(int64_t *)(bytes + 16);

      //if (eobi_md_processor_.idmap_.find(eobi_md_processor_.o_security_id_) == eobi_md_processor_.idmap_.end()) return;
#if IS_LOGGING_ENABLE
      std::cout << "security_id_: " << eobi_md_processor_.o_security_id_ << std::endl;
#endif
      eobi_md_processor_.o_action_ = '0';
      eobi_md_processor_.o_seq_num_ = *(uint32_t *)(bytes + 4);
      eobi_md_processor_.o_trd_reg_ts_ = *(uint64_t *)(bytes + 8);
      eobi_md_processor_.o_priority_ts_ = *(uint64_t *)(bytes + 24);
      eobi_md_processor_.o_size_ = *(int32_t *)(bytes + 32);
      eobi_md_processor_.o_side_ = *(uint8_t *)(bytes + 36);
      eobi_md_processor_.o_price_ = *(int64_t *)(bytes + 40);
      eobi_md_processor_.o_intermediate_ = false;

#if IS_LOGGING_ENABLE
       char t_side_ = eobi_md_processor_.o_side_ == 1 ? 'B': 'S';
       std::cout << "ADD\t\t" << eobi_md_processor_.o_security_id_ /*eobi_md_processor_.idmap_[eobi_md_processor_.o_security_id_]<< "\t"*/ 
	        << "\t" << t_side_ << "\t"
		<< "Price: " << eobi_md_processor_.o_price_ << "\t"
                << eobi_md_processor_.o_price_/ EOBI_PRICE_DIVIDER << "\t" << eobi_md_processor_.o_size_ << "\t"
                << eobi_md_processor_.o_seq_num_ << std::endl;
#endif

#if IS_LOGGING_ENABLE
        std::cout << "eobi_md_processor_.initial_recovery_not_finished_: " << eobi_md_processor_.initial_recovery_not_finished_ << std::endl;
      if (eobi_md_processor_.initial_recovery_not_finished_) {
        eobi_md_processor_.CheckSequenceNum();
        //return;
      }
#endif

      eobi_md_processor_.ProcessOrder();
    } break;
    case ORDER_MODIFY: {
#if IS_LOGGING_ENABLE
      std::cout << "ORDER_MODIFY:" << std::endl;
#endif
      eobi_md_processor_.o_security_id_ = *(int64_t *)(bytes + 40);

      //if (eobi_md_processor_.idmap_.find(eobi_md_processor_.o_security_id_) == eobi_md_processor_.idmap_.end()) return;
#if IS_LOGGING_ENABLE
      std::cout << "security_id_: " << eobi_md_processor_.o_security_id_ << std::endl;
#endif

      eobi_md_processor_.o_action_ = '1';
      eobi_md_processor_.o_seq_num_ = *(uint32_t *)(bytes + 4);
      eobi_md_processor_.o_trd_reg_ts_ = *(uint64_t *)(bytes + 8);
      eobi_md_processor_.o_prev_priority_ts_ = *(uint64_t *)(bytes + 16);
      eobi_md_processor_.o_prev_price_ = *(int64_t *)(bytes + 24);
      eobi_md_processor_.o_prev_size_ = *(int32_t *)(bytes + 32);
      eobi_md_processor_.o_priority_ts_ = *(uint64_t *)(bytes + 48);
      eobi_md_processor_.o_size_ = *(int32_t *)(bytes + 56);
      eobi_md_processor_.o_side_ = *(uint8_t *)(bytes + 60);
      eobi_md_processor_.o_price_ = *(int64_t *)(bytes + 64);

#if IS_LOGGING_ENABLE
       char t_side_ = eobi_md_processor_.o_side_ == 1 ? 'B': 'S';
       std::cout << "MODIFY\t\t" << /*eobi_md_processor_.idmap_[eobi_md_processor_.o_security_id_]<< "\t" <<*/ t_side_ << "\t"
		<< "Price: " << eobi_md_processor_.o_price_ << "\t"
                << eobi_md_processor_.o_price_/ EOBI_PRICE_DIVIDER << "\t" << eobi_md_processor_.o_size_ << "\t"
                << eobi_md_processor_.o_seq_num_ << "\t"
                << eobi_md_processor_.o_prev_price_ /EOBI_PRICE_DIVIDER << "\t"
                << eobi_md_processor_.o_prev_size_ << "\t"
                << std::endl;

        std::cout << "eobi_md_processor_.initial_recovery_not_finished_: " << eobi_md_processor_.initial_recovery_not_finished_ << std::endl;
      if (eobi_md_processor_.initial_recovery_not_finished_) {
        eobi_md_processor_.CheckSequenceNum();
       // return;
      }
#endif

      eobi_md_processor_.ProcessOrder();
    } break;
    case ORDER_MODIFY_SAME_PRIORITY: {
#if IS_LOGGING_ENABLE
      std::cout << "ORDER_MODIFY_SAME_PRIORITY:" << std::endl;
#endif
      eobi_md_processor_.o_security_id_ = *(int64_t *)(bytes + 32);

#if IS_LOGGING_ENABLE
      //if (eobi_md_processor_.idmap_.find(eobi_md_processor_.o_security_id_) == eobi_md_processor_.idmap_.end()) return;
      std::cout << "security_id_: " << eobi_md_processor_.o_security_id_ << std::endl;
#endif

      eobi_md_processor_.o_action_ = '1';
      eobi_md_processor_.o_seq_num_ = *(uint32_t *)(bytes + 4);
      eobi_md_processor_.o_trd_reg_ts_ = *(uint64_t *)(bytes + 8);
      eobi_md_processor_.o_prev_size_ = *(int32_t *)(bytes + 24);
      eobi_md_processor_.o_priority_ts_ = *(uint64_t *)(bytes + 40);
      eobi_md_processor_.o_prev_priority_ts_ = *(uint64_t *)(bytes + 40);
      eobi_md_processor_.o_size_ = *(int32_t *)(bytes + 48);
      eobi_md_processor_.o_side_ = *(uint8_t *)(bytes + 52);
      eobi_md_processor_.o_price_ = *(int64_t *)(bytes + 56);
      eobi_md_processor_.o_prev_price_ = *(int64_t *)(bytes + 56);

       char t_side_ = eobi_md_processor_.o_side_ == 1 ? 'B': 'S';
#if IS_LOGGING_ENABLE
       std::cout << "MODIFY_SAME\t" 
		 //<< eobi_md_processor_.idmap_[eobi_md_processor_.o_security_id_]<< "\t" 
		 << t_side_ << "\t"
		<< "Price: " << eobi_md_processor_.o_price_ << "\t"
                << eobi_md_processor_.o_price_/ EOBI_PRICE_DIVIDER << "\t" << eobi_md_processor_.o_size_ << "\t"
                << eobi_md_processor_.o_seq_num_ << "\t"
                << eobi_md_processor_.o_prev_price_ /EOBI_PRICE_DIVIDER << "\t"
                << eobi_md_processor_.o_prev_size_ << "\t"
                << std::endl;

        std::cout << "eobi_md_processor_.initial_recovery_not_finished_: " << eobi_md_processor_.initial_recovery_not_finished_ << std::endl;
      if (eobi_md_processor_.initial_recovery_not_finished_) {
        eobi_md_processor_.CheckSequenceNum();
        return;
      }
#endif

      eobi_md_processor_.ProcessOrder();
    } break;
    case ORDER_DELETE: {
#if IS_LOGGING_ENABLE
      std::cout << "ORDER_DELETE:" << std::endl;
#endif
      eobi_md_processor_.o_security_id_ = *(int64_t *)(bytes + 24);

      //if (eobi_md_processor_.idmap_.find(eobi_md_processor_.o_security_id_) == eobi_md_processor_.idmap_.end()) return;
#if IS_LOGGING_ENABLE
      std::cout << "security_id_: " << eobi_md_processor_.o_security_id_ << std::endl;
#endif

      eobi_md_processor_.o_action_ = '2';
      eobi_md_processor_.o_seq_num_ = *(uint32_t *)(bytes + 4);
      eobi_md_processor_.o_trd_reg_ts_ = *(uint64_t *)(bytes + 8);
      eobi_md_processor_.o_priority_ts_ = *(uint64_t *)(bytes + 32);
      eobi_md_processor_.o_size_ = *(int32_t *)(bytes + 40);
      eobi_md_processor_.o_side_ = *(uint8_t *)(bytes + 44);
      eobi_md_processor_.o_price_ = *(int64_t *)(bytes + 48);

       char t_side_ = eobi_md_processor_.o_side_ == 1 ? 'B': 'S';
#if IS_LOGGING_ENABLE
       std::cout << "DELETE\t\t"
		 //<< eobi_md_processor_.idmap_[eobi_md_processor_.o_security_id_]<< "\t" 
		 << t_side_ << "\t"
                << eobi_md_processor_.o_price_/ EOBI_PRICE_DIVIDER << "\t" << eobi_md_processor_.o_size_ << "\t"
                << eobi_md_processor_.o_seq_num_ << std::endl;

      if (eobi_md_processor_.initial_recovery_not_finished_) {
        eobi_md_processor_.CheckSequenceNum();
        return;
      }
#endif

      eobi_md_processor_.ProcessOrder();
    } break;
    case ORDER_MASS_DELETE: {

#if IS_LOGGING_ENABLE
      std::cout << "ORDER_MASS_DELETE:" << std::endl;
#endif
      eobi_md_processor_.o_security_id_ = *(int64_t *)(bytes + 8);

#if IS_LOGGING_ENABLE
      std::cout << "security_id_: " << eobi_md_processor_.o_security_id_ << std::endl;
#endif
      //if (eobi_md_processor_.idmap_.find(eobi_md_processor_.o_security_id_) == eobi_md_processor_.idmap_.end()) return;

      eobi_md_processor_.o_action_ = '3';
      eobi_md_processor_.o_seq_num_ = *(uint32_t *)(bytes + 4);

#if IS_LOGGING_ENABLE
      if (eobi_md_processor_.initial_recovery_not_finished_) {
        eobi_md_processor_.CheckSequenceNum();
        return;
      }
#endif

      eobi_md_processor_.ProcessOrder();
    } break;
    case PARTIAL_EXECUTION: {
#if IS_LOGGING_ENABLE
      std::cout << "PARTIAL_EXECUTION:" << std::endl;
#endif
      eobi_md_processor_.o_security_id_ = *(int64_t *)(bytes + 32);

      //if (eobi_md_processor_.idmap_.find(eobi_md_processor_.o_security_id_) == eobi_md_processor_.idmap_.end()) return;

#if IS_LOGGING_ENABLE
      std::cout << "security_id_: " << eobi_md_processor_.o_security_id_ << std::endl;
#endif
      eobi_md_processor_.o_action_ = '4';
      eobi_md_processor_.o_seq_num_ = *(uint32_t *)(bytes + 4);
      eobi_md_processor_.o_side_ = *(uint8_t *)(bytes + 8);
      eobi_md_processor_.o_priority_ts_ = *(uint64_t *)(bytes + 24);
      eobi_md_processor_.o_size_ = *(int32_t *)(bytes + 44);
      eobi_md_processor_.o_price_ = *(int64_t *)(bytes + 48);

       char t_side_ = eobi_md_processor_.o_side_ == 1 ? 'B': 'S';

#if IS_LOGGING_ENABLE
       std::cout << "PARTIAL_EXEC\t" 
		 //<< eobi_md_processor_.idmap_[eobi_md_processor_.o_security_id_]<< "\t" 
		 << t_side_ << "\t"
                << eobi_md_processor_.o_price_/ EOBI_PRICE_DIVIDER << "\t" << eobi_md_processor_.o_size_ << "\t"
               << eobi_md_processor_.o_seq_num_ << std::endl;

      if (eobi_md_processor_.initial_recovery_not_finished_) {
        eobi_md_processor_.CheckSequenceNum();
        return;
      }
#endif

      eobi_md_processor_.ProcessOrder();
    } break;
    case FULL_EXECUTION: {
#if IS_LOGGING_ENABLE
      std::cout << "FULL_EXECUTION:" << std::endl;
#endif
      eobi_md_processor_.o_security_id_ = *(int64_t *)(bytes + 32);

      //if (eobi_md_processor_.idmap_.find(eobi_md_processor_.o_security_id_) == eobi_md_processor_.idmap_.end()) return;
#if IS_LOGGING_ENABLE
      std::cout << "security_id_: " << eobi_md_processor_.o_security_id_ << std::endl;
#endif

      eobi_md_processor_.o_action_ = '5';
      eobi_md_processor_.o_seq_num_ = *(uint32_t *)(bytes + 4);
      eobi_md_processor_.o_side_ = *(uint8_t *)(bytes + 8);
      eobi_md_processor_.o_priority_ts_ = *(uint64_t *)(bytes + 24);
      eobi_md_processor_.o_size_ = *(int32_t *)(bytes + 44);
      eobi_md_processor_.o_price_ = *(int64_t *)(bytes + 48);

       char t_side_ = eobi_md_processor_.o_side_ == 1 ? 'B': 'S';
#if IS_LOGGING_ENABLE
       std::cout << "FULL_EXEC\t" 
		 //<< eobi_md_processor_.idmap_[eobi_md_processor_.o_security_id_]<< "\t" 
		 << t_side_ << "\t"
                 << eobi_md_processor_.o_price_/ EOBI_PRICE_DIVIDER << "\t" << eobi_md_processor_.o_size_ << "\t"
                 << eobi_md_processor_.o_seq_num_ << std::endl;

      if (eobi_md_processor_.initial_recovery_not_finished_) {
        eobi_md_processor_.CheckSequenceNum();
        return;
      }
#endif

      eobi_md_processor_.ProcessOrder();
    } break;
    case EXECUTION_SUMMARY: {
#if IS_LOGGING_ENABLE
      std::cout << "EXECUTION_SUMMARY:" << std::endl;
#endif
      eobi_md_processor_.o_security_id_ = *(int64_t *)(bytes + 8);

      //if (eobi_md_processor_.idmap_.find(eobi_md_processor_.o_security_id_) == eobi_md_processor_.idmap_.end()) return;
#if IS_LOGGING_ENABLE
      std::cout << "security_id_: " << eobi_md_processor_.o_security_id_ << std::endl;
#endif

      eobi_md_processor_.o_action_ = '6';
      eobi_md_processor_.o_seq_num_ = *(uint32_t *)(bytes + 4);
      eobi_md_processor_.o_trd_reg_ts_ = *(uint64_t *)(bytes + 24);
      eobi_md_processor_.o_size_ = *(int32_t *)(bytes + 32); //40);
      eobi_md_processor_.o_side_ = *(uint8_t *)(bytes + 36); //44);
      eobi_md_processor_.o_price_ = *(int64_t *)(bytes + 40); //48);
      eobi_md_processor_.o_synthetic_match_ =
          *(uint8_t *)(bytes + 37); //45);  // Hack to get synthetic match indicator for EXEC summary

       char t_side_ = eobi_md_processor_.o_side_ == 1 ? 'B': 'S';
#if IS_LOGGING_ENABLE
       std::cout << "EXEC_SUMMARY:\t" << /*eobi_md_processor_.idmap_[eobi_md_processor_.o_security_id_]<<*/ "\t" << t_side_ << "\t"
                << eobi_md_processor_.o_price_/ EOBI_PRICE_DIVIDER << "\t" << eobi_md_processor_.o_size_ << "\t"
                << eobi_md_processor_.o_seq_num_ << "\t" << (int)eobi_md_processor_.o_synthetic_match_
                << "\t" << eobi_md_processor_.o_trd_reg_ts_ << std::endl;

      if (eobi_md_processor_.initial_recovery_not_finished_) {
        eobi_md_processor_.CheckSequenceNum();
        return;
      }
#endif

      eobi_md_processor_.ProcessOrder();
    } break;
    case PRODUCT_SUMMARY: {
#if IS_LOGGING_ENABLE
      std::cout << "PRODUCT_SUMMARY:" << std::endl;
#endif
      //eobi_md_processor_.mkt_seg_id_to_seq_num_[eobi_md_processor_.current_mkt_seg_id_] = *(uint32_t *)(bytes + 8);
      uint32_t seq_n = *(uint32_t *)(bytes + 8);

#if IS_LOGGING_ENABLE
       std::cout << "PROD_SUMMARY: LastMsgSeqNum:: " << seq_n << std::endl;
#endif
       //<< eobi_md_processor_.mkt_seg_id_to_seq_num_[eobi_md_processor_.current_mkt_seg_id_] << "\t"
       //<< eobi_md_processor_.mkt_seg_id_to_prod_name_[eobi_md_processor_.current_mkt_seg_id_]<< "\t"
       //         << eobi_md_processor_.mkt_seg_id_to_seq_num_[eobi_md_processor_.current_mkt_seg_id_]<< std::endl;
    } break;
    case SNAPSHOT_ORDER: {
#if IS_LOGGING_ENABLE
      std::cout << "SNAPSHOT_ORDER:" << std::endl;
#endif
      int64_t security_id_ = eobi_md_processor_.curr_sec_id_[eobi_md_processor_.current_mkt_seg_id_];
      //if (eobi_md_processor_.idmap_.find(security_id_) == eobi_md_processor_.idmap_.end()) return;

//      if (!eobi_md_processor_.security_in_recovery[security_id_]) return;
//      if (!eobi_md_processor_.instr_summry_rcvd_[security_id_]) return;

      eobi_md_processor_.o_action_ = '0';
      eobi_md_processor_.o_seq_num_ = *(uint32_t *)(bytes + 4);
      eobi_md_processor_.o_priority_ts_ = *(uint64_t *)(bytes + 8);
      eobi_md_processor_.o_size_ = *(int32_t *)(bytes + 16);
      eobi_md_processor_.o_side_ = *(uint8_t *)(bytes + 20);
      eobi_md_processor_.o_price_ = *(int64_t *)(bytes + 24);
      eobi_md_processor_.o_security_id_ = security_id_;
      eobi_md_processor_.o_intermediate_ = true;
      eobi_md_processor_.o_trd_reg_ts_ = 0;

       char t_side_ = eobi_md_processor_.o_side_ == 1 ? 'B': 'S';
#if IS_LOGGING_ENABLE
       std::cout << "SNAP:\t" << security_id_ << " " << eobi_md_processor_.o_security_id_ << "\t"
#endif
		 //<< eobi_md_processor_.idmap_[eobi_md_processor_.o_security_id_]<< "\t" 
		 << t_side_ << "\t"
                << eobi_md_processor_.o_price_/ EOBI_PRICE_DIVIDER << "\t" << eobi_md_processor_.o_size_ << "\t"
                << eobi_md_processor_.o_seq_num_ << std::endl;
#endif

      eobi_md_processor_.ProcessOrder();
/*
      if (eobi_md_processor_.o_seq_num_ == eobi_md_processor_.target_msg_seq_num_[security_id_]) {
        eobi_md_processor_.security_in_recovery[security_id_] = false;
        eobi_md_processor_.instr_summry_rcvd_[security_id_] = false;
        eobi_md_processor_.EndRecovery(security_id_);
      }
*/
    } break;
    case INSTRUMENT_SUMMARY: {
#if IS_LOGGING_ENABLE
      std::cout << "INSTRUMENT_SUMMARY:" << std::endl;
#endif
      uint32_t seq_no_ = *(uint32_t *)(bytes + 4);
      int64_t security_id_ = *(int64_t *)(bytes + 8);
      uint16_t total_orders_ = *(uint16_t *)(bytes + 32);

 //     if (!eobi_md_processor_.security_in_recovery[security_id_]) return;

      eobi_md_processor_.target_msg_seq_num_[security_id_] = seq_no_ + total_orders_;
      eobi_md_processor_.curr_sec_id_[eobi_md_processor_.current_mkt_seg_id_] = security_id_;
      eobi_md_processor_.instr_summry_rcvd_[security_id_] = true;

#if IS_LOGGING_ENABLE
       std::cout << "INSTR_SUMMARY:\t" << security_id_ << "\t" << eobi_md_processor_.current_mkt_seg_id_ << "\t"
        //<< eobi_md_processor_.mkt_seg_id_to_prod_name_[eobi_md_processor_.current_mkt_seg_id_]<< "\t"
                << eobi_md_processor_.target_msg_seq_num_[security_id_]<< std::endl;
/*
      if (total_orders_ == 0) {
        eobi_md_processor_.security_in_recovery[security_id_] = false;
        eobi_md_processor_.instr_summry_rcvd_[security_id_] = false;
        eobi_md_processor_.EndRecovery(security_id_);
      }
*/
    } break;
    default:
      break;
  }
#if IS_LOGGING_ENABLE
  std::cout << "EobiDecoder::DecodeTemplate END ******" << std::endl;
#endif
}
}
