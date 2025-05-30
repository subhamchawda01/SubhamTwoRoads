/**
   \file auto_puma_template.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "infracore/lwfixfast/auto_puma_template.hpp"
#include "infracore/lwfixfast/puma_md_processor.hpp"

/* .hpp template contains // implementation flag //
   MDTcpRequestReject_117  // 0 // // new version //

   MDSecurityList_148       // 1 //  // new version //
   MDSecurityList_141       // 1 //  // old version //
   MDSecurityList_111       // 1 //

   MDIncRefresh_81            // 1 //    // new version this is used for book reset //
   MDIncRefresh_150           // 1 //  // new version  //
   MDIncRefresh_138           // 1 //  // old version //
   MDIncRefresh_126           // 1 //

   MDSecurityStatus_142         // 1 //  // new version //
   MDSecurityStatus_134         // 1 //  // old version //
   MDSecurityStatus_125         // 1 //

   MDSnapshotFullRefresh_147       // 1 //  // new version //
   MDSnapshotFullRefresh_146       // 1 //  // old version //
   MDSnapshotFullRefresh_139       // 1 //
   MDSnapshotFullRefresh_128       // 1 //

   MDNewsMessage_143   // 1 //   //  new version //
   MDNewsMessage_137   // 1 //   // old version //
   MDNewsMessage_120   // 1 //

   MDHeartbeat_144    // 1 //  // new version //
   MDHeartbeat_129    // 1 //  // old version //
   MDHeartbeat_101    // 1 //

   MDLogon_118       // 0 //   // new version //
   MDLogout_119      // 0 //   // new version //
   MDSequenceReset (122)  // 1 // // new version //
 */

/*************************** MDSecurityList_148/141/111 START **********************************************/
// received on incremental stream and instrument definition stream
// we are interested in symbol security_id security_group
// NoOpField CopyField NoOpField ("" 0 "")
// no checks are placed
void PUMA_TEMPLATE_DECODER::MDSecurityList_148::process() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& pumaMdProcessor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();
  std::string symbol = Symbol->previousValue.getValue().toString();

  if (pumaMdProcessor.product_seen.find(this->SecurityID->previousValue.getValue()) ==
      pumaMdProcessor.product_seen.end()) {
    pumaMdProcessor.product_seen[this->SecurityID->previousValue.getValue()] = true;
    pumaMdProcessor.dumpSecurityDefinition(this->SecurityID->previousValue.getValue(), symbol,
                                           this->SecurityGroup->previousValue.getValue().toString());
  }
}

void PUMA_TEMPLATE_DECODER::MDSecurityList_141::process() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& pumaMdProcessor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();
  std::string symbol = Symbol->previousValue.getValue().toString();

  if (pumaMdProcessor.product_seen.find(this->SecurityID->previousValue.getValue()) ==
      pumaMdProcessor.product_seen.end()) {
    pumaMdProcessor.product_seen[this->SecurityID->previousValue.getValue()] = true;
    pumaMdProcessor.dumpSecurityDefinition(this->SecurityID->previousValue.getValue(), symbol,
                                           this->SecurityGroup->previousValue.getValue().toString());
  }
}
void PUMA_TEMPLATE_DECODER::MDSecurityList_111::process() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& pumaMdProcessor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();
  std::string symbol = Symbol->previousValue.getValue().toString();

  if (pumaMdProcessor.product_seen.find(this->SecurityID->previousValue.getValue()) ==
      pumaMdProcessor.product_seen.end()) {
    pumaMdProcessor.product_seen[this->SecurityID->previousValue.getValue()] = true;
    pumaMdProcessor.dumpSecurityDefinition(this->SecurityID->previousValue.getValue(), symbol,
                                           this->SecurityGroup->previousValue.getValue().toString());
  }
}
/*************************** MDSecurityList_148/141/111 END **********************************************/

/*************************** MDIncRefresh_81/145/138/126 START **********************************************/
// received on increament stream

