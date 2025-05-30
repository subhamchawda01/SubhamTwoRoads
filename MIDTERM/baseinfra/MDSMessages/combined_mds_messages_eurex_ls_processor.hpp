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

class CombinedMDSMessagesEUREXLSProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  PriceLevelGlobalListener*
      p_price_level_global_listener_;  ///< Listeners of EUREX_LS messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;

  std::map<uint8_t, std::string> product_code_to_shortcode_;
  std::map<std::string, std::string> shortcode_to_exchange_symbol_;

  struct timeval msg_received_time_;

 public:
  CombinedMDSMessagesEUREXLSProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_time_keeper_(NULL),
        product_code_to_shortcode_(),
        shortcode_to_exchange_symbol_()

  {
    // For Reducing the bandwidth consumption & jittery latency

    std::ifstream livesource_contractcode_shortcode_mapping_file_;

    livesource_contractcode_shortcode_mapping_file_.open(DEF_LS_PRODUCTCODE_SHORTCODE_);

    if (!livesource_contractcode_shortcode_mapping_file_.is_open()) {
      std::cerr << " Couldn't open broadcast product list file : " << DEF_LS_PRODUCTCODE_SHORTCODE_ << "\n";
      exit(1);
    }

    std::cerr << " Read Cntract Code To Shortcode List File : " << DEF_LS_PRODUCTCODE_SHORTCODE_ << "\n";

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

  ~CombinedMDSMessagesEUREXLSProcessor() {}

  inline void SetPriceLevelGlobalListener(PriceLevelGlobalListener* p_new_listener_) {
    p_price_level_global_listener_ = p_new_listener_;
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline void ProcessEUREXLSEvent(EUREX_MDS::EUREXLSCommonStruct* next_event_, struct timeval& _shm_writer_time_) {
    switch (next_event_->msg_) {
      case EUREX_MDS::EUREX_DELTA: {
        if (product_code_to_shortcode_.find(next_event_->data_.eurex_dels_.contract_code_) ==
            product_code_to_shortcode_.end()) {
          std::cout << "Product Not Found : " << (int)next_event_->data_.eurex_dels_.contract_code_ << "\n";
          break;
        }

        if (shortcode_to_exchange_symbol_.find(
                product_code_to_shortcode_[next_event_->data_.eurex_dels_.contract_code_]) ==
            shortcode_to_exchange_symbol_.end()) {
          std::cout << "shortcode : " << product_code_to_shortcode_[next_event_->data_.eurex_dels_.contract_code_]
                    << "\n";
          break;
        }

        int security_id_ = sec_name_indexer_.GetIdFromSecname(
            shortcode_to_exchange_symbol_[product_code_to_shortcode_[next_event_->data_.eurex_dels_.contract_code_]]
                .c_str());

        if (security_id_ < 0) return;

        if (!next_event_->data_.eurex_dels_.intermediate_) {
          p_time_keeper_->OnTimeReceived(_shm_writer_time_);
        }

        if (next_event_->data_.eurex_dels_.level_ > 0) {
          TradeType_t _buysell_ = TradeType_t('2' - next_event_->data_.eurex_dels_.type_);

          switch (next_event_->data_.eurex_dels_.action_) {
            case '1': {
              p_price_level_global_listener_->OnPriceLevelNew(
                  security_id_, _buysell_, next_event_->data_.eurex_dels_.level_, next_event_->data_.eurex_dels_.price_,
                  next_event_->data_.eurex_dels_.size_, next_event_->data_.eurex_dels_.num_ords_,
                  next_event_->data_.eurex_dels_.intermediate_);
            } break;
            case '2': {
              p_price_level_global_listener_->OnPriceLevelChange(
                  security_id_, _buysell_, next_event_->data_.eurex_dels_.level_, next_event_->data_.eurex_dels_.price_,
                  next_event_->data_.eurex_dels_.size_, next_event_->data_.eurex_dels_.num_ords_,
                  next_event_->data_.eurex_dels_.intermediate_);
            } break;
            case '3': {
              p_price_level_global_listener_->OnPriceLevelDelete(
                  security_id_, _buysell_, next_event_->data_.eurex_dels_.level_, next_event_->data_.eurex_dels_.price_,
                  next_event_->data_.eurex_dels_.intermediate_);
            } break;
            case '4': {
              p_price_level_global_listener_->OnPriceLevelDeleteFrom(security_id_, _buysell_,
                                                                     next_event_->data_.eurex_dels_.level_,
                                                                     next_event_->data_.eurex_dels_.intermediate_);
            } break;
            case '5': {
              p_price_level_global_listener_->OnPriceLevelDeleteThrough(security_id_, _buysell_,
                                                                        next_event_->data_.eurex_dels_.level_,
                                                                        next_event_->data_.eurex_dels_.intermediate_);
            } break;
            default: {
              fprintf(stderr, "Weird action type in EUREXLiveDataSource::ProcessAllEvents EUREX_DELTA %c \n",
                      next_event_->data_.eurex_dels_.action_);
            } break;
          }
        }
      } break;
      case EUREX_MDS::EUREX_TRADE: {
        if (product_code_to_shortcode_.find(next_event_->data_.eurex_trds_.contract_code_) ==
            product_code_to_shortcode_.end()) {
          break;
        }

        if (shortcode_to_exchange_symbol_.find(
                product_code_to_shortcode_[next_event_->data_.eurex_trds_.contract_code_]) ==
            shortcode_to_exchange_symbol_.end()) {
          break;
        }

        int security_id_ = sec_name_indexer_.GetIdFromSecname(
            shortcode_to_exchange_symbol_[product_code_to_shortcode_[next_event_->data_.eurex_trds_.contract_code_]]
                .c_str());

        if (security_id_ < 0) return;

        p_time_keeper_->OnTimeReceived(_shm_writer_time_);

        TradeType_t _buysell_ =
            ((next_event_->data_.eurex_trds_.agg_side_ == 'B')
                 ? (kTradeTypeBuy)
                 : ((next_event_->data_.eurex_trds_.agg_side_ == 'S')
                        ? kTradeTypeSell
                        : kTradeTypeNoInfo));  // TODO see if semantics of 'B' and kTradeTypeBuy match

        p_price_level_global_listener_->OnTrade(security_id_, next_event_->data_.eurex_trds_.trd_px_,
                                                next_event_->data_.eurex_trds_.trd_qty_, _buysell_);
      } break;

      default: { std::cerr << " Weird Message Type in EUREXLSLiveDataSource : " << next_event_->msg_ << "\n"; } break;
    }
  }
};
}
