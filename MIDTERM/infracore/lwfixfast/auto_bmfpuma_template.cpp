/**
   \file auto_bmfpuma_template.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "infracore/lwfixfast/auto_bmfpuma_template.hpp"
#include "infracore/lwfixfast/ntp_md_processor.hpp"

void BMFPUMA_TEMPLATE_DECODER::MDNewsMessage_120::process() {}

// Possible Instrument states:
// 02 - Trading halt
// 04 - No-Open
// 17 - Ready to trade
// 18 - Not available for trading
// 21 - Pre-Open
// 101 - Final Closing Call
void BMFPUMA_TEMPLATE_DECODER::MDSecurityStatus_125::process() {
  NTP_MD_PROCESSOR::NtpMDProcessor& md_processor = NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance();

  uint64_t sec_id = SecurityID->previousValue.getValue();
  uint64_t open_time = TradSesOpenTime->previousValue.getValue();
  uint64_t transact_time = TransactTime->previousValue.getValue();
  uint32_t trading_event = SecurityTradingEvent->previousValue.getValue();
  uint32_t trading_status = 0;

  if (sec_id == 0) {
    // This is a group status update
    char* group = SecurityGroup->previousValue.getValue().bytes;
    int group_id = group[0] * 256 + group[1];
    trading_status = TradingSessionSubID->previousValue.getValue().parse_uint64_t_without_errors();
    md_processor.UpdateGroupTradingStatusInc(group_id, trading_status, open_time, transact_time, trading_event, 0);

  } else if (md_processor.IsProcessingSecurity(sec_id)) {
    // This is security specific status update
    trading_status = SecurityTradingStatus->previousValue.getValue();

    md_processor.UpdateSecurityTradingStatus(sec_id, trading_status, open_time, transact_time, trading_event, 0);
  }
}

// entry_type == 2 for this message
// Extract trade parameters and pass that to the md processor
void BMFPUMA_TEMPLATE_DECODER::MDIncRefresh_150::ProcessTrade(char entry_type, uint64_t sec_id) {
  NTP_MD_PROCESSOR::NtpMDProcessor& md_processor = NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance();

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

// entry_type == 0 || entry_type == 1 for this message
// Extract book delta parameters and pass that to the md processor
void BMFPUMA_TEMPLATE_DECODER::MDIncRefresh_150::ProcessBookDelta(char entry_type, uint64_t sec_id) {
  NTP_MD_PROCESSOR::NtpMDProcessor& md_processor = NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance();

  uint16_t level = static_cast<uint16_t>(MDEntryPositionNo->isValueSet ? MDEntryPositionNo->currVal : 0);
  uint16_t num_ords = static_cast<uint16_t>(NumberOfOrders->isValueSet ? NumberOfOrders->currVal : 0);
  double price =
      MDEntryPx_exponent->isValueSet
          ? (FFUtils::Decimal::value(MDEntryPx_exponent->currVal, MDEntryPx_mantissa->previousValue.getValue()))
          : 0;
  int32_t size = MDEntrySize->isValueSet ? (MDEntrySize->previousValue).getValue() : 0;
  uint32_t seq_no = (RptSeq->previousValue).getValue();
  uint32_t update_action = (MDUpdateAction->previousValue).getValue();
  uint64_t buyer = MDEntryBuyer->currVal.parse_uint64_t_without_errors();
  uint64_t seller = MDEntrySeller->currVal.parse_uint64_t_without_errors();

  md_processor.UpdateBookDelta(sec_id, level, num_ords, price, size, seq_no, entry_type, update_action, buyer, seller);
}

void BMFPUMA_TEMPLATE_DECODER::MDIncRefresh_150::ProcessOpeninigPrice(char entry_type, uint64_t sec_id) {
  NTP_MD_PROCESSOR::NtpMDProcessor& md_processor = NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance();

  double price =
      MDEntryPx_exponent->isValueSet
          ? (FFUtils::Decimal::value(MDEntryPx_exponent->currVal, MDEntryPx_mantissa->previousValue.getValue()))
          : 0;
  int32_t size = MDEntrySize->isValueSet ? (MDEntrySize->previousValue).getValue() : 0;
  uint32_t seq_no = (RptSeq->previousValue).getValue();
  uint32_t open_close_flag = OpenCloseSettlFlag->previousValue.getValue();

  md_processor.UpdateOpeningPrice(sec_id, price, size, seq_no, open_close_flag);
}

void BMFPUMA_TEMPLATE_DECODER::MDIncRefresh_150::ProcessImbalance(char entry_type, uint64_t sec_id) {
  NTP_MD_PROCESSOR::NtpMDProcessor& md_processor = NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance();

  int32_t size = MDEntrySize->isValueSet ? (MDEntrySize->previousValue).getValue() : 0;
  uint32_t seq_no = (RptSeq->previousValue).getValue();
  char condition = ' ';

  if (TradeCondition->previousValue.isAssigned() && !TradeCondition->previousValue.getValue().isnull) {
    condition = TradeCondition->previousValue.getValue().bytes[0];
  }

  md_processor.UpdateImbalance(sec_id, size, seq_no, condition);
}

// Process incremental market update
void BMFPUMA_TEMPLATE_DECODER::MDIncRefresh_150::process() {
  uint64_t sec_id = SecurityID->previousValue.getValue();

  NTP_MD_PROCESSOR::NtpMDProcessor& md_processor = NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance();

  // Check if we're interested in this instrument
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
    case 'A': {
      ProcessImbalance(entry_type, sec_id);
      break;
    }
    default: { } break; }
}

void BMFPUMA_TEMPLATE_DECODER::MDIncRefresh_150::process_end() {
  NTP_MD_PROCESSOR::NtpMDProcessor& md_processor = NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance();

  md_processor.flushQuoteQueue(false);
}

void BMFPUMA_TEMPLATE_DECODER::MDSequenceReset::process() {
  NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance().seqReset();
}

void BMFPUMA_TEMPLATE_DECODER::MDSecurityList_141::process() {
  NTP_MD_PROCESSOR::NtpMDProcessor& md_processor = NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance();
  std::string symbol = Symbol->previousValue.getValue().toString();

  if (md_processor.product_seen.find(SecurityID->previousValue.getValue()) == md_processor.product_seen.end()) {
    md_processor.product_seen[SecurityID->previousValue.getValue()] = true;
    md_processor.DumpSecurityDefinition(SecurityID->previousValue.getValue(), symbol,
                                        SecurityGroup->previousValue.getValue().toString());
  }
}

void BMFPUMA_TEMPLATE_DECODER::MDSecurityList_111::process() {
  NTP_MD_PROCESSOR::NtpMDProcessor& md_processor = NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance();
  std::string symbol = Symbol->previousValue.getValue().toString();

  if (md_processor.product_seen.find(SecurityID->previousValue.getValue()) == md_processor.product_seen.end()) {
    md_processor.product_seen[SecurityID->previousValue.getValue()] = true;
    md_processor.DumpSecurityDefinition(SecurityID->previousValue.getValue(), symbol,
                                        SecurityGroup->previousValue.getValue().toString());
  }
}

void BMFPUMA_TEMPLATE_DECODER::MDSnapshotFullRefresh_147::process() {
  NTP_MD_PROCESSOR::NtpMDProcessor& md_processor = NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance();

  uint64_t sec_id = SecurityID->previousValue.getValue();

  uint64_t open_time = TradSesOpenTime->isValueSet ? TradSesOpenTime->currVal : 0;
  uint64_t rpt_seq = RptSeq->previousValue.getValue();

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

bool BMFPUMA_TEMPLATE_DECODER::MDSnapshotFullRefresh_147::findSecurity() {
  NTP_MD_PROCESSOR::NtpMDProcessor& md_processor = NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance();

  uint64_t sec_id = SecurityID->previousValue.getValue();

  // Not interested in instrument or instrument not in recovery mode
  if (!md_processor.IsProcessingSecurity(sec_id) || !md_processor.ref_data_[sec_id]->recovery) {
    return false;
  }

  return true;
}

void BMFPUMA_TEMPLATE_DECODER::MDSnapshotFullRefresh_147::process_end() {}

void BMFPUMA_TEMPLATE_DECODER::MDHeartbeat_101::process() {}
