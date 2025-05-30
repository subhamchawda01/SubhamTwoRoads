/**
   \file lwfixfast/auto_ntp_ord_template.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "infracore/lwfixfast/auto_ntp_ord_template.hpp"
#include "infracore/lwfixfast/ntp_ord_md_processor.hpp"

void NTP_ORD_TEMPLATE_DECODER::MDNewsMessage_120::process() {}

void NTP_ORD_TEMPLATE_DECODER::MDSecurityStatus_125::process() {
  // Trading Status 21 is reserved for pre-open/auction functionality, any other phase change would trigger deletion of
  // TOP price stored/used
  // SecurityTradingEvent - 4 indicates TOP price

  NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor& NtpOrdMdProcessor = NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor::GetInstance();

  if (!NtpOrdMdProcessor.IsProcessingSecurity((SecurityID->previousValue).getValue())) return;

  uint64_t secId = SecurityID->previousValue.getValue();
  std::cerr << " Symbol : " << NtpOrdMdProcessor.idmap_[secId] << " SecurityId : " << secId
            << " SecurityTradingStatus : " << (SecurityTradingStatus->previousValue).getValue()
            << " Trade Session Open Time : " << (TradSesOpenTime->previousValue).getValue()
            << " TransactTime : " << (TransactTime->previousValue).getValue()
            << " SecurityTradingEvent : " << (SecurityTradingEvent->previousValue).getValue()
            << " SendingTime : " << (SendingTime->previousValue).getValue() << "\n";

  NtpOrdMdProcessor.updateSecurityTradingStatus(secId, SecurityTradingStatus->previousValue.getValue());
}

void NTP_ORD_TEMPLATE_DECODER::MDIncRefresh_123::process() {
  NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor& NtpOrdMdProcessor = NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor::GetInstance();

  if (!NtpOrdMdProcessor.IsProcessingSecurity(SecurityID->previousValue.getValue())) {
    return;
  }

  if (RptSeq->previousValue.getValue() < NtpOrdMdProcessor.rpt_seq_map_[SecurityID->previousValue.getValue()]) {
    return;
  }

  NtpOrdMdProcessor.updateSeqno(SecurityID->previousValue.getValue(), RptSeq->previousValue.getValue());

  if (!NtpOrdMdProcessor.IsSecurityInTrdingPhase(SecurityID->previousValue.getValue())) {
    return;
  }

  if ((MDEntryType->previousValue.getValue().bytes[0] == '2') && (MDUpdateAction->previousValue.getValue() == 2)) {
    //  processTradeBust (sec_id , sq_no, trd_qty_, tot_qty_);
    return;
  }

  /// process trade
  if (MDEntryType->previousValue.getValue().bytes[0] == '2') {
    memset((void*)NtpOrdMdProcessor.cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

    NtpOrdMdProcessor.cstr->msg_ = NTP_MDS::NTP_TRADE;

    if (SecurityID->previousValue.getValue()) {
      strcpy(NtpOrdMdProcessor.cstr->data_.ntp_trds_.contract_,
             NtpOrdMdProcessor.idmap_[SecurityID->previousValue.getValue()]);
    } else {
      if (!NtpOrdMdProcessor.IsProcessingSecurity(
              NtpOrdMdProcessor.sec_to_id_map_[(Symbol->previousValue).getValue().bytes]))
        return;
      strcpy(NtpOrdMdProcessor.cstr->data_.ntp_trds_.contract_,
             NtpOrdMdProcessor.idmap_[NtpOrdMdProcessor.sec_to_id_map_[(Symbol->previousValue).getValue().bytes]]);
    }

    NtpOrdMdProcessor.cstr->data_.ntp_trds_.trd_qty_ =
        (MDEntrySize->isValueSet) ? (MDEntrySize->previousValue).getValue() : 0;
    NtpOrdMdProcessor.cstr->data_.ntp_trds_.tot_qty_ =
        (TradeVolume->isValueSet) ? (TradeVolume->previousValue).getValue() : 0;
    NtpOrdMdProcessor.cstr->data_.ntp_trds_.seqno_ = (RptSeq->previousValue).getValue();
    NtpOrdMdProcessor.cstr->data_.ntp_trds_.trd_px_ =
        FFUtils::Decimal::value(MDEntryPx_exponent->currVal, (MDEntryPx_mantissa->previousValue).getValue());
    NtpOrdMdProcessor.cstr->data_.ntp_trds_.is_last_ =
        strstr((TradeCondition->previousValue).getValue().bytes, "L") != NULL;

    if (TradeCondition->previousValue.isAssigned() && !TradeCondition->previousValue.getValue().isnull) {
      for (uint32_t i = 0; i < TradeCondition->previousValue.getValue().len; ++i) {
        if (TradeCondition->previousValue.getValue().bytes[i] == 'X') {
          return;
          fprintf(stderr, " Bad TradeCondition %s \n", (TradeCondition->previousValue).getValue().toString().c_str());

          if (SecurityID->previousValue.getValue()) {
            strcpy(NtpOrdMdProcessor.cstr->data_.ntp_trds_.contract_,
                   NtpOrdMdProcessor.idmap_[(SecurityID->previousValue).getValue()]);
          } else {
            if (!NtpOrdMdProcessor.IsProcessingSecurity(
                    NtpOrdMdProcessor.sec_to_id_map_[(Symbol->previousValue).getValue().bytes]))
              return;
            strcpy(
                NtpOrdMdProcessor.cstr->data_.ntp_trds_.contract_,
                NtpOrdMdProcessor.idmap_[NtpOrdMdProcessor.sec_to_id_map_[(Symbol->previousValue).getValue().bytes]]);
          }

          int32_t t_trade_trd_qty_ = (MDEntrySize->isValueSet) ? (MDEntrySize->previousValue).getValue() : 0;
          uint64_t t_trade_tot_qty_ = (TradeVolume->isValueSet) ? (TradeVolume->previousValue).getValue() : 0;
          double t_trade_px_ =
              FFUtils::Decimal::value(MDEntryPx_exponent->currVal, (MDEntryPx_mantissa->previousValue).getValue());

          fprintf(stderr, "Trade: Contract: %s, Trade_size: %d, Tot_Qty: %d, Trade_Px: %f\n",
                  NtpOrdMdProcessor.cstr->data_.ntp_trds_.contract_, t_trade_trd_qty_, (int)t_trade_tot_qty_,
                  t_trade_px_);

          NtpOrdMdProcessor.cstr->data_.ntp_trds_.flags_[0] = 'X';
          // return;
        }
      }
    }

    if (NtpOrdMdProcessor.recovery_[SecurityID->previousValue.getValue()]) {
      NTP_MDS::NTPCommonStruct* new_ntp_mds_ = new NTP_MDS::NTPCommonStruct();
      memset((void*)new_ntp_mds_, 0, sizeof(NTP_MDS::NTPCommonStruct));
      memcpy(new_ntp_mds_, NtpOrdMdProcessor.cstr, sizeof(NTP_MDS::NTPCommonStruct));
      NtpOrdMdProcessor.message_buffer_[SecurityID->previousValue.getValue()].push_back(new_ntp_mds_);
    } else {
      NtpOrdMdProcessor.dispatchTrade(SecurityID->previousValue.getValue());
    }
    return;
  } else if (MDEntryType->previousValue.getValue().bytes[0] == '0' ||
             MDEntryType->previousValue.getValue().bytes[0] == '1') {
    if (QuoteCondition->isValueSet) {
      for (uint32_t i = 0; i < QuoteCondition->currVal.len; ++i)
        if (QuoteCondition->currVal.bytes[i] == 'R' || QuoteCondition->currVal.bytes[i] == 'K') {
          // never observed in practice
          fprintf(stderr, " Bad QuoteCondition %s \n", QuoteCondition->currVal.toString().c_str());
        }
    }

    memset((void*)NtpOrdMdProcessor.cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

    NtpOrdMdProcessor.cstr->msg_ = NTP_MDS::NTP_ORDER;

    strcpy(NtpOrdMdProcessor.cstr->data_.ntp_ordr_.contract_,
           NtpOrdMdProcessor.idmap_[(SecurityID->previousValue).getValue()]);

    NtpOrdMdProcessor.cstr->data_.ntp_ordr_.level_ =
        static_cast<uint16_t>((MDEntryPositionNo->isValueSet) ? MDEntryPositionNo->currVal : 0);
    NtpOrdMdProcessor.cstr->data_.ntp_ordr_.order_id_ =
        OrderID->isValueSet ? OrderID->currVal.parse_uint64_t_without_errors() : 0;
    NtpOrdMdProcessor.cstr->data_.ntp_ordr_.price_ =
        MDEntryPx_exponent->isValueSet
            ? (FFUtils::Decimal::value(MDEntryPx_exponent->currVal, (MDEntryPx_mantissa->previousValue).getValue()))
            : 0;
    NtpOrdMdProcessor.cstr->data_.ntp_ordr_.size_ =
        MDEntrySize->isValueSet ? (MDEntrySize->previousValue).getValue() : 0;
    NtpOrdMdProcessor.cstr->data_.ntp_ordr_.trd_qty_ =
        (uint32_t)(TradeVolume->isValueSet) ? (TradeVolume->previousValue.getValue()) : 0;
    NtpOrdMdProcessor.cstr->data_.ntp_ordr_.seqno_ = (RptSeq->previousValue).getValue();
    NtpOrdMdProcessor.cstr->data_.ntp_ordr_.action_ = (MDUpdateAction->previousValue).getValue();
    NtpOrdMdProcessor.cstr->data_.ntp_ordr_.type_ = (MDEntryType->previousValue).getValue().bytes[0];
    NtpOrdMdProcessor.cstr->data_.ntp_ordr_.buyer_ =
        MDEntryBuyer->isValueSet ? (uint16_t)MDEntryBuyer->currVal.parse_uint64_t_without_errors() : 0;
    NtpOrdMdProcessor.cstr->data_.ntp_ordr_.seller_ =
        MDEntrySeller->isValueSet ? (uint16_t)MDEntrySeller->currVal.parse_uint64_t_without_errors() : 0;

    if (NtpOrdMdProcessor.recovery_[SecurityID->previousValue.getValue()]) {
      NTP_MDS::NTPCommonStruct* new_ntp_mds_ = new NTP_MDS::NTPCommonStruct();
      memset((void*)new_ntp_mds_, 0, sizeof(NTP_MDS::NTPCommonStruct));
      memcpy(new_ntp_mds_, NtpOrdMdProcessor.cstr, sizeof(NTP_MDS::NTPCommonStruct));
      NtpOrdMdProcessor.message_buffer_[SecurityID->previousValue.getValue()].push_back(new_ntp_mds_);
    } else {
      NtpOrdMdProcessor.dispatchQuote(SecurityID->previousValue.getValue());
    }
  }
}

void NTP_ORD_TEMPLATE_DECODER::MDIncRefresh_123::process_end() {
  NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor& NtpOrdMdProcessor = NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor::GetInstance();

  NtpOrdMdProcessor.flushQuoteQueue(false);
}

void NTP_ORD_TEMPLATE_DECODER::MDSequenceReset::process() {
  NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor::GetInstance().seqReset();
}

void NTP_ORD_TEMPLATE_DECODER::MDSecurityList_111::process() {
  NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor& NtpOrdMdProcessor = NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor::GetInstance();
  std::string symbol = Symbol->previousValue.getValue().toString();

  if (NtpOrdMdProcessor.product_seen.find(this->SecurityID->previousValue.getValue()) ==
      NtpOrdMdProcessor.product_seen.end()) {
    NtpOrdMdProcessor.product_seen[this->SecurityID->previousValue.getValue()] = true;
    NtpOrdMdProcessor.dumpSecurityDefinition(this->SecurityID->previousValue.getValue(), symbol,
                                             this->SecurityGroup->previousValue.getValue().toString());
  }
}

void NTP_ORD_TEMPLATE_DECODER::MDSnapshotFullRefresh_124::process_start() {
  NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor& NtpOrdMdProcessor = NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor::GetInstance();

  if (!NtpOrdMdProcessor.IsProcessingSecurity(SecurityID->previousValue.getValue())) {
    return;
  }

  if (RptSeq->previousValue.getValue() < NtpOrdMdProcessor.rpt_seq_map_[SecurityID->previousValue.getValue()]) {
    return;
  }

  NtpOrdMdProcessor.updateSecurityTradingStatus(SecurityID->previousValue.getValue(), SecurityTradingStatus->currVal);

  if (!NtpOrdMdProcessor.IsSecurityInTrdingPhase(SecurityID->previousValue.getValue())) {
    return;
  }

  // Reset book update
  memset((void*)NtpOrdMdProcessor.cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));
  NtpOrdMdProcessor.cstr->msg_ = NTP_MDS::NTP_ORDER;
  strcpy(NtpOrdMdProcessor.cstr->data_.ntp_ordr_.contract_,
         NtpOrdMdProcessor.idmap_[SecurityID->previousValue.getValue()]);
  NtpOrdMdProcessor.cstr->data_.ntp_ordr_.level_ = 1;
  NtpOrdMdProcessor.cstr->data_.ntp_ordr_.type_ = 'J';
  NtpOrdMdProcessor.dispatchQuote(SecurityID->previousValue.getValue());
}

void NTP_ORD_TEMPLATE_DECODER::MDSnapshotFullRefresh_124::process() {
  NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor& NtpOrdMdProcessor = NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor::GetInstance();

  if (!NtpOrdMdProcessor.IsProcessingSecurity(SecurityID->previousValue.getValue())) {
    return;
  }

  if (RptSeq->previousValue.getValue() < NtpOrdMdProcessor.rpt_seq_map_[SecurityID->previousValue.getValue()]) {
    return;
  }

  NtpOrdMdProcessor.updateSecurityTradingStatus(SecurityID->previousValue.getValue(), SecurityTradingStatus->currVal);

  if (!NtpOrdMdProcessor.IsSecurityInTrdingPhase(SecurityID->previousValue.getValue())) {
    return;
  }

  // must check to prevent segfault
  if (MDEntryType->isValueSet) {
    if (MDEntryType->currVal.bytes[0] == '0' || MDEntryType->currVal.bytes[0] == '1') {
      // This is either a new bid or offer.
      memset((void*)NtpOrdMdProcessor.cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

      NtpOrdMdProcessor.cstr->msg_ = NTP_MDS::NTP_ORDER;

      strcpy(NtpOrdMdProcessor.cstr->data_.ntp_ordr_.contract_,
             NtpOrdMdProcessor.idmap_[(SecurityID->previousValue).getValue()]);

      NtpOrdMdProcessor.cstr->data_.ntp_ordr_.level_ =
          static_cast<uint16_t>((MDEntryPositionNo->isValueSet) ? MDEntryPositionNo->currVal : 0);
      NtpOrdMdProcessor.cstr->data_.ntp_ordr_.order_id_ =
          OrderID->isValueSet ? OrderID->currVal.parse_uint64_t_without_errors() : 0;
      NtpOrdMdProcessor.cstr->data_.ntp_ordr_.price_ =
          MDEntryPx_exponent->isValueSet
              ? (FFUtils::Decimal::value(MDEntryPx_exponent->currVal, (MDEntryPx_mantissa->previousValue).getValue()))
              : 0;
      NtpOrdMdProcessor.cstr->data_.ntp_ordr_.size_ =
          MDEntrySize->isValueSet ? (MDEntrySize->previousValue).getValue() : 0;
      NtpOrdMdProcessor.cstr->data_.ntp_ordr_.trd_qty_ =
          (uint32_t)(TradeVolume->isValueSet) ? (TradeVolume->previousValue.getValue()) : 0;
      NtpOrdMdProcessor.cstr->data_.ntp_ordr_.seqno_ = (RptSeq->previousValue).getValue();
      NtpOrdMdProcessor.cstr->data_.ntp_ordr_.action_ = 0;
      NtpOrdMdProcessor.cstr->data_.ntp_ordr_.type_ = (MDEntryType->currVal).bytes[0];
      NtpOrdMdProcessor.cstr->data_.ntp_ordr_.buyer_ =
          MDEntryBuyer->isValueSet ? (uint16_t)MDEntryBuyer->currVal.parse_uint64_t_without_errors() : 0;
      NtpOrdMdProcessor.cstr->data_.ntp_ordr_.seller_ =
          MDEntrySeller->isValueSet ? (uint16_t)MDEntrySeller->currVal.parse_uint64_t_without_errors() : 0;

      NtpOrdMdProcessor.dispatchQuote(SecurityID->previousValue.getValue());
    }
  }
}

void NTP_ORD_TEMPLATE_DECODER::MDSnapshotFullRefresh_124::process_end() {
  NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor& NtpOrdMdProcessor = NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor::GetInstance();
  // don't mark end of recovery till we have open sec trading status

  uint64_t security_id_ = SecurityID->previousValue.getValue();

  if (!NtpOrdMdProcessor.IsProcessingSecurity(security_id_)) {
    return;
  }

  if (RptSeq->previousValue.getValue() < NtpOrdMdProcessor.rpt_seq_map_[security_id_]) {
    return;
  }

  if (!NtpOrdMdProcessor.IsSecurityInTrdingPhase(security_id_)) {
    return;
  }

  NtpOrdMdProcessor.flushQuoteQueue(false);

  // Concurrent recovery method similar to what is used for NTP.
  // Instrument level sequencing.
  for (size_t i = 0; i < NtpOrdMdProcessor.message_buffer_[security_id_].size(); i++) {
    NTP_MDS::NTPCommonStruct* ntp_prev_md_ = NtpOrdMdProcessor.message_buffer_[security_id_][i];

    if (ntp_prev_md_->msg_ == NTP_MDS::NTP_ORDER &&
        ntp_prev_md_->data_.ntp_dels_.seqno_ <= NtpOrdMdProcessor.rpt_seq_map_[security_id_]) {
      continue;
    }

    memcpy(NtpOrdMdProcessor.cstr, ntp_prev_md_, sizeof(NTP_MDS::NTPCommonStruct));

    if (NtpOrdMdProcessor.cstr->msg_ == NTP_MDS::NTP_ORDER) {
      NtpOrdMdProcessor.dispatchQuote(security_id_);
    } else {
      NtpOrdMdProcessor.dispatchTrade(security_id_);
    }
  }

  NtpOrdMdProcessor.flushQuoteQueue(false);
  NtpOrdMdProcessor.message_buffer_[security_id_].clear();

  struct timeval tv;
  gettimeofday(&tv, NULL);
  fprintf(stderr, "Ending recovery for %u at seqno %u at time %ld \n", (unsigned)security_id_,
          (unsigned)(RptSeq->previousValue).getValue(), tv.tv_sec);
  NtpOrdMdProcessor.endRecovery(NtpOrdMdProcessor.idchnmap_[security_id_]);
  NtpOrdMdProcessor.recovery_[security_id_] = false;
  fprintf(stderr, "Snapshot jump from %u to %u \n", NtpOrdMdProcessor.rpt_seq_map_[security_id_],
          RptSeq->previousValue.getValue());
  NtpOrdMdProcessor.rpt_seq_map_[security_id_] = RptSeq->previousValue.getValue();
}

bool NTP_ORD_TEMPLATE_DECODER::MDSnapshotFullRefresh_124::findSecurity() {
  NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor& NtpOrdMdProcessor = NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor::GetInstance();

  /// not interested in instrument or instrument not in recovery mode
  if ((!NtpOrdMdProcessor.IsProcessingSecurity((SecurityID->previousValue).getValue())) ||
      (!NtpOrdMdProcessor.recovery_[(SecurityID->previousValue).getValue()])) {
    return false;
  }

  return true;
}

void NTP_ORD_TEMPLATE_DECODER::MDHeartbeat_101::process() {}
