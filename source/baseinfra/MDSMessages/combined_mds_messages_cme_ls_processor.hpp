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

namespace HFSAT {

class CombinedMDSMessagesCMELSProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  PriceLevelGlobalListener*
      p_price_level_global_listener_;  ///< Listeners of CME_LS messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;

  std::map<uint8_t, std::string> product_code_to_shortcode_;
  std::map<std::string, std::string> shortcode_to_exchange_symbol_;

  struct timeval msg_received_time_;

  std::set<unsigned int> is_processing_quincy_source_for_this_security_;
  HFSAT::HybridSecurityManager* hybrid_security_manager_;

 public:
  CombinedMDSMessagesCMELSProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_,
                                    HFSAT::HybridSecurityManager* _hybrid_security_manager_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_time_keeper_(NULL),
        product_code_to_shortcode_(),
        shortcode_to_exchange_symbol_(),
        is_processing_quincy_source_for_this_security_(),
        hybrid_security_manager_(_hybrid_security_manager_)

  {
    // For Reducing the bandwidth consumption & jittery latency

    std::ifstream livesource_contractcode_shortcode_mapping_file_;

    livesource_contractcode_shortcode_mapping_file_.open(DEF_CME_LS_PRODUCTCODE_SHORTCODE_);

    if (!livesource_contractcode_shortcode_mapping_file_.is_open()) {
      std::cerr << " Couldn't open broadcast product list file : " << DEF_CME_LS_PRODUCTCODE_SHORTCODE_ << "\n";
      exit(1);
    }

    std::cerr << " Read Cntract Code To Shortcode List File : " << DEF_CME_LS_PRODUCTCODE_SHORTCODE_ << "\n";

    char productcode_shortcode_line_[1024];
    int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

    while (livesource_contractcode_shortcode_mapping_file_.good()) {
      livesource_contractcode_shortcode_mapping_file_.getline(productcode_shortcode_line_, 1024);

      std::string check_for_comment_ = productcode_shortcode_line_;

      if (check_for_comment_.find("#") != std::string::npos) continue;  // comments

      HFSAT::PerishableStringTokenizer st_(productcode_shortcode_line_, 1024);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() != 2) continue;  // mal formatted

      uint8_t this_contract_code_ = uint8_t(atoi(tokens_[0]));
      std::string this_shortcode_ = tokens_[1];
      std::string this_exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(this_shortcode_);

      // don't really need two maps, can use only one
      if (product_code_to_shortcode_.find(this_contract_code_) == product_code_to_shortcode_.end()) {
        product_code_to_shortcode_[this_contract_code_] = this_shortcode_;

        if (shortcode_to_exchange_symbol_.find(this_shortcode_) == shortcode_to_exchange_symbol_.end()) {
          shortcode_to_exchange_symbol_[this_shortcode_] = this_exch_symbol_;
        }
      }
    }