// MessageType == X // MDIncRefresh
// NoMDEntries //
// MDEntryType == 0 (bid) 1 (offer) 2 (trade) 3 (index value) 4 (opening price)
//                5 (closing price) 6 (settlement price) B (Trade Volume) //
// MDUpdateAction == 0 (new) 1 (change) 2 (delete) 3 (delete thru) 4 (delete from) 5 (overlay)
// MDEntryPx
// MDEntrySize == Qty if (New && Bid || Offer || Trade || TradeVolume || OpeningPrice)
// MDEntryDate
// MDEntryTime
// TickDirection if (Trade || OpeningPrice) 0(+) 1(0+) 2(-) 3(0-)
// SecurityIDSource == 8 (exchange symbol)
// OrderID if (Bid || Offer && market by order)
// TradeID Trade
// NumberOfOrders if (price-depth book entry)
// MDEntryPositionNo if (bid || offer)
// RptSeq (sequence number per instrument update unique to MessageType = X)
// QuoteCondition A (open / active) B (closed / inactive) G (depth)
// TradeCondition A (cash market)
void PUMA_TEMPLATE_DECODER::MDIncRefresh_150::ProcessTrade(char entry_type, uint64_t sec_id) {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();

  uint32_t update_action = MDUpdateAction->previousValue.getValue();

  // Ignore trade bust for now
  if (update_action == 2) {
    return;
  }

  int32_t size = MDEntrySize->isValueSet ? MDEntrySize->previousValue.getValue() : 0;
  uint64_t total_qty = TradeVolume->isValueSet ? TradeVolume->previousValue.getValue() : 0;
  uint32_t seq_no = RptSeq->previousValue.getValue();
  double price = FFUtils::Decimal::value(MDEntryPx_exponent->currVal, MDEntryPx_mantissa->previousValue.getValue());
  bool is_last = false;
  bool is_cross = false;

  if (TradeCondition->previousValue.isAssigned() && !TradeCondition->previousValue.getValue().isnull) {
    for (uint32_t i = 0; i < TradeCondition->previousValue.getValue().len; i++) {
      switch (TradeCondition->previousValue.getValue().bytes[i]) {
        // Cross trade
        case 'X': {
          is_cross = true;
          break;
        }

        // Last
        case 'L': {
          is_last = true;
          break;
        }
        default:
          break;
      }
    }
  }

  md_processor.UpdateTrade(sec_id, size, total_qty, seq_no, price, is_last, is_cross);
}

