/**
   \file SimMarketMaker/ors_message_stats.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#define BMF_ORS_LOGGING_CHANGE_DATE 20180517
#define BMF_ORS_LOGGING_REVERT_DATE 20180801

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"

#include "dvccode/CommonDataStructures/simple_mempool.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"

#include "dvccode/CDef/ors_defines.hpp"
#include "baseinfra/LoggedSources/ors_message_filesource.hpp"

#include "baseinfra/MarketAdapter/security_market_view.hpp"

namespace HFSAT {

struct OrsOrderInfo {
  int saos_;
  int size_;
  double price_;
  int int_price_;
  ttime_t seq_time_;
  ttime_t conf_time_;
  ttime_t mkt_time_;
  ttime_t cxl_time_;
  ttime_t cxlseq_time_;
  ttime_t cxlmkt_time_;
  unsigned long seq_cpu_cycles_;
  unsigned long cxl_cpu_cycles_;
};

class ORSMessageStats {
 private:
  SimpleMempool<OrsOrderInfo> ors_order_mempool_;

  // We have three maps
  // 1st map>seq_time_to_ors_order_map_: is used to store time attributes specific to ors latencies and uses seq time as
  // key
  // 2nd map>exch_seq_to_ors_order_map_: is used to  store time attributes specific to market latencies and uses
  // order_id as key
  // 3rd map>saos_to_ors_order_map_: is used as map from 1st map to 2nd map as 1st and 2nd map  essentially contain
  // pointers pointing to the same same order with different key values

  std::map<ttime_t, OrsOrderInfo*> seq_time_to_ors_order_map_;
  std::map<int, OrsOrderInfo*> saos_to_ors_order_map_;
  std::map<int64_t, OrsOrderInfo*> exch_seq_to_ors_order_map_;

  std::map<ttime_t, OrsOrderInfo*> cxlseq_to_ors_order_map_;

  DebugLogger& dbglogger_;
  const Watch& watch_;

  HFSAT::TradingLocation_t trading_location_;

  ttime_t min_delay_;
  ttime_t normal_delay_;
  ttime_t max_delay_;
  ttime_t ors_mkt_diff_;
  ttime_t send_delay_diff_;
  ttime_t cxl_delay_diff_;

  int tradingdate_;
  int security_id_;

  ttime_t median_conf_delay_;
  ttime_t median_mkt_delay_;

  bool use_cpu_cycles_;
  int cpu_cycle_freq_;

  void GenerateLatencyMaps();
  void GenerateORSLatencies();
  void GenerateMktLatencies();
  void GenerateBMFMktLatencies();
  void GenerateEUREXMktLatencies();
  void GenerateOSEMktLatencies();
  void GenerateHKMktLatencies();
  void GenerateICEMktLatencies();
  void GenerateASXMktLatencies();
  void GenerateCMEMktLatencies();

  void ProcessSeqd(GenericORSReplyStruct& next_event);
  void ProcessConf(GenericORSReplyStruct& next_event);
  void ProcessRejc(GenericORSReplyStruct& next_event);
  void ProcessCxlSeqd(GenericORSReplyStruct& next_event);
  void ProcessCxld(GenericORSReplyStruct& next_event);
  void ProcessCxlRejc(GenericORSReplyStruct& next_event);
  void ProcessExec(GenericORSReplyStruct& next_event);

  void ComputeMedian();
  void RemoveInvalidDelays();
  void CleanupMaps();

 public:
  ORSMessageStats(DebugLogger& dbglogger, Watch& watch, const unsigned security_id, TradingLocation_t trading_location,
                  int min_delay_usecs, int normal_delay_usecs, int max_delay_usecs, int ors_mkt_diff,
                  int send_delay_diff, int cxl_delay_diff);

  ~ORSMessageStats() {
    for (auto& seq_it_ : seq_time_to_ors_order_map_) {
      ors_order_mempool_.DeAlloc(seq_it_.second);
    }
    seq_time_to_ors_order_map_.clear();
    saos_to_ors_order_map_.clear();
  }

  const std::map<ttime_t, OrsOrderInfo*>& GetSeqTimeToOrsOrderMap() const { return seq_time_to_ors_order_map_; }
  const std::map<ttime_t, OrsOrderInfo*>& GetCxlSeqTimeToOrsOrderMap() const { return cxlseq_to_ors_order_map_; }

  OrsOrderInfo* const GetNearestConfOrderInfo(ttime_t current_time);
  OrsOrderInfo* const GetNearestCxlOrderInfo(ttime_t current_time);

  void PrintAllDelays();
  ttime_t GetMedianConfDelay() { return median_conf_delay_; }
  ttime_t GetMedianMktDelay() { return median_mkt_delay_; }
};
}