    livesource_contractcode_shortcode_mapping_file_.close();
  }

  ~CombinedMDSMessagesCMELSProcessor() {}

  inline void SetPriceLevelGlobalListener(PriceLevelGlobalListener* p_new_listener_) {
    p_price_level_global_listener_ = p_new_listener_;
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline void ExcludeSecurityFromProcessing(unsigned int _security_id_) {
    is_processing_quincy_source_for_this_security_.insert(_security_id_);
  }

  inline void ProcessCMEBookDelta(int& security_id_, CME_MDS::CMELSCommonStruct* next_event_) {
    if (next_event_->data_.cme_dels_.level_ > 0) {  // ignoring level 0 events right now

      TradeType_t _buysell_ = TradeType_t(next_event_->data_.cme_dels_.type_ - '0');

      switch (next_event_->data_.cme_dels_.action_) {
        case '0': {
          p_price_level_global_listener_->OnPriceLevelNew(
              security_id_, _buysell_, next_event_->data_.cme_dels_.level_, next_event_->data_.cme_dels_.price_,
              next_event_->data_.cme_dels_.size_, next_event_->data_.cme_dels_.num_ords_,
              next_event_->data_.cme_dels_.intermediate_);
        } break;
        case '1': {
          p_price_level_global_listener_->OnPriceLevelChange(
              security_id_, _buysell_, next_event_->data_.cme_dels_.level_, next_event_->data_.cme_dels_.price_,
              next_event_->data_.cme_dels_.size_, next_event_->data_.cme_dels_.num_ords_,
              next_event_->data_.cme_dels_.intermediate_);
        } break;
        case '2': {
          p_price_level_global_listener_->OnPriceLevelDelete(
              security_id_, _buysell_, next_event_->data_.cme_dels_.level_, next_event_->data_.cme_dels_.price_,
              next_event_->data_.cme_dels_.intermediate_);
        } break;
        case '9': {
          p_price_level_global_listener_->OnPriceLevelDeleteSynthetic(
              security_id_, _buysell_, next_event_->data_.cme_dels_.price_, next_event_->data_.cme_dels_.intermediate_);
        } break;
        default: {
          fprintf(stderr, "Weird message type in CMELiveDataSource::ProcessAllEvents CME_DELTA %d \n",
                  (int)next_event_->msg_);
        } break;
      }
    }
  }

  inline void ProcessCMELSEvent(CME_MDS::CMELSCommonStruct* next_event_, struct timeval& _shm_writer_time_) {
    switch (next_event_->msg_) {
      case CME_MDS::CME_DELTA: {
        if (product_code_to_shortcode_.find(next_event_->data_.cme_dels_.contract_code_) ==
            product_code_to_shortcode_.end()) {
          break;
        }

        if (shortcode_to_exchange_symbol_.find(
                product_code_to_shortcode_[next_event_->data_.cme_dels_.contract_code_]) ==
            shortcode_to_exchange_symbol_.end()) {
          break;
        }

        int security_id_ = sec_name_indexer_.GetIdFromSecname(
            shortcode_to_exchange_symbol_[product_code_to_shortcode_[next_event_->data_.cme_dels_.contract_code_]]
                .c_str());

        if (security_id_ < 0) break;

        //        // Since QUincy Source is Added For this Source, discard processing
        //        if (is_processing_quincy_source_for_this_security_.find(security_id_) !=
        //            is_processing_quincy_source_for_this_security_.end())
        //          return;

        p_time_keeper_->OnTimeReceived(_shm_writer_time_);

        ProcessCMEBookDelta(security_id_, next_event_);
        // Call the same function for hybrid security
        if (hybrid_security_manager_ != NULL) {
          int hybrid_sec_id = hybrid_security_manager_->GetHybridSecurityIdFromActual(security_id_);
          if (hybrid_sec_id != -1) {
            ProcessCMEBookDelta(hybrid_sec_id, next_event_);
          }
        }

      } break;
      case CME_MDS::CME_TRADE: {
        if (product_code_to_shortcode_.find(next_event_->data_.cme_trds_.contract_code_) ==
            product_code_to_shortcode_.end()) {
          break;
        }

        if (shortcode_to_exchange_symbol_.find(
                product_code_to_shortcode_[next_event_->data_.cme_trds_.contract_code_]) ==
            shortcode_to_exchange_symbol_.end()) {
          break;
        }

        int security_id_ = sec_name_indexer_.GetIdFromSecname(
            shortcode_to_exchange_symbol_[product_code_to_shortcode_[next_event_->data_.cme_trds_.contract_code_]]
                .c_str());

        if (security_id_ < 0) break;

        //        // Since QUincy Source is Added For this Source, discard processing
        //        if (is_processing_quincy_source_for_this_security_.find(security_id_) !=
        //            is_processing_quincy_source_for_this_security_.end())
        //          return;

        p_time_keeper_->OnTimeReceived(_shm_writer_time_);

        TradeType_t _buysell_ =
            ((next_event_->data_.cme_trds_.agg_side_ == 1)
                 ? (kTradeTypeBuy)
                 : ((next_event_->data_.cme_trds_.agg_side_ == 2) ? kTradeTypeSell : kTradeTypeNoInfo));

        p_price_level_global_listener_->OnTrade(security_id_, next_event_->data_.cme_trds_.trd_px_,
                                                next_event_->data_.cme_trds_.trd_qty_, _buysell_);
        // Call the same function for hybrid security
        if (hybrid_security_manager_ != NULL) {
          int hybrid_sec_id = hybrid_security_manager_->GetHybridSecurityIdFromActual(security_id_);
          if (hybrid_sec_id != -1) {
            p_price_level_global_listener_->OnTrade(hybrid_sec_id, next_event_->data_.cme_trds_.trd_px_,
                                                    next_event_->data_.cme_trds_.trd_qty_, _buysell_);
          }
        }
      } break;
      default: {
        fprintf(stderr, "Weird message type in CMELoggedMessageFileSource::ProcessAllEvents %d \n",
                (int)next_event_->msg_);
      } break;
    }
  }
};
}