void PUMA_TEMPLATE_DECODER::MDIncRefresh_150::ProcessOrderFeed() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();

  if (!md_processor.IsProcessingSecurity(SecurityID->previousValue.getValue())) {
    return;
  }

  if (RptSeq->previousValue.getValue() < md_processor.rpt_seq_map_[SecurityID->previousValue.getValue()]) {
    return;
  }

  md_processor.updateSeqno(SecurityID->previousValue.getValue(), RptSeq->previousValue.getValue());

  if ((MDEntryType->previousValue.getValue().bytes[0] == '2') && (MDUpdateAction->previousValue.getValue() == 2)) {
    //  processTradeBust (sec_id , sq_no, trd_qty_, tot_qty_);
    return;
  }

  /// process trade
  if (MDEntryType->previousValue.getValue().bytes[0] == '2') {
    memset((void*)md_processor.cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

    md_processor.cstr->msg_ = NTP_MDS::NTP_TRADE;

    if (SecurityID->previousValue.getValue()) {
      strcpy(md_processor.cstr->data_.ntp_trds_.contract_,
             md_processor.ref_data_[SecurityID->previousValue.getValue()]->secname);
    }

    md_processor.cstr->data_.ntp_trds_.trd_qty_ =
        (MDEntrySize->isValueSet) ? (MDEntrySize->previousValue).getValue() : 0;
    md_processor.cstr->data_.ntp_trds_.tot_qty_ =
        (TradeVolume->isValueSet) ? (TradeVolume->previousValue).getValue() : 0;
    md_processor.cstr->data_.ntp_trds_.seqno_ = (RptSeq->previousValue).getValue();
    md_processor.cstr->data_.ntp_trds_.trd_px_ =
        FFUtils::Decimal::value(MDEntryPx_exponent->currVal, (MDEntryPx_mantissa->previousValue).getValue());
    md_processor.cstr->data_.ntp_trds_.is_last_ = strstr((TradeCondition->previousValue).getValue().bytes, "L") != NULL;

    if (TradeCondition->previousValue.isAssigned() && !TradeCondition->previousValue.getValue().isnull) {
      for (uint32_t i = 0; i < TradeCondition->previousValue.getValue().len; ++i) {
        if (TradeCondition->previousValue.getValue().bytes[i] == 'X') {
          return;
          fprintf(stderr, " Bad TradeCondition %s \n", (TradeCondition->previousValue).getValue().toString().c_str());

          if (SecurityID->previousValue.getValue()) {
            strcpy(md_processor.cstr->data_.ntp_trds_.contract_,
                   md_processor.ref_data_[SecurityID->previousValue.getValue()]->secname);
          }

          int32_t t_trade_trd_qty_ = (MDEntrySize->isValueSet) ? (MDEntrySize->previousValue).getValue() : 0;
          uint64_t t_trade_tot_qty_ = (TradeVolume->isValueSet) ? (TradeVolume->previousValue).getValue() : 0;
          double t_trade_px_ =
              FFUtils::Decimal::value(MDEntryPx_exponent->currVal, (MDEntryPx_mantissa->previousValue).getValue());

          fprintf(stderr, "Trade: Contract: %s, Trade_size: %d, Tot_Qty: %d, Trade_Px: %f\n",
                  md_processor.cstr->data_.ntp_trds_.contract_, t_trade_trd_qty_, (int)t_trade_tot_qty_, t_trade_px_);

          md_processor.cstr->data_.ntp_trds_.flags_[0] = 'X';
          // return;
        }
      }
    }

    if (md_processor.recovery_[SecurityID->previousValue.getValue()]) {
      NTP_MDS::NTPCommonStruct* new_ntp_mds_ = new NTP_MDS::NTPCommonStruct();
      memset((void*)new_ntp_mds_, 0, sizeof(NTP_MDS::NTPCommonStruct));
      memcpy(new_ntp_mds_, md_processor.cstr, sizeof(NTP_MDS::NTPCommonStruct));
      md_processor.message_buffer_[SecurityID->previousValue.getValue()].push_back(new_ntp_mds_);
    } else {
      md_processor.dispatchTrade(SecurityID->previousValue.getValue());
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

    memset((void*)md_processor.cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

    md_processor.cstr->msg_ = NTP_MDS::NTP_ORDER;

    strcpy(md_processor.cstr->data_.ntp_ordr_.contract_, md_processor.idmap_[(SecurityID->previousValue).getValue()]);

    md_processor.cstr->data_.ntp_ordr_.level_ =
        static_cast<uint16_t>((MDEntryPositionNo->isValueSet) ? MDEntryPositionNo->currVal : 0);
    md_processor.cstr->data_.ntp_ordr_.order_id_ =
        OrderID->isValueSet ? OrderID->currVal.parse_uint64_t_without_errors() : 0;
    md_processor.cstr->data_.ntp_ordr_.price_ =
        MDEntryPx_exponent->isValueSet
            ? (FFUtils::Decimal::value(MDEntryPx_exponent->currVal, (MDEntryPx_mantissa->previousValue).getValue()))
            : 0;
    md_processor.cstr->data_.ntp_ordr_.size_ = MDEntrySize->isValueSet ? (MDEntrySize->previousValue).getValue() : 0;
    md_processor.cstr->data_.ntp_ordr_.trd_qty_ =
        (uint32_t)(TradeVolume->isValueSet) ? (TradeVolume->previousValue.getValue()) : 0;
    md_processor.cstr->data_.ntp_ordr_.seqno_ = (RptSeq->previousValue).getValue();
    md_processor.cstr->data_.ntp_ordr_.action_ = (MDUpdateAction->previousValue).getValue();
    md_processor.cstr->data_.ntp_ordr_.type_ = (MDEntryType->previousValue).getValue().bytes[0];
    md_processor.cstr->data_.ntp_ordr_.buyer_ =
        MDEntryBuyer->isValueSet ? (uint16_t)MDEntryBuyer->currVal.parse_uint64_t_without_errors() : 0;
    md_processor.cstr->data_.ntp_ordr_.seller_ =
        MDEntrySeller->isValueSet ? (uint16_t)MDEntrySeller->currVal.parse_uint64_t_without_errors() : 0;

    if (md_processor.recovery_[SecurityID->previousValue.getValue()]) {
      NTP_MDS::NTPCommonStruct* new_ntp_mds_ = new NTP_MDS::NTPCommonStruct();
      memset((void*)new_ntp_mds_, 0, sizeof(NTP_MDS::NTPCommonStruct));
      memcpy(new_ntp_mds_, md_processor.cstr, sizeof(NTP_MDS::NTPCommonStruct));
      md_processor.message_buffer_[SecurityID->previousValue.getValue()].push_back(new_ntp_mds_);
    } else {
      md_processor.dispatchQuote(SecurityID->previousValue.getValue());
    }
  }
}

void PUMA_TEMPLATE_DECODER::MDIncRefresh_150::ProcessPriceFeed() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();
  uint64_t sec_id = SecurityID->previousValue.getValue();

  if (!md_processor.IsProcessingSecurity(sec_id)) {
    return;
  }

  uint32_t seq_no = RptSeq->previousValue.getValue();

  // Update the sequence number for this sec_id
  if (md_processor.UpdateSeqno(sec_id, seq_no)) {
    return;
  }

  // From "Market Data Entry Types" chapter of
  // http://www.bmfbovespa.com.br/en-us/services/download/UMDF_MarketDataSpecification_v2.0.13.pdf
  // 0 - Bid
  // 1 - Ask
  // 2 - Trade
  // 3 - Index Value
  // 4 - Opening price
  // 5 - Closing price
  // 6 - Settlement price
  // 7 - Trading session high price
  // 8 - Trading session low price
  // 9 - Trading session VWAP price
  // A - Imbalance
  // B - Trade Volume
  // C - Open Interest
  // J - Empty Book
  // c - Security trading state
  // g - Price band
  // h - Quantity band
  // D - Composite underlying price
  char entry_type = MDEntryType->previousValue.getValue().bytes[0];

  switch (entry_type) {
    case '0':
    case '1': {
      ProcessBookDelta(entry_type, sec_id);
      break;
    }
    case '2': {
      ProcessTrade(entry_type, sec_id);
      break;
    }
    case '4': {
      ProcessOpeninigPrice(entry_type, sec_id);
      break;
    }
    case '6': {
      ProcessSettlementPrice(entry_type, sec_id);
      break;
    }
    case 'A': {
      ProcessImbalance(entry_type, sec_id);
      break;
    }
    default: { } break; }
}

void PUMA_TEMPLATE_DECODER::MDIncRefresh_150::ProcessBookDelta(char entry_type, uint64_t sec_id) {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();

  uint16_t level = static_cast<uint16_t>(MDEntryPositionNo->isValueSet ? MDEntryPositionNo->currVal : 0);
  uint16_t num_ords = static_cast<uint16_t>(NumberOfOrders->isValueSet ? NumberOfOrders->currVal : 0);
  double price =
      MDEntryPx_exponent->isValueSet
          ? (FFUtils::Decimal::value(MDEntryPx_exponent->currVal, MDEntryPx_mantissa->previousValue.getValue()))
          : 0;
  int32_t size = MDEntrySize->isValueSet ? (MDEntrySize->previousValue).getValue() : 0;
  uint32_t seq_no = (RptSeq->previousValue).getValue();
  uint32_t update_action = (MDUpdateAction->previousValue).getValue();

  md_processor.UpdateBookDelta(sec_id, level, num_ords, price, size, seq_no, entry_type, update_action);
}

void PUMA_TEMPLATE_DECODER::MDIncRefresh_150::ProcessOpeninigPrice(char entry_type, uint64_t sec_id) {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();

  double price =
      MDEntryPx_exponent->isValueSet
          ? (FFUtils::Decimal::value(MDEntryPx_exponent->currVal, MDEntryPx_mantissa->previousValue.getValue()))
          : 0;
  int32_t size = MDEntrySize->isValueSet ? (MDEntrySize->previousValue).getValue() : 0;
  uint32_t seq_no = (RptSeq->previousValue).getValue();
  uint32_t open_close_flag = OpenCloseSettlFlag->previousValue.getValue();

  md_processor.UpdateOpeningPrice(sec_id, price, size, seq_no, open_close_flag);
}

void PUMA_TEMPLATE_DECODER::MDIncRefresh_150::ProcessSettlementPrice(char entry_type, uint64_t sec_id) {
  // Only process the last day price
  if (OpenCloseSettlFlag->previousValue.getValue() != 4) {
    return;
  }

  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();

  double price = FFUtils::Decimal::value(MDEntryPx_exponent->currVal, (MDEntryPx_mantissa->previousValue).getValue());
  md_processor.DumpClosePrice(sec_id, price);
}

void PUMA_TEMPLATE_DECODER::MDIncRefresh_150::ProcessImbalance(char entry_type, uint64_t sec_id) {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();

  int32_t size = MDEntrySize->isValueSet ? (MDEntrySize->previousValue).getValue() : 0;
  uint32_t seq_no = (RptSeq->previousValue).getValue();
  char condition = ' ';

  if (TradeCondition->previousValue.isAssigned() && !TradeCondition->previousValue.getValue().isnull) {
    condition = TradeCondition->previousValue.getValue().bytes[0];
  }

  md_processor.UpdateImbalance(sec_id, size, seq_no, condition);
}

void PUMA_TEMPLATE_DECODER::MDIncRefresh_150::process() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();
  int bmf_feed_type = md_processor.GetFeedType();

  // Check if we're interested in this instrument
  if (bmf_feed_type == 3) {
    ProcessOrderFeed();
  } else {
    ProcessPriceFeed();
  }
}

