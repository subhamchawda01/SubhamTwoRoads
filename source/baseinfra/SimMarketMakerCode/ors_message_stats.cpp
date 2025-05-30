/**
 \file SimMarketMakerCode/ors_message_stats.cpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 353, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551
 */

#include "baseinfra/SimMarketMaker/ors_message_stats.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#include "baseinfra/LoggedSources/ose_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eobi_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkomd_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/common_logged_message_filenamer.hpp"

namespace HFSAT {

ORSMessageStats::ORSMessageStats(DebugLogger& dbglogger, Watch& watch, const unsigned security_id,
                                 TradingLocation_t trading_location, int min_delay_usecs, int normal_delay_usecs,
                                 int max_delay_usecs, int ors_mkt_diff, int send_delay_diff, int cxl_delay_diff)
    : dbglogger_(dbglogger),
      watch_(watch),
      trading_location_(trading_location),
      tradingdate_(watch.YYYYMMDD()),
      security_id_(security_id),
      use_cpu_cycles_(false),
      cpu_cycle_freq_(1) {
  min_delay_ = ttime_t(0, min_delay_usecs);
  normal_delay_ = ttime_t(0, normal_delay_usecs);
  max_delay_ = ttime_t(0, max_delay_usecs);
  ors_mkt_diff_ = ttime_t(0, ors_mkt_diff);
  send_delay_diff_ = ttime_t(0, send_delay_diff);
  cxl_delay_diff_ = ttime_t(0, cxl_delay_diff);

  median_conf_delay_ = ttime_t(0, 0);
  median_mkt_delay_ = ttime_t(0, 0);

  if ((trading_location == kTLocBMF) &&
      ((tradingdate_ >= BMF_ORS_LOGGING_CHANGE_DATE) && (tradingdate_ < BMF_ORS_LOGGING_REVERT_DATE))) {
    use_cpu_cycles_ = true;
    cpu_cycle_freq_ = 3300;
  }

  GenerateLatencyMaps();
  CleanupMaps();
}

void ORSMessageStats::GenerateLatencyMaps() {
  GenerateORSLatencies();
  GenerateMktLatencies();

  ComputeMedian();
  RemoveInvalidDelays();
  CleanupMaps();
}

void ORSMessageStats::ComputeMedian() {
  std::vector<ttime_t> conf_delays;
  std::vector<ttime_t> mkt_delays;

  for (auto& pair : seq_time_to_ors_order_map_) {
    if (pair.second->conf_time_ > pair.second->seq_time_) {
      ttime_t conf_diff = pair.second->conf_time_ - pair.second->seq_time_;
      conf_delays.push_back(conf_diff);
    }

    if (pair.second->mkt_time_ > pair.second->seq_time_) {
      ttime_t mkt_diff = pair.second->mkt_time_ - pair.second->seq_time_;
      mkt_delays.push_back(mkt_diff);
    }
  }

  if (!conf_delays.empty()) {
    std::sort(conf_delays.begin(), conf_delays.end());

    // 75% percentile is used instead of median
    median_conf_delay_ = conf_delays.at(conf_delays.size() * 3 / 4);
    DBGLOG_TIME_CLASS_FUNC << "Median Conf Delay: " << median_conf_delay_ << DBGLOG_ENDL_FLUSH;
  }

  if (!mkt_delays.empty()) {
    std::sort(mkt_delays.begin(), mkt_delays.end());

    // 75% percentile is used instead of median
    median_mkt_delay_ = mkt_delays.at(mkt_delays.size() * 3 / 4);
    DBGLOG_TIME_CLASS_FUNC << "Median Mkt Delay: " << median_mkt_delay_ << DBGLOG_ENDL_FLUSH;
  }

  // Reset this to normal delay, if median is 0
  if (median_conf_delay_ == ttime_t(0, 0)) {
    median_conf_delay_ = normal_delay_;
  }

  if (median_mkt_delay_ == ttime_t(0, 0)) {
    median_mkt_delay_ = median_conf_delay_;
  }
}

// Remove entries with invalid conf/mkt times,
// so that the correct ones can get picked in nearest search
void ORSMessageStats::RemoveInvalidDelays() {
  auto seq_it = seq_time_to_ors_order_map_.begin();

  for (; seq_it != seq_time_to_ors_order_map_.end();) {
    OrsOrderInfo* ord = seq_it->second;
    if (ord == NULL) {
      seq_it++;
      continue;
    }
    if (ord->conf_time_ == ttime_t(0, 0) && ord->mkt_time_ == ttime_t(0, 0)) {
      ors_order_mempool_.DeAlloc(seq_it->second);
      seq_time_to_ors_order_map_.erase(seq_it++);
    } else {
      seq_it++;
    }
  }

  //    auto cxl_it_ = cxlseq_to_ors_order_map_.begin();
  //
  //    for (; cxl_it_ != cxlseq_to_ors_order_map_.end(); cxl_it_++)
  //      {
  //        OrsOrderInfo * ord_ = cxl_it_->second;
  //        if (ord_ == NULL)
  //          {
  //            continue;
  //          }
  //        if (ord_->cxlseq_time_ == ttime_t (0, 0) &&
  //            ord_->cxlmkt_time_ == ttime_t (0, 0))
  //          {
  //            cxlseq_to_ors_order_map_.erase (cxl_it_);
  //          }
  //      }
}

// Cleanup intermediate maps
void ORSMessageStats::CleanupMaps() {
  saos_to_ors_order_map_.clear();
  exch_seq_to_ors_order_map_.clear();
}

void ORSMessageStats::GenerateMktLatencies() {
  SecurityNameIndexer& sec_name_indexer_ = SecurityNameIndexer::GetUniqueInstance();
  std::string shortcode = sec_name_indexer_.GetShortcodeFromId(security_id_);
  ExchSource_t exch_source = SecurityDefinitions::GetContractExchSource(shortcode, tradingdate_);

  switch (exch_source) {
    case kExchSourceBMF: {
      GenerateBMFMktLatencies();
      break;
    }
    case kExchSourceEUREX: {
      GenerateEUREXMktLatencies();
      break;
    }
    case kExchSourceJPY: {
      GenerateOSEMktLatencies();
      break;
    }
    case kExchSourceHONGKONG: {
      GenerateHKMktLatencies();
      break;
    }
    case kExchSourceICE: {
      GenerateICEMktLatencies();
      break;
    }
    case kExchSourceASX: {
      GenerateASXMktLatencies();
      break;
    }
    case kExchSourceCME: {
      GenerateCMEMktLatencies();
      break;
    }
    default:
      break;
  }
}

void ORSMessageStats::GenerateBMFMktLatencies() {
  TradingLocation_t trading_location_file = trading_location_;
  BulkFileReader reader;
  SecurityNameIndexer& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  const char* secname = sec_name_indexer.GetSecurityNameFromId(security_id_);

  std::string filename = NTPLoggedMessageFileNamer::GetName(secname, tradingdate_, trading_location_file, true);

  if (trading_location_ != trading_location_file) {
    return;
  }

  reader.open(filename);

  if (!reader.is_open()) {
    return;
  }

  NTP_MDS::NTPCommonStruct next_event;
  size_t len = reader.read(&next_event, sizeof(NTP_MDS::NTPCommonStruct));

  while (len == sizeof(NTP_MDS::NTPCommonStruct)) {
    if (next_event.msg_ == NTP_MDS::NTP_ORDER) {
      switch (next_event.data_.ntp_ordr_.action_) {
        case 0: {
          int64_t order_id_ = next_event.data_.ntp_ordr_.order_id_;
          auto it_ = exch_seq_to_ors_order_map_.find(order_id_);

          if (it_ != exch_seq_to_ors_order_map_.end()) {
            it_->second->mkt_time_ = next_event.time_;
          }
          break;
        }
        case 2: {
          int64_t order_id = next_event.data_.ntp_ordr_.order_id_;
          auto it = exch_seq_to_ors_order_map_.find(order_id);

          if (it != exch_seq_to_ors_order_map_.end()) {
            it->second->cxlmkt_time_ = next_event.time_;
          }
          break;
        }
        default: { break; }
      }
    }

    len = reader.read(&next_event, sizeof(NTP_MDS::NTPCommonStruct));
  }
}

void ORSMessageStats::GenerateEUREXMktLatencies() {
  TradingLocation_t trading_location_file = trading_location_;
  BulkFileReader reader;
  SecurityNameIndexer& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  const char* secname = sec_name_indexer.GetSecurityNameFromId(security_id_);

  std::string filename = EOBILoggedMessageFileNamer::GetName(secname, tradingdate_, trading_location_file);

  if (trading_location_ != trading_location_file) {
    return;
  }

  reader.open(filename);

  if (!reader.is_open()) {
    return;
  }

  EOBI_MDS::EOBICommonStructOld next_event;
  size_t len = reader.read(&next_event, sizeof(EOBI_MDS::EOBICommonStructOld));

  while (len == sizeof(EOBI_MDS::EOBICommonStructOld)) {
    if (next_event.msg_ == EOBI_MDS::EOBI_ORDER) {
      switch (next_event.data_.order_.action_) {
        // Order Add
        case '0': {
          int64_t order_id = next_event.data_.order_.trd_reg_ts;
          auto it = exch_seq_to_ors_order_map_.find(order_id);

          if (it != exch_seq_to_ors_order_map_.end()) {
            it->second->mkt_time_ = next_event.time_;
          }
          break;
        }

        // Order Delete
        case '2': {
          int64_t order_id = next_event.data_.order_.trd_reg_ts;
          auto it = exch_seq_to_ors_order_map_.find(order_id);

          if (it != exch_seq_to_ors_order_map_.end()) {
            it->second->cxlmkt_time_ = next_event.time_;
          }
          break;
        }

        default:
          break;
      }
    }

    len = reader.read(&next_event, sizeof(EOBI_MDS::EOBICommonStructOld));
  }
}

void ORSMessageStats::GenerateOSEMktLatencies() {
  TradingLocation_t trading_location_file = trading_location_;
  BulkFileReader reader;
  SecurityNameIndexer& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  const char* secname = sec_name_indexer.GetSecurityNameFromId(security_id_);

  std::string filename = OSELoggedMessageFileNamer::GetName(secname, tradingdate_, trading_location_file);

  if (trading_location_ != trading_location_file) {
    return;
  }

  reader.open(filename);

  if (!reader.is_open()) {
    return;
  }

  OSE_ITCH_MDS::OSECommonStruct next_event;
  size_t len = reader.read(&next_event, sizeof(OSE_ITCH_MDS::OSECommonStruct));

  while (len == sizeof(OSE_ITCH_MDS::OSECommonStruct)) {
    switch (next_event.msg_type) {
      case OSE_ITCH_MDS::kOSEAdd: {
        uint64_t order_num = next_event.data.add.order_id;
        auto it = exch_seq_to_ors_order_map_.find(order_num);
        if (it != exch_seq_to_ors_order_map_.end()) {
          it->second->mkt_time_ = next_event.time_;
        }
        break;
      }
      case OSE_ITCH_MDS::kOSEDelete: {
        uint64_t order_num = next_event.data.del.order_id;
        auto it = exch_seq_to_ors_order_map_.find(order_num);
        if (it != exch_seq_to_ors_order_map_.end()) {
          it->second->cxlmkt_time_ = next_event.time_;
        }
        break;
      }
      case OSE_ITCH_MDS::kOSEExec:
      case OSE_ITCH_MDS::kOSEExecWithTrade:
      case OSE_ITCH_MDS::kOSETradingStatus:
      case OSE_ITCH_MDS::kOSEEquilibriumPrice:
      case OSE_ITCH_MDS::kOSEPriceNotification: {
      } break;
      default: {}
    }

    len = reader.read(&next_event, sizeof(OSE_ITCH_MDS::OSECommonStruct));
  }
}

void ORSMessageStats::GenerateHKMktLatencies() {
  TradingLocation_t trading_location_file = trading_location_;
  BulkFileReader reader;
  SecurityNameIndexer& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  const char* secname = sec_name_indexer.GetSecurityNameFromId(security_id_);

  std::string filename = HKOMDLoggedMessageFileNamer::GetName(secname, tradingdate_, trading_location_file);

  if (trading_location_ != trading_location_file) {
    return;
  }

  reader.open(filename);

  if (!reader.is_open()) {
    return;
  }

  HKOMD_MDS::HKOMDCommonStruct next_event;
  size_t len = reader.read(&next_event, sizeof(HKOMD_MDS::HKOMDCommonStruct));

  while (len == sizeof(HKOMD_MDS::HKOMDCommonStruct)) {
    if (next_event.msg_ == HKOMD_MDS::HKOMD_ORDER) {
      switch (next_event.data_.order_.action_) {
        // Order Add
        case '0': {
          int64_t order_id = next_event.data_.order_.order_id_;
          auto it = exch_seq_to_ors_order_map_.find(order_id);

          if (it != exch_seq_to_ors_order_map_.end()) {
            it->second->mkt_time_ = next_event.time_;
          }
          break;
        }

        // Order Delete
        case '2': {
          int64_t order_id = next_event.data_.order_.order_id_;
          auto it = exch_seq_to_ors_order_map_.find(order_id);

          if (it != exch_seq_to_ors_order_map_.end()) {
            it->second->cxlmkt_time_ = next_event.time_;
          }
          break;
        }
        default:
          break;
      }
    }

    else if (next_event.msg_ == HKOMD_MDS::HKOMD_TRADE) {
      int64_t order_id = next_event.data_.trade_.order_id_;
      auto it = exch_seq_to_ors_order_map_.find(order_id);

      if (it != exch_seq_to_ors_order_map_.end() && it->second->cxlmkt_time_.tv_sec == 0) {
        it->second->cxlmkt_time_ = next_event.time_;
        // Added delay of 2 us to add room for execution due to the corresponding exec update from mkt
        it->second->cxlmkt_time_ = it->second->cxlmkt_time_ + ttime_t(0, 2);
      }
    }

    len = reader.read(&next_event, sizeof(HKOMD_MDS::HKOMDCommonStruct));
  }
}

void ORSMessageStats::GenerateICEMktLatencies() {
  TradingLocation_t trading_location_file = trading_location_;
  BulkFileReader reader;
  SecurityNameIndexer& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  const char* secname = sec_name_indexer.GetSecurityNameFromId(security_id_);

  std::string filename =
      CommonLoggedMessageFileNamer::GetOrderFeedFilename(kExchSourceICE, secname, tradingdate_, trading_location_file);

  if (trading_location_ != trading_location_file) {
    return;
  }

  reader.open(filename);

  if (!reader.is_open()) {
    return;
  }

  ICE_MDS::ICECommonStruct next_event;
  size_t len = reader.read(&next_event, sizeof(ICE_MDS::ICECommonStruct));

  while (len == sizeof(ICE_MDS::ICECommonStruct)) {
    if (next_event.msg_ == ICE_MDS::ICE_FOD) {
      switch (next_event.data_.ice_fods_.type_) {
        // Order Add
        case 0: {
          int64_t order_id = next_event.data_.ice_fods_.order_id_;
          auto it = exch_seq_to_ors_order_map_.find(order_id);

          if (it != exch_seq_to_ors_order_map_.end()) {
            it->second->mkt_time_ = next_event.time_;
          }
          break;
        }

        // Order Delete
        case 2: {
          int64_t order_id = next_event.data_.ice_fods_.order_id_;
          auto it = exch_seq_to_ors_order_map_.find(order_id);

          if (it != exch_seq_to_ors_order_map_.end()) {
            it->second->cxlmkt_time_ = next_event.time_;
          }
          break;
        }
        default:
          break;
      }
    }

    len = reader.read(&next_event, sizeof(ICE_MDS::ICECommonStruct));
  }
}

void ORSMessageStats::GenerateASXMktLatencies() {
  TradingLocation_t trading_location_file = trading_location_;
  BulkFileReader reader;
  SecurityNameIndexer& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  const char* secname = sec_name_indexer.GetSecurityNameFromId(security_id_);

  std::string filename =
      CommonLoggedMessageFileNamer::GetOrderFeedFilename(kExchSourceASX, secname, tradingdate_, trading_location_file);

  if (trading_location_ != trading_location_file) {
    return;
  }

  reader.open(filename);

  if (!reader.is_open()) {
    return;
  }
  if (tradingdate_ < USING_ASX_NTP_ITCH_FROM) {
    ASX_MDS::ASXCommonStruct next_event;
    size_t len = reader.read(&next_event, sizeof(ASX_MDS::ASXCommonStruct));

    while (len == sizeof(ASX_MDS::ASXCommonStruct)) {
      switch (next_event.msg_type) {
        // Order Add
        case ASX_MDS::kASXAdd: {
          int64_t order_id = next_event.data.add.order_id;
          auto it = exch_seq_to_ors_order_map_.find(order_id);

          if (it != exch_seq_to_ors_order_map_.end() && (int)next_event.data.add.size == it->second->size_) {
            it->second->mkt_time_ = next_event.time_;
          }
          break;
        }

        // Order Delete
        case ASX_MDS::kASXDelete: {
          int64_t order_id = next_event.data.del.order_id;
          auto it = exch_seq_to_ors_order_map_.find(order_id);

          if (it != exch_seq_to_ors_order_map_.end()) {
            it->second->cxlmkt_time_ = next_event.time_;
          }
          break;
        }
        default:
          break;
      }

      len = reader.read(&next_event, sizeof(ASX_MDS::ASXCommonStruct));
    }
  } else {
    ASX_ITCH_MDS::ASXItchOrder next_event;
    size_t len = reader.read(&next_event, sizeof(ASX_ITCH_MDS::ASXItchOrder));
    while (len == sizeof(ASX_ITCH_MDS::ASXItchOrder)) {
      switch (next_event.msg_type) {
        // Order Add
        case ASX_ITCH_MDS::kASXAdd: {
          int64_t order_id = next_event.add.order_id;
          auto it = exch_seq_to_ors_order_map_.find(order_id);

          if (it != exch_seq_to_ors_order_map_.end() && (int)next_event.add.size == it->second->size_) {
            it->second->mkt_time_ = next_event.time;
          }
          break;
        }

        // Order Delete
        case ASX_ITCH_MDS::kASXDelete: {
          int64_t order_id = next_event.del.order_id;
          auto it = exch_seq_to_ors_order_map_.find(order_id);

          if (it != exch_seq_to_ors_order_map_.end() && it->second->mkt_time_ != ttime_t(0, 0)) {
            // If the order wasn't found for add, don't use it for cxl as well
            it->second->cxlmkt_time_ = next_event.time;
          }
          break;
        }
        default:
          break;
      }

      len = reader.read(&next_event, sizeof(ASX_ITCH_MDS::ASXItchOrder));
    }
  }
}

/**
 * Generate latencies for CME
 */
void ORSMessageStats::GenerateCMEMktLatencies() {
  TradingLocation_t trading_location_file = trading_location_;
  BulkFileReader reader;
  SecurityNameIndexer& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  const char* secname = sec_name_indexer.GetSecurityNameFromId(security_id_);

  std::string filename =
      CommonLoggedMessageFileNamer::GetOrderFeedFilename(kExchSourceCME, secname, tradingdate_, trading_location_file);

  if (trading_location_ != trading_location_file) {
    return;
  }

  reader.open(filename);

  if (!reader.is_open()) {
    return;
  }
  CME_MDS::CMEOBFCommonStruct next_event;

  size_t len = reader.read(&next_event, sizeof(CME_MDS::CMEOBFCommonStruct));

  while (len == sizeof(CME_MDS::CMEOBFCommonStruct)) {
    switch (next_event.msg_) {
      // Order Add
      case CME_MDS::CME_DELTA: {
        switch (next_event.data_.cme_dels_.action) {
          case '1':  // setting  for both cancel and add
          case '0': {
            int64_t order_id = next_event.data_.cme_dels_.order_id;
            auto it = exch_seq_to_ors_order_map_.find(order_id);

            if (it != exch_seq_to_ors_order_map_.end()) {
              it->second->mkt_time_ = next_event.time_;
            }
          } break;

          case '2': {
            // order delete update, need to add for cancel time
            int64_t order_id = next_event.data_.cme_dels_.order_id;
            auto it = exch_seq_to_ors_order_map_.find(order_id);

            if (it != exch_seq_to_ors_order_map_.end()) {
              it->second->cxlmkt_time_ = next_event.time_;
            }
          } break;
          default: { fprintf(stderr, "Unknown Delta message type in CME OBF CME_DELTA  ORSMessageStats\n"); } break;
        }
        break;
      }
      default:
        break;
    }

    len = reader.read(&next_event, sizeof(CME_MDS::CMEOBFCommonStruct));
  }
}

void ORSMessageStats::GenerateORSLatencies() {
  TradingLocation_t location_file_read = trading_location_;
  BulkFileReader file_reader;
  SecurityNameIndexer& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  const char* secname = sec_name_indexer.GetSecurityNameFromId(security_id_);

  std::string filename = ORSMessageFileNamer::GetName(secname, tradingdate_, location_file_read);

  if (trading_location_ != location_file_read) {
    return;
  }

  file_reader.open(filename);

  if (!file_reader.is_open()) {
    return;
  }

  GenericORSReplyStruct next_event;
  size_t available_len = file_reader.read(&next_event, sizeof(GenericORSReplyStruct));

  while (available_len == sizeof(GenericORSReplyStruct)) {
    switch (next_event.orr_type_) {
      case kORRType_Seqd: {
        ProcessSeqd(next_event);
        break;
      }
      case kORRType_Conf: {
        ProcessConf(next_event);
        break;
      }
      case kORRType_Rejc: {
        ProcessRejc(next_event);
        break;
      }
      case kORRType_CxlSeqd: {
        ProcessCxlSeqd(next_event);
        break;
      }
      case kORRType_Cxld: {
        ProcessCxld(next_event);
        break;
      }
      case kORRType_CxlRejc: {
        ProcessCxlRejc(next_event);
        break;
      }
      case kORRType_Exec: {
        ProcessExec(next_event);
        break;
      }
      default:
        break;
    }

    available_len = file_reader.read(&next_event, sizeof(GenericORSReplyStruct));
  }
}

void ORSMessageStats::ProcessSeqd(GenericORSReplyStruct& next_event) {
  int saos = next_event.server_assigned_order_sequence_;
  auto it = saos_to_ors_order_map_.find(saos);

  if (it != saos_to_ors_order_map_.end()) {
    return;
  }

  OrsOrderInfo* ors_order = ors_order_mempool_.Alloc();
  ttime_t seq_time = next_event.time_set_by_server_;
  ttime_t client_time_ = next_event.client_request_time_;

  ors_order->saos_ = saos;
  ors_order->size_ = next_event.size_remaining_;
  ors_order->price_ = next_event.price_;
  ors_order->int_price_ = next_event.int_price_;

  if (use_cpu_cycles_) {
    ors_order->seq_cpu_cycles_ = seq_time.val;
    ors_order->seq_time_ = client_time_;
    seq_time_to_ors_order_map_[client_time_] = ors_order;
  } else {
    ors_order->seq_time_ = seq_time;
    seq_time_to_ors_order_map_[seq_time] = ors_order;
  }

  saos_to_ors_order_map_[saos] = ors_order;
}

void ORSMessageStats::ProcessConf(GenericORSReplyStruct& next_event) {
  int saos = next_event.server_assigned_order_sequence_;
  auto it = saos_to_ors_order_map_.find(saos);

  if (it == saos_to_ors_order_map_.end() || it->second == NULL) {
    return;
  }

  ttime_t conf_time = next_event.time_set_by_server_;
  uint64_t exch_seq = next_event.exch_assigned_sequence_;

  if (use_cpu_cycles_) {
    it->second->conf_time_ =
        it->second->seq_time_ + ttime_t(0, (conf_time.val - it->second->seq_cpu_cycles_) / (cpu_cycle_freq_));
  } else {
    it->second->conf_time_ = conf_time;
  }

  if (exch_seq_to_ors_order_map_.find(exch_seq) == exch_seq_to_ors_order_map_.end()) {
    exch_seq_to_ors_order_map_[exch_seq] = it->second;
  }
}

void ORSMessageStats::ProcessRejc(GenericORSReplyStruct& next_event) {
  // For the purpose of computing delay, Rejc is same as Conf
  ProcessConf(next_event);
}

void ORSMessageStats::ProcessCxlSeqd(GenericORSReplyStruct& next_event) {
  int saos = next_event.server_assigned_order_sequence_;
  auto it = saos_to_ors_order_map_.find(saos);

  if (it == saos_to_ors_order_map_.end() || it->second == NULL) {
    return;
  }

  ttime_t cxlseq_time = next_event.time_set_by_server_;
  ttime_t client_time_ = next_event.client_request_time_;

  if (use_cpu_cycles_) {
    it->second->cxlseq_time_ = client_time_;
    it->second->cxl_cpu_cycles_ = cxlseq_time.val;
    cxlseq_to_ors_order_map_[client_time_] = it->second;
  } else {
    it->second->cxlseq_time_ = cxlseq_time;
    cxlseq_to_ors_order_map_[cxlseq_time] = it->second;
  }
}

void ORSMessageStats::ProcessCxld(GenericORSReplyStruct& next_event) {
  int saos = next_event.server_assigned_order_sequence_;
  auto it = saos_to_ors_order_map_.find(saos);
  ttime_t cxl_time = next_event.time_set_by_server_;

  // No need to proceed further if we can't find the order
  if (it == saos_to_ors_order_map_.end() || it->second == NULL) {
    return;
  }

  // Return if cxl_time_ is already set
  if (it->second->cxl_time_.tv_sec != 0) {
    return;
  }

  if (use_cpu_cycles_) {
    it->second->cxl_time_ =
        it->second->cxlseq_time_ + ttime_t(0, (cxl_time.val - it->second->cxl_cpu_cycles_) / (cpu_cycle_freq_));
  } else {
    it->second->cxl_time_ = cxl_time;
  }
}

void ORSMessageStats::ProcessCxlRejc(GenericORSReplyStruct& next_event) {
  // For the purpose of computing delay, CxlRejc is same Cxld
  ProcessCxld(next_event);
}

void ORSMessageStats::ProcessExec(GenericORSReplyStruct& next_event) {
  int saos = next_event.server_assigned_order_sequence_;
  auto it = saos_to_ors_order_map_.find(saos);
  ttime_t exec_time = next_event.time_set_by_server_;

  // No need to proceed further if we can't find the order
  if (it == saos_to_ors_order_map_.end() || it->second == NULL) {
    return;
  }

  // Return if cxl_time_ is already set or if cxl_seq_time isn't set
  if (it->second->cxl_time_.tv_sec != 0 || it->second->cxlseq_time_.tv_sec == 0) {
    return;
  }

  // giving some time for execution, so adding 20 usecs
  if (use_cpu_cycles_) {
    ttime_t tdiff = ttime_t(0, (exec_time.val - it->second->cxl_cpu_cycles_) / (cpu_cycle_freq_));
    if (tdiff < normal_delay_) {
      cxlseq_to_ors_order_map_.erase(it->second->cxlseq_time_);
      return;
    }
    it->second->cxl_time_ = it->second->cxlseq_time_ + tdiff + ttime_t(0, 20) + normal_delay_;
  } else {
    // Return if the difference between exec and cxl seq is less than what we
    // normally send for cxl reject
    ttime_t tdiff = exec_time - it->second->cxlseq_time_;
    if (tdiff < normal_delay_) {
      cxlseq_to_ors_order_map_.erase(it->second->cxlseq_time_);
      return;
    }
    it->second->cxl_time_ = exec_time + ttime_t(0, 20) + normal_delay_;
  }
}

void ORSMessageStats::PrintAllDelays() {
  // Headers for  the table
  std::cout << std::setw(15) << "\tSEQ TIME\t" << std::setw(15) << "\tCONF TIME\t" << std::setw(15) << "\tMKT TIME\t"
            << std::setw(15) << "\tCXLSEQ TIME\t" << std::setw(15) << "\tCXLCONF TIME\t" << std::setw(15)
            << "\tCXLMKT TIME\t\n"
            << std::endl;

  for (auto& pair : seq_time_to_ors_order_map_) {
    std::cout << std::setw(15) << pair.second->seq_time_ << "\t" << std::setw(15) << pair.second->conf_time_ << "\t"
              << std::setw(15) << pair.second->mkt_time_ << "\t" << std::setw(15) << pair.second->cxlseq_time_ << "\t"
              << std::setw(15) << pair.second->cxl_time_ << "\t" << std::setw(15) << pair.second->cxlmkt_time_
              << std::endl;
  }
}

OrsOrderInfo* const ORSMessageStats::GetNearestConfOrderInfo(ttime_t current_time) {
  auto upper_it = seq_time_to_ors_order_map_.lower_bound(current_time);
  auto lower_it = seq_time_to_ors_order_map_.lower_bound(current_time);

  if (lower_it != seq_time_to_ors_order_map_.begin()) {
    --lower_it;
  }

  if (lower_it == seq_time_to_ors_order_map_.end() && upper_it != seq_time_to_ors_order_map_.end()) {
    return upper_it->second;
  }

  if (upper_it == seq_time_to_ors_order_map_.end() && lower_it != seq_time_to_ors_order_map_.end()) {
    return lower_it->second;
  }

  if ((current_time - lower_it->first) >= (upper_it->first - current_time)) {
    return upper_it->second;
  } else {
    return lower_it->second;
  }
}

OrsOrderInfo* const ORSMessageStats::GetNearestCxlOrderInfo(ttime_t current_time) {
  auto upper_it = cxlseq_to_ors_order_map_.lower_bound(current_time);
  auto lower_it = cxlseq_to_ors_order_map_.lower_bound(current_time);

  if (lower_it != cxlseq_to_ors_order_map_.begin()) {
    --lower_it;
  }

  if (lower_it == cxlseq_to_ors_order_map_.end() && upper_it != cxlseq_to_ors_order_map_.end()) {
    return upper_it->second;
  }

  if (upper_it == cxlseq_to_ors_order_map_.end() && lower_it != cxlseq_to_ors_order_map_.end()) {
    return lower_it->second;
  }

  if ((current_time - lower_it->first) >= (upper_it->first - current_time)) {
    return upper_it->second;
  } else {
    return lower_it->second;
  }
}

}  // namespace HFSAT
