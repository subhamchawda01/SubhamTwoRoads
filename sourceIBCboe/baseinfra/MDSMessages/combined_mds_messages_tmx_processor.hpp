/**
    \file MDSMessages/combined_mds_messages_tmx_processor.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#pragma once

#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/MarketAdapter/tmx_trade_time_manager.hpp"

namespace HFSAT {

class CombinedMDSMessagesTMXProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  FullBookGlobalListener* p_full_book_global_listener_;  ///< Listener of TMX full book
  ExternalTimeListener* p_time_keeper_;
  HFSAT::TmxTradeTimeManager& tmx_trade_time_manager_;

  FullBook* fullbook_;

  std::vector<HFSAT::FastPriceConvertor*> fast_price_convertor_map_;

 public:
  CombinedMDSMessagesTMXProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_full_book_global_listener_(NULL),
        p_time_keeper_(NULL),
        tmx_trade_time_manager_(HFSAT::TmxTradeTimeManager::GetUniqueInstance(_sec_name_indexer_)),
        fullbook_(new FullBook()),
        fast_price_convertor_map_(_sec_name_indexer_.NumSecurityId(), NULL) {
    HFSAT::SecurityDefinitions& security_definitions_ =
        HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());
    HFSAT::ShortcodeContractSpecificationMap this_contract_specification_map_ =
        security_definitions_.contract_specification_map_;

    HFSAT::ShortcodeContractSpecificationMapCIter_t itr_ = this_contract_specification_map_.begin();

    for (itr_ = this_contract_specification_map_.begin(); itr_ != this_contract_specification_map_.end(); itr_++) {
      std::string shortcode_ = (itr_->first);
      HFSAT::ContractSpecification contract_spec_ = (itr_->second);

      if (contract_spec_.exch_source_ != HFSAT::kExchSourceTMX) {
        continue;
      }

      std::string this_exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

      int security_id_ = sec_name_indexer_.GetIdFromSecname(this_exch_symbol_.c_str());

      if (security_id_ < 0) {
        continue;
      }

      fast_price_convertor_map_[security_id_] = new HFSAT::FastPriceConvertor(contract_spec_.min_price_increment_);
    }
  }

  ~CombinedMDSMessagesTMXProcessor() {}

  inline void SetFullBookGlobalListener(FullBookGlobalListener* p_new_listener_) {
    p_full_book_global_listener_ = p_new_listener_;
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline bool IsNormalTradeTime(int t_security_id_, ttime_t tv) {
    return tmx_trade_time_manager_.isValidTimeToTrade(t_security_id_, tv.tv_sec % 86400);
  }

  inline void ProcessTMXEvent(TMX_MDS::TMXLSCommonStruct* next_event_, struct timeval& _shm_writer_time_) {
    int security_id_ = sec_name_indexer_.GetIdFromSecname(next_event_->getContract());
    if (0 > security_id_) {
      return;
    }

    if (fast_price_convertor_map_[security_id_] == NULL) {
      // This shouldn't happen.
      return;
    }

    switch (next_event_->type_) {
      case 1:  // Trade
      {
        if (!IsNormalTradeTime(security_id_, _shm_writer_time_)) {
          return;
        }

        p_time_keeper_->OnTimeReceived(_shm_writer_time_);

        HFSAT::FastPriceConvertor* this_fast_px_convertor_ = fast_price_convertor_map_[security_id_];
        double trade_px_ = this_fast_px_convertor_->GetDoublePx(next_event_->bid_int_pxs_[0]);

        p_full_book_global_listener_->OnTrade(security_id_, trade_px_, next_event_->bid_szs_[0], kTradeTypeNoInfo);
      } break;

      case 2:  // Quote
      {
        // Igore quotes
      } break;

      case 3:  // Book ( with status 'T' )
      {
        p_time_keeper_->OnTimeReceived(_shm_writer_time_);

        memset(fullbook_, 0, sizeof(FullBook));
        HFSAT::FastPriceConvertor* this_fast_px_convertor_ = fast_price_convertor_map_[security_id_];

        for (int i = 0; i < FULL_BOOK_DEPTH; i++) {
          fullbook_->bid_price_[i] = this_fast_px_convertor_->GetDoublePx(next_event_->bid_int_pxs_[i]);
          fullbook_->ask_price_[i] = this_fast_px_convertor_->GetDoublePx(next_event_->ask_int_pxs_[i]);

          fullbook_->bid_size_[i] = next_event_->bid_szs_[i];
          fullbook_->ask_size_[i] = next_event_->ask_szs_[i];

          fullbook_->bid_ordercount_[i] = next_event_->num_bid_ords_[i];
          fullbook_->ask_ordercount_[i] = next_event_->num_ask_ords_[i];
        }

        p_full_book_global_listener_->OnFullBookChange(security_id_, fullbook_);
      } break;

      default: { } break; }
  }
};
}