void PUMA_TEMPLATE_DECODER::MDIncRefresh_150::process_end() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();
  md_processor.flushQuoteQueue(false);
}

/*************************** MDIncRefresh_81/145/138/126 END **********************************************/

/*************************** MDSecurityStatus_142/134/125 START **********************************************/

// Possible Instrument states:
// 02 - Trading halt
// 04 - No-Open
// 17 - Ready to trade
// 18 - Not available for trading
// 21 - Pre-Open
// 101 - Final Closing Call
void PUMA_TEMPLATE_DECODER::MDSecurityStatus_142::process() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();
  uint64_t sec_id = SecurityID->previousValue.getValue();
  uint64_t open_time = TradSesOpenTime->previousValue.getValue();
  uint64_t transact_time = TransactTime->previousValue.getValue();
  uint32_t trading_event = SecurityTradingEvent->previousValue.getValue();
  uint32_t trading_status = 0;

  if (sec_id == 0) {
    char* group = SecurityGroup->previousValue.getValue().bytes;
    int group_id = group[0] * 256 + group[1];
    trading_status = TradingSessionSubID->previousValue.getValue().parse_uint64_t_without_errors();
    md_processor.UpdateGroupTradingStatusInc(group_id, trading_status, open_time, transact_time, trading_event, 0);

  } else if (md_processor.IsProcessingSecurity(sec_id)) {
    trading_status = SecurityTradingStatus->previousValue.getValue();
    md_processor.UpdateSecurityTradingStatus(sec_id, trading_status, open_time, transact_time, trading_event, 0);
  }
}

