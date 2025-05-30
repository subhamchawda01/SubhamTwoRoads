/**
    \file auto_ntp_template.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include "infracore/lwfixfast/auto_bmf_template.hpp"
#include "infracore/lwfixfast/bmf_md_processor.hpp"

void BMF_TEMPLATE_DECODER::News_29::process() {}

void BMF_TEMPLATE_DECODER::SecurityStatus_22::process() {}

void BMF_TEMPLATE_DECODER::SecurityStatus_21::process() {}

void BMF_TEMPLATE_DECODER::MarketDataIncrementalRefresh_26::process() {
  BMF_MD_PROCESSOR::BmfMDProcessor& bmfMdProcessor = BMF_MD_PROCESSOR::BmfMDProcessor::GetInstance();

  std::string secIdStr = SecurityID->previousValue.getValue().toString();
  if (bmfMdProcessor.sec_to_id_map_.find(secIdStr) == bmfMdProcessor.sec_to_id_map_.end()) return;
  uint64_t secId = bmfMdProcessor.sec_to_id_map_[secIdStr];

  bmfMdProcessor.updateSeqno(
      secId, (RptSeq->previousValue).getValue());  // we need to call this before we check if inst is in recovery or not

  bool isInstInRecovery = bmfMdProcessor.recovery_[secId];
  NtpCstrQ* ntpQ = isInstInRecovery ? bmfMdProcessor.getWaitQ(secId) : NULL;

  if (isInstInRecovery) ntpQ->updateSeqNum((RptSeq->previousValue).getValue());

  if (((MDEntryType->previousValue).getValue().bytes[0] == '2') &&
      ((MDUpdateAction->previousValue).getValue().bytes[0] == '2')) {
    // bmfMdProcessor.processTradeBust
    return;
  }

  /// process trade
  if ((MDEntryType->previousValue).getValue().bytes[0] == '2') {
    if (TradeCondition->isValueSet &&
        (TradeCondition->currVal.contains_char('R') || TradeCondition->currVal.contains_char('X'))) {
      fprintf(stderr, " Bad TradeCondition %s \n", TradeCondition->currVal.toString().c_str());
      return;
    }

    memset((void*)bmfMdProcessor.cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

    bmfMdProcessor.cstr->msg_ = NTP_MDS::NTP_TRADE;

    // gettimeofday( &(bmfMdProcessor.cstr->time_), NULL );

    if (bmfMdProcessor.idmap_.find(secId) == bmfMdProcessor.idmap_.end()) return;
    SecurityID->previousValue.getValue().copyToDest(bmfMdProcessor.cstr->data_.ntp_trds_.contract_);
    bmfMdProcessor.cstr->data_.ntp_trds_.trd_qty_ =
        (int32_t)(MDEntrySize->isValueSet) ? (MDEntrySize->previousValue).getValue().value() : 0;
    bmfMdProcessor.cstr->data_.ntp_trds_.tot_qty_ = (int32_t)(TradeVolume->isValueSet) ? (TradeVolume->currVal) : 0;
    bmfMdProcessor.cstr->data_.ntp_trds_.seqno_ = (RptSeq->previousValue).getValue();
    bmfMdProcessor.cstr->data_.ntp_trds_.trd_px_ =
        MDEntryPx->isValueSet ? MDEntryPx->previousValue.getValue().value() : 0;
    if (TradeCondition->isValueSet && !TradeCondition->currVal.isnull)
      bmfMdProcessor.cstr->data_.ntp_trds_.is_last_ = TradeCondition->currVal.contains_char('L');
    else
      bmfMdProcessor.cstr->data_.ntp_trds_.is_last_ = false;

    if (MDEntryBuyer->previousValue.isAssigned()) {  // TODO can also use tradetype check

      bmfMdProcessor.cstr->data_.ntp_trds_.buyer_ =
          (int)(OrderID->currVal.parse_uint64_t_without_errors() & ORDER_ID_MASK);  // assuming won't result in overflow

    } else if (MDEntrySeller->previousValue.isAssigned()) {
      bmfMdProcessor.cstr->data_.ntp_trds_.seller_ =
          (int)(OrderID->currVal.parse_uint64_t_without_errors() & ORDER_ID_MASK);
    }

    if (isInstInRecovery)  // queue it up
      ntpQ->add(bmfMdProcessor.cstr);
    else  // consume it
      bmfMdProcessor.dispatchTrade(secId);

    return;

  } else if ((MDEntryType->previousValue).getValue().bytes[0] == '0' ||
             (MDEntryType->previousValue).getValue().bytes[0] == '1') {
    if (QuoteCondition->isValueSet) {
      if (QuoteCondition->currVal.contains_char('R') || QuoteCondition->currVal.contains_char('K')) {
        // never observed in practice
        fprintf(stderr, " Bad QuoteCondition %s \n", QuoteCondition->currVal.toString().c_str());
      }
    }

    memset((void*)bmfMdProcessor.cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

    bmfMdProcessor.cstr->msg_ = NTP_MDS::NTP_ORDER;
    // gettimeofday( &(bmfMdProcessor.cstr->time_), NULL );
    SecurityID->previousValue.getValue().copyToDest(bmfMdProcessor.cstr->data_.ntp_ordr_.contract_);
    bmfMdProcessor.cstr->data_.ntp_ordr_.level_ =
        static_cast<uint16_t>((MDEntryPositionNo->isValueSet) ? MDEntryPositionNo->previousValue.getValue() : 0);
    if (MDEntryPx->isValueSet) {
      bmfMdProcessor.cstr->data_.ntp_ordr_.price_ = MDEntryPx->previousValue.getValue().value();
    } else {
      if (OpenCloseSettlFlag->isValueSet) {
        if (OpenCloseSettlFlag->currVal.contains_char('1')) {
          bmfMdProcessor.cstr->data_.ntp_ordr_.price_ = bmfMdProcessor.id2SettlementPrice_[secId];
        } else if (OpenCloseSettlFlag->currVal.contains_char('4')) {
          bmfMdProcessor.cstr->data_.ntp_ordr_.price_ = bmfMdProcessor.id2ClosingPrice_[secId];
        } else if (OpenCloseSettlFlag->currVal.contains_char('5')) {
          bmfMdProcessor.cstr->data_.ntp_ordr_.price_ = bmfMdProcessor.id2OpenPrice_[secId];
        }
      } else
        bmfMdProcessor.cstr->data_.ntp_ordr_.price_ = 0;
    }
    bmfMdProcessor.cstr->data_.ntp_ordr_.size_ =
        (uint32_t)((MDEntrySize->isValueSet) ? MDEntrySize->previousValue.getValue().value() : 0);
    bmfMdProcessor.cstr->data_.ntp_ordr_.seqno_ = (RptSeq->previousValue).getValue();
    bmfMdProcessor.cstr->data_.ntp_ordr_.type_ = (MDEntryType->previousValue).getValue().bytes[0];
    bmfMdProcessor.cstr->data_.ntp_ordr_.action_ = (MDUpdateAction->previousValue).getValue().bytes[0] - '0';

    if (MDEntryBuyer->previousValue.isAssigned()) {  // TODO can also use tradetype check

      bmfMdProcessor.cstr->data_.ntp_ordr_.order_id_ =
          (OrderID->currVal.parse_uint64_t_without_errors() & ORDER_ID_MASK) |
          ((MDEntryBuyer->previousValue.getValue().parse_uint64_t_without_errors() << 32) & TRADER_ID_MASK);

    } else if (MDEntrySeller->previousValue.isAssigned()) {
      bmfMdProcessor.cstr->data_.ntp_ordr_.order_id_ =
          (OrderID->currVal.parse_uint64_t_without_errors() & ORDER_ID_MASK) |
          ((MDEntrySeller->previousValue.getValue().parse_uint64_t_without_errors() << 32) & TRADER_ID_MASK);

    } else {
      bmfMdProcessor.cstr->data_.ntp_ordr_.order_id_ = (OrderID->currVal.parse_uint64_t_without_errors());
    }

    if (isInstInRecovery)  // queue it up
      ntpQ->add(bmfMdProcessor.cstr);
    else  // consume it
      bmfMdProcessor.dispatchQuote(secId);
  } else if ((MDEntryType->previousValue.getValue()).bytes[0] == '4')
    bmfMdProcessor.id2OpenPrice_[secId] = MDEntryPx->previousValue.getValue().value();
  else if ((MDEntryType->previousValue.getValue()).bytes[0] == '5')
    bmfMdProcessor.id2ClosingPrice_[secId] = MDEntryPx->previousValue.getValue().value();
  else if ((MDEntryType->previousValue.getValue().bytes[0] == '6'))
    bmfMdProcessor.id2SettlementPrice_[secId] = MDEntryPx->previousValue.getValue().value();
  else if ((MDEntryType->previousValue.getValue().bytes[0] == 'J')) {
    memset((void*)bmfMdProcessor.cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

    // gettimeofday( &bmfMdProcessor.cstr->time_, NULL );

    // send book reset
    bmfMdProcessor.cstr->msg_ = NTP_MDS::NTP_ORDER;
    bmfMdProcessor.cstr->data_.ntp_ordr_.type_ = 'J';  // reset
    SecurityID->previousValue.getValue().copyToDest(bmfMdProcessor.cstr->data_.ntp_ordr_.contract_);
    bmfMdProcessor.dispatchQuote(bmfMdProcessor.cstr);

    std::cerr << " Book Reset Msg : " << bmfMdProcessor.cstr->data_.ntp_ordr_.contract_ << "\n";
  }
}

void BMF_TEMPLATE_DECODER::MarketDataIncrementalRefresh_26::process_end() {
  BMF_MD_PROCESSOR::BmfMDProcessor& bmfMdProcessor = BMF_MD_PROCESSOR::BmfMDProcessor::GetInstance();
  bmfMdProcessor.flushTradeQueue();
  bmfMdProcessor.flushQuoteQueue(false);
}

void BMF_TEMPLATE_DECODER::SequenceReset_10::process() { BMF_MD_PROCESSOR::BmfMDProcessor::GetInstance().seqReset(); }

void BMF_TEMPLATE_DECODER::SecurityList_30::process() {
  BMF_MD_PROCESSOR::BmfMDProcessor& bmfMdProcessor = BMF_MD_PROCESSOR::BmfMDProcessor::GetInstance();

  std::string symbol = Symbol->previousValue.getValue().toString();
  if (bmfMdProcessor.product_seen.find(symbol) == bmfMdProcessor.product_seen.end()) {
    bmfMdProcessor.product_seen[symbol] = true;
    bmfMdProcessor.dumpSecurityDefinition(this->SecurityID->previousValue.getValue().toString(), symbol);
  }
}

void BMF_TEMPLATE_DECODER::MarketDataSnapshotFullRefresh_28::process(uint64_t secId) {
  BMF_MD_PROCESSOR::BmfMDProcessor& bmfMdProcessor = BMF_MD_PROCESSOR::BmfMDProcessor::GetInstance();

  if (bmfMdProcessor.idmap_.find(secId) != bmfMdProcessor.idmap_.end()) {
    bmfMdProcessor.max_rpt_seq = std::max(bmfMdProcessor.max_rpt_seq, RptSeq->previousValue.getValue());

    // TODO added just for logging
    if (!MDEntryType->previousValue.isAssigned()) fprintf(stderr, "MDEntryType Null in Snapshot\n");

    // must check to prevent segfault
    if (MDEntryType->previousValue.isAssigned()) {
      if ((MDEntryType->previousValue.getValue()).bytes[0] == '4')
        bmfMdProcessor.id2OpenPrice_[secId] = MDEntryPx->previousValue.getValue().value();
      else if ((MDEntryType->previousValue.getValue()).bytes[0] == '5')
        bmfMdProcessor.id2ClosingPrice_[secId] = MDEntryPx->previousValue.getValue().value();
      else if ((MDEntryType->previousValue.getValue().bytes[0] == '6'))
        bmfMdProcessor.id2SettlementPrice_[secId] = MDEntryPx->previousValue.getValue().value();

      else if ((MDEntryType->previousValue.getValue().bytes[0] == 'J')) {
        memset((void*)bmfMdProcessor.cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

        // gettimeofday( &bmfMdProcessor.cstr->time_, NULL );

        // send book reset
        bmfMdProcessor.cstr->msg_ = NTP_MDS::NTP_ORDER;
        bmfMdProcessor.cstr->data_.ntp_ordr_.type_ = 'J';  // reset
        SecurityID->previousValue.getValue().copyToDest(bmfMdProcessor.cstr->data_.ntp_ordr_.contract_);
        bmfMdProcessor.dispatchQuote(bmfMdProcessor.cstr);
        std::cerr << " Book Reset Msg : " << bmfMdProcessor.cstr->data_.ntp_ordr_.contract_ << "\n";

      }

      else if (((MDEntryType->previousValue.getValue()).bytes[0] == '0') ||
               (MDEntryType->previousValue.getValue().bytes[0] == '1')) {
        // This is either a new bid or offer.
        memset((void*)bmfMdProcessor.cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

        bmfMdProcessor.cstr->msg_ = NTP_MDS::NTP_ORDER;

        // gettimeofday( &bmfMdProcessor.cstr->time_, NULL );
        SecurityID->previousValue.getValue().copyToDest(bmfMdProcessor.cstr->data_.ntp_ordr_.contract_);

        bmfMdProcessor.cstr->data_.ntp_ordr_.level_ = static_cast<uint16_t>(
            (MDEntryPositionNo->previousValue.isAssigned()) ? MDEntryPositionNo->previousValue.getValue() : 0);
        if (MDEntryPx->isValueSet) {
          bmfMdProcessor.cstr->data_.ntp_ordr_.price_ = MDEntryPx->previousValue.getValue().value();
        } else {
          if (OpenCloseSettlFlag->isValueSet) {
            if (OpenCloseSettlFlag->currVal.contains_char('1')) {
              bmfMdProcessor.cstr->data_.ntp_ordr_.price_ = bmfMdProcessor.id2SettlementPrice_[secId];
            } else if (OpenCloseSettlFlag->currVal.contains_char('4')) {
              bmfMdProcessor.cstr->data_.ntp_ordr_.price_ = bmfMdProcessor.id2ClosingPrice_[secId];
            } else if (OpenCloseSettlFlag->currVal.contains_char('5')) {
              bmfMdProcessor.cstr->data_.ntp_ordr_.price_ = bmfMdProcessor.id2OpenPrice_[secId];
            }
          } else
            bmfMdProcessor.cstr->data_.ntp_ordr_.price_ = 0;
        }
        bmfMdProcessor.cstr->data_.ntp_ordr_.size_ =
            (uint32_t)MDEntrySize->isValueSet ? MDEntrySize->previousValue.getValue().value() : 0;
        bmfMdProcessor.cstr->data_.ntp_ordr_.seqno_ = (RptSeq->previousValue).getValue();
        bmfMdProcessor.cstr->data_.ntp_ordr_.trd_qty_ =
            (uint32_t)(TradeVolume->isValueSet) ? (TradeVolume->currVal) : 0;
        bmfMdProcessor.cstr->data_.ntp_ordr_.type_ = MDEntryType->previousValue.getValue().bytes[0];
        bmfMdProcessor.cstr->data_.ntp_ordr_.action_ = 0;

        if (MDEntryBuyer->previousValue.isAssigned()) {  // TODO can also use tradetype check

          bmfMdProcessor.cstr->data_.ntp_ordr_.order_id_ =
              (OrderID->currVal.parse_uint64_t_without_errors() & ORDER_ID_MASK) |
              ((MDEntryBuyer->previousValue.getValue().parse_uint64_t_without_errors() << 32) & TRADER_ID_MASK);

        } else if (MDEntrySeller->previousValue.isAssigned()) {
          bmfMdProcessor.cstr->data_.ntp_ordr_.order_id_ =
              (OrderID->currVal.parse_uint64_t_without_errors() & ORDER_ID_MASK) |
              ((MDEntrySeller->previousValue.getValue().parse_uint64_t_without_errors() << 32) & TRADER_ID_MASK);

        } else {
          bmfMdProcessor.cstr->data_.ntp_ordr_.order_id_ = (OrderID->currVal.parse_uint64_t_without_errors());
        }

        bmfMdProcessor.ntpCstrQ.add(bmfMdProcessor.cstr);
      }
    }
  }
}

void BMF_TEMPLATE_DECODER::MarketDataSnapshotFullRefresh_28::process_end(uint64_t secId) {
  BMF_MD_PROCESSOR::BmfMDProcessor& bmfMdProcessor = BMF_MD_PROCESSOR::BmfMDProcessor::GetInstance();

  memset((void*)bmfMdProcessor.cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

  // gettimeofday( &bmfMdProcessor.cstr->time_, NULL );

  // send book reset
  bmfMdProcessor.cstr->msg_ = NTP_MDS::NTP_ORDER;
  bmfMdProcessor.cstr->data_.ntp_ordr_.type_ = 'J';  // reset
  SecurityID->previousValue.getValue().copyToDest(bmfMdProcessor.cstr->data_.ntp_ordr_.contract_);
  bmfMdProcessor.dispatchQuote(bmfMdProcessor.cstr);

  NTP_MDS::NTPCommonStruct* list = bmfMdProcessor.ntpCstrQ.getSorted();
  int sz = bmfMdProcessor.ntpCstrQ.getSize();
  if (sz <= 0) return;

  for (int i = 0; i < sz; ++i) {
    list[i].data_.ntp_ordr_.intermediate_ = (i != sz - 1);
    bmfMdProcessor.dispatchQuote(list + i);
  }

  // Concurrent recovery method similar to what is used for NTP.
  // Instrument level sequencing.
  struct timeval tv;

  NtpCstrQ* waitQ = bmfMdProcessor.getWaitQ(secId);
  if ((int)(bmfMdProcessor.max_rpt_seq) >= (int)bmfMdProcessor.rpt_seq_map_[secId] ||
      (waitQ->getSize() > 0 && waitQ->getMinSeq() <= bmfMdProcessor.max_rpt_seq + 1)) {
    gettimeofday(&tv, NULL);
    fprintf(stderr, "Ending recovery for %s ( %u ) at seqno %u at time %ld \n",
            SecurityID->previousValue.getValue().toString().c_str(), (unsigned)secId,
            (unsigned)(bmfMdProcessor.max_rpt_seq), tv.tv_sec);
    bmfMdProcessor.endRecovery(bmfMdProcessor.idchnmap_[secId]);
    bmfMdProcessor.recovery_[secId] = false;
    fprintf(stderr, "Snapshot jump from %u to %u \n", bmfMdProcessor.rpt_seq_map_[secId], bmfMdProcessor.max_rpt_seq);
    bmfMdProcessor.rpt_seq_map_[secId] = bmfMdProcessor.max_rpt_seq;
    bmfMdProcessor.clearWaitQ(secId);
  } else {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    fprintf(stderr, "Failed Recovery for %s ( %u ) at time %ld. Expecting max_rpt_seq >= %u, found %u \n",
            SecurityID->previousValue.getValue().toString().c_str(), (unsigned)secId, tv.tv_sec,
            (int)bmfMdProcessor.rpt_seq_map_[secId], (unsigned)(bmfMdProcessor.max_rpt_seq));
  }

  bmfMdProcessor.ntpCstrQ.clear();
}
uint64_t BMF_TEMPLATE_DECODER::MarketDataSnapshotFullRefresh_28::findSecurity() {
  BMF_MD_PROCESSOR::BmfMDProcessor& bmfMdProcessor = BMF_MD_PROCESSOR::BmfMDProcessor::GetInstance();
  bmfMdProcessor.max_rpt_seq = 0;
  std::string secIdStr = SecurityID->previousValue.getValue().toString();

  /// not interested in instrument or instrument not in recovery mode
  if (bmfMdProcessor.sec_to_id_map_.find(secIdStr) == bmfMdProcessor.sec_to_id_map_.end()) {
    //      fprintf(stderr, "sec not found %s ", secIdStr.c_str());
    return -1;
  }

  uint64_t secId = bmfMdProcessor.sec_to_id_map_[secIdStr];
  if (!bmfMdProcessor.recovery_[secId]) {
    //      fprintf(stderr, "sec not in recovery %s ", secIdStr.c_str());
    return -1;
  }
  //  fprintf(stderr, "sec id returnign for security %s : %lld ", secIdStr.c_str(), (int64_t)secId);
  return secId;
}

void BMF_TEMPLATE_DECODER::Heartbeat_11::process() {}