void PUMA_TEMPLATE_DECODER::MDSecurityStatus_134::process() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();
  uint64_t sec_id = SecurityID->previousValue.getValue();

  // Check if we're interested in this instrument
  if (!md_processor.IsProcessingSecurity(sec_id)) {
    return;
  }

  uint32_t trading_status = SecurityTradingStatus->previousValue.getValue();
  uint64_t open_time = TradSesOpenTime->previousValue.getValue();
  uint64_t transact_time = TransactTime->previousValue.getValue();
  uint32_t trading_event = SecurityTradingEvent->previousValue.getValue();

  md_processor.UpdateSecurityTradingStatus(sec_id, trading_status, open_time, transact_time, trading_event, 0);
}

void PUMA_TEMPLATE_DECODER::MDSecurityStatus_125::process() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();
  uint64_t sec_id = SecurityID->previousValue.getValue();

  // Check if we're interested in this instrument
  if (!md_processor.IsProcessingSecurity(sec_id)) {
    return;
  }

  uint32_t trading_status = SecurityTradingStatus->previousValue.getValue();
  uint64_t open_time = TradSesOpenTime->previousValue.getValue();
  uint64_t transact_time = TransactTime->previousValue.getValue();
  uint32_t trading_event = SecurityTradingEvent->previousValue.getValue();

  md_processor.UpdateSecurityTradingStatus(sec_id, trading_status, open_time, transact_time, trading_event, 0);
}
/*************************** MDSecurityStatus_142/134/125 END **********************************************/

/*************************** MDSnapshotFullRefresh_147/146/139/128 START **********************************************/
// received on snapshot stream

void PUMA_TEMPLATE_DECODER::MDSnapshotFullRefresh_147::process_order_feed() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();

  if (!md_processor.IsProcessingSecurity(SecurityID->previousValue.getValue())) {
    return;
  }

  if (RptSeq->previousValue.getValue() < md_processor.rpt_seq_map_[SecurityID->previousValue.getValue()]) {
    return;
  }

  // must check to prevent segfault
  if (MDEntryType->isValueSet) {
    if (MDEntryType->currVal.bytes[0] == '0' || MDEntryType->currVal.bytes[0] == '1') {
      // This is either a new bid or offer.
      memset((void*)md_processor.cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

      md_processor.cstr->msg_ = NTP_MDS::NTP_ORDER;

      strcpy(md_processor.cstr->data_.ntp_ordr_.contract_, md_processor.idmap_[(SecurityID->previousValue).getValue()]);

      md_processor.cstr->data_.ntp_ordr_.level_ =
          static_cast<uint16_t>((MDEntryPositionNo->isValueSet) ? MDEntryPositionNo->currVal : 0);
      md_processor.cstr->data_.ntp_ordr_.order_id_ =
          OrderID->isValueSet ? OrderID->currVal.parse_uint64_t_without_errors() : 0;
      md_processor.cstr->data_.ntp_ordr_.price_ =
          MDEntryPx_exponent->isValueSet
              ? (FFUtils::Decimal::value(MDEntryPx_exponent->currVal, (MDEntryPx_mantissa->previousValue).getValue()))
              : 0;
      md_processor.cstr->data_.ntp_ordr_.size_ = MDEntrySize->isValueSet ? (MDEntrySize->previousValue).getValue() : 0;
      md_processor.cstr->data_.ntp_ordr_.trd_qty_ =
          (uint32_t)(TradeVolume->isValueSet) ? (TradeVolume->previousValue.getValue()) : 0;
      md_processor.cstr->data_.ntp_ordr_.seqno_ = (RptSeq->previousValue).getValue();
      md_processor.cstr->data_.ntp_ordr_.action_ = 0;
      md_processor.cstr->data_.ntp_ordr_.type_ = (MDEntryType->currVal).bytes[0];
      md_processor.cstr->data_.ntp_ordr_.buyer_ =
          MDEntryBuyer->isValueSet ? (uint16_t)MDEntryBuyer->currVal.parse_uint64_t_without_errors() : 0;
      md_processor.cstr->data_.ntp_ordr_.seller_ =
          MDEntrySeller->isValueSet ? (uint16_t)MDEntrySeller->currVal.parse_uint64_t_without_errors() : 0;

      md_processor.dispatchQuote(SecurityID->previousValue.getValue());
    }
  }
}
void PUMA_TEMPLATE_DECODER::MDSnapshotFullRefresh_147::process() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();
  int bmf_feed_type = md_processor.GetFeedType();

  if (bmf_feed_type == 3) {
    process_order_feed();
  } else {
    uint64_t sec_id = SecurityID->previousValue.getValue();
    uint64_t open_time = TradSesOpenTime->isValueSet ? TradSesOpenTime->currVal : 0;
    uint32_t rpt_seq = RptSeq->previousValue.getValue();

    uint32_t trading_status = 0;
    if (SecurityTradingStatus->isValueSet) {
      if (!md_processor.IsProcessingSecurity(sec_id)) {
        return;
      }

      trading_status = SecurityTradingStatus->currVal;
      md_processor.UpdateSecurityTradingStatus(sec_id, trading_status, open_time, 0, 0, rpt_seq);

    } else {
      trading_status = TradingSessionSubID->previousValue.getValue().parse_uint64_t_without_errors();
      md_processor.UpdateGroupTradingStatusRefresh(sec_id, trading_status, open_time, 0, 0, rpt_seq);
    }
  }
}

void PUMA_TEMPLATE_DECODER::MDSnapshotFullRefresh_147::process_end() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();
  int bmf_feed_type = md_processor.GetFeedType();
  if (bmf_feed_type == 3) {
    PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();
    // don't mark end of recovery till we have open sec trading status

    uint64_t security_id_ = SecurityID->previousValue.getValue();

    if (!md_processor.IsProcessingSecurity(security_id_)) {
      return;
    }

    if (RptSeq->previousValue.getValue() < md_processor.rpt_seq_map_[security_id_]) {
      return;
    }

    md_processor.flushQuoteQueue(false);

    // Concurrent recovery method similar to what is used for NTP.
    // Instrument level sequencing.
    for (size_t i = 0; i < md_processor.message_buffer_[security_id_].size(); i++) {
      NTP_MDS::NTPCommonStruct* ntp_prev_md_ = md_processor.message_buffer_[security_id_][i];

      if (ntp_prev_md_->msg_ == NTP_MDS::NTP_ORDER &&
          ntp_prev_md_->data_.ntp_dels_.seqno_ <= md_processor.rpt_seq_map_[security_id_]) {
        continue;
      }

      memcpy(md_processor.cstr, ntp_prev_md_, sizeof(NTP_MDS::NTPCommonStruct));

      if (md_processor.cstr->msg_ == NTP_MDS::NTP_ORDER) {
        md_processor.dispatchQuote(security_id_);
      } else {
        md_processor.dispatchTrade(security_id_);
      }
    }

    md_processor.flushQuoteQueue(false);
    md_processor.message_buffer_[security_id_].clear();

    struct timeval tv;
    gettimeofday(&tv, NULL);
    fprintf(stderr, "Ending recovery for %u at seqno %u at time %ld \n", (unsigned)security_id_,
            (unsigned)(RptSeq->previousValue).getValue(), tv.tv_sec);
    md_processor.EndRecovery(security_id_);
    md_processor.recovery_[security_id_] = false;
    fprintf(stderr, "Snapshot jump from %u to %u \n", md_processor.rpt_seq_map_[security_id_],
            RptSeq->previousValue.getValue());
    md_processor.rpt_seq_map_[security_id_] = RptSeq->previousValue.getValue();
  }
}

void PUMA_TEMPLATE_DECODER::MDSnapshotFullRefresh_147::process_start() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();
  int bmf_feed_type = md_processor.GetFeedType();

  if (bmf_feed_type == 3) {
    if (!md_processor.IsProcessingSecurity(SecurityID->previousValue.getValue())) {
      return;
    }

    if (RptSeq->previousValue.getValue() < md_processor.rpt_seq_map_[SecurityID->previousValue.getValue()]) {
      return;
    }

    // Reset book update
    memset((void*)md_processor.cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));
    md_processor.cstr->msg_ = NTP_MDS::NTP_ORDER;
    strcpy(md_processor.cstr->data_.ntp_ordr_.contract_, md_processor.idmap_[SecurityID->previousValue.getValue()]);
    md_processor.cstr->data_.ntp_ordr_.level_ = 1;
    md_processor.cstr->data_.ntp_ordr_.type_ = 'J';
    md_processor.dispatchQuote(SecurityID->previousValue.getValue());
  }
}

void PUMA_TEMPLATE_DECODER::MDSnapshotFullRefresh_151::process() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();

  uint64_t sec_id = SecurityID->previousValue.getValue();
  uint64_t open_time = TradSesOpenTime->isValueSet ? TradSesOpenTime->currVal : 0;
  uint32_t rpt_seq = RptSeq->previousValue.getValue();

  uint32_t trading_status = 0;
  if (SecurityTradingStatus->isValueSet) {
    if (!md_processor.IsProcessingSecurity(sec_id)) {
      return;
    }

    trading_status = SecurityTradingStatus->currVal;
    md_processor.UpdateSecurityTradingStatus(sec_id, trading_status, open_time, 0, 0, rpt_seq);

  } else {
    trading_status = TradingSessionSubID->previousValue.getValue().parse_uint64_t_without_errors();
    md_processor.UpdateGroupTradingStatusRefresh(sec_id, trading_status, open_time, 0, 0, rpt_seq);
  }
}

void PUMA_TEMPLATE_DECODER::MDSnapshotFullRefresh_151::process_end() {}

/*************************** MDSnapshotFullRefresh_147/146/139/128 END **********************************************/

/*************************** MDNewsMessage_143/137/120 START **********************************************/
void PUMA_TEMPLATE_DECODER::MDNewsMessage_143::process() {}

void PUMA_TEMPLATE_DECODER::MDNewsMessage_137::process() {}

void PUMA_TEMPLATE_DECODER::MDNewsMessage_120::process() {}

/*************************** MDNewsMessage_143/137/120 END **********************************************/

/*************************** MDHeartbeat_144/129/101 START **********************************************/
void PUMA_TEMPLATE_DECODER::MDHeartbeat_144::process() {}

void PUMA_TEMPLATE_DECODER::MDHeartbeat_129::process() {}

void PUMA_TEMPLATE_DECODER::MDHeartbeat_101::process() {}
/*************************** MDHeartbeat_144/129/101 END **********************************************/

/*************************** MDSequenceReset START **********************************************/
void PUMA_TEMPLATE_DECODER::MDSequenceReset::process() { PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance().seqReset(); }
/*************************** MDSequenceReset END **********************************************/

/********************************SHOULD BE STATIC FUNCTIONS *********************************************/
bool PUMA_TEMPLATE_DECODER::MDSnapshotFullRefresh_147::findSecurity() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();

  uint64_t sec_id = SecurityID->previousValue.getValue();

  // Not interested in instrument or instrument not in recovery mode
  if (!md_processor.IsProcessingSecurity(sec_id) || !md_processor.ref_data_[sec_id]->recovery) {
    return false;
  }

  return true;
}

bool PUMA_TEMPLATE_DECODER::MDSnapshotFullRefresh_151::findSecurity() {
  PUMA_MD_PROCESSOR::PumaMDProcessor& md_processor = PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance();

  uint64_t sec_id = SecurityID->previousValue.getValue();

  // Not interested in instrument or instrument not in recovery mode
  if (!md_processor.IsProcessingSecurity(sec_id) || !md_processor.ref_data_[sec_id]->recovery) {
    return false;
  }

  return true;
}
/********************************STATIC FUNCTIONS *********************************************/
