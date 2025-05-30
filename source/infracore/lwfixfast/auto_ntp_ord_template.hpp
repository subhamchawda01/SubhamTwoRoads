// AUTO GENERATED CLASS FOR TEMPLATE files/BMF/templates-UMDF-NTP.xml
// DO NOT MODIFY

#pragma once

#include "infracore/lwfixfast/FastDecoder.hpp"
namespace NTP_ORD_TEMPLATE_DECODER {
class MDTcpRequestReject_117 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  CopyField<FFUtils::ByteArr> *MDReqID;
  CopyField<FFUtils::ByteArr> *MDReqRejReason;
  NoOpField<FFUtils::ByteArr> *Text;

  // Constructor
  MDTcpRequestReject_117() {
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("Y"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MDReqID = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    MDReqRejReason = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    Text = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MessageType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode();
    MDReqID->decode(input, pmap0);
    MDReqRejReason->decode(input, pmap0);
    Text->decode(input);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MessageType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
    MDReqID->reset();
    MDReqRejReason->reset();
    Text->reset();
  }

  // Destructor
  virtual ~MDTcpRequestReject_117() {
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete MDReqID;
    delete MDReqRejReason;
    delete Text;
  }
};
class MDSecurityList_111 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *TotNoRelatedSym;
  NoOpField<FFUtils::ByteArr> *LastFragment;
  // fields inside sequence RelatedSymbols
  NoOpField<uint32_t> *NoRelatedSym;
  NoOpField<FFUtils::ByteArr> *Symbol;
  CopyField<uint64_t> *SecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence ApplIds
  NoOpField<uint32_t> *NoApplIds;
  NoOpField<FFUtils::ByteArr> *ApplId;
  // fields inside sequence FeedTypes
  NoOpField<uint32_t> *NoMDFeedTypes;
  NoOpField<FFUtils::ByteArr> *MDFeedType;
  NoOpField<uint32_t> *MarketDepth;
  // fields inside sequence FeedTypes ends
  // fields inside sequence ApplIds ends
  // fields inside sequence SecurityAltIDs
  NoOpField<uint32_t> *NoSecurityAltID;
  NoOpField<FFUtils::ByteArr> *SecurityAltID;
  CopyField<FFUtils::ByteArr> *SecurityAltIDSource;
  // fields inside sequence SecurityAltIDs ends
  // fields inside sequence Underlyings
  NoOpField<uint32_t> *NoUnderlyings;
  NoOpField<FFUtils::ByteArr> *UnderlyingSymbol;
  NoOpField<uint64_t> *UnderlyingSecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *UnderlyingSecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *UnderlyingSecurityExchange;
  // fields inside sequence Underlyings ends
  // fields inside sequence Legs
  NoOpField<uint32_t> *NoLegs;
  NoOpField<FFUtils::ByteArr> *LegSymbol;
  NoOpField<uint64_t> *LegSecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *LegSecurityIDSource;
  CopyField<int32_t> *LegRatioQty;
  NoOpField<FFUtils::ByteArr> *LegType;
  NoOpField<FFUtils::ByteArr> *BuyersPerspective;
  ConstantFieldMandatory<FFUtils::ByteArr> *LegSecurityExchange;
  // fields inside sequence Legs ends
  NoOpField<FFUtils::ByteArr> *SecurityUpdateAction;
  NoOpField<uint32_t> *RoundLot;
  NoOpField<uint64_t> *MinTradeVol;
  // decimal fields MinPriceIncrement
  CopyField<int32_t> *MinPriceIncrement_exponent;
  DeltaField<int64_t> *MinPriceIncrement_mantissa;
  NoOpField<uint32_t> *TickSizeDenominator;
  NoOpField<uint32_t> *MinOrderQty;
  NoOpField<uint32_t> *MaxOrderQty;
  NoOpField<FFUtils::ByteArr> *InstrumentID;
  NoOpField<FFUtils::ByteArr> *Currency;
  NoOpField<FFUtils::ByteArr> *SettlCurrency;
  NoOpField<FFUtils::ByteArr> *SecurityType;
  NoOpField<FFUtils::ByteArr> *SecuritySubType;
  NoOpField<FFUtils::ByteArr> *Asset;
  NoOpField<FFUtils::ByteArr> *SecurityDesc;
  NoOpField<uint32_t> *MaturityDate;
  NoOpField<uint32_t> *MaturityMonthYear;
  // decimal fields StrikePrice
  CopyField<int32_t> *StrikePrice_exponent;
  DeltaField<int64_t> *StrikePrice_mantissa;
  NoOpField<FFUtils::ByteArr> *StrikeCurrency;
  // decimal fields ContractMultiplier
  CopyField<int32_t> *ContractMultiplier_exponent;
  DeltaField<int64_t> *ContractMultiplier_mantissa;
  NoOpField<uint32_t> *ContractSettlMonth;
  NoOpField<FFUtils::ByteArr> *CFICode;
  NoOpField<FFUtils::ByteArr> *CountryOfIssue;
  NoOpField<uint32_t> *IssueDate;
  NoOpField<uint32_t> *DatedDate;
  NoOpField<uint32_t> *StartDate;
  NoOpField<uint32_t> *EndDate;
  NoOpField<FFUtils::ByteArr> *SettlType;
  NoOpField<uint32_t> *SettlDate;
  DefaultField<uint64_t> *SecurityValidityTimestamp;
  NoOpField<FFUtils::ByteArr> *SecurityGroup;
  // fields inside sequence RelatedSymbols ends

  // Constructor
  MDSecurityList_111() {
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("y"));
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    TotNoRelatedSym = new NoOpField<uint32_t>(true, false, 0);
    LastFragment = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence RelatedSymbols
    NoRelatedSym = new NoOpField<uint32_t>(true, false, 0);
    Symbol = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityID = new CopyField<uint64_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    SecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence ApplIds
    NoApplIds = new NoOpField<uint32_t>(true, false, 0);
    ApplId = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence FeedTypes
    NoMDFeedTypes = new NoOpField<uint32_t>(false, false, 0);
    MDFeedType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    MarketDepth = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence FeedTypes ends
    // constructor for sequence ApplIds ends
    // constructor for sequence SecurityAltIDs
    NoSecurityAltID = new NoOpField<uint32_t>(false, false, 0);
    SecurityAltID = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityAltIDSource = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence SecurityAltIDs ends
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t>(false, false, 0);
    UnderlyingSymbol = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    UnderlyingSecurityID = new NoOpField<uint64_t>(true, false, 0);
    UnderlyingSecurityIDSource = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    UnderlyingSecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence Underlyings ends
    // constructor for sequence Legs
    NoLegs = new NoOpField<uint32_t>(false, false, 0);
    LegSymbol = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    LegSecurityID = new NoOpField<uint64_t>(true, false, 0);
    LegSecurityIDSource = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    LegRatioQty = new CopyField<int32_t>(true, false, 0);
    LegType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    BuyersPerspective = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    LegSecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence Legs ends
    SecurityUpdateAction = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    RoundLot = new NoOpField<uint32_t>(true, false, 0);
    MinTradeVol = new NoOpField<uint64_t>(false, false, 0);
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent = new CopyField<int32_t>(true, true, -2);
    MinPriceIncrement_mantissa = new DeltaField<int64_t>(true, false, 0);
    TickSizeDenominator = new NoOpField<uint32_t>(true, false, 0);
    MinOrderQty = new NoOpField<uint32_t>(true, false, 0);
    MaxOrderQty = new NoOpField<uint32_t>(true, false, 0);
    InstrumentID = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    Currency = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SettlCurrency = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecuritySubType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    Asset = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityDesc = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    MaturityDate = new NoOpField<uint32_t>(false, false, 0);
    MaturityMonthYear = new NoOpField<uint32_t>(false, false, 0);
    // decimal fields StrikePrice
    StrikePrice_exponent = new CopyField<int32_t>(false, true, -2);
    StrikePrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    StrikeCurrency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields ContractMultiplier
    ContractMultiplier_exponent = new CopyField<int32_t>(true, true, -2);
    ContractMultiplier_mantissa = new DeltaField<int64_t>(true, false, 0);
    ContractSettlMonth = new NoOpField<uint32_t>(false, false, 0);
    CFICode = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    CountryOfIssue = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    IssueDate = new NoOpField<uint32_t>(true, false, 0);
    DatedDate = new NoOpField<uint32_t>(true, false, 0);
    StartDate = new NoOpField<uint32_t>(false, false, 0);
    EndDate = new NoOpField<uint32_t>(false, false, 0);
    SettlType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlDate = new NoOpField<uint32_t>(false, false, 0);
    SecurityValidityTimestamp = new DefaultField<uint64_t>(false, false, 0);
    SecurityGroup = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence RelatedSymbols ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MessageType->decode();
    ApplVerID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    TotNoRelatedSym->decode(input);
    LastFragment->decode(input);
    // decode for sequence RelatedSymbols
    NoRelatedSym->decode(input);

    int NoRelatedSym_len = NoRelatedSym->previousValue.getValue();
    for (int i = 0; i < NoRelatedSym_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      Symbol->decode(input);
      SecurityID->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityExchange->decode();
      // decode for sequence ApplIds
      NoApplIds->decode(input);

      int NoApplIds_len = NoApplIds->previousValue.getValue();
      for (int i = 0; i < NoApplIds_len; ++i) {
        ApplId->decode(input);
        // decode for sequence FeedTypes
        NoMDFeedTypes->decode(input);

        int NoMDFeedTypes_len = NoMDFeedTypes->previousValue.getValue();
        if (NoMDFeedTypes->previousValue.isAssigned()) {
          for (int i = 0; i < NoMDFeedTypes_len; ++i) {
            MDFeedType->decode(input);
            MarketDepth->decode(input);
          }
        }
        // decode for sequence FeedTypes ends
      }
      // decode for sequence ApplIds ends
      // decode for sequence SecurityAltIDs
      NoSecurityAltID->decode(input);

      int NoSecurityAltID_len = NoSecurityAltID->previousValue.getValue();
      if (NoSecurityAltID->previousValue.isAssigned()) {
        for (int i = 0; i < NoSecurityAltID_len; ++i) {
          // extract pmap
          FFUtils::PMap pmap2 = input.extractPmap();

          SecurityAltID->decode(input);
          SecurityAltIDSource->decode(input, pmap2);
        }
      }
      // decode for sequence SecurityAltIDs ends
      // decode for sequence Underlyings
      NoUnderlyings->decode(input);

      int NoUnderlyings_len = NoUnderlyings->previousValue.getValue();
      if (NoUnderlyings->previousValue.isAssigned()) {
        for (int i = 0; i < NoUnderlyings_len; ++i) {
          UnderlyingSymbol->decode(input);
          UnderlyingSecurityID->decode(input);
          UnderlyingSecurityIDSource->decode();
          UnderlyingSecurityExchange->decode();
        }
      }
      // decode for sequence Underlyings ends
      // decode for sequence Legs
      NoLegs->decode(input);

      int NoLegs_len = NoLegs->previousValue.getValue();
      if (NoLegs->previousValue.isAssigned()) {
        for (int i = 0; i < NoLegs_len; ++i) {
          // extract pmap
          FFUtils::PMap pmap2 = input.extractPmap();

          LegSymbol->decode(input);
          LegSecurityID->decode(input);
          LegSecurityIDSource->decode();
          LegRatioQty->decode(input, pmap2);
          LegType->decode(input);
          BuyersPerspective->decode(input);
          LegSecurityExchange->decode();
        }
      }
      // decode for sequence Legs ends
      SecurityUpdateAction->decode(input);
      RoundLot->decode(input);
      MinTradeVol->decode(input);
      // decimal fields MinPriceIncrement
      MinPriceIncrement_exponent->decode(input, pmap1);
      if (MinPriceIncrement_exponent->isMandatory || MinPriceIncrement_exponent->previousValue.isAssigned())
        MinPriceIncrement_mantissa->decode(input);
      TickSizeDenominator->decode(input);
      MinOrderQty->decode(input);
      MaxOrderQty->decode(input);
      InstrumentID->decode(input);
      Currency->decode(input);
      SettlCurrency->decode(input);
      SecurityType->decode(input);
      SecuritySubType->decode(input);
      Asset->decode(input);
      SecurityDesc->decode(input);
      MaturityDate->decode(input);
      MaturityMonthYear->decode(input);
      // decimal fields StrikePrice
      StrikePrice_exponent->decode(input, pmap1);
      if (StrikePrice_exponent->isMandatory || StrikePrice_exponent->previousValue.isAssigned())
        StrikePrice_mantissa->decode(input);
      StrikeCurrency->decode(input);
      // decimal fields ContractMultiplier
      ContractMultiplier_exponent->decode(input, pmap1);
      if (ContractMultiplier_exponent->isMandatory || ContractMultiplier_exponent->previousValue.isAssigned())
        ContractMultiplier_mantissa->decode(input);
      ContractSettlMonth->decode(input);
      CFICode->decode(input);
      CountryOfIssue->decode(input);
      IssueDate->decode(input);
      DatedDate->decode(input);
      StartDate->decode(input);
      EndDate->decode(input);
      SettlType->decode(input);
      SettlDate->decode(input);
      SecurityValidityTimestamp->decode(input, pmap1);
      SecurityGroup->decode(input);

      process();
    }
    // decode for sequence RelatedSymbols ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MessageType->reset();
    ApplVerID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    TotNoRelatedSym->reset();
    LastFragment->reset();
    // reset for sequence RelatedSymbols
    NoRelatedSym->reset();
    Symbol->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence ApplIds
    NoApplIds->reset();
    ApplId->reset();
    // reset for sequence FeedTypes
    NoMDFeedTypes->reset();
    MDFeedType->reset();
    MarketDepth->reset();
    // reset for sequence FeedTypes ends
    // reset for sequence ApplIds ends
    // reset for sequence SecurityAltIDs
    NoSecurityAltID->reset();
    SecurityAltID->reset();
    SecurityAltIDSource->reset();
    // reset for sequence SecurityAltIDs ends
    // reset for sequence Underlyings
    NoUnderlyings->reset();
    UnderlyingSymbol->reset();
    UnderlyingSecurityID->reset();
    UnderlyingSecurityIDSource->reset();
    UnderlyingSecurityExchange->reset();
    // reset for sequence Underlyings ends
    // reset for sequence Legs
    NoLegs->reset();
    LegSymbol->reset();
    LegSecurityID->reset();
    LegSecurityIDSource->reset();
    LegRatioQty->reset();
    LegType->reset();
    BuyersPerspective->reset();
    LegSecurityExchange->reset();
    // reset for sequence Legs ends
    SecurityUpdateAction->reset();
    RoundLot->reset();
    MinTradeVol->reset();
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent->reset();
    MinPriceIncrement_mantissa->reset();
    TickSizeDenominator->reset();
    MinOrderQty->reset();
    MaxOrderQty->reset();
    InstrumentID->reset();
    Currency->reset();
    SettlCurrency->reset();
    SecurityType->reset();
    SecuritySubType->reset();
    Asset->reset();
    SecurityDesc->reset();
    MaturityDate->reset();
    MaturityMonthYear->reset();
    // decimal fields StrikePrice
    StrikePrice_exponent->reset();
    StrikePrice_mantissa->reset();
    StrikeCurrency->reset();
    // decimal fields ContractMultiplier
    ContractMultiplier_exponent->reset();
    ContractMultiplier_mantissa->reset();
    ContractSettlMonth->reset();
    CFICode->reset();
    CountryOfIssue->reset();
    IssueDate->reset();
    DatedDate->reset();
    StartDate->reset();
    EndDate->reset();
    SettlType->reset();
    SettlDate->reset();
    SecurityValidityTimestamp->reset();
    SecurityGroup->reset();
    // reset for sequence RelatedSymbols ends
  }

  // Destructor
  virtual ~MDSecurityList_111() {
    delete MessageType;
    delete ApplVerID;
    delete MsgSeqNum;
    delete SendingTime;
    delete TotNoRelatedSym;
    delete LastFragment;
    // destructor for sequence RelatedSymbols
    delete NoRelatedSym;
    delete Symbol;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence ApplIds
    delete NoApplIds;
    delete ApplId;
    // destructor for sequence FeedTypes
    delete NoMDFeedTypes;
    delete MDFeedType;
    delete MarketDepth;
    // destructor for sequence FeedTypes ends
    // destructor for sequence ApplIds ends
    // destructor for sequence SecurityAltIDs
    delete NoSecurityAltID;
    delete SecurityAltID;
    delete SecurityAltIDSource;
    // destructor for sequence SecurityAltIDs ends
    // destructor for sequence Underlyings
    delete NoUnderlyings;
    delete UnderlyingSymbol;
    delete UnderlyingSecurityID;
    delete UnderlyingSecurityIDSource;
    delete UnderlyingSecurityExchange;
    // destructor for sequence Underlyings ends
    // destructor for sequence Legs
    delete NoLegs;
    delete LegSymbol;
    delete LegSecurityID;
    delete LegSecurityIDSource;
    delete LegRatioQty;
    delete LegType;
    delete BuyersPerspective;
    delete LegSecurityExchange;
    // destructor for sequence Legs ends
    delete SecurityUpdateAction;
    delete RoundLot;
    delete MinTradeVol;
    // decimal fields MinPriceIncrement
    delete MinPriceIncrement_exponent;
    delete MinPriceIncrement_mantissa;
    delete TickSizeDenominator;
    delete MinOrderQty;
    delete MaxOrderQty;
    delete InstrumentID;
    delete Currency;
    delete SettlCurrency;
    delete SecurityType;
    delete SecuritySubType;
    delete Asset;
    delete SecurityDesc;
    delete MaturityDate;
    delete MaturityMonthYear;
    // decimal fields StrikePrice
    delete StrikePrice_exponent;
    delete StrikePrice_mantissa;
    delete StrikeCurrency;
    // decimal fields ContractMultiplier
    delete ContractMultiplier_exponent;
    delete ContractMultiplier_mantissa;
    delete ContractSettlMonth;
    delete CFICode;
    delete CountryOfIssue;
    delete IssueDate;
    delete DatedDate;
    delete StartDate;
    delete EndDate;
    delete SettlType;
    delete SettlDate;
    delete SecurityValidityTimestamp;
    delete SecurityGroup;
    // destructor for sequence RelatedSymbols ends
  }
};
class MDIncRefresh_81 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  DefaultField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldOptional<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int32_t> *MDEntrySize;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  DeltaField<uint32_t> *NumberOfOrders;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_81() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(false, true, 0);
    MDPriceLevel = new DefaultField<uint32_t>(false, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("J"));
    SecurityIDSource = new ConstantFieldOptional<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(false, false, 0);
    RptSeq = new IncrementField<uint32_t>(false, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new DeltaField<uint32_t>(false, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode(pmap1);
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      QuoteCondition->decode(input, pmap1);
      NumberOfOrders->decode(input);
      TradingSessionID->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    QuoteCondition->reset();
    NumberOfOrders->reset();
    TradingSessionID->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_81() {
    delete ApplVerID;
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntryTime;
    delete MDEntrySize;
    delete QuoteCondition;
    delete NumberOfOrders;
    delete TradingSessionID;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_126 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  IncrementField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  NoOpField<FFUtils::ByteArr> *Symbol;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityExchange;
  CopyField<uint64_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DefaultField<uint32_t> *NumberOfOrders;
  NoOpField<FFUtils::ByteArr> *PriceType;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int64_t> *MDEntrySize;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<uint32_t> *MDInsertDate;
  CopyField<uint32_t> *MDInsertTime;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  CopyField<FFUtils::ByteArr> *Currency;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *SellerDays;
  DeltaField<uint64_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  NoOpField<FFUtils::ByteArr> *TradeCondition;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  DefaultField<FFUtils::ByteArr> *MDEntryBuyer;
  DefaultField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<uint32_t> *MDEntryPositionNo;
  DefaultField<FFUtils::ByteArr> *SettlType;
  DefaultField<uint32_t> *SettlDate;
  NoOpField<uint32_t> *SettlePriceType;
  DefaultField<FFUtils::ByteArr> *PriceBandType;
  DefaultField<uint32_t> *PriceLimitType;
  // decimal fields LowLimitPrice
  DefaultField<int32_t> *LowLimitPrice_exponent;
  DeltaField<int64_t> *LowLimitPrice_mantissa;
  // decimal fields HighLimitPrice
  DefaultField<int32_t> *HighLimitPrice_exponent;
  DeltaField<int64_t> *HighLimitPrice_mantissa;
  // decimal fields TradingRefPrice
  DefaultField<int32_t> *TradingRefPrice_exponent;
  DeltaField<int64_t> *TradingRefPrice_mantissa;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_126() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new IncrementField<uint32_t>(false, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    Symbol = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    SecurityID = new CopyField<uint64_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    PriceType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int64_t>(false, false, 0);
    MDEntryDate = new CopyField<uint32_t>(false, false, 0);
    MDInsertDate = new CopyField<uint32_t>(false, false, 0);
    MDInsertTime = new CopyField<uint32_t>(false, false, 0);
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Currency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    TradeVolume = new DeltaField<uint64_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryPositionNo = new DefaultField<uint32_t>(false, false, 0);
    SettlType = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlDate = new DefaultField<uint32_t>(false, false, 0);
    SettlePriceType = new NoOpField<uint32_t>(false, false, 0);
    PriceBandType = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    PriceLimitType = new DefaultField<uint32_t>(false, false, 0);
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    LowLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    HighLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields TradingRefPrice
    TradingRefPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    TradingRefPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      Symbol->decode(input);
      SecurityIDSource->decode();
      SecurityExchange->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      NumberOfOrders->decode(input, pmap1);
      PriceType->decode(input);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDInsertDate->decode(input, pmap1);
      MDInsertTime->decode(input, pmap1);
      MDStreamID->decode(input, pmap1);
      Currency->decode(input, pmap1);
      TradingSessionID->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      SellerDays->decode(input, pmap1);
      TradeVolume->decode(input);
      TickDirection->decode(input, pmap1);
      TradeCondition->decode(input);
      OpenCloseSettleFlag->decode(input);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      SettlType->decode(input, pmap1);
      SettlDate->decode(input, pmap1);
      SettlePriceType->decode(input);
      PriceBandType->decode(input, pmap1);
      PriceLimitType->decode(input, pmap1);
      // decimal fields LowLimitPrice
      LowLimitPrice_exponent->decode(input, pmap1);
      if (LowLimitPrice_exponent->isMandatory || LowLimitPrice_exponent->isValueSet)
        LowLimitPrice_mantissa->decode(input);
      // decimal fields HighLimitPrice
      HighLimitPrice_exponent->decode(input, pmap1);
      if (HighLimitPrice_exponent->isMandatory || HighLimitPrice_exponent->isValueSet)
        HighLimitPrice_mantissa->decode(input);
      // decimal fields TradingRefPrice
      TradingRefPrice_exponent->decode(input, pmap1);
      if (TradingRefPrice_exponent->isMandatory || TradingRefPrice_exponent->isValueSet)
        TradingRefPrice_mantissa->decode(input);

      // will eventually process MdMsg
      //        process( );
    }

    // decode for sequence MDEntries ends

    // signifies end of sequences should process flush based stuff
    //      process_end( );
  }

  // process : TO be done in corresponding cpp file manually
  void process();
  void process_end();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    Symbol->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    SecurityID->reset();
    RptSeq->reset();
    QuoteCondition->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    NumberOfOrders->reset();
    PriceType->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    MDEntryDate->reset();
    MDInsertDate->reset();
    MDInsertTime->reset();
    MDStreamID->reset();
    Currency->reset();
    TradingSessionID->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    SellerDays->reset();
    TradeVolume->reset();
    TickDirection->reset();
    TradeCondition->reset();
    OpenCloseSettleFlag->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    MDEntryPositionNo->reset();
    SettlType->reset();
    SettlDate->reset();
    SettlePriceType->reset();
    PriceBandType->reset();
    PriceLimitType->reset();
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent->reset();
    LowLimitPrice_mantissa->reset();
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent->reset();
    HighLimitPrice_mantissa->reset();
    // decimal fields TradingRefPrice
    TradingRefPrice_exponent->reset();
    TradingRefPrice_mantissa->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_126() {
    delete ApplVerID;
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete Symbol;
    delete SecurityIDSource;
    delete SecurityExchange;
    delete SecurityID;
    delete RptSeq;
    delete QuoteCondition;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete NumberOfOrders;
    delete PriceType;
    delete MDEntryTime;
    delete MDEntrySize;
    delete MDEntryDate;
    delete MDInsertDate;
    delete MDInsertTime;
    delete MDStreamID;
    delete Currency;
    delete TradingSessionID;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete SellerDays;
    delete TradeVolume;
    delete TickDirection;
    delete TradeCondition;
    delete OpenCloseSettleFlag;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete MDEntryPositionNo;
    delete SettlType;
    delete SettlDate;
    delete SettlePriceType;
    delete PriceBandType;
    delete PriceLimitType;
    // decimal fields LowLimitPrice
    delete LowLimitPrice_exponent;
    delete LowLimitPrice_mantissa;
    // decimal fields HighLimitPrice
    delete HighLimitPrice_exponent;
    delete HighLimitPrice_mantissa;
    // decimal fields TradingRefPrice
    delete TradingRefPrice_exponent;
    delete TradingRefPrice_mantissa;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_123 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  IncrementField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  NoOpField<FFUtils::ByteArr> *Symbol;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityExchange;
  CopyField<uint64_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DefaultField<uint32_t> *NumberOfOrders;
  NoOpField<FFUtils::ByteArr> *PriceType;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int32_t> *MDEntrySize;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<uint32_t> *MDInsertDate;
  CopyField<uint32_t> *MDInsertTime;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  CopyField<FFUtils::ByteArr> *Currency;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *SellerDays;
  DeltaField<uint64_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  NoOpField<FFUtils::ByteArr> *TradeCondition;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  DefaultField<FFUtils::ByteArr> *MDEntryBuyer;
  DefaultField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<uint32_t> *MDEntryPositionNo;
  DefaultField<FFUtils::ByteArr> *SettlType;
  DefaultField<uint32_t> *SettlDate;
  NoOpField<uint32_t> *SettlePriceType;
  DefaultField<FFUtils::ByteArr> *PriceBandType;
  DefaultField<uint32_t> *PriceLimitType;
  // decimal fields LowLimitPrice
  DefaultField<int32_t> *LowLimitPrice_exponent;
  DeltaField<int64_t> *LowLimitPrice_mantissa;
  // decimal fields HighLimitPrice
  DefaultField<int32_t> *HighLimitPrice_exponent;
  DeltaField<int64_t> *HighLimitPrice_mantissa;
  // decimal fields TradingRefPrice
  DefaultField<int32_t> *TradingRefPrice_exponent;
  DeltaField<int64_t> *TradingRefPrice_mantissa;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_123() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new IncrementField<uint32_t>(false, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    Symbol = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    SecurityID = new CopyField<uint64_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    PriceType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    MDEntryDate = new CopyField<uint32_t>(false, false, 0);
    MDInsertDate = new CopyField<uint32_t>(false, false, 0);
    MDInsertTime = new CopyField<uint32_t>(false, false, 0);
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Currency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    TradeVolume = new DeltaField<uint64_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryPositionNo = new DefaultField<uint32_t>(false, false, 0);
    SettlType = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlDate = new DefaultField<uint32_t>(false, false, 0);
    SettlePriceType = new NoOpField<uint32_t>(false, false, 0);
    PriceBandType = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    PriceLimitType = new DefaultField<uint32_t>(false, false, 0);
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    LowLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    HighLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields TradingRefPrice
    TradingRefPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    TradingRefPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      Symbol->decode(input);
      SecurityIDSource->decode();
      SecurityExchange->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      NumberOfOrders->decode(input, pmap1);
      PriceType->decode(input);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDInsertDate->decode(input, pmap1);
      MDInsertTime->decode(input, pmap1);
      MDStreamID->decode(input, pmap1);
      Currency->decode(input, pmap1);
      TradingSessionID->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      SellerDays->decode(input, pmap1);
      TradeVolume->decode(input);
      TickDirection->decode(input, pmap1);
      TradeCondition->decode(input);
      OpenCloseSettleFlag->decode(input);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      SettlType->decode(input, pmap1);
      SettlDate->decode(input, pmap1);
      SettlePriceType->decode(input);
      PriceBandType->decode(input, pmap1);
      PriceLimitType->decode(input, pmap1);
      // decimal fields LowLimitPrice
      LowLimitPrice_exponent->decode(input, pmap1);
      if (LowLimitPrice_exponent->isMandatory || LowLimitPrice_exponent->isValueSet)
        LowLimitPrice_mantissa->decode(input);
      // decimal fields HighLimitPrice
      HighLimitPrice_exponent->decode(input, pmap1);
      if (HighLimitPrice_exponent->isMandatory || HighLimitPrice_exponent->isValueSet)
        HighLimitPrice_mantissa->decode(input);
      // decimal fields TradingRefPrice
      TradingRefPrice_exponent->decode(input, pmap1);
      if (TradingRefPrice_exponent->isMandatory || TradingRefPrice_exponent->isValueSet)
        TradingRefPrice_mantissa->decode(input);

      // will eventually process MdMsg
      process();
    }
    // decode for sequence MDEntries ends

    // signifies end of sequences should process flush based stuff
    process_end();
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  void process_end();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    Symbol->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    SecurityID->reset();
    RptSeq->reset();
    QuoteCondition->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    NumberOfOrders->reset();
    PriceType->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    MDEntryDate->reset();
    MDInsertDate->reset();
    MDInsertTime->reset();
    MDStreamID->reset();
    Currency->reset();
    TradingSessionID->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    SellerDays->reset();
    TradeVolume->reset();
    TickDirection->reset();
    TradeCondition->reset();
    OpenCloseSettleFlag->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    MDEntryPositionNo->reset();
    SettlType->reset();
    SettlDate->reset();
    SettlePriceType->reset();
    PriceBandType->reset();
    PriceLimitType->reset();
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent->reset();
    LowLimitPrice_mantissa->reset();
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent->reset();
    HighLimitPrice_mantissa->reset();
    // decimal fields TradingRefPrice
    TradingRefPrice_exponent->reset();
    TradingRefPrice_mantissa->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_123() {
    delete ApplVerID;
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete Symbol;
    delete SecurityIDSource;
    delete SecurityExchange;
    delete SecurityID;
    delete RptSeq;
    delete QuoteCondition;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete NumberOfOrders;
    delete PriceType;
    delete MDEntryTime;
    delete MDEntrySize;
    delete MDEntryDate;
    delete MDInsertDate;
    delete MDInsertTime;
    delete MDStreamID;
    delete Currency;
    delete TradingSessionID;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete SellerDays;
    delete TradeVolume;
    delete TickDirection;
    delete TradeCondition;
    delete OpenCloseSettleFlag;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete MDEntryPositionNo;
    delete SettlType;
    delete SettlDate;
    delete SettlePriceType;
    delete PriceBandType;
    delete PriceLimitType;
    // decimal fields LowLimitPrice
    delete LowLimitPrice_exponent;
    delete LowLimitPrice_mantissa;
    // decimal fields HighLimitPrice
    delete HighLimitPrice_exponent;
    delete HighLimitPrice_mantissa;
    // decimal fields TradingRefPrice
    delete TradingRefPrice_exponent;
    delete TradingRefPrice_mantissa;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_110 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  IncrementField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  NoOpField<FFUtils::ByteArr> *Symbol;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityExchange;
  CopyField<uint64_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DefaultField<uint32_t> *NumberOfOrders;
  NoOpField<FFUtils::ByteArr> *PriceType;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int32_t> *MDEntrySize;
  CopyField<uint32_t> *MDEntryDate;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  CopyField<FFUtils::ByteArr> *Currency;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *SellerDays;
  DeltaField<uint64_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  NoOpField<FFUtils::ByteArr> *TradeCondition;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  DefaultField<FFUtils::ByteArr> *MDEntryBuyer;
  DefaultField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<uint32_t> *MDEntryPositionNo;
  DefaultField<FFUtils::ByteArr> *SettlType;
  DefaultField<uint32_t> *SettlDate;
  NoOpField<uint32_t> *SettlePriceType;
  DefaultField<FFUtils::ByteArr> *PriceBandType;
  DefaultField<uint32_t> *PriceLimitType;
  // decimal fields LowLimitPrice
  DefaultField<int32_t> *LowLimitPrice_exponent;
  DeltaField<int64_t> *LowLimitPrice_mantissa;
  // decimal fields HighLimitPrice
  DefaultField<int32_t> *HighLimitPrice_exponent;
  DeltaField<int64_t> *HighLimitPrice_mantissa;
  // decimal fields TradingRefPrice
  DefaultField<int32_t> *TradingRefPrice_exponent;
  DeltaField<int64_t> *TradingRefPrice_mantissa;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_110() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new IncrementField<uint32_t>(false, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    Symbol = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    SecurityID = new CopyField<uint64_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    PriceType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    MDEntryDate = new CopyField<uint32_t>(false, false, 0);
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Currency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    TradeVolume = new DeltaField<uint64_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryPositionNo = new DefaultField<uint32_t>(false, false, 0);
    SettlType = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlDate = new DefaultField<uint32_t>(false, false, 0);
    SettlePriceType = new NoOpField<uint32_t>(false, false, 0);
    PriceBandType = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    PriceLimitType = new DefaultField<uint32_t>(false, false, 0);
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    LowLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    HighLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields TradingRefPrice
    TradingRefPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    TradingRefPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      Symbol->decode(input);
      SecurityIDSource->decode();
      SecurityExchange->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      NumberOfOrders->decode(input, pmap1);
      PriceType->decode(input);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDStreamID->decode(input, pmap1);
      Currency->decode(input, pmap1);
      TradingSessionID->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      SellerDays->decode(input, pmap1);
      TradeVolume->decode(input);
      TickDirection->decode(input, pmap1);
      TradeCondition->decode(input);
      OpenCloseSettleFlag->decode(input);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      SettlType->decode(input, pmap1);
      SettlDate->decode(input, pmap1);
      SettlePriceType->decode(input);
      PriceBandType->decode(input, pmap1);
      PriceLimitType->decode(input, pmap1);
      // decimal fields LowLimitPrice
      LowLimitPrice_exponent->decode(input, pmap1);
      if (LowLimitPrice_exponent->isMandatory || LowLimitPrice_exponent->isValueSet)
        LowLimitPrice_mantissa->decode(input);
      // decimal fields HighLimitPrice
      HighLimitPrice_exponent->decode(input, pmap1);
      if (HighLimitPrice_exponent->isMandatory || HighLimitPrice_exponent->isValueSet)
        HighLimitPrice_mantissa->decode(input);
      // decimal fields TradingRefPrice
      TradingRefPrice_exponent->decode(input, pmap1);
      if (TradingRefPrice_exponent->isMandatory || TradingRefPrice_exponent->isValueSet)
        TradingRefPrice_mantissa->decode(input);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    Symbol->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    SecurityID->reset();
    RptSeq->reset();
    QuoteCondition->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    NumberOfOrders->reset();
    PriceType->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    MDEntryDate->reset();
    MDStreamID->reset();
    Currency->reset();
    TradingSessionID->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    SellerDays->reset();
    TradeVolume->reset();
    TickDirection->reset();
    TradeCondition->reset();
    OpenCloseSettleFlag->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    MDEntryPositionNo->reset();
    SettlType->reset();
    SettlDate->reset();
    SettlePriceType->reset();
    PriceBandType->reset();
    PriceLimitType->reset();
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent->reset();
    LowLimitPrice_mantissa->reset();
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent->reset();
    HighLimitPrice_mantissa->reset();
    // decimal fields TradingRefPrice
    TradingRefPrice_exponent->reset();
    TradingRefPrice_mantissa->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_110() {
    delete ApplVerID;
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete Symbol;
    delete SecurityIDSource;
    delete SecurityExchange;
    delete SecurityID;
    delete RptSeq;
    delete QuoteCondition;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete NumberOfOrders;
    delete PriceType;
    delete MDEntryTime;
    delete MDEntrySize;
    delete MDEntryDate;
    delete MDStreamID;
    delete Currency;
    delete TradingSessionID;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete SellerDays;
    delete TradeVolume;
    delete TickDirection;
    delete TradeCondition;
    delete OpenCloseSettleFlag;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete MDEntryPositionNo;
    delete SettlType;
    delete SettlDate;
    delete SettlePriceType;
    delete PriceBandType;
    delete PriceLimitType;
    // decimal fields LowLimitPrice
    delete LowLimitPrice_exponent;
    delete LowLimitPrice_mantissa;
    // decimal fields HighLimitPrice
    delete HighLimitPrice_exponent;
    delete HighLimitPrice_mantissa;
    // decimal fields TradingRefPrice
    delete TradingRefPrice_exponent;
    delete TradingRefPrice_mantissa;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_109 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  IncrementField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DefaultField<uint32_t> *NumberOfOrders;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int32_t> *MDEntrySize;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  DefaultField<uint32_t> *SettlDate;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_109() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new IncrementField<uint32_t>(false, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    SettlDate = new DefaultField<uint32_t>(false, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      NumberOfOrders->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      TradingSessionID->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TickDirection->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input);
      SettlDate->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    QuoteCondition->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    NumberOfOrders->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    TradingSessionID->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TickDirection->reset();
    OpenCloseSettleFlag->reset();
    SettlDate->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_109() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    delete QuoteCondition;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete NumberOfOrders;
    delete MDEntryTime;
    delete MDEntrySize;
    delete TradingSessionID;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TickDirection;
    delete OpenCloseSettleFlag;
    delete SettlDate;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_108 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  IncrementField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DefaultField<uint32_t> *NumberOfOrders;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int32_t> *MDEntrySize;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  DefaultField<uint32_t> *SettlDate;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_108() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new IncrementField<uint32_t>(false, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    SettlDate = new DefaultField<uint32_t>(false, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      NumberOfOrders->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      TradingSessionID->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TickDirection->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input);
      SettlDate->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    QuoteCondition->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    NumberOfOrders->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    TradingSessionID->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TickDirection->reset();
    OpenCloseSettleFlag->reset();
    SettlDate->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_108() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    delete QuoteCondition;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete NumberOfOrders;
    delete MDEntryTime;
    delete MDEntrySize;
    delete TradingSessionID;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TickDirection;
    delete OpenCloseSettleFlag;
    delete SettlDate;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_97 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  CopyField<uint32_t> *MDEntryTime;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  DefaultField<uint32_t> *SettlDate;
  CopyField<FFUtils::ByteArr> *FixingBracket;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_97() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("B"));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    SettlDate = new DefaultField<uint32_t>(false, false, 0);
    FixingBracket = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      MDEntryTime->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input);
      SettlDate->decode(input, pmap1);
      FixingBracket->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    MDEntryType->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    MDEntryTime->reset();
    OpenCloseSettleFlag->reset();
    SettlDate->reset();
    FixingBracket->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_97() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    delete MDEntryType;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete MDEntryTime;
    delete OpenCloseSettleFlag;
    delete SettlDate;
    delete FixingBracket;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_77 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  CopyField<uint32_t> *MDEntryTime;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  DefaultField<uint32_t> *SettlDate;
  CopyField<FFUtils::ByteArr> *FixingBracket;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_77() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("B"));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    SettlDate = new DefaultField<uint32_t>(false, false, 0);
    FixingBracket = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      MDEntryTime->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input);
      SettlDate->decode(input, pmap1);
      FixingBracket->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    MDEntryType->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    MDEntryTime->reset();
    OpenCloseSettleFlag->reset();
    SettlDate->reset();
    FixingBracket->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_77() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    delete MDEntryType;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete MDEntryTime;
    delete OpenCloseSettleFlag;
    delete SettlDate;
    delete FixingBracket;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_103 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  DefaultField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  DefaultField<uint32_t> *SettlDate;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int32_t> *MDEntrySize;
  DeltaField<uint32_t> *NumberOfOrders;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  DefaultField<uint32_t> *AggressorSide;
  DefaultField<FFUtils::ByteArr> *MatchEventIndicator;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_103() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new DefaultField<uint32_t>(false, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    SettlDate = new DefaultField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    NumberOfOrders = new DeltaField<uint32_t>(false, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DefaultField<uint32_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    MatchEventIndicator = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input);
      SettlDate->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      NumberOfOrders->decode(input);
      TradingSessionID->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
      MatchEventIndicator->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    OpenCloseSettleFlag->reset();
    SettlDate->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    NumberOfOrders->reset();
    TradingSessionID->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    TradeCondition->reset();
    TickDirection->reset();
    QuoteCondition->reset();
    AggressorSide->reset();
    MatchEventIndicator->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_103() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete OpenCloseSettleFlag;
    delete SettlDate;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntryTime;
    delete MDEntrySize;
    delete NumberOfOrders;
    delete TradingSessionID;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete TradeCondition;
    delete TickDirection;
    delete QuoteCondition;
    delete AggressorSide;
    delete MatchEventIndicator;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_69 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  DefaultField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int32_t> *MDEntrySize;
  DeltaField<uint32_t> *NumberOfOrders;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  DefaultField<uint32_t> *AggressorSide;
  DefaultField<FFUtils::ByteArr> *MatchEventIndicator;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_69() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new DefaultField<uint32_t>(false, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    NumberOfOrders = new DeltaField<uint32_t>(false, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DefaultField<uint32_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    MatchEventIndicator = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      NumberOfOrders->decode(input);
      TradingSessionID->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
      MatchEventIndicator->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    OpenCloseSettleFlag->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    NumberOfOrders->reset();
    TradingSessionID->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    TradeCondition->reset();
    TickDirection->reset();
    QuoteCondition->reset();
    AggressorSide->reset();
    MatchEventIndicator->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_69() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete OpenCloseSettleFlag;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntryTime;
    delete MDEntrySize;
    delete NumberOfOrders;
    delete TradingSessionID;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete TradeCondition;
    delete TickDirection;
    delete QuoteCondition;
    delete AggressorSide;
    delete MatchEventIndicator;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_83 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  IncrementField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  CopyField<uint32_t> *MDEntryTime;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  DeltaField<uint32_t> *NumberOfOrders;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_83() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new IncrementField<uint32_t>(true, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(true, false, 0);
    NumberOfOrders = new DeltaField<uint32_t>(true, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      NumberOfOrders->decode(input);
      TradingSessionID->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    MDEntryTime->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    NumberOfOrders->reset();
    TradingSessionID->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_83() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete MDEntryTime;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete NumberOfOrders;
    delete TradingSessionID;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_32 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  IncrementField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  CopyField<uint32_t> *MDEntryTime;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  DeltaField<uint32_t> *NumberOfOrders;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_32() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new IncrementField<uint32_t>(true, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(true, false, 0);
    NumberOfOrders = new DeltaField<uint32_t>(true, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      NumberOfOrders->decode(input);
      TradingSessionID->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    MDEntryTime->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    NumberOfOrders->reset();
    TradingSessionID->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_32() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete MDEntryTime;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete NumberOfOrders;
    delete TradingSessionID;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_84 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  DefaultField<uint32_t> *MDUpdateAction;
  DefaultField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DeltaField<uint32_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  CopyField<uint32_t> *MDEntryTime;
  DefaultField<uint32_t> *AggressorSide;
  DefaultField<FFUtils::ByteArr> *MatchEventIndicator;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_84() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new DefaultField<uint32_t>(true, true, 0);
    MDEntryType = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, true, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DeltaField<uint32_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    MatchEventIndicator = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input);
      TickDirection->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
      MatchEventIndicator->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    TickDirection->reset();
    TradeCondition->reset();
    MDEntryTime->reset();
    AggressorSide->reset();
    MatchEventIndicator->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_84() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete TickDirection;
    delete TradeCondition;
    delete MDEntryTime;
    delete AggressorSide;
    delete MatchEventIndicator;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_68 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  DefaultField<uint32_t> *MDUpdateAction;
  DefaultField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DeltaField<uint32_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  CopyField<uint32_t> *MDEntryTime;
  DefaultField<uint32_t> *AggressorSide;
  DefaultField<FFUtils::ByteArr> *MatchEventIndicator;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_68() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new DefaultField<uint32_t>(true, true, 0);
    MDEntryType = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, true, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DeltaField<uint32_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    MatchEventIndicator = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input);
      TickDirection->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
      MatchEventIndicator->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    TickDirection->reset();
    TradeCondition->reset();
    MDEntryTime->reset();
    AggressorSide->reset();
    MatchEventIndicator->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_68() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete TickDirection;
    delete TradeCondition;
    delete MDEntryTime;
    delete AggressorSide;
    delete MatchEventIndicator;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_104 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  CopyField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  DefaultField<uint32_t> *SettlDate;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  CopyField<FFUtils::ByteArr> *QuoteCondition;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DefaultField<uint32_t> *NumberOfOrders;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int32_t> *MDEntrySize;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<uint32_t> *AggressorSide;
  DefaultField<FFUtils::ByteArr> *MatchEventIndicator;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_104() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new CopyField<uint32_t>(false, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    SettlDate = new DefaultField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    QuoteCondition = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("K"));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, true, -2);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DefaultField<uint32_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    MatchEventIndicator = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input);
      SettlDate->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      NumberOfOrders->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      TradingSessionID->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
      MatchEventIndicator->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    OpenCloseSettleFlag->reset();
    SettlDate->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    QuoteCondition->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    NumberOfOrders->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    TradingSessionID->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    TradeCondition->reset();
    TickDirection->reset();
    AggressorSide->reset();
    MatchEventIndicator->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_104() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete OpenCloseSettleFlag;
    delete SettlDate;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    delete QuoteCondition;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete NumberOfOrders;
    delete MDEntryTime;
    delete MDEntrySize;
    delete TradingSessionID;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete TradeCondition;
    delete TickDirection;
    delete AggressorSide;
    delete MatchEventIndicator;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_71 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  CopyField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  CopyField<FFUtils::ByteArr> *QuoteCondition;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DefaultField<uint32_t> *NumberOfOrders;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int32_t> *MDEntrySize;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<uint32_t> *AggressorSide;
  DefaultField<FFUtils::ByteArr> *MatchEventIndicator;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_71() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new CopyField<uint32_t>(false, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    QuoteCondition = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("K"));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, true, -2);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DefaultField<uint32_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    MatchEventIndicator = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      NumberOfOrders->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      TradingSessionID->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
      MatchEventIndicator->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    OpenCloseSettleFlag->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    QuoteCondition->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    NumberOfOrders->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    TradingSessionID->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    TradeCondition->reset();
    TickDirection->reset();
    AggressorSide->reset();
    MatchEventIndicator->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_71() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete OpenCloseSettleFlag;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    delete QuoteCondition;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete NumberOfOrders;
    delete MDEntryTime;
    delete MDEntrySize;
    delete TradingSessionID;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete TradeCondition;
    delete TickDirection;
    delete AggressorSide;
    delete MatchEventIndicator;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_59 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  CopyField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  CopyField<FFUtils::ByteArr> *QuoteCondition;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DefaultField<uint32_t> *NumberOfOrders;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int32_t> *MDEntrySize;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<uint32_t> *AggressorSide;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_59() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new CopyField<uint32_t>(false, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    QuoteCondition = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("K"));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, true, -2);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DefaultField<uint32_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      NumberOfOrders->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      TradingSessionID->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    OpenCloseSettleFlag->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    QuoteCondition->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    NumberOfOrders->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    TradingSessionID->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    TradeCondition->reset();
    TickDirection->reset();
    AggressorSide->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_59() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete OpenCloseSettleFlag;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    delete QuoteCondition;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete NumberOfOrders;
    delete MDEntryTime;
    delete MDEntrySize;
    delete TradingSessionID;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete TradeCondition;
    delete TickDirection;
    delete AggressorSide;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_86 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  CopyField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  CopyField<FFUtils::ByteArr> *QuoteCondition;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  CopyField<uint32_t> *NumberOfOrders;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int32_t> *MDEntrySize;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_86() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new CopyField<uint32_t>(true, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    QuoteCondition = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("K"));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    NumberOfOrders = new CopyField<uint32_t>(false, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(true, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      NumberOfOrders->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      TradingSessionID->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    QuoteCondition->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    NumberOfOrders->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    TradingSessionID->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_86() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    delete QuoteCondition;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete NumberOfOrders;
    delete MDEntryTime;
    delete MDEntrySize;
    delete TradingSessionID;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_35 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  CopyField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  DeltaField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  CopyField<FFUtils::ByteArr> *QuoteCondition;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  CopyField<uint32_t> *NumberOfOrders;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int32_t> *MDEntrySize;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_35() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new CopyField<uint32_t>(true, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new DeltaField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    QuoteCondition = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("K"));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    NumberOfOrders = new CopyField<uint32_t>(false, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(true, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input);
      RptSeq->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      NumberOfOrders->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      TradingSessionID->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    QuoteCondition->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    NumberOfOrders->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    TradingSessionID->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_35() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    delete QuoteCondition;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete NumberOfOrders;
    delete MDEntryTime;
    delete MDEntrySize;
    delete TradingSessionID;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_87 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  CopyField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int32_t> *MDEntrySize;
  CopyField<FFUtils::ByteArr> *QuoteCondition;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_87() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new CopyField<uint32_t>(true, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(true, false, 0);
    QuoteCondition = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("K"));
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      QuoteCondition->decode(input, pmap1);
      TradingSessionID->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    QuoteCondition->reset();
    TradingSessionID->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_87() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntryTime;
    delete MDEntrySize;
    delete QuoteCondition;
    delete TradingSessionID;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_36 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  CopyField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int32_t> *MDEntrySize;
  CopyField<FFUtils::ByteArr> *QuoteCondition;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_36() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new CopyField<uint32_t>(true, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(true, false, 0);
    QuoteCondition = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("K"));
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      QuoteCondition->decode(input, pmap1);
      TradingSessionID->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    QuoteCondition->reset();
    TradingSessionID->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_36() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntryTime;
    delete MDEntrySize;
    delete QuoteCondition;
    delete TradingSessionID;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_88 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  DefaultField<uint32_t> *MDUpdateAction;
  DefaultField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  DeltaField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int32_t> *MDEntrySize;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DeltaField<uint32_t> *TradeVolume;
  CopyField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<uint32_t> *AggressorSide;
  DefaultField<FFUtils::ByteArr> *MatchEventIndicator;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_88() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new DefaultField<uint32_t>(true, true, 0);
    MDEntryType = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new DeltaField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, -1);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, true, -1);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DeltaField<uint32_t>(false, false, 0);
    TradeCondition = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    MatchEventIndicator = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input);
      TradeCondition->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
      MatchEventIndicator->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    TradeCondition->reset();
    TickDirection->reset();
    AggressorSide->reset();
    MatchEventIndicator->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_88() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntryTime;
    delete MDEntrySize;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete TradeCondition;
    delete TickDirection;
    delete AggressorSide;
    delete MatchEventIndicator;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_72 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  DefaultField<uint32_t> *MDUpdateAction;
  DefaultField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  DeltaField<uint32_t> *SecurityID;
  DeltaField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int32_t> *MDEntrySize;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DeltaField<uint32_t> *TradeVolume;
  CopyField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<uint32_t> *AggressorSide;
  DefaultField<FFUtils::ByteArr> *MatchEventIndicator;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_72() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new DefaultField<uint32_t>(true, true, 0);
    MDEntryType = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new DeltaField<uint32_t>(true, false, 0);
    RptSeq = new DeltaField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, -1);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, true, -1);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DeltaField<uint32_t>(false, false, 0);
    TradeCondition = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    MatchEventIndicator = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input);
      RptSeq->decode(input);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input);
      TradeCondition->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
      MatchEventIndicator->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    TradeCondition->reset();
    TickDirection->reset();
    AggressorSide->reset();
    MatchEventIndicator->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_72() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntryTime;
    delete MDEntrySize;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete TradeCondition;
    delete TickDirection;
    delete AggressorSide;
    delete MatchEventIndicator;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_105 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  IncrementField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  DefaultField<uint32_t> *SettlDate;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *TradeVolume;
  DefaultField<uint32_t> *NumberOfOrders;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  CopyField<uint32_t> *MDEntryTime;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  DefaultField<uint32_t> *AggressorSide;
  DefaultField<FFUtils::ByteArr> *MatchEventIndicator;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_105() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new IncrementField<uint32_t>(false, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    SettlDate = new DefaultField<uint32_t>(false, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DefaultField<uint32_t>(false, false, 0);
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    MatchEventIndicator = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input);
      SettlDate->decode(input, pmap1);
      TradingSessionID->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      TradeCondition->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
      MatchEventIndicator->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    OpenCloseSettleFlag->reset();
    SettlDate->reset();
    TradingSessionID->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    NumberOfOrders->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    MDEntryTime->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    TradeCondition->reset();
    TickDirection->reset();
    QuoteCondition->reset();
    AggressorSide->reset();
    MatchEventIndicator->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_105() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete OpenCloseSettleFlag;
    delete SettlDate;
    delete TradingSessionID;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete NumberOfOrders;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    delete MDEntryTime;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete TradeCondition;
    delete TickDirection;
    delete QuoteCondition;
    delete AggressorSide;
    delete MatchEventIndicator;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_67 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  IncrementField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *TradeVolume;
  DefaultField<uint32_t> *NumberOfOrders;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  CopyField<uint32_t> *MDEntryTime;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  DefaultField<uint32_t> *AggressorSide;
  DefaultField<FFUtils::ByteArr> *MatchEventIndicator;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_67() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new IncrementField<uint32_t>(false, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DefaultField<uint32_t>(false, false, 0);
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    MatchEventIndicator = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input);
      TradingSessionID->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      TradeCondition->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
      MatchEventIndicator->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    OpenCloseSettleFlag->reset();
    TradingSessionID->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    NumberOfOrders->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    MDEntryTime->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    TradeCondition->reset();
    TickDirection->reset();
    QuoteCondition->reset();
    AggressorSide->reset();
    MatchEventIndicator->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_67() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete OpenCloseSettleFlag;
    delete TradingSessionID;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete NumberOfOrders;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    delete MDEntryTime;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete TradeCondition;
    delete TickDirection;
    delete QuoteCondition;
    delete AggressorSide;
    delete MatchEventIndicator;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_90 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  IncrementField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  CopyField<uint32_t> *MDEntryTime;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  DeltaField<uint32_t> *NumberOfOrders;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_90() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new IncrementField<uint32_t>(true, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(true, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    NumberOfOrders = new DeltaField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      TradingSessionID->decode(input, pmap1);
      NumberOfOrders->decode(input);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    MDEntryTime->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    TradingSessionID->reset();
    NumberOfOrders->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_90() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete MDEntryTime;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete TradingSessionID;
    delete NumberOfOrders;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_39 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  IncrementField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  CopyField<uint32_t> *MDEntryTime;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  DeltaField<uint32_t> *NumberOfOrders;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_39() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDPriceLevel = new IncrementField<uint32_t>(true, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(true, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    NumberOfOrders = new DeltaField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      TradingSessionID->decode(input, pmap1);
      NumberOfOrders->decode(input);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    MDEntryTime->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    TradingSessionID->reset();
    NumberOfOrders->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_39() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete MDEntryTime;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete TradingSessionID;
    delete NumberOfOrders;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_91 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  DefaultField<uint32_t> *MDUpdateAction;
  DefaultField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DeltaField<uint32_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  CopyField<uint32_t> *MDEntryTime;
  DefaultField<uint32_t> *AggressorSide;
  DefaultField<FFUtils::ByteArr> *MatchEventIndicator;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_91() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new DefaultField<uint32_t>(true, true, 0);
    MDEntryType = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, true, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DeltaField<uint32_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    MatchEventIndicator = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input);
      TickDirection->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
      MatchEventIndicator->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    TickDirection->reset();
    TradeCondition->reset();
    MDEntryTime->reset();
    AggressorSide->reset();
    MatchEventIndicator->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_91() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete TickDirection;
    delete TradeCondition;
    delete MDEntryTime;
    delete AggressorSide;
    delete MatchEventIndicator;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_70 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  DefaultField<uint32_t> *MDUpdateAction;
  DefaultField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DeltaField<uint32_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  CopyField<uint32_t> *MDEntryTime;
  DefaultField<uint32_t> *AggressorSide;
  DefaultField<FFUtils::ByteArr> *MatchEventIndicator;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_70() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new DefaultField<uint32_t>(true, true, 0);
    MDEntryType = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(true, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, true, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DeltaField<uint32_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    MatchEventIndicator = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input);
      TickDirection->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
      MatchEventIndicator->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    TickDirection->reset();
    TradeCondition->reset();
    MDEntryTime->reset();
    AggressorSide->reset();
    MatchEventIndicator->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_70() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete TickDirection;
    delete TradeCondition;
    delete MDEntryTime;
    delete AggressorSide;
    delete MatchEventIndicator;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_93 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  ConstantFieldMandatory<uint32_t> *MDUpdateAction;
  ConstantFieldMandatory<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  DeltaField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  DeltaField<uint32_t> *MDEntryTime;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  CopyField<uint32_t> *NumberOfOrders;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_93() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new ConstantFieldMandatory<uint32_t>(true, 5);
    MDPriceLevel = new ConstantFieldMandatory<uint32_t>(true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("1"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new DeltaField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(true, false, 0);
    MDEntryTime = new DeltaField<uint32_t>(true, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    NumberOfOrders = new CopyField<uint32_t>(true, true, 1);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode();
      MDPriceLevel->decode();
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      MDEntryTime->decode(input);
      TradingSessionID->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    MDEntryTime->reset();
    TradingSessionID->reset();
    NumberOfOrders->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_93() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete MDEntryTime;
    delete TradingSessionID;
    delete NumberOfOrders;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_41 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  ConstantFieldMandatory<uint32_t> *MDUpdateAction;
  ConstantFieldMandatory<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  DeltaField<uint32_t> *SecurityID;
  DeltaField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  DeltaField<uint32_t> *MDEntryTime;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  CopyField<uint32_t> *NumberOfOrders;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_41() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new ConstantFieldMandatory<uint32_t>(true, 5);
    MDPriceLevel = new ConstantFieldMandatory<uint32_t>(true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("1"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new DeltaField<uint32_t>(true, false, 0);
    RptSeq = new DeltaField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(true, false, 0);
    MDEntryTime = new DeltaField<uint32_t>(true, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    NumberOfOrders = new CopyField<uint32_t>(true, true, 1);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode();
      MDPriceLevel->decode();
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input);
      RptSeq->decode(input);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      MDEntryTime->decode(input);
      TradingSessionID->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    MDEntryTime->reset();
    TradingSessionID->reset();
    NumberOfOrders->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_41() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete MDEntryTime;
    delete TradingSessionID;
    delete NumberOfOrders;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_106 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  DefaultField<uint32_t> *MDUpdateAction;
  CopyField<uint32_t> *NumberOfOrders;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  DefaultField<uint32_t> *SettlDate;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  DeltaField<int32_t> *MDEntrySize;
  CopyField<uint32_t> *MDEntryTime;
  DefaultField<uint32_t> *MDPriceLevel;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<uint32_t> *AggressorSide;
  DefaultField<FFUtils::ByteArr> *MatchEventIndicator;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_106() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new DefaultField<uint32_t>(true, true, 5);
    NumberOfOrders = new CopyField<uint32_t>(false, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("1"));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    SettlDate = new DefaultField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDPriceLevel = new DefaultField<uint32_t>(false, true, 1);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DefaultField<uint32_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    MatchEventIndicator = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input);
      SettlDate->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      TradingSessionID->decode(input, pmap1);
      MDEntrySize->decode(input);
      MDEntryTime->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
      MatchEventIndicator->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    NumberOfOrders->reset();
    MDEntryType->reset();
    OpenCloseSettleFlag->reset();
    SettlDate->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    TradingSessionID->reset();
    MDEntrySize->reset();
    MDEntryTime->reset();
    MDPriceLevel->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    TickDirection->reset();
    QuoteCondition->reset();
    TradeCondition->reset();
    AggressorSide->reset();
    MatchEventIndicator->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_106() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete NumberOfOrders;
    delete MDEntryType;
    delete OpenCloseSettleFlag;
    delete SettlDate;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    delete TradingSessionID;
    delete MDEntrySize;
    delete MDEntryTime;
    delete MDPriceLevel;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete TickDirection;
    delete QuoteCondition;
    delete TradeCondition;
    delete AggressorSide;
    delete MatchEventIndicator;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_73 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  DefaultField<uint32_t> *MDUpdateAction;
  CopyField<uint32_t> *NumberOfOrders;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  DeltaField<int32_t> *MDEntrySize;
  CopyField<uint32_t> *MDEntryTime;
  DefaultField<uint32_t> *MDPriceLevel;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<uint32_t> *AggressorSide;
  DefaultField<FFUtils::ByteArr> *MatchEventIndicator;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_73() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new DefaultField<uint32_t>(true, true, 5);
    NumberOfOrders = new CopyField<uint32_t>(false, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("1"));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDPriceLevel = new DefaultField<uint32_t>(false, true, 1);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DefaultField<uint32_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    MatchEventIndicator = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      TradingSessionID->decode(input, pmap1);
      MDEntrySize->decode(input);
      MDEntryTime->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
      MatchEventIndicator->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    NumberOfOrders->reset();
    MDEntryType->reset();
    OpenCloseSettleFlag->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    TradingSessionID->reset();
    MDEntrySize->reset();
    MDEntryTime->reset();
    MDPriceLevel->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    TickDirection->reset();
    QuoteCondition->reset();
    TradeCondition->reset();
    AggressorSide->reset();
    MatchEventIndicator->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_73() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete NumberOfOrders;
    delete MDEntryType;
    delete OpenCloseSettleFlag;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    delete TradingSessionID;
    delete MDEntrySize;
    delete MDEntryTime;
    delete MDPriceLevel;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete TickDirection;
    delete QuoteCondition;
    delete TradeCondition;
    delete AggressorSide;
    delete MatchEventIndicator;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_95 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  ConstantFieldMandatory<uint32_t> *MDUpdateAction;
  ConstantFieldMandatory<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  DeltaField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  CopyField<uint32_t> *MDEntryTime;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  CopyField<uint32_t> *NumberOfOrders;
  DefaultField<uint32_t> *MDQuoteType;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_95() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new ConstantFieldMandatory<uint32_t>(true, 5);
    MDPriceLevel = new ConstantFieldMandatory<uint32_t>(true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("1"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new DeltaField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    NumberOfOrders = new CopyField<uint32_t>(true, false, 0);
    MDQuoteType = new DefaultField<uint32_t>(false, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode();
      MDPriceLevel->decode();
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      MDEntryTime->decode(input, pmap1);
      TradingSessionID->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDQuoteType->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    MDEntryTime->reset();
    TradingSessionID->reset();
    NumberOfOrders->reset();
    MDQuoteType->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_95() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete MDEntryTime;
    delete TradingSessionID;
    delete NumberOfOrders;
    delete MDQuoteType;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_43 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  ConstantFieldMandatory<uint32_t> *MDUpdateAction;
  ConstantFieldMandatory<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  DeltaField<uint32_t> *SecurityID;
  DeltaField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  CopyField<uint32_t> *MDEntryTime;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  CopyField<uint32_t> *NumberOfOrders;
  DefaultField<uint32_t> *MDQuoteType;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_43() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new ConstantFieldMandatory<uint32_t>(true, 5);
    MDPriceLevel = new ConstantFieldMandatory<uint32_t>(true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("1"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new DeltaField<uint32_t>(true, false, 0);
    RptSeq = new DeltaField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    NumberOfOrders = new CopyField<uint32_t>(true, false, 0);
    MDQuoteType = new DefaultField<uint32_t>(false, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode();
      MDPriceLevel->decode();
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input);
      RptSeq->decode(input);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      MDEntryTime->decode(input, pmap1);
      TradingSessionID->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDQuoteType->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    MDEntryTime->reset();
    TradingSessionID->reset();
    NumberOfOrders->reset();
    MDQuoteType->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_43() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete MDEntryTime;
    delete TradingSessionID;
    delete NumberOfOrders;
    delete MDQuoteType;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_96 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  ConstantFieldMandatory<uint32_t> *MDUpdateAction;
  ConstantFieldMandatory<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  CopyField<uint32_t> *MDEntryTime;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  CopyField<uint32_t> *NumberOfOrders;
  ConstantFieldOptional<uint32_t> *MDQuoteType;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_96() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new ConstantFieldMandatory<uint32_t>(true, 5);
    MDPriceLevel = new ConstantFieldMandatory<uint32_t>(true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("1"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    NumberOfOrders = new CopyField<uint32_t>(true, false, 0);
    MDQuoteType = new ConstantFieldOptional<uint32_t>(true, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode();
      MDPriceLevel->decode();
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      MDEntryTime->decode(input, pmap1);
      TradingSessionID->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDQuoteType->decode(pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    MDEntryTime->reset();
    TradingSessionID->reset();
    NumberOfOrders->reset();
    MDQuoteType->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_96() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete MDEntryTime;
    delete TradingSessionID;
    delete NumberOfOrders;
    delete MDQuoteType;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_44 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  ConstantFieldMandatory<uint32_t> *MDUpdateAction;
  ConstantFieldMandatory<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  CopyField<uint32_t> *MDEntryTime;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  CopyField<uint32_t> *NumberOfOrders;
  ConstantFieldOptional<uint32_t> *MDQuoteType;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_44() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new ConstantFieldMandatory<uint32_t>(true, 5);
    MDPriceLevel = new ConstantFieldMandatory<uint32_t>(true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("1"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, 0);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    NumberOfOrders = new CopyField<uint32_t>(true, false, 0);
    MDQuoteType = new ConstantFieldOptional<uint32_t>(true, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode();
      MDPriceLevel->decode();
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      MDEntryTime->decode(input, pmap1);
      TradingSessionID->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDQuoteType->decode(pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    MDEntryTime->reset();
    TradingSessionID->reset();
    NumberOfOrders->reset();
    MDQuoteType->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_44() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete MDEntryTime;
    delete TradingSessionID;
    delete NumberOfOrders;
    delete MDQuoteType;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_107 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  CopyField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  DefaultField<uint32_t> *SettlDate;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  CopyField<uint32_t> *MDEntryTime;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  CopyField<uint32_t> *NumberOfOrders;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  DefaultField<uint32_t> *MDQuoteType;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<uint32_t> *AggressorSide;
  DefaultField<FFUtils::ByteArr> *MatchEventIndicator;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_107() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 5);
    MDPriceLevel = new CopyField<uint32_t>(false, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("1"));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    SettlDate = new DefaultField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    NumberOfOrders = new CopyField<uint32_t>(false, true, 1);
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DefaultField<uint32_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDQuoteType = new DefaultField<uint32_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    MatchEventIndicator = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input);
      SettlDate->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      MDEntryTime->decode(input, pmap1);
      TradingSessionID->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      MDQuoteType->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
      MatchEventIndicator->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    OpenCloseSettleFlag->reset();
    SettlDate->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    MDEntryTime->reset();
    TradingSessionID->reset();
    NumberOfOrders->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    TickDirection->reset();
    QuoteCondition->reset();
    MDQuoteType->reset();
    TradeCondition->reset();
    AggressorSide->reset();
    MatchEventIndicator->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_107() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete OpenCloseSettleFlag;
    delete SettlDate;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete MDEntryTime;
    delete TradingSessionID;
    delete NumberOfOrders;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete TickDirection;
    delete QuoteCondition;
    delete MDQuoteType;
    delete TradeCondition;
    delete AggressorSide;
    delete MatchEventIndicator;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_74 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  CopyField<uint32_t> *MDPriceLevel;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  CopyField<uint32_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  CopyField<uint32_t> *MDEntryTime;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  CopyField<uint32_t> *NumberOfOrders;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  DefaultField<uint32_t> *MDQuoteType;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<uint32_t> *AggressorSide;
  DefaultField<FFUtils::ByteArr> *MatchEventIndicator;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_74() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 5);
    MDPriceLevel = new CopyField<uint32_t>(false, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("1"));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityID = new CopyField<uint32_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    NumberOfOrders = new CopyField<uint32_t>(false, true, 1);
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    TradeVolume = new DefaultField<uint32_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDQuoteType = new DefaultField<uint32_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    AggressorSide = new DefaultField<uint32_t>(false, false, 0);
    MatchEventIndicator = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input);
      SecurityIDSource->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      MDEntryTime->decode(input, pmap1);
      TradingSessionID->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      TradeVolume->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      MDQuoteType->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      AggressorSide->decode(input, pmap1);
      MatchEventIndicator->decode(input, pmap1);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDPriceLevel->reset();
    MDEntryType->reset();
    OpenCloseSettleFlag->reset();
    SecurityIDSource->reset();
    SecurityID->reset();
    RptSeq->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    MDEntryTime->reset();
    TradingSessionID->reset();
    NumberOfOrders->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    TradeVolume->reset();
    TickDirection->reset();
    QuoteCondition->reset();
    MDQuoteType->reset();
    TradeCondition->reset();
    AggressorSide->reset();
    MatchEventIndicator->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDIncRefresh_74() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDPriceLevel;
    delete MDEntryType;
    delete OpenCloseSettleFlag;
    delete SecurityIDSource;
    delete SecurityID;
    delete RptSeq;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete MDEntryTime;
    delete TradingSessionID;
    delete NumberOfOrders;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete TradeVolume;
    delete TickDirection;
    delete QuoteCondition;
    delete MDQuoteType;
    delete TradeCondition;
    delete AggressorSide;
    delete MatchEventIndicator;
    // destructor for sequence MDEntries ends
  }
};
class MDSecurityDefinition_79 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TotNumReports;
  // fields inside sequence Events
  NoOpField<uint32_t> *NoEvents;
  DeltaField<uint32_t> *EventType;
  DeltaField<uint64_t> *EventDate;
  DeltaField<uint64_t> *EventTime;
  // fields inside sequence Events ends
  // decimal fields TradingReferencePrice
  DefaultField<int32_t> *TradingReferencePrice_exponent;
  NoOpField<int64_t> *TradingReferencePrice_mantissa;
  NoOpField<FFUtils::ByteArr> *SettlePriceType;
  // decimal fields HighLimitPx
  DefaultField<int32_t> *HighLimitPx_exponent;
  NoOpField<int64_t> *HighLimitPx_mantissa;
  // decimal fields LowLimitPx
  DefaultField<int32_t> *LowLimitPx_exponent;
  NoOpField<int64_t> *LowLimitPx_mantissa;
  NoOpField<FFUtils::ByteArr> *SecurityGroup;
  NoOpField<FFUtils::ByteArr> *Symbol;
  NoOpField<FFUtils::ByteArr> *SecurityDesc;
  NoOpField<uint32_t> *SecurityID;
  ConstantFieldOptional<uint32_t> *SecurityIDSource;
  NoOpField<FFUtils::ByteArr> *CFICode;
  NoOpField<FFUtils::ByteArr> *UnderlyingProduct;
  NoOpField<FFUtils::ByteArr> *SecurityExchange;
  NoOpField<FFUtils::ByteArr> *PricingModel;
  // decimal fields MinCabPrice
  DefaultField<int32_t> *MinCabPrice_exponent;
  NoOpField<int64_t> *MinCabPrice_mantissa;
  // fields inside sequence SecurityAltIDs
  NoOpField<uint32_t> *NoSecurityAltID;
  NoOpField<FFUtils::ByteArr> *SecurityAltID;
  ConstantFieldOptional<uint32_t> *SecurityAltIDSource;
  // fields inside sequence SecurityAltIDs ends
  NoOpField<uint32_t> *ExpirationCycle;
  NoOpField<FFUtils::ByteArr> *UnitOfMeasureQty;
  // decimal fields StrikePrice
  DefaultField<int32_t> *StrikePrice_exponent;
  NoOpField<int64_t> *StrikePrice_mantissa;
  NoOpField<FFUtils::ByteArr> *StrikeCurrency;
  NoOpField<uint64_t> *MinTradeVol;
  NoOpField<uint64_t> *MaxTradeVol;
  NoOpField<FFUtils::ByteArr> *Currency;
  NoOpField<FFUtils::ByteArr> *SettlCurrency;
  // fields inside sequence MDFeedTypes
  NoOpField<uint32_t> *NoMDFeedTypes;
  DefaultField<FFUtils::ByteArr> *MDFeedType;
  NoOpField<uint32_t> *MarketDepth;
  // fields inside sequence MDFeedTypes ends
  NoOpField<FFUtils::ByteArr> *MatchAlgo;
  NoOpField<FFUtils::ByteArr> *SecuritySubType;
  // fields inside sequence Underlyings
  NoOpField<uint32_t> *NoUnderlyings;
  ConstantFieldMandatory<FFUtils::ByteArr> *UnderlyingSymbol;
  DeltaField<uint32_t> *UnderlyingSecurityID;
  ConstantFieldMandatory<uint32_t> *UnderlyingSecurityIDSource;
  // fields inside sequence Underlyings ends
  NoOpField<FFUtils::ByteArr> *MaxPriceVariation;
  NoOpField<FFUtils::ByteArr> *ImpliedMarketIndicator;
  // fields inside sequence InstrAttrib
  NoOpField<uint32_t> *NoInstrAttrib;
  DeltaField<uint64_t> *InstrAttribType;
  CopyField<FFUtils::ByteArr> *InstrAttribValue;
  // fields inside sequence InstrAttrib ends
  NoOpField<uint64_t> *MaturityMonthYear;
  // decimal fields MinPriceIncrement
  CopyField<int32_t> *MinPriceIncrement_exponent;
  DeltaField<int64_t> *MinPriceIncrement_mantissa;
  // decimal fields MinPriceIncrementAmount
  DefaultField<int32_t> *MinPriceIncrementAmount_exponent;
  NoOpField<int64_t> *MinPriceIncrementAmount_mantissa;
  NoOpField<uint64_t> *LastUpdateTime;
  NoOpField<FFUtils::ByteArr> *SecurityUpdateAction;
  // decimal fields DisplayFactor
  DefaultField<int32_t> *DisplayFactor_exponent;
  NoOpField<int64_t> *DisplayFactor_mantissa;
  // fields inside sequence Legs
  NoOpField<uint32_t> *NoLegs;
  DefaultField<FFUtils::ByteArr> *LegSymbol;
  CopyField<uint32_t> *LegRatioQty;
  DeltaField<uint64_t> *LegSecurityID;
  NoOpField<FFUtils::ByteArr> *LegSecurityDesc;
  ConstantFieldMandatory<uint32_t> *LegSecurityIDSource;
  DefaultField<FFUtils::ByteArr> *LegSide;
  CopyField<FFUtils::ByteArr> *LegSecurityGroup;
  CopyField<FFUtils::ByteArr> *LegCFICode;
  CopyField<FFUtils::ByteArr> *LegSecuritySubType;
  CopyField<FFUtils::ByteArr> *LegCurrency;
  DeltaField<uint64_t> *LegMaturityMonthYear;
  // decimal fields LegStrikePrice
  CopyField<int32_t> *LegStrikePrice_exponent;
  DeltaField<int64_t> *LegStrikePrice_mantissa;
  CopyField<FFUtils::ByteArr> *LegSecurityExchange;
  CopyField<FFUtils::ByteArr> *LegStrikeCurrency;
  // decimal fields LegPrice
  CopyField<int32_t> *LegPrice_exponent;
  DeltaField<int64_t> *LegPrice_mantissa;
  // decimal fields LegOptionDelta
  CopyField<int32_t> *LegOptionDelta_exponent;
  DeltaField<int64_t> *LegOptionDelta_mantissa;
  // fields inside sequence Legs ends
  NoOpField<FFUtils::ByteArr> *ApplID;
  ConstantFieldOptional<FFUtils::ByteArr> *UserDefinedInstrument;
  // decimal fields PriceRatio
  DefaultField<int32_t> *PriceRatio_exponent;
  NoOpField<int64_t> *PriceRatio_mantissa;
  DefaultField<uint32_t> *ContractMultiplierType;
  NoOpField<uint32_t> *FlowScheduleType;
  NoOpField<uint32_t> *ContractMultiplier;
  NoOpField<FFUtils::ByteArr> *UnitofMeasure;
  NoOpField<uint64_t> *DecayQuantity;
  NoOpField<uint64_t> *DecayStartDate;
  NoOpField<uint64_t> *OriginalContractSize;
  NoOpField<uint32_t> *ClearedVolume;
  NoOpField<uint32_t> *OpenInterestQty;
  NoOpField<uint32_t> *TradingReferenceDate;
  // fields inside sequence LotTypeRules
  NoOpField<uint32_t> *NumLotTypeRules;
  NoOpField<FFUtils::ByteArr> *LotType;
  // decimal fields MinLotSize
  DefaultField<int32_t> *MinLotSize_exponent;
  NoOpField<int64_t> *MinLotSize_mantissa;
  // fields inside sequence LotTypeRules ends

  // Constructor
  MDSecurityDefinition_79() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("d"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TotNumReports = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence Events
    NoEvents = new NoOpField<uint32_t>(false, false, 0);
    EventType = new DeltaField<uint32_t>(false, false, 0);
    EventDate = new DeltaField<uint64_t>(false, false, 0);
    EventTime = new DeltaField<uint64_t>(false, false, 0);
    // constructor for sequence Events ends
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent = new DefaultField<int32_t>(false, true, -2);
    TradingReferencePrice_mantissa = new NoOpField<int64_t>(true, false, 0);
    SettlePriceType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields HighLimitPx
    HighLimitPx_exponent = new DefaultField<int32_t>(false, true, -2);
    HighLimitPx_mantissa = new NoOpField<int64_t>(true, false, 0);
    // decimal fields LowLimitPx
    LowLimitPx_exponent = new DefaultField<int32_t>(false, true, -2);
    LowLimitPx_mantissa = new NoOpField<int64_t>(true, false, 0);
    SecurityGroup = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Symbol = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityDesc = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new NoOpField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldOptional<uint32_t>(true, 8);
    CFICode = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    UnderlyingProduct = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityExchange = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    PricingModel = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MinCabPrice
    MinCabPrice_exponent = new DefaultField<int32_t>(false, true, -2);
    MinCabPrice_mantissa = new NoOpField<int64_t>(true, false, 0);
    // constructor for sequence SecurityAltIDs
    NoSecurityAltID = new NoOpField<uint32_t>(false, false, 0);
    SecurityAltID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityAltIDSource = new ConstantFieldOptional<uint32_t>(true, 8);
    // constructor for sequence SecurityAltIDs ends
    ExpirationCycle = new NoOpField<uint32_t>(false, false, 0);
    UnitOfMeasureQty = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields StrikePrice
    StrikePrice_exponent = new DefaultField<int32_t>(false, true, -2);
    StrikePrice_mantissa = new NoOpField<int64_t>(true, false, 0);
    StrikeCurrency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MinTradeVol = new NoOpField<uint64_t>(false, false, 0);
    MaxTradeVol = new NoOpField<uint64_t>(false, false, 0);
    Currency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlCurrency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence MDFeedTypes
    NoMDFeedTypes = new NoOpField<uint32_t>(false, false, 0);
    MDFeedType = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("GBX"));
    MarketDepth = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDFeedTypes ends
    MatchAlgo = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecuritySubType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t>(false, false, 0);
    UnderlyingSymbol = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("[N/A]"));
    UnderlyingSecurityID = new DeltaField<uint32_t>(true, false, 0);
    UnderlyingSecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    // constructor for sequence Underlyings ends
    MaxPriceVariation = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    ImpliedMarketIndicator = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence InstrAttrib
    NoInstrAttrib = new NoOpField<uint32_t>(false, false, 0);
    InstrAttribType = new DeltaField<uint64_t>(true, false, 0);
    InstrAttribValue = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence InstrAttrib ends
    MaturityMonthYear = new NoOpField<uint64_t>(false, false, 0);
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent = new CopyField<int32_t>(false, true, -2);
    MinPriceIncrement_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields MinPriceIncrementAmount
    MinPriceIncrementAmount_exponent = new DefaultField<int32_t>(false, true, -2);
    MinPriceIncrementAmount_mantissa = new NoOpField<int64_t>(true, false, 0);
    LastUpdateTime = new NoOpField<uint64_t>(false, false, 0);
    SecurityUpdateAction = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields DisplayFactor
    DisplayFactor_exponent = new DefaultField<int32_t>(false, true, -2);
    DisplayFactor_mantissa = new NoOpField<int64_t>(true, false, 0);
    // constructor for sequence Legs
    NoLegs = new NoOpField<uint32_t>(false, false, 0);
    LegSymbol = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("[N/A]"));
    LegRatioQty = new CopyField<uint32_t>(true, false, 0);
    LegSecurityID = new DeltaField<uint64_t>(true, false, 0);
    LegSecurityDesc = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegSecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    LegSide = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    LegSecurityGroup = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegCFICode = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegSecuritySubType = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegCurrency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegMaturityMonthYear = new DeltaField<uint64_t>(false, false, 0);
    // decimal fields LegStrikePrice
    LegStrikePrice_exponent = new CopyField<int32_t>(false, true, -2);
    LegStrikePrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    LegSecurityExchange = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegStrikeCurrency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields LegPrice
    LegPrice_exponent = new CopyField<int32_t>(false, true, -2);
    LegPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields LegOptionDelta
    LegOptionDelta_exponent = new CopyField<int32_t>(false, true, -2);
    LegOptionDelta_mantissa = new DeltaField<int64_t>(true, false, 0);
    // constructor for sequence Legs ends
    ApplID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    UserDefinedInstrument = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("Y"));
    // decimal fields PriceRatio
    PriceRatio_exponent = new DefaultField<int32_t>(false, true, 0);
    PriceRatio_mantissa = new NoOpField<int64_t>(true, false, 0);
    ContractMultiplierType = new DefaultField<uint32_t>(false, true, 1);
    FlowScheduleType = new NoOpField<uint32_t>(false, false, 0);
    ContractMultiplier = new NoOpField<uint32_t>(false, false, 0);
    UnitofMeasure = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    DecayQuantity = new NoOpField<uint64_t>(false, false, 0);
    DecayStartDate = new NoOpField<uint64_t>(false, false, 0);
    OriginalContractSize = new NoOpField<uint64_t>(false, false, 0);
    ClearedVolume = new NoOpField<uint32_t>(false, false, 0);
    OpenInterestQty = new NoOpField<uint32_t>(false, false, 0);
    TradingReferenceDate = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence LotTypeRules
    NumLotTypeRules = new NoOpField<uint32_t>(false, false, 0);
    LotType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MinLotSize
    MinLotSize_exponent = new DefaultField<int32_t>(false, true, 0);
    MinLotSize_mantissa = new NoOpField<int64_t>(true, false, 0);
    // constructor for sequence LotTypeRules ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TotNumReports->decode(input);
    // decode for sequence Events
    NoEvents->decode(input);

    int NoEvents_len = NoEvents->previousValue.getValue();
    if (NoEvents->previousValue.isAssigned()) {
      for (int i = 0; i < NoEvents_len; ++i) {
        EventType->decode(input);
        EventDate->decode(input);
        EventTime->decode(input);
      }
    }
    // decode for sequence Events ends
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent->decode(input, pmap0);
    if (TradingReferencePrice_exponent->isMandatory || TradingReferencePrice_exponent->isValueSet)
      TradingReferencePrice_mantissa->decode(input);
    SettlePriceType->decode(input);
    // decimal fields HighLimitPx
    HighLimitPx_exponent->decode(input, pmap0);
    if (HighLimitPx_exponent->isMandatory || HighLimitPx_exponent->isValueSet) HighLimitPx_mantissa->decode(input);
    // decimal fields LowLimitPx
    LowLimitPx_exponent->decode(input, pmap0);
    if (LowLimitPx_exponent->isMandatory || LowLimitPx_exponent->isValueSet) LowLimitPx_mantissa->decode(input);
    SecurityGroup->decode(input);
    Symbol->decode(input);
    SecurityDesc->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode(pmap0);
    CFICode->decode(input);
    UnderlyingProduct->decode(input);
    SecurityExchange->decode(input);
    PricingModel->decode(input);
    // decimal fields MinCabPrice
    MinCabPrice_exponent->decode(input, pmap0);
    if (MinCabPrice_exponent->isMandatory || MinCabPrice_exponent->isValueSet) MinCabPrice_mantissa->decode(input);
    // decode for sequence SecurityAltIDs
    NoSecurityAltID->decode(input);

    int NoSecurityAltID_len = NoSecurityAltID->previousValue.getValue();
    if (NoSecurityAltID->previousValue.isAssigned()) {
      for (int i = 0; i < NoSecurityAltID_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap1 = input.extractPmap();

        SecurityAltID->decode(input);
        SecurityAltIDSource->decode(pmap1);
      }
    }
    // decode for sequence SecurityAltIDs ends
    ExpirationCycle->decode(input);
    UnitOfMeasureQty->decode(input);
    // decimal fields StrikePrice
    StrikePrice_exponent->decode(input, pmap0);
    if (StrikePrice_exponent->isMandatory || StrikePrice_exponent->isValueSet) StrikePrice_mantissa->decode(input);
    StrikeCurrency->decode(input);
    MinTradeVol->decode(input);
    MaxTradeVol->decode(input);
    Currency->decode(input);
    SettlCurrency->decode(input);
    // decode for sequence MDFeedTypes
    NoMDFeedTypes->decode(input);

    int NoMDFeedTypes_len = NoMDFeedTypes->previousValue.getValue();
    if (NoMDFeedTypes->previousValue.isAssigned()) {
      for (int i = 0; i < NoMDFeedTypes_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap1 = input.extractPmap();

        MDFeedType->decode(input, pmap1);
        MarketDepth->decode(input);
      }
    }
    // decode for sequence MDFeedTypes ends
    MatchAlgo->decode(input);
    SecuritySubType->decode(input);
    // decode for sequence Underlyings
    NoUnderlyings->decode(input);

    int NoUnderlyings_len = NoUnderlyings->previousValue.getValue();
    if (NoUnderlyings->previousValue.isAssigned()) {
      for (int i = 0; i < NoUnderlyings_len; ++i) {
        UnderlyingSymbol->decode();
        UnderlyingSecurityID->decode(input);
        UnderlyingSecurityIDSource->decode();
      }
    }
    // decode for sequence Underlyings ends
    MaxPriceVariation->decode(input);
    ImpliedMarketIndicator->decode(input);
    // decode for sequence InstrAttrib
    NoInstrAttrib->decode(input);

    int NoInstrAttrib_len = NoInstrAttrib->previousValue.getValue();
    if (NoInstrAttrib->previousValue.isAssigned()) {
      for (int i = 0; i < NoInstrAttrib_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap1 = input.extractPmap();

        InstrAttribType->decode(input);
        InstrAttribValue->decode(input, pmap1);
      }
    }
    // decode for sequence InstrAttrib ends
    MaturityMonthYear->decode(input);
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent->decode(input, pmap0);
    if (MinPriceIncrement_exponent->isMandatory || MinPriceIncrement_exponent->previousValue.isAssigned())
      MinPriceIncrement_mantissa->decode(input);
    // decimal fields MinPriceIncrementAmount
    MinPriceIncrementAmount_exponent->decode(input, pmap0);
    if (MinPriceIncrementAmount_exponent->isMandatory || MinPriceIncrementAmount_exponent->isValueSet)
      MinPriceIncrementAmount_mantissa->decode(input);
    LastUpdateTime->decode(input);
    SecurityUpdateAction->decode(input);
    // decimal fields DisplayFactor
    DisplayFactor_exponent->decode(input, pmap0);
    if (DisplayFactor_exponent->isMandatory || DisplayFactor_exponent->isValueSet)
      DisplayFactor_mantissa->decode(input);
    // decode for sequence Legs
    NoLegs->decode(input);

    int NoLegs_len = NoLegs->previousValue.getValue();
    if (NoLegs->previousValue.isAssigned()) {
      for (int i = 0; i < NoLegs_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap1 = input.extractPmap();

        LegSymbol->decode(input, pmap1);
        LegRatioQty->decode(input, pmap1);
        LegSecurityID->decode(input);
        LegSecurityDesc->decode(input);
        LegSecurityIDSource->decode();
        LegSide->decode(input, pmap1);
        LegSecurityGroup->decode(input, pmap1);
        LegCFICode->decode(input, pmap1);
        LegSecuritySubType->decode(input, pmap1);
        LegCurrency->decode(input, pmap1);
        LegMaturityMonthYear->decode(input);
        // decimal fields LegStrikePrice
        LegStrikePrice_exponent->decode(input, pmap1);
        if (LegStrikePrice_exponent->isMandatory || LegStrikePrice_exponent->previousValue.isAssigned())
          LegStrikePrice_mantissa->decode(input);
        LegSecurityExchange->decode(input, pmap1);
        LegStrikeCurrency->decode(input, pmap1);
        // decimal fields LegPrice
        LegPrice_exponent->decode(input, pmap1);
        if (LegPrice_exponent->isMandatory || LegPrice_exponent->previousValue.isAssigned())
          LegPrice_mantissa->decode(input);
        // decimal fields LegOptionDelta
        LegOptionDelta_exponent->decode(input, pmap1);
        if (LegOptionDelta_exponent->isMandatory || LegOptionDelta_exponent->previousValue.isAssigned())
          LegOptionDelta_mantissa->decode(input);
      }
    }
    // decode for sequence Legs ends
    ApplID->decode(input);
    UserDefinedInstrument->decode(pmap0);
    // decimal fields PriceRatio
    PriceRatio_exponent->decode(input, pmap0);
    if (PriceRatio_exponent->isMandatory || PriceRatio_exponent->isValueSet) PriceRatio_mantissa->decode(input);
    ContractMultiplierType->decode(input, pmap0);
    FlowScheduleType->decode(input);
    ContractMultiplier->decode(input);
    UnitofMeasure->decode(input);
    DecayQuantity->decode(input);
    DecayStartDate->decode(input);
    OriginalContractSize->decode(input);
    ClearedVolume->decode(input);
    OpenInterestQty->decode(input);
    TradingReferenceDate->decode(input);
    // decode for sequence LotTypeRules
    NumLotTypeRules->decode(input);

    int NumLotTypeRules_len = NumLotTypeRules->previousValue.getValue();
    if (NumLotTypeRules->previousValue.isAssigned()) {
      for (int i = 0; i < NumLotTypeRules_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap1 = input.extractPmap();

        LotType->decode(input);
        // decimal fields MinLotSize
        MinLotSize_exponent->decode(input, pmap1);
        if (MinLotSize_exponent->isMandatory || MinLotSize_exponent->isValueSet) MinLotSize_mantissa->decode(input);
      }
    }
    // decode for sequence LotTypeRules ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TotNumReports->reset();
    // reset for sequence Events
    NoEvents->reset();
    EventType->reset();
    EventDate->reset();
    EventTime->reset();
    // reset for sequence Events ends
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent->reset();
    TradingReferencePrice_mantissa->reset();
    SettlePriceType->reset();
    // decimal fields HighLimitPx
    HighLimitPx_exponent->reset();
    HighLimitPx_mantissa->reset();
    // decimal fields LowLimitPx
    LowLimitPx_exponent->reset();
    LowLimitPx_mantissa->reset();
    SecurityGroup->reset();
    Symbol->reset();
    SecurityDesc->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    CFICode->reset();
    UnderlyingProduct->reset();
    SecurityExchange->reset();
    PricingModel->reset();
    // decimal fields MinCabPrice
    MinCabPrice_exponent->reset();
    MinCabPrice_mantissa->reset();
    // reset for sequence SecurityAltIDs
    NoSecurityAltID->reset();
    SecurityAltID->reset();
    SecurityAltIDSource->reset();
    // reset for sequence SecurityAltIDs ends
    ExpirationCycle->reset();
    UnitOfMeasureQty->reset();
    // decimal fields StrikePrice
    StrikePrice_exponent->reset();
    StrikePrice_mantissa->reset();
    StrikeCurrency->reset();
    MinTradeVol->reset();
    MaxTradeVol->reset();
    Currency->reset();
    SettlCurrency->reset();
    // reset for sequence MDFeedTypes
    NoMDFeedTypes->reset();
    MDFeedType->reset();
    MarketDepth->reset();
    // reset for sequence MDFeedTypes ends
    MatchAlgo->reset();
    SecuritySubType->reset();
    // reset for sequence Underlyings
    NoUnderlyings->reset();
    UnderlyingSymbol->reset();
    UnderlyingSecurityID->reset();
    UnderlyingSecurityIDSource->reset();
    // reset for sequence Underlyings ends
    MaxPriceVariation->reset();
    ImpliedMarketIndicator->reset();
    // reset for sequence InstrAttrib
    NoInstrAttrib->reset();
    InstrAttribType->reset();
    InstrAttribValue->reset();
    // reset for sequence InstrAttrib ends
    MaturityMonthYear->reset();
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent->reset();
    MinPriceIncrement_mantissa->reset();
    // decimal fields MinPriceIncrementAmount
    MinPriceIncrementAmount_exponent->reset();
    MinPriceIncrementAmount_mantissa->reset();
    LastUpdateTime->reset();
    SecurityUpdateAction->reset();
    // decimal fields DisplayFactor
    DisplayFactor_exponent->reset();
    DisplayFactor_mantissa->reset();
    // reset for sequence Legs
    NoLegs->reset();
    LegSymbol->reset();
    LegRatioQty->reset();
    LegSecurityID->reset();
    LegSecurityDesc->reset();
    LegSecurityIDSource->reset();
    LegSide->reset();
    LegSecurityGroup->reset();
    LegCFICode->reset();
    LegSecuritySubType->reset();
    LegCurrency->reset();
    LegMaturityMonthYear->reset();
    // decimal fields LegStrikePrice
    LegStrikePrice_exponent->reset();
    LegStrikePrice_mantissa->reset();
    LegSecurityExchange->reset();
    LegStrikeCurrency->reset();
    // decimal fields LegPrice
    LegPrice_exponent->reset();
    LegPrice_mantissa->reset();
    // decimal fields LegOptionDelta
    LegOptionDelta_exponent->reset();
    LegOptionDelta_mantissa->reset();
    // reset for sequence Legs ends
    ApplID->reset();
    UserDefinedInstrument->reset();
    // decimal fields PriceRatio
    PriceRatio_exponent->reset();
    PriceRatio_mantissa->reset();
    ContractMultiplierType->reset();
    FlowScheduleType->reset();
    ContractMultiplier->reset();
    UnitofMeasure->reset();
    DecayQuantity->reset();
    DecayStartDate->reset();
    OriginalContractSize->reset();
    ClearedVolume->reset();
    OpenInterestQty->reset();
    TradingReferenceDate->reset();
    // reset for sequence LotTypeRules
    NumLotTypeRules->reset();
    LotType->reset();
    // decimal fields MinLotSize
    MinLotSize_exponent->reset();
    MinLotSize_mantissa->reset();
    // reset for sequence LotTypeRules ends
  }

  // Destructor
  virtual ~MDSecurityDefinition_79() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TotNumReports;
    // destructor for sequence Events
    delete NoEvents;
    delete EventType;
    delete EventDate;
    delete EventTime;
    // destructor for sequence Events ends
    // decimal fields TradingReferencePrice
    delete TradingReferencePrice_exponent;
    delete TradingReferencePrice_mantissa;
    delete SettlePriceType;
    // decimal fields HighLimitPx
    delete HighLimitPx_exponent;
    delete HighLimitPx_mantissa;
    // decimal fields LowLimitPx
    delete LowLimitPx_exponent;
    delete LowLimitPx_mantissa;
    delete SecurityGroup;
    delete Symbol;
    delete SecurityDesc;
    delete SecurityID;
    delete SecurityIDSource;
    delete CFICode;
    delete UnderlyingProduct;
    delete SecurityExchange;
    delete PricingModel;
    // decimal fields MinCabPrice
    delete MinCabPrice_exponent;
    delete MinCabPrice_mantissa;
    // destructor for sequence SecurityAltIDs
    delete NoSecurityAltID;
    delete SecurityAltID;
    delete SecurityAltIDSource;
    // destructor for sequence SecurityAltIDs ends
    delete ExpirationCycle;
    delete UnitOfMeasureQty;
    // decimal fields StrikePrice
    delete StrikePrice_exponent;
    delete StrikePrice_mantissa;
    delete StrikeCurrency;
    delete MinTradeVol;
    delete MaxTradeVol;
    delete Currency;
    delete SettlCurrency;
    // destructor for sequence MDFeedTypes
    delete NoMDFeedTypes;
    delete MDFeedType;
    delete MarketDepth;
    // destructor for sequence MDFeedTypes ends
    delete MatchAlgo;
    delete SecuritySubType;
    // destructor for sequence Underlyings
    delete NoUnderlyings;
    delete UnderlyingSymbol;
    delete UnderlyingSecurityID;
    delete UnderlyingSecurityIDSource;
    // destructor for sequence Underlyings ends
    delete MaxPriceVariation;
    delete ImpliedMarketIndicator;
    // destructor for sequence InstrAttrib
    delete NoInstrAttrib;
    delete InstrAttribType;
    delete InstrAttribValue;
    // destructor for sequence InstrAttrib ends
    delete MaturityMonthYear;
    // decimal fields MinPriceIncrement
    delete MinPriceIncrement_exponent;
    delete MinPriceIncrement_mantissa;
    // decimal fields MinPriceIncrementAmount
    delete MinPriceIncrementAmount_exponent;
    delete MinPriceIncrementAmount_mantissa;
    delete LastUpdateTime;
    delete SecurityUpdateAction;
    // decimal fields DisplayFactor
    delete DisplayFactor_exponent;
    delete DisplayFactor_mantissa;
    // destructor for sequence Legs
    delete NoLegs;
    delete LegSymbol;
    delete LegRatioQty;
    delete LegSecurityID;
    delete LegSecurityDesc;
    delete LegSecurityIDSource;
    delete LegSide;
    delete LegSecurityGroup;
    delete LegCFICode;
    delete LegSecuritySubType;
    delete LegCurrency;
    delete LegMaturityMonthYear;
    // decimal fields LegStrikePrice
    delete LegStrikePrice_exponent;
    delete LegStrikePrice_mantissa;
    delete LegSecurityExchange;
    delete LegStrikeCurrency;
    // decimal fields LegPrice
    delete LegPrice_exponent;
    delete LegPrice_mantissa;
    // decimal fields LegOptionDelta
    delete LegOptionDelta_exponent;
    delete LegOptionDelta_mantissa;
    // destructor for sequence Legs ends
    delete ApplID;
    delete UserDefinedInstrument;
    // decimal fields PriceRatio
    delete PriceRatio_exponent;
    delete PriceRatio_mantissa;
    delete ContractMultiplierType;
    delete FlowScheduleType;
    delete ContractMultiplier;
    delete UnitofMeasure;
    delete DecayQuantity;
    delete DecayStartDate;
    delete OriginalContractSize;
    delete ClearedVolume;
    delete OpenInterestQty;
    delete TradingReferenceDate;
    // destructor for sequence LotTypeRules
    delete NumLotTypeRules;
    delete LotType;
    // decimal fields MinLotSize
    delete MinLotSize_exponent;
    delete MinLotSize_mantissa;
    // destructor for sequence LotTypeRules ends
  }
};
class MDSecurityDefinition_78 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TotNumReports;
  // fields inside sequence Events
  NoOpField<uint32_t> *NoEvents;
  DeltaField<uint32_t> *EventType;
  DeltaField<uint64_t> *EventDate;
  DeltaField<uint64_t> *EventTime;
  // fields inside sequence Events ends
  // decimal fields TradingReferencePrice
  DefaultField<int32_t> *TradingReferencePrice_exponent;
  NoOpField<int64_t> *TradingReferencePrice_mantissa;
  // decimal fields HighLimitPx
  DefaultField<int32_t> *HighLimitPx_exponent;
  NoOpField<int64_t> *HighLimitPx_mantissa;
  // decimal fields LowLimitPx
  DefaultField<int32_t> *LowLimitPx_exponent;
  NoOpField<int64_t> *LowLimitPx_mantissa;
  NoOpField<FFUtils::ByteArr> *SecurityGroup;
  NoOpField<FFUtils::ByteArr> *Symbol;
  NoOpField<FFUtils::ByteArr> *SecurityDesc;
  NoOpField<uint32_t> *SecurityID;
  ConstantFieldOptional<uint32_t> *SecurityIDSource;
  NoOpField<FFUtils::ByteArr> *CFICode;
  NoOpField<FFUtils::ByteArr> *UnderlyingProduct;
  NoOpField<FFUtils::ByteArr> *SecurityExchange;
  NoOpField<FFUtils::ByteArr> *PricingModel;
  // decimal fields MinCabPrice
  DefaultField<int32_t> *MinCabPrice_exponent;
  NoOpField<int64_t> *MinCabPrice_mantissa;
  NoOpField<uint32_t> *ExpirationCycle;
  NoOpField<FFUtils::ByteArr> *UnitOfMeasureQty;
  // decimal fields StrikePrice
  DefaultField<int32_t> *StrikePrice_exponent;
  NoOpField<int64_t> *StrikePrice_mantissa;
  NoOpField<FFUtils::ByteArr> *StrikeCurrency;
  NoOpField<uint64_t> *MinTradeVol;
  NoOpField<uint64_t> *MaxTradeVol;
  NoOpField<FFUtils::ByteArr> *Currency;
  NoOpField<FFUtils::ByteArr> *SettlCurrency;
  // fields inside sequence MDFeedTypes
  NoOpField<uint32_t> *NoMDFeedTypes;
  DefaultField<FFUtils::ByteArr> *MDFeedType;
  NoOpField<uint32_t> *MarketDepth;
  // fields inside sequence MDFeedTypes ends
  NoOpField<FFUtils::ByteArr> *MatchAlgo;
  NoOpField<FFUtils::ByteArr> *SecuritySubType;
  // fields inside sequence Underlyings
  NoOpField<uint32_t> *NoUnderlyings;
  ConstantFieldMandatory<FFUtils::ByteArr> *UnderlyingSymbol;
  DeltaField<uint32_t> *UnderlyingSecurityID;
  ConstantFieldMandatory<uint32_t> *UnderlyingSecurityIDSource;
  // fields inside sequence Underlyings ends
  NoOpField<FFUtils::ByteArr> *MaxPriceVariation;
  NoOpField<FFUtils::ByteArr> *ImpliedMarketIndicator;
  // fields inside sequence InstrAttrib
  NoOpField<uint32_t> *NoInstrAttrib;
  DeltaField<uint64_t> *InstrAttribType;
  CopyField<FFUtils::ByteArr> *InstrAttribValue;
  // fields inside sequence InstrAttrib ends
  NoOpField<uint64_t> *MaturityMonthYear;
  // decimal fields MinPriceIncrement
  CopyField<int32_t> *MinPriceIncrement_exponent;
  DeltaField<int64_t> *MinPriceIncrement_mantissa;
  // decimal fields MinPriceIncrementAmount
  DefaultField<int32_t> *MinPriceIncrementAmount_exponent;
  NoOpField<int64_t> *MinPriceIncrementAmount_mantissa;
  // decimal fields DisplayFactor
  DefaultField<int32_t> *DisplayFactor_exponent;
  NoOpField<int64_t> *DisplayFactor_mantissa;
  // fields inside sequence Legs
  NoOpField<uint32_t> *NoLegs;
  DefaultField<FFUtils::ByteArr> *LegSymbol;
  CopyField<uint32_t> *LegRatioQty;
  DeltaField<uint64_t> *LegSecurityID;
  ConstantFieldMandatory<uint32_t> *LegSecurityIDSource;
  DefaultField<FFUtils::ByteArr> *LegSide;
  CopyField<FFUtils::ByteArr> *LegCFICode;
  CopyField<FFUtils::ByteArr> *LegSecuritySubType;
  CopyField<FFUtils::ByteArr> *LegCurrency;
  DeltaField<uint64_t> *LegMaturityMonthYear;
  // decimal fields LegStrikePrice
  CopyField<int32_t> *LegStrikePrice_exponent;
  DeltaField<int64_t> *LegStrikePrice_mantissa;
  CopyField<FFUtils::ByteArr> *LegSecurityExchange;
  CopyField<FFUtils::ByteArr> *LegStrikeCurrency;
  // decimal fields LegPrice
  CopyField<int32_t> *LegPrice_exponent;
  DeltaField<int64_t> *LegPrice_mantissa;
  // decimal fields LegOptionDelta
  CopyField<int32_t> *LegOptionDelta_exponent;
  DeltaField<int64_t> *LegOptionDelta_mantissa;
  // fields inside sequence Legs ends
  NoOpField<FFUtils::ByteArr> *ApplFeedId;
  ConstantFieldOptional<FFUtils::ByteArr> *UserDefinedInstrument;
  // decimal fields PriceRatio
  DefaultField<int32_t> *PriceRatio_exponent;
  NoOpField<int64_t> *PriceRatio_mantissa;
  DefaultField<uint32_t> *ContractMultiplierType;
  NoOpField<uint32_t> *FlowScheduleType;
  // decimal fields MinLotSize
  DefaultField<int32_t> *MinLotSize_exponent;
  NoOpField<int64_t> *MinLotSize_mantissa;
  NoOpField<uint32_t> *ContractMultiplier;
  NoOpField<FFUtils::ByteArr> *UnitofMeasure;
  NoOpField<uint64_t> *DecayQuantity;
  NoOpField<uint64_t> *DecayStartDate;
  NoOpField<uint64_t> *OriginalContractSize;
  NoOpField<uint32_t> *ClearedVolume;
  NoOpField<uint32_t> *OpenInterestQty;
  NoOpField<uint32_t> *TradingReferenceDate;

  // Constructor
  MDSecurityDefinition_78() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("d"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TotNumReports = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence Events
    NoEvents = new NoOpField<uint32_t>(false, false, 0);
    EventType = new DeltaField<uint32_t>(false, false, 0);
    EventDate = new DeltaField<uint64_t>(false, false, 0);
    EventTime = new DeltaField<uint64_t>(false, false, 0);
    // constructor for sequence Events ends
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent = new DefaultField<int32_t>(false, true, -2);
    TradingReferencePrice_mantissa = new NoOpField<int64_t>(true, false, 0);
    // decimal fields HighLimitPx
    HighLimitPx_exponent = new DefaultField<int32_t>(false, true, -2);
    HighLimitPx_mantissa = new NoOpField<int64_t>(true, false, 0);
    // decimal fields LowLimitPx
    LowLimitPx_exponent = new DefaultField<int32_t>(false, true, -2);
    LowLimitPx_mantissa = new NoOpField<int64_t>(true, false, 0);
    SecurityGroup = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Symbol = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityDesc = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new NoOpField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldOptional<uint32_t>(true, 8);
    CFICode = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    UnderlyingProduct = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityExchange = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    PricingModel = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MinCabPrice
    MinCabPrice_exponent = new DefaultField<int32_t>(false, true, -2);
    MinCabPrice_mantissa = new NoOpField<int64_t>(true, false, 0);
    ExpirationCycle = new NoOpField<uint32_t>(false, false, 0);
    UnitOfMeasureQty = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields StrikePrice
    StrikePrice_exponent = new DefaultField<int32_t>(false, true, -2);
    StrikePrice_mantissa = new NoOpField<int64_t>(true, false, 0);
    StrikeCurrency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MinTradeVol = new NoOpField<uint64_t>(false, false, 0);
    MaxTradeVol = new NoOpField<uint64_t>(false, false, 0);
    Currency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlCurrency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence MDFeedTypes
    NoMDFeedTypes = new NoOpField<uint32_t>(false, false, 0);
    MDFeedType = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("GBX"));
    MarketDepth = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDFeedTypes ends
    MatchAlgo = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecuritySubType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t>(false, false, 0);
    UnderlyingSymbol = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("[N/A]"));
    UnderlyingSecurityID = new DeltaField<uint32_t>(true, false, 0);
    UnderlyingSecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    // constructor for sequence Underlyings ends
    MaxPriceVariation = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    ImpliedMarketIndicator = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence InstrAttrib
    NoInstrAttrib = new NoOpField<uint32_t>(false, false, 0);
    InstrAttribType = new DeltaField<uint64_t>(true, false, 0);
    InstrAttribValue = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence InstrAttrib ends
    MaturityMonthYear = new NoOpField<uint64_t>(false, false, 0);
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent = new CopyField<int32_t>(false, true, -2);
    MinPriceIncrement_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields MinPriceIncrementAmount
    MinPriceIncrementAmount_exponent = new DefaultField<int32_t>(false, true, -2);
    MinPriceIncrementAmount_mantissa = new NoOpField<int64_t>(true, false, 0);
    // decimal fields DisplayFactor
    DisplayFactor_exponent = new DefaultField<int32_t>(false, true, -2);
    DisplayFactor_mantissa = new NoOpField<int64_t>(true, false, 0);
    // constructor for sequence Legs
    NoLegs = new NoOpField<uint32_t>(false, false, 0);
    LegSymbol = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("[N/A]"));
    LegRatioQty = new CopyField<uint32_t>(true, false, 0);
    LegSecurityID = new DeltaField<uint64_t>(true, false, 0);
    LegSecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    LegSide = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    LegCFICode = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegSecuritySubType = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegCurrency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegMaturityMonthYear = new DeltaField<uint64_t>(false, false, 0);
    // decimal fields LegStrikePrice
    LegStrikePrice_exponent = new CopyField<int32_t>(false, true, -2);
    LegStrikePrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    LegSecurityExchange = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegStrikeCurrency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields LegPrice
    LegPrice_exponent = new CopyField<int32_t>(false, true, -2);
    LegPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields LegOptionDelta
    LegOptionDelta_exponent = new CopyField<int32_t>(false, true, -2);
    LegOptionDelta_mantissa = new DeltaField<int64_t>(true, false, 0);
    // constructor for sequence Legs ends
    ApplFeedId = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    UserDefinedInstrument = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("Y"));
    // decimal fields PriceRatio
    PriceRatio_exponent = new DefaultField<int32_t>(false, true, 0);
    PriceRatio_mantissa = new NoOpField<int64_t>(true, false, 0);
    ContractMultiplierType = new DefaultField<uint32_t>(false, true, 1);
    FlowScheduleType = new NoOpField<uint32_t>(false, false, 0);
    // decimal fields MinLotSize
    MinLotSize_exponent = new DefaultField<int32_t>(false, true, 0);
    MinLotSize_mantissa = new NoOpField<int64_t>(true, false, 0);
    ContractMultiplier = new NoOpField<uint32_t>(false, false, 0);
    UnitofMeasure = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    DecayQuantity = new NoOpField<uint64_t>(false, false, 0);
    DecayStartDate = new NoOpField<uint64_t>(false, false, 0);
    OriginalContractSize = new NoOpField<uint64_t>(false, false, 0);
    ClearedVolume = new NoOpField<uint32_t>(false, false, 0);
    OpenInterestQty = new NoOpField<uint32_t>(false, false, 0);
    TradingReferenceDate = new NoOpField<uint32_t>(false, false, 0);
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TotNumReports->decode(input);
    // decode for sequence Events
    NoEvents->decode(input);

    int NoEvents_len = NoEvents->previousValue.getValue();
    if (NoEvents->previousValue.isAssigned()) {
      for (int i = 0; i < NoEvents_len; ++i) {
        EventType->decode(input);
        EventDate->decode(input);
        EventTime->decode(input);
      }
    }
    // decode for sequence Events ends
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent->decode(input, pmap0);
    if (TradingReferencePrice_exponent->isMandatory || TradingReferencePrice_exponent->isValueSet)
      TradingReferencePrice_mantissa->decode(input);
    // decimal fields HighLimitPx
    HighLimitPx_exponent->decode(input, pmap0);
    if (HighLimitPx_exponent->isMandatory || HighLimitPx_exponent->isValueSet) HighLimitPx_mantissa->decode(input);
    // decimal fields LowLimitPx
    LowLimitPx_exponent->decode(input, pmap0);
    if (LowLimitPx_exponent->isMandatory || LowLimitPx_exponent->isValueSet) LowLimitPx_mantissa->decode(input);
    SecurityGroup->decode(input);
    Symbol->decode(input);
    SecurityDesc->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode(pmap0);
    CFICode->decode(input);
    UnderlyingProduct->decode(input);
    SecurityExchange->decode(input);
    PricingModel->decode(input);
    // decimal fields MinCabPrice
    MinCabPrice_exponent->decode(input, pmap0);
    if (MinCabPrice_exponent->isMandatory || MinCabPrice_exponent->isValueSet) MinCabPrice_mantissa->decode(input);
    ExpirationCycle->decode(input);
    UnitOfMeasureQty->decode(input);
    // decimal fields StrikePrice
    StrikePrice_exponent->decode(input, pmap0);
    if (StrikePrice_exponent->isMandatory || StrikePrice_exponent->isValueSet) StrikePrice_mantissa->decode(input);
    StrikeCurrency->decode(input);
    MinTradeVol->decode(input);
    MaxTradeVol->decode(input);
    Currency->decode(input);
    SettlCurrency->decode(input);
    // decode for sequence MDFeedTypes
    NoMDFeedTypes->decode(input);

    int NoMDFeedTypes_len = NoMDFeedTypes->previousValue.getValue();
    if (NoMDFeedTypes->previousValue.isAssigned()) {
      for (int i = 0; i < NoMDFeedTypes_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap1 = input.extractPmap();

        MDFeedType->decode(input, pmap1);
        MarketDepth->decode(input);
      }
    }
    // decode for sequence MDFeedTypes ends
    MatchAlgo->decode(input);
    SecuritySubType->decode(input);
    // decode for sequence Underlyings
    NoUnderlyings->decode(input);

    int NoUnderlyings_len = NoUnderlyings->previousValue.getValue();
    if (NoUnderlyings->previousValue.isAssigned()) {
      for (int i = 0; i < NoUnderlyings_len; ++i) {
        UnderlyingSymbol->decode();
        UnderlyingSecurityID->decode(input);
        UnderlyingSecurityIDSource->decode();
      }
    }
    // decode for sequence Underlyings ends
    MaxPriceVariation->decode(input);
    ImpliedMarketIndicator->decode(input);
    // decode for sequence InstrAttrib
    NoInstrAttrib->decode(input);

    int NoInstrAttrib_len = NoInstrAttrib->previousValue.getValue();
    if (NoInstrAttrib->previousValue.isAssigned()) {
      for (int i = 0; i < NoInstrAttrib_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap1 = input.extractPmap();

        InstrAttribType->decode(input);
        InstrAttribValue->decode(input, pmap1);
      }
    }
    // decode for sequence InstrAttrib ends
    MaturityMonthYear->decode(input);
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent->decode(input, pmap0);
    if (MinPriceIncrement_exponent->isMandatory || MinPriceIncrement_exponent->previousValue.isAssigned())
      MinPriceIncrement_mantissa->decode(input);
    // decimal fields MinPriceIncrementAmount
    MinPriceIncrementAmount_exponent->decode(input, pmap0);
    if (MinPriceIncrementAmount_exponent->isMandatory || MinPriceIncrementAmount_exponent->isValueSet)
      MinPriceIncrementAmount_mantissa->decode(input);
    // decimal fields DisplayFactor
    DisplayFactor_exponent->decode(input, pmap0);
    if (DisplayFactor_exponent->isMandatory || DisplayFactor_exponent->isValueSet)
      DisplayFactor_mantissa->decode(input);
    // decode for sequence Legs
    NoLegs->decode(input);

    int NoLegs_len = NoLegs->previousValue.getValue();
    if (NoLegs->previousValue.isAssigned()) {
      for (int i = 0; i < NoLegs_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap1 = input.extractPmap();

        LegSymbol->decode(input, pmap1);
        LegRatioQty->decode(input, pmap1);
        LegSecurityID->decode(input);
        LegSecurityIDSource->decode();
        LegSide->decode(input, pmap1);
        LegCFICode->decode(input, pmap1);
        LegSecuritySubType->decode(input, pmap1);
        LegCurrency->decode(input, pmap1);
        LegMaturityMonthYear->decode(input);
        // decimal fields LegStrikePrice
        LegStrikePrice_exponent->decode(input, pmap1);
        if (LegStrikePrice_exponent->isMandatory || LegStrikePrice_exponent->previousValue.isAssigned())
          LegStrikePrice_mantissa->decode(input);
        LegSecurityExchange->decode(input, pmap1);
        LegStrikeCurrency->decode(input, pmap1);
        // decimal fields LegPrice
        LegPrice_exponent->decode(input, pmap1);
        if (LegPrice_exponent->isMandatory || LegPrice_exponent->previousValue.isAssigned())
          LegPrice_mantissa->decode(input);
        // decimal fields LegOptionDelta
        LegOptionDelta_exponent->decode(input, pmap1);
        if (LegOptionDelta_exponent->isMandatory || LegOptionDelta_exponent->previousValue.isAssigned())
          LegOptionDelta_mantissa->decode(input);
      }
    }
    // decode for sequence Legs ends
    ApplFeedId->decode(input);
    UserDefinedInstrument->decode(pmap0);
    // decimal fields PriceRatio
    PriceRatio_exponent->decode(input, pmap0);
    if (PriceRatio_exponent->isMandatory || PriceRatio_exponent->isValueSet) PriceRatio_mantissa->decode(input);
    ContractMultiplierType->decode(input, pmap0);
    FlowScheduleType->decode(input);
    // decimal fields MinLotSize
    MinLotSize_exponent->decode(input, pmap0);
    if (MinLotSize_exponent->isMandatory || MinLotSize_exponent->isValueSet) MinLotSize_mantissa->decode(input);
    ContractMultiplier->decode(input);
    UnitofMeasure->decode(input);
    DecayQuantity->decode(input);
    DecayStartDate->decode(input);
    OriginalContractSize->decode(input);
    ClearedVolume->decode(input);
    OpenInterestQty->decode(input);
    TradingReferenceDate->decode(input);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TotNumReports->reset();
    // reset for sequence Events
    NoEvents->reset();
    EventType->reset();
    EventDate->reset();
    EventTime->reset();
    // reset for sequence Events ends
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent->reset();
    TradingReferencePrice_mantissa->reset();
    // decimal fields HighLimitPx
    HighLimitPx_exponent->reset();
    HighLimitPx_mantissa->reset();
    // decimal fields LowLimitPx
    LowLimitPx_exponent->reset();
    LowLimitPx_mantissa->reset();
    SecurityGroup->reset();
    Symbol->reset();
    SecurityDesc->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    CFICode->reset();
    UnderlyingProduct->reset();
    SecurityExchange->reset();
    PricingModel->reset();
    // decimal fields MinCabPrice
    MinCabPrice_exponent->reset();
    MinCabPrice_mantissa->reset();
    ExpirationCycle->reset();
    UnitOfMeasureQty->reset();
    // decimal fields StrikePrice
    StrikePrice_exponent->reset();
    StrikePrice_mantissa->reset();
    StrikeCurrency->reset();
    MinTradeVol->reset();
    MaxTradeVol->reset();
    Currency->reset();
    SettlCurrency->reset();
    // reset for sequence MDFeedTypes
    NoMDFeedTypes->reset();
    MDFeedType->reset();
    MarketDepth->reset();
    // reset for sequence MDFeedTypes ends
    MatchAlgo->reset();
    SecuritySubType->reset();
    // reset for sequence Underlyings
    NoUnderlyings->reset();
    UnderlyingSymbol->reset();
    UnderlyingSecurityID->reset();
    UnderlyingSecurityIDSource->reset();
    // reset for sequence Underlyings ends
    MaxPriceVariation->reset();
    ImpliedMarketIndicator->reset();
    // reset for sequence InstrAttrib
    NoInstrAttrib->reset();
    InstrAttribType->reset();
    InstrAttribValue->reset();
    // reset for sequence InstrAttrib ends
    MaturityMonthYear->reset();
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent->reset();
    MinPriceIncrement_mantissa->reset();
    // decimal fields MinPriceIncrementAmount
    MinPriceIncrementAmount_exponent->reset();
    MinPriceIncrementAmount_mantissa->reset();
    // decimal fields DisplayFactor
    DisplayFactor_exponent->reset();
    DisplayFactor_mantissa->reset();
    // reset for sequence Legs
    NoLegs->reset();
    LegSymbol->reset();
    LegRatioQty->reset();
    LegSecurityID->reset();
    LegSecurityIDSource->reset();
    LegSide->reset();
    LegCFICode->reset();
    LegSecuritySubType->reset();
    LegCurrency->reset();
    LegMaturityMonthYear->reset();
    // decimal fields LegStrikePrice
    LegStrikePrice_exponent->reset();
    LegStrikePrice_mantissa->reset();
    LegSecurityExchange->reset();
    LegStrikeCurrency->reset();
    // decimal fields LegPrice
    LegPrice_exponent->reset();
    LegPrice_mantissa->reset();
    // decimal fields LegOptionDelta
    LegOptionDelta_exponent->reset();
    LegOptionDelta_mantissa->reset();
    // reset for sequence Legs ends
    ApplFeedId->reset();
    UserDefinedInstrument->reset();
    // decimal fields PriceRatio
    PriceRatio_exponent->reset();
    PriceRatio_mantissa->reset();
    ContractMultiplierType->reset();
    FlowScheduleType->reset();
    // decimal fields MinLotSize
    MinLotSize_exponent->reset();
    MinLotSize_mantissa->reset();
    ContractMultiplier->reset();
    UnitofMeasure->reset();
    DecayQuantity->reset();
    DecayStartDate->reset();
    OriginalContractSize->reset();
    ClearedVolume->reset();
    OpenInterestQty->reset();
    TradingReferenceDate->reset();
  }

  // Destructor
  virtual ~MDSecurityDefinition_78() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TotNumReports;
    // destructor for sequence Events
    delete NoEvents;
    delete EventType;
    delete EventDate;
    delete EventTime;
    // destructor for sequence Events ends
    // decimal fields TradingReferencePrice
    delete TradingReferencePrice_exponent;
    delete TradingReferencePrice_mantissa;
    // decimal fields HighLimitPx
    delete HighLimitPx_exponent;
    delete HighLimitPx_mantissa;
    // decimal fields LowLimitPx
    delete LowLimitPx_exponent;
    delete LowLimitPx_mantissa;
    delete SecurityGroup;
    delete Symbol;
    delete SecurityDesc;
    delete SecurityID;
    delete SecurityIDSource;
    delete CFICode;
    delete UnderlyingProduct;
    delete SecurityExchange;
    delete PricingModel;
    // decimal fields MinCabPrice
    delete MinCabPrice_exponent;
    delete MinCabPrice_mantissa;
    delete ExpirationCycle;
    delete UnitOfMeasureQty;
    // decimal fields StrikePrice
    delete StrikePrice_exponent;
    delete StrikePrice_mantissa;
    delete StrikeCurrency;
    delete MinTradeVol;
    delete MaxTradeVol;
    delete Currency;
    delete SettlCurrency;
    // destructor for sequence MDFeedTypes
    delete NoMDFeedTypes;
    delete MDFeedType;
    delete MarketDepth;
    // destructor for sequence MDFeedTypes ends
    delete MatchAlgo;
    delete SecuritySubType;
    // destructor for sequence Underlyings
    delete NoUnderlyings;
    delete UnderlyingSymbol;
    delete UnderlyingSecurityID;
    delete UnderlyingSecurityIDSource;
    // destructor for sequence Underlyings ends
    delete MaxPriceVariation;
    delete ImpliedMarketIndicator;
    // destructor for sequence InstrAttrib
    delete NoInstrAttrib;
    delete InstrAttribType;
    delete InstrAttribValue;
    // destructor for sequence InstrAttrib ends
    delete MaturityMonthYear;
    // decimal fields MinPriceIncrement
    delete MinPriceIncrement_exponent;
    delete MinPriceIncrement_mantissa;
    // decimal fields MinPriceIncrementAmount
    delete MinPriceIncrementAmount_exponent;
    delete MinPriceIncrementAmount_mantissa;
    // decimal fields DisplayFactor
    delete DisplayFactor_exponent;
    delete DisplayFactor_mantissa;
    // destructor for sequence Legs
    delete NoLegs;
    delete LegSymbol;
    delete LegRatioQty;
    delete LegSecurityID;
    delete LegSecurityIDSource;
    delete LegSide;
    delete LegCFICode;
    delete LegSecuritySubType;
    delete LegCurrency;
    delete LegMaturityMonthYear;
    // decimal fields LegStrikePrice
    delete LegStrikePrice_exponent;
    delete LegStrikePrice_mantissa;
    delete LegSecurityExchange;
    delete LegStrikeCurrency;
    // decimal fields LegPrice
    delete LegPrice_exponent;
    delete LegPrice_mantissa;
    // decimal fields LegOptionDelta
    delete LegOptionDelta_exponent;
    delete LegOptionDelta_mantissa;
    // destructor for sequence Legs ends
    delete ApplFeedId;
    delete UserDefinedInstrument;
    // decimal fields PriceRatio
    delete PriceRatio_exponent;
    delete PriceRatio_mantissa;
    delete ContractMultiplierType;
    delete FlowScheduleType;
    // decimal fields MinLotSize
    delete MinLotSize_exponent;
    delete MinLotSize_mantissa;
    delete ContractMultiplier;
    delete UnitofMeasure;
    delete DecayQuantity;
    delete DecayStartDate;
    delete OriginalContractSize;
    delete ClearedVolume;
    delete OpenInterestQty;
    delete TradingReferenceDate;
  }
};
class MDSecurityDefinition_52 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *TotNumReports;
  // fields inside sequence Events
  NoOpField<uint32_t> *NoEvents;
  DeltaField<uint32_t> *EventType;
  DeltaField<uint64_t> *EventDate;
  DeltaField<uint64_t> *EventTime;
  // fields inside sequence Events ends
  // decimal fields TradingReferencePrice
  DefaultField<int32_t> *TradingReferencePrice_exponent;
  NoOpField<int64_t> *TradingReferencePrice_mantissa;
  // decimal fields HighLimitPx
  DefaultField<int32_t> *HighLimitPx_exponent;
  NoOpField<int64_t> *HighLimitPx_mantissa;
  // decimal fields LowLimitPx
  DefaultField<int32_t> *LowLimitPx_exponent;
  NoOpField<int64_t> *LowLimitPx_mantissa;
  NoOpField<FFUtils::ByteArr> *SecurityGroup;
  NoOpField<FFUtils::ByteArr> *Symbol;
  NoOpField<FFUtils::ByteArr> *SecurityDesc;
  NoOpField<uint32_t> *SecurityID;
  ConstantFieldOptional<uint32_t> *SecurityIDSource;
  NoOpField<FFUtils::ByteArr> *CFICode;
  NoOpField<FFUtils::ByteArr> *UnderlyingProduct;
  NoOpField<FFUtils::ByteArr> *SecurityExchange;
  NoOpField<FFUtils::ByteArr> *PricingModel;
  // decimal fields MinCabPrice
  DefaultField<int32_t> *MinCabPrice_exponent;
  NoOpField<int64_t> *MinCabPrice_mantissa;
  NoOpField<uint32_t> *ExpirationCycle;
  NoOpField<FFUtils::ByteArr> *UnitOfMeasureQty;
  // decimal fields StrikePrice
  DefaultField<int32_t> *StrikePrice_exponent;
  NoOpField<int64_t> *StrikePrice_mantissa;
  NoOpField<FFUtils::ByteArr> *StrikeCurrency;
  NoOpField<uint64_t> *MinTradeVol;
  NoOpField<uint64_t> *MaxTradeVol;
  NoOpField<FFUtils::ByteArr> *Currency;
  NoOpField<FFUtils::ByteArr> *SettlCurrency;
  // fields inside sequence MDFeedTypes
  NoOpField<uint32_t> *NoMDFeedTypes;
  ConstantFieldMandatory<FFUtils::ByteArr> *MDFeedType;
  NoOpField<uint32_t> *MarketDepth;
  // fields inside sequence MDFeedTypes ends
  NoOpField<FFUtils::ByteArr> *MatchAlgo;
  NoOpField<FFUtils::ByteArr> *SecuritySubType;
  // fields inside sequence Underlyings
  NoOpField<uint32_t> *NoUnderlyings;
  ConstantFieldMandatory<FFUtils::ByteArr> *UnderlyingSymbol;
  DeltaField<uint32_t> *UnderlyingSecurityID;
  ConstantFieldMandatory<uint32_t> *UnderlyingSecurityIDSource;
  // fields inside sequence Underlyings ends
  NoOpField<FFUtils::ByteArr> *MaxPriceVariation;
  NoOpField<FFUtils::ByteArr> *ImpliedMarketIndicator;
  // fields inside sequence InstrAttrib
  NoOpField<uint32_t> *NoInstrAttrib;
  DeltaField<uint64_t> *InstrAttribType;
  CopyField<FFUtils::ByteArr> *InstrAttribValue;
  // fields inside sequence InstrAttrib ends
  NoOpField<uint64_t> *MaturityDate;
  // decimal fields MinPriceIncrement
  CopyField<int32_t> *MinPriceIncrement_exponent;
  DeltaField<int64_t> *MinPriceIncrement_mantissa;
  // decimal fields MinPriceIncrementAmount
  DefaultField<int32_t> *MinPriceIncrementAmount_exponent;
  NoOpField<int64_t> *MinPriceIncrementAmount_mantissa;
  // decimal fields DisplayFactor
  DefaultField<int32_t> *DisplayFactor_exponent;
  NoOpField<int64_t> *DisplayFactor_mantissa;
  // fields inside sequence Legs
  NoOpField<uint32_t> *NoLegs;
  DefaultField<FFUtils::ByteArr> *LegSymbol;
  CopyField<uint32_t> *LegRatioQty;
  DeltaField<uint64_t> *LegSecurityID;
  ConstantFieldMandatory<uint32_t> *LegSecurityIDSource;
  DefaultField<FFUtils::ByteArr> *LegSide;
  CopyField<FFUtils::ByteArr> *LegCFICode;
  CopyField<FFUtils::ByteArr> *LegSecuritySubType;
  CopyField<FFUtils::ByteArr> *LegCurrency;
  DeltaField<uint64_t> *LegMaturityMonthYear;
  // decimal fields LegStrikePrice
  CopyField<int32_t> *LegStrikePrice_exponent;
  DeltaField<int64_t> *LegStrikePrice_mantissa;
  CopyField<FFUtils::ByteArr> *LegStrikeCurrency;
  // decimal fields LegPrice
  CopyField<int32_t> *LegPrice_exponent;
  DeltaField<int64_t> *LegPrice_mantissa;
  // decimal fields LegOptionDelta
  CopyField<int32_t> *LegOptionDelta_exponent;
  DeltaField<int64_t> *LegOptionDelta_mantissa;
  // fields inside sequence Legs ends
  NoOpField<FFUtils::ByteArr> *ApplFeedId;
  ConstantFieldOptional<FFUtils::ByteArr> *UserDefinedInstrument;

  // Constructor
  MDSecurityDefinition_52() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("d"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TotNumReports = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence Events
    NoEvents = new NoOpField<uint32_t>(false, false, 0);
    EventType = new DeltaField<uint32_t>(false, false, 0);
    EventDate = new DeltaField<uint64_t>(false, false, 0);
    EventTime = new DeltaField<uint64_t>(false, false, 0);
    // constructor for sequence Events ends
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent = new DefaultField<int32_t>(false, true, -2);
    TradingReferencePrice_mantissa = new NoOpField<int64_t>(true, false, 0);
    // decimal fields HighLimitPx
    HighLimitPx_exponent = new DefaultField<int32_t>(false, true, -2);
    HighLimitPx_mantissa = new NoOpField<int64_t>(true, false, 0);
    // decimal fields LowLimitPx
    LowLimitPx_exponent = new DefaultField<int32_t>(false, true, -2);
    LowLimitPx_mantissa = new NoOpField<int64_t>(true, false, 0);
    SecurityGroup = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Symbol = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityDesc = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new NoOpField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldOptional<uint32_t>(true, 8);
    CFICode = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    UnderlyingProduct = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityExchange = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    PricingModel = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MinCabPrice
    MinCabPrice_exponent = new DefaultField<int32_t>(false, true, -2);
    MinCabPrice_mantissa = new NoOpField<int64_t>(true, false, 0);
    ExpirationCycle = new NoOpField<uint32_t>(false, false, 0);
    UnitOfMeasureQty = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields StrikePrice
    StrikePrice_exponent = new DefaultField<int32_t>(false, true, -2);
    StrikePrice_mantissa = new NoOpField<int64_t>(true, false, 0);
    StrikeCurrency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MinTradeVol = new NoOpField<uint64_t>(false, false, 0);
    MaxTradeVol = new NoOpField<uint64_t>(false, false, 0);
    Currency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlCurrency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence MDFeedTypes
    NoMDFeedTypes = new NoOpField<uint32_t>(false, false, 0);
    MDFeedType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("GBX"));
    MarketDepth = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence MDFeedTypes ends
    MatchAlgo = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecuritySubType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t>(false, false, 0);
    UnderlyingSymbol = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("[N/A]"));
    UnderlyingSecurityID = new DeltaField<uint32_t>(true, false, 0);
    UnderlyingSecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    // constructor for sequence Underlyings ends
    MaxPriceVariation = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    ImpliedMarketIndicator = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence InstrAttrib
    NoInstrAttrib = new NoOpField<uint32_t>(false, false, 0);
    InstrAttribType = new DeltaField<uint64_t>(true, false, 0);
    InstrAttribValue = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence InstrAttrib ends
    MaturityDate = new NoOpField<uint64_t>(false, false, 0);
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent = new CopyField<int32_t>(false, true, -2);
    MinPriceIncrement_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields MinPriceIncrementAmount
    MinPriceIncrementAmount_exponent = new DefaultField<int32_t>(false, true, -2);
    MinPriceIncrementAmount_mantissa = new NoOpField<int64_t>(true, false, 0);
    // decimal fields DisplayFactor
    DisplayFactor_exponent = new DefaultField<int32_t>(false, true, -2);
    DisplayFactor_mantissa = new NoOpField<int64_t>(true, false, 0);
    // constructor for sequence Legs
    NoLegs = new NoOpField<uint32_t>(false, false, 0);
    LegSymbol = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("[N/A]"));
    LegRatioQty = new CopyField<uint32_t>(true, false, 0);
    LegSecurityID = new DeltaField<uint64_t>(true, false, 0);
    LegSecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    LegSide = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("1"));
    LegCFICode = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegSecuritySubType = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegCurrency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegMaturityMonthYear = new DeltaField<uint64_t>(false, false, 0);
    // decimal fields LegStrikePrice
    LegStrikePrice_exponent = new CopyField<int32_t>(false, true, -2);
    LegStrikePrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    LegStrikeCurrency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields LegPrice
    LegPrice_exponent = new CopyField<int32_t>(false, true, -2);
    LegPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields LegOptionDelta
    LegOptionDelta_exponent = new CopyField<int32_t>(false, true, -2);
    LegOptionDelta_mantissa = new DeltaField<int64_t>(true, false, 0);
    // constructor for sequence Legs ends
    ApplFeedId = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    UserDefinedInstrument = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("Y"));
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    TotNumReports->decode(input);
    // decode for sequence Events
    NoEvents->decode(input);

    int NoEvents_len = NoEvents->previousValue.getValue();
    if (NoEvents->previousValue.isAssigned()) {
      for (int i = 0; i < NoEvents_len; ++i) {
        EventType->decode(input);
        EventDate->decode(input);
        EventTime->decode(input);
      }
    }
    // decode for sequence Events ends
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent->decode(input, pmap0);
    if (TradingReferencePrice_exponent->isMandatory || TradingReferencePrice_exponent->isValueSet)
      TradingReferencePrice_mantissa->decode(input);
    // decimal fields HighLimitPx
    HighLimitPx_exponent->decode(input, pmap0);
    if (HighLimitPx_exponent->isMandatory || HighLimitPx_exponent->isValueSet) HighLimitPx_mantissa->decode(input);
    // decimal fields LowLimitPx
    LowLimitPx_exponent->decode(input, pmap0);
    if (LowLimitPx_exponent->isMandatory || LowLimitPx_exponent->isValueSet) LowLimitPx_mantissa->decode(input);
    SecurityGroup->decode(input);
    Symbol->decode(input);
    SecurityDesc->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode(pmap0);
    CFICode->decode(input);
    UnderlyingProduct->decode(input);
    SecurityExchange->decode(input);
    PricingModel->decode(input);
    // decimal fields MinCabPrice
    MinCabPrice_exponent->decode(input, pmap0);
    if (MinCabPrice_exponent->isMandatory || MinCabPrice_exponent->isValueSet) MinCabPrice_mantissa->decode(input);
    ExpirationCycle->decode(input);
    UnitOfMeasureQty->decode(input);
    // decimal fields StrikePrice
    StrikePrice_exponent->decode(input, pmap0);
    if (StrikePrice_exponent->isMandatory || StrikePrice_exponent->isValueSet) StrikePrice_mantissa->decode(input);
    StrikeCurrency->decode(input);
    MinTradeVol->decode(input);
    MaxTradeVol->decode(input);
    Currency->decode(input);
    SettlCurrency->decode(input);
    // decode for sequence MDFeedTypes
    NoMDFeedTypes->decode(input);

    int NoMDFeedTypes_len = NoMDFeedTypes->previousValue.getValue();
    if (NoMDFeedTypes->previousValue.isAssigned()) {
      for (int i = 0; i < NoMDFeedTypes_len; ++i) {
        MDFeedType->decode();
        MarketDepth->decode(input);
      }
    }
    // decode for sequence MDFeedTypes ends
    MatchAlgo->decode(input);
    SecuritySubType->decode(input);
    // decode for sequence Underlyings
    NoUnderlyings->decode(input);

    int NoUnderlyings_len = NoUnderlyings->previousValue.getValue();
    if (NoUnderlyings->previousValue.isAssigned()) {
      for (int i = 0; i < NoUnderlyings_len; ++i) {
        UnderlyingSymbol->decode();
        UnderlyingSecurityID->decode(input);
        UnderlyingSecurityIDSource->decode();
      }
    }
    // decode for sequence Underlyings ends
    MaxPriceVariation->decode(input);
    ImpliedMarketIndicator->decode(input);
    // decode for sequence InstrAttrib
    NoInstrAttrib->decode(input);

    int NoInstrAttrib_len = NoInstrAttrib->previousValue.getValue();
    if (NoInstrAttrib->previousValue.isAssigned()) {
      for (int i = 0; i < NoInstrAttrib_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap1 = input.extractPmap();

        InstrAttribType->decode(input);
        InstrAttribValue->decode(input, pmap1);
      }
    }
    // decode for sequence InstrAttrib ends
    MaturityDate->decode(input);
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent->decode(input, pmap0);
    if (MinPriceIncrement_exponent->isMandatory || MinPriceIncrement_exponent->previousValue.isAssigned())
      MinPriceIncrement_mantissa->decode(input);
    // decimal fields MinPriceIncrementAmount
    MinPriceIncrementAmount_exponent->decode(input, pmap0);
    if (MinPriceIncrementAmount_exponent->isMandatory || MinPriceIncrementAmount_exponent->isValueSet)
      MinPriceIncrementAmount_mantissa->decode(input);
    // decimal fields DisplayFactor
    DisplayFactor_exponent->decode(input, pmap0);
    if (DisplayFactor_exponent->isMandatory || DisplayFactor_exponent->isValueSet)
      DisplayFactor_mantissa->decode(input);
    // decode for sequence Legs
    NoLegs->decode(input);

    int NoLegs_len = NoLegs->previousValue.getValue();
    if (NoLegs->previousValue.isAssigned()) {
      for (int i = 0; i < NoLegs_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap1 = input.extractPmap();

        LegSymbol->decode(input, pmap1);
        LegRatioQty->decode(input, pmap1);
        LegSecurityID->decode(input);
        LegSecurityIDSource->decode();
        LegSide->decode(input, pmap1);
        LegCFICode->decode(input, pmap1);
        LegSecuritySubType->decode(input, pmap1);
        LegCurrency->decode(input, pmap1);
        LegMaturityMonthYear->decode(input);
        // decimal fields LegStrikePrice
        LegStrikePrice_exponent->decode(input, pmap1);
        if (LegStrikePrice_exponent->isMandatory || LegStrikePrice_exponent->previousValue.isAssigned())
          LegStrikePrice_mantissa->decode(input);
        LegStrikeCurrency->decode(input, pmap1);
        // decimal fields LegPrice
        LegPrice_exponent->decode(input, pmap1);
        if (LegPrice_exponent->isMandatory || LegPrice_exponent->previousValue.isAssigned())
          LegPrice_mantissa->decode(input);
        // decimal fields LegOptionDelta
        LegOptionDelta_exponent->decode(input, pmap1);
        if (LegOptionDelta_exponent->isMandatory || LegOptionDelta_exponent->previousValue.isAssigned())
          LegOptionDelta_mantissa->decode(input);
      }
    }
    // decode for sequence Legs ends
    ApplFeedId->decode(input);
    UserDefinedInstrument->decode(pmap0);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    TotNumReports->reset();
    // reset for sequence Events
    NoEvents->reset();
    EventType->reset();
    EventDate->reset();
    EventTime->reset();
    // reset for sequence Events ends
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent->reset();
    TradingReferencePrice_mantissa->reset();
    // decimal fields HighLimitPx
    HighLimitPx_exponent->reset();
    HighLimitPx_mantissa->reset();
    // decimal fields LowLimitPx
    LowLimitPx_exponent->reset();
    LowLimitPx_mantissa->reset();
    SecurityGroup->reset();
    Symbol->reset();
    SecurityDesc->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    CFICode->reset();
    UnderlyingProduct->reset();
    SecurityExchange->reset();
    PricingModel->reset();
    // decimal fields MinCabPrice
    MinCabPrice_exponent->reset();
    MinCabPrice_mantissa->reset();
    ExpirationCycle->reset();
    UnitOfMeasureQty->reset();
    // decimal fields StrikePrice
    StrikePrice_exponent->reset();
    StrikePrice_mantissa->reset();
    StrikeCurrency->reset();
    MinTradeVol->reset();
    MaxTradeVol->reset();
    Currency->reset();
    SettlCurrency->reset();
    // reset for sequence MDFeedTypes
    NoMDFeedTypes->reset();
    MDFeedType->reset();
    MarketDepth->reset();
    // reset for sequence MDFeedTypes ends
    MatchAlgo->reset();
    SecuritySubType->reset();
    // reset for sequence Underlyings
    NoUnderlyings->reset();
    UnderlyingSymbol->reset();
    UnderlyingSecurityID->reset();
    UnderlyingSecurityIDSource->reset();
    // reset for sequence Underlyings ends
    MaxPriceVariation->reset();
    ImpliedMarketIndicator->reset();
    // reset for sequence InstrAttrib
    NoInstrAttrib->reset();
    InstrAttribType->reset();
    InstrAttribValue->reset();
    // reset for sequence InstrAttrib ends
    MaturityDate->reset();
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent->reset();
    MinPriceIncrement_mantissa->reset();
    // decimal fields MinPriceIncrementAmount
    MinPriceIncrementAmount_exponent->reset();
    MinPriceIncrementAmount_mantissa->reset();
    // decimal fields DisplayFactor
    DisplayFactor_exponent->reset();
    DisplayFactor_mantissa->reset();
    // reset for sequence Legs
    NoLegs->reset();
    LegSymbol->reset();
    LegRatioQty->reset();
    LegSecurityID->reset();
    LegSecurityIDSource->reset();
    LegSide->reset();
    LegCFICode->reset();
    LegSecuritySubType->reset();
    LegCurrency->reset();
    LegMaturityMonthYear->reset();
    // decimal fields LegStrikePrice
    LegStrikePrice_exponent->reset();
    LegStrikePrice_mantissa->reset();
    LegStrikeCurrency->reset();
    // decimal fields LegPrice
    LegPrice_exponent->reset();
    LegPrice_mantissa->reset();
    // decimal fields LegOptionDelta
    LegOptionDelta_exponent->reset();
    LegOptionDelta_mantissa->reset();
    // reset for sequence Legs ends
    ApplFeedId->reset();
    UserDefinedInstrument->reset();
  }

  // Destructor
  virtual ~MDSecurityDefinition_52() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete TotNumReports;
    // destructor for sequence Events
    delete NoEvents;
    delete EventType;
    delete EventDate;
    delete EventTime;
    // destructor for sequence Events ends
    // decimal fields TradingReferencePrice
    delete TradingReferencePrice_exponent;
    delete TradingReferencePrice_mantissa;
    // decimal fields HighLimitPx
    delete HighLimitPx_exponent;
    delete HighLimitPx_mantissa;
    // decimal fields LowLimitPx
    delete LowLimitPx_exponent;
    delete LowLimitPx_mantissa;
    delete SecurityGroup;
    delete Symbol;
    delete SecurityDesc;
    delete SecurityID;
    delete SecurityIDSource;
    delete CFICode;
    delete UnderlyingProduct;
    delete SecurityExchange;
    delete PricingModel;
    // decimal fields MinCabPrice
    delete MinCabPrice_exponent;
    delete MinCabPrice_mantissa;
    delete ExpirationCycle;
    delete UnitOfMeasureQty;
    // decimal fields StrikePrice
    delete StrikePrice_exponent;
    delete StrikePrice_mantissa;
    delete StrikeCurrency;
    delete MinTradeVol;
    delete MaxTradeVol;
    delete Currency;
    delete SettlCurrency;
    // destructor for sequence MDFeedTypes
    delete NoMDFeedTypes;
    delete MDFeedType;
    delete MarketDepth;
    // destructor for sequence MDFeedTypes ends
    delete MatchAlgo;
    delete SecuritySubType;
    // destructor for sequence Underlyings
    delete NoUnderlyings;
    delete UnderlyingSymbol;
    delete UnderlyingSecurityID;
    delete UnderlyingSecurityIDSource;
    // destructor for sequence Underlyings ends
    delete MaxPriceVariation;
    delete ImpliedMarketIndicator;
    // destructor for sequence InstrAttrib
    delete NoInstrAttrib;
    delete InstrAttribType;
    delete InstrAttribValue;
    // destructor for sequence InstrAttrib ends
    delete MaturityDate;
    // decimal fields MinPriceIncrement
    delete MinPriceIncrement_exponent;
    delete MinPriceIncrement_mantissa;
    // decimal fields MinPriceIncrementAmount
    delete MinPriceIncrementAmount_exponent;
    delete MinPriceIncrementAmount_mantissa;
    // decimal fields DisplayFactor
    delete DisplayFactor_exponent;
    delete DisplayFactor_mantissa;
    // destructor for sequence Legs
    delete NoLegs;
    delete LegSymbol;
    delete LegRatioQty;
    delete LegSecurityID;
    delete LegSecurityIDSource;
    delete LegSide;
    delete LegCFICode;
    delete LegSecuritySubType;
    delete LegCurrency;
    delete LegMaturityMonthYear;
    // decimal fields LegStrikePrice
    delete LegStrikePrice_exponent;
    delete LegStrikePrice_mantissa;
    delete LegStrikeCurrency;
    // decimal fields LegPrice
    delete LegPrice_exponent;
    delete LegPrice_mantissa;
    // decimal fields LegOptionDelta
    delete LegOptionDelta_exponent;
    delete LegOptionDelta_mantissa;
    // destructor for sequence Legs ends
    delete ApplFeedId;
    delete UserDefinedInstrument;
  }
};
class MDQuoteRequest_98 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<FFUtils::ByteArr> *QuoteReqID;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t> *NoRelatedSym;
  ConstantFieldMandatory<FFUtils::ByteArr> *Symbol;
  NoOpField<uint64_t> *OrderQty;
  DefaultField<uint32_t> *Side;
  NoOpField<uint64_t> *TransactTime;
  DefaultField<uint32_t> *QuoteType;
  NoOpField<uint32_t> *SecurityID;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  // fields inside sequence RelatedSym ends

  // Constructor
  MDQuoteRequest_98() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("R"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteReqID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t>(true, false, 0);
    Symbol = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("[N/A]"));
    OrderQty = new NoOpField<uint64_t>(false, false, 0);
    Side = new DefaultField<uint32_t>(false, true, 1);
    TransactTime = new NoOpField<uint64_t>(true, false, 0);
    QuoteType = new DefaultField<uint32_t>(true, true, 1);
    SecurityID = new NoOpField<uint32_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    // constructor for sequence RelatedSym ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    QuoteReqID->decode(input);
    // decode for sequence RelatedSym
    NoRelatedSym->decode(input);

    int NoRelatedSym_len = NoRelatedSym->previousValue.getValue();
    for (int i = 0; i < NoRelatedSym_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      Symbol->decode();
      OrderQty->decode(input);
      Side->decode(input, pmap1);
      TransactTime->decode(input);
      QuoteType->decode(input, pmap1);
      SecurityID->decode(input);
      SecurityIDSource->decode();
    }
    // decode for sequence RelatedSym ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    QuoteReqID->reset();
    // reset for sequence RelatedSym
    NoRelatedSym->reset();
    Symbol->reset();
    OrderQty->reset();
    Side->reset();
    TransactTime->reset();
    QuoteType->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    // reset for sequence RelatedSym ends
  }

  // Destructor
  virtual ~MDQuoteRequest_98() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete QuoteReqID;
    // destructor for sequence RelatedSym
    delete NoRelatedSym;
    delete Symbol;
    delete OrderQty;
    delete Side;
    delete TransactTime;
    delete QuoteType;
    delete SecurityID;
    delete SecurityIDSource;
    // destructor for sequence RelatedSym ends
  }
};
class MDQuoteRequest_54 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<FFUtils::ByteArr> *QuoteReqID;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t> *NoRelatedSym;
  ConstantFieldMandatory<FFUtils::ByteArr> *Symbol;
  NoOpField<uint64_t> *OrderQty;
  DefaultField<uint32_t> *Side;
  NoOpField<uint64_t> *TransactTime;
  DefaultField<uint32_t> *QuoteType;
  NoOpField<uint32_t> *SecurityID;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  // fields inside sequence RelatedSym ends

  // Constructor
  MDQuoteRequest_54() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("R"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteReqID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t>(true, false, 0);
    Symbol = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("[N/A]"));
    OrderQty = new NoOpField<uint64_t>(false, false, 0);
    Side = new DefaultField<uint32_t>(false, true, 1);
    TransactTime = new NoOpField<uint64_t>(true, false, 0);
    QuoteType = new DefaultField<uint32_t>(true, true, 1);
    SecurityID = new NoOpField<uint32_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    // constructor for sequence RelatedSym ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    QuoteReqID->decode(input);
    // decode for sequence RelatedSym
    NoRelatedSym->decode(input);

    int NoRelatedSym_len = NoRelatedSym->previousValue.getValue();
    for (int i = 0; i < NoRelatedSym_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      Symbol->decode();
      OrderQty->decode(input);
      Side->decode(input, pmap1);
      TransactTime->decode(input);
      QuoteType->decode(input, pmap1);
      SecurityID->decode(input);
      SecurityIDSource->decode();
    }
    // decode for sequence RelatedSym ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    QuoteReqID->reset();
    // reset for sequence RelatedSym
    NoRelatedSym->reset();
    Symbol->reset();
    OrderQty->reset();
    Side->reset();
    TransactTime->reset();
    QuoteType->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    // reset for sequence RelatedSym ends
  }

  // Destructor
  virtual ~MDQuoteRequest_54() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete QuoteReqID;
    // destructor for sequence RelatedSym
    delete NoRelatedSym;
    delete Symbol;
    delete OrderQty;
    delete Side;
    delete TransactTime;
    delete QuoteType;
    delete SecurityID;
    delete SecurityIDSource;
    // destructor for sequence RelatedSym ends
  }
};
class MDSecurityStatus_125 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *TradeDate;
  NoOpField<FFUtils::ByteArr> *SecurityGroup;
  NoOpField<FFUtils::ByteArr> *Symbol;
  NoOpField<uint64_t> *SecurityID;
  ConstantFieldOptional<uint32_t> *SecurityIDSource;
  NoOpField<FFUtils::ByteArr> *SecurityExchange;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  NoOpField<uint32_t> *SecurityTradingStatus;
  NoOpField<uint64_t> *TradSesOpenTime;
  NoOpField<uint64_t> *TransactTime;
  NoOpField<uint32_t> *SecurityTradingEvent;

  // Constructor
  MDSecurityStatus_125() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("f"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    SecurityGroup = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Symbol = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new NoOpField<uint64_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldOptional<uint32_t>(true, 8);
    SecurityExchange = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new NoOpField<uint32_t>(false, false, 0);
    TradSesOpenTime = new NoOpField<uint64_t>(false, false, 0);
    TransactTime = new NoOpField<uint64_t>(true, false, 0);
    SecurityTradingEvent = new NoOpField<uint32_t>(false, false, 0);
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    TradeDate->decode(input);
    SecurityGroup->decode(input);
    Symbol->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode(pmap0);
    SecurityExchange->decode(input);
    TradingSessionSubID->decode(input);
    SecurityTradingStatus->decode(input);
    TradSesOpenTime->decode(input);
    TransactTime->decode(input);
    SecurityTradingEvent->decode(input);

    process();
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    TradeDate->reset();
    SecurityGroup->reset();
    Symbol->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    TradingSessionSubID->reset();
    SecurityTradingStatus->reset();
    TradSesOpenTime->reset();
    TransactTime->reset();
    SecurityTradingEvent->reset();
  }

  // Destructor
  virtual ~MDSecurityStatus_125() {
    delete ApplVerID;
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
    delete TradeDate;
    delete SecurityGroup;
    delete Symbol;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    delete TradingSessionSubID;
    delete SecurityTradingStatus;
    delete TradSesOpenTime;
    delete TransactTime;
    delete SecurityTradingEvent;
  }
};
class MDSecurityStatus_113 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *TradeDate;
  NoOpField<FFUtils::ByteArr> *SecurityGroup;
  NoOpField<FFUtils::ByteArr> *Symbol;
  NoOpField<uint64_t> *SecurityID;
  ConstantFieldOptional<uint32_t> *SecurityIDSource;
  NoOpField<FFUtils::ByteArr> *SecurityExchange;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  NoOpField<uint32_t> *SecurityTradingStatus;
  NoOpField<uint32_t> *TradSesOpenTime;
  NoOpField<uint64_t> *TransactTime;
  NoOpField<uint32_t> *SecurityTradingEvent;

  // Constructor
  MDSecurityStatus_113() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("f"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    SecurityGroup = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Symbol = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new NoOpField<uint64_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldOptional<uint32_t>(true, 8);
    SecurityExchange = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new NoOpField<uint32_t>(false, false, 0);
    TradSesOpenTime = new NoOpField<uint32_t>(false, false, 0);
    TransactTime = new NoOpField<uint64_t>(true, false, 0);
    SecurityTradingEvent = new NoOpField<uint32_t>(false, false, 0);
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    TradeDate->decode(input);
    SecurityGroup->decode(input);
    Symbol->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode(pmap0);
    SecurityExchange->decode(input);
    TradingSessionSubID->decode(input);
    SecurityTradingStatus->decode(input);
    TradSesOpenTime->decode(input);
    TransactTime->decode(input);
    SecurityTradingEvent->decode(input);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    TradeDate->reset();
    SecurityGroup->reset();
    Symbol->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    TradingSessionSubID->reset();
    SecurityTradingStatus->reset();
    TradSesOpenTime->reset();
    TransactTime->reset();
    SecurityTradingEvent->reset();
  }

  // Destructor
  virtual ~MDSecurityStatus_113() {
    delete ApplVerID;
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
    delete TradeDate;
    delete SecurityGroup;
    delete Symbol;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    delete TradingSessionSubID;
    delete SecurityTradingStatus;
    delete TradSesOpenTime;
    delete TransactTime;
    delete SecurityTradingEvent;
  }
};
class MDSecurityStatus_102 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *SecurityID;
  ConstantFieldOptional<uint32_t> *SecurityIDSource;
  NoOpField<uint32_t> *TradeDate;
  // decimal fields HighPx
  DefaultField<int32_t> *HighPx_exponent;
  NoOpField<int64_t> *HighPx_mantissa;
  // decimal fields LowPx
  DefaultField<int32_t> *LowPx_exponent;
  NoOpField<int64_t> *LowPx_mantissa;
  NoOpField<FFUtils::ByteArr> *Symbol;
  NoOpField<uint32_t> *SecurityTradingStatus;
  NoOpField<uint32_t> *HaltReason;
  NoOpField<uint32_t> *SecurityTradingEvent;

  // Constructor
  MDSecurityStatus_102() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("f"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new NoOpField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldOptional<uint32_t>(true, 8);
    TradeDate = new NoOpField<uint32_t>(false, false, 0);
    // decimal fields HighPx
    HighPx_exponent = new DefaultField<int32_t>(false, true, -2);
    HighPx_mantissa = new NoOpField<int64_t>(true, false, 0);
    // decimal fields LowPx
    LowPx_exponent = new DefaultField<int32_t>(false, true, -2);
    LowPx_mantissa = new NoOpField<int64_t>(true, false, 0);
    Symbol = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new NoOpField<uint32_t>(false, false, 0);
    HaltReason = new NoOpField<uint32_t>(false, false, 0);
    SecurityTradingEvent = new NoOpField<uint32_t>(false, false, 0);
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    SecurityID->decode(input);
    SecurityIDSource->decode(pmap0);
    TradeDate->decode(input);
    // decimal fields HighPx
    HighPx_exponent->decode(input, pmap0);
    if (HighPx_exponent->isMandatory || HighPx_exponent->isValueSet) HighPx_mantissa->decode(input);
    // decimal fields LowPx
    LowPx_exponent->decode(input, pmap0);
    if (LowPx_exponent->isMandatory || LowPx_exponent->isValueSet) LowPx_mantissa->decode(input);
    Symbol->decode(input);
    SecurityTradingStatus->decode(input);
    HaltReason->decode(input);
    SecurityTradingEvent->decode(input);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    TradeDate->reset();
    // decimal fields HighPx
    HighPx_exponent->reset();
    HighPx_mantissa->reset();
    // decimal fields LowPx
    LowPx_exponent->reset();
    LowPx_mantissa->reset();
    Symbol->reset();
    SecurityTradingStatus->reset();
    HaltReason->reset();
    SecurityTradingEvent->reset();
  }

  // Destructor
  virtual ~MDSecurityStatus_102() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete SecurityID;
    delete SecurityIDSource;
    delete TradeDate;
    // decimal fields HighPx
    delete HighPx_exponent;
    delete HighPx_mantissa;
    // decimal fields LowPx
    delete LowPx_exponent;
    delete LowPx_mantissa;
    delete Symbol;
    delete SecurityTradingStatus;
    delete HaltReason;
    delete SecurityTradingEvent;
  }
};
class MDSecurityStatus_99 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *SecurityID;
  ConstantFieldOptional<uint32_t> *SecurityIDSource;
  NoOpField<uint32_t> *TradeDate;
  // decimal fields HighPx
  DefaultField<int32_t> *HighPx_exponent;
  NoOpField<int64_t> *HighPx_mantissa;
  // decimal fields LowPx
  DefaultField<int32_t> *LowPx_exponent;
  NoOpField<int64_t> *LowPx_mantissa;
  NoOpField<FFUtils::ByteArr> *Symbol;
  NoOpField<uint32_t> *SecurityTradingStatus;
  NoOpField<uint32_t> *HaltReason;
  NoOpField<uint32_t> *SecurityTradingEvent;

  // Constructor
  MDSecurityStatus_99() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("f"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new NoOpField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldOptional<uint32_t>(true, 8);
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // decimal fields HighPx
    HighPx_exponent = new DefaultField<int32_t>(false, true, -2);
    HighPx_mantissa = new NoOpField<int64_t>(true, false, 0);
    // decimal fields LowPx
    LowPx_exponent = new DefaultField<int32_t>(false, true, -2);
    LowPx_mantissa = new NoOpField<int64_t>(true, false, 0);
    Symbol = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new NoOpField<uint32_t>(false, false, 0);
    HaltReason = new NoOpField<uint32_t>(false, false, 0);
    SecurityTradingEvent = new NoOpField<uint32_t>(false, false, 0);
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    SecurityID->decode(input);
    SecurityIDSource->decode(pmap0);
    TradeDate->decode(input);
    // decimal fields HighPx
    HighPx_exponent->decode(input, pmap0);
    if (HighPx_exponent->isMandatory || HighPx_exponent->isValueSet) HighPx_mantissa->decode(input);
    // decimal fields LowPx
    LowPx_exponent->decode(input, pmap0);
    if (LowPx_exponent->isMandatory || LowPx_exponent->isValueSet) LowPx_mantissa->decode(input);
    Symbol->decode(input);
    SecurityTradingStatus->decode(input);
    HaltReason->decode(input);
    SecurityTradingEvent->decode(input);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    TradeDate->reset();
    // decimal fields HighPx
    HighPx_exponent->reset();
    HighPx_mantissa->reset();
    // decimal fields LowPx
    LowPx_exponent->reset();
    LowPx_mantissa->reset();
    Symbol->reset();
    SecurityTradingStatus->reset();
    HaltReason->reset();
    SecurityTradingEvent->reset();
  }

  // Destructor
  virtual ~MDSecurityStatus_99() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete SecurityID;
    delete SecurityIDSource;
    delete TradeDate;
    // decimal fields HighPx
    delete HighPx_exponent;
    delete HighPx_mantissa;
    // decimal fields LowPx
    delete LowPx_exponent;
    delete LowPx_mantissa;
    delete Symbol;
    delete SecurityTradingStatus;
    delete HaltReason;
    delete SecurityTradingEvent;
  }
};
class MDSecurityStatus_76 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *SecurityID;
  ConstantFieldOptional<uint32_t> *SecurityIDSource;
  NoOpField<uint32_t> *TradeDate;
  // decimal fields HighPx
  DefaultField<int32_t> *HighPx_exponent;
  NoOpField<int64_t> *HighPx_mantissa;
  // decimal fields LowPx
  DefaultField<int32_t> *LowPx_exponent;
  NoOpField<int64_t> *LowPx_mantissa;
  NoOpField<FFUtils::ByteArr> *Symbol;
  NoOpField<uint32_t> *SecurityTradingStatus;
  NoOpField<uint32_t> *HaltReason;
  NoOpField<uint32_t> *SecurityTradingEvent;

  // Constructor
  MDSecurityStatus_76() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("f"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new NoOpField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldOptional<uint32_t>(true, 8);
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    // decimal fields HighPx
    HighPx_exponent = new DefaultField<int32_t>(false, true, -2);
    HighPx_mantissa = new NoOpField<int64_t>(true, false, 0);
    // decimal fields LowPx
    LowPx_exponent = new DefaultField<int32_t>(false, true, -2);
    LowPx_mantissa = new NoOpField<int64_t>(true, false, 0);
    Symbol = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new NoOpField<uint32_t>(false, false, 0);
    HaltReason = new NoOpField<uint32_t>(false, false, 0);
    SecurityTradingEvent = new NoOpField<uint32_t>(false, false, 0);
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    SecurityID->decode(input);
    SecurityIDSource->decode(pmap0);
    TradeDate->decode(input);
    // decimal fields HighPx
    HighPx_exponent->decode(input, pmap0);
    if (HighPx_exponent->isMandatory || HighPx_exponent->isValueSet) HighPx_mantissa->decode(input);
    // decimal fields LowPx
    LowPx_exponent->decode(input, pmap0);
    if (LowPx_exponent->isMandatory || LowPx_exponent->isValueSet) LowPx_mantissa->decode(input);
    Symbol->decode(input);
    SecurityTradingStatus->decode(input);
    HaltReason->decode(input);
    SecurityTradingEvent->decode(input);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    TradeDate->reset();
    // decimal fields HighPx
    HighPx_exponent->reset();
    HighPx_mantissa->reset();
    // decimal fields LowPx
    LowPx_exponent->reset();
    LowPx_mantissa->reset();
    Symbol->reset();
    SecurityTradingStatus->reset();
    HaltReason->reset();
    SecurityTradingEvent->reset();
  }

  // Destructor
  virtual ~MDSecurityStatus_76() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete SecurityID;
    delete SecurityIDSource;
    delete TradeDate;
    // decimal fields HighPx
    delete HighPx_exponent;
    delete HighPx_mantissa;
    // decimal fields LowPx
    delete LowPx_exponent;
    delete LowPx_mantissa;
    delete Symbol;
    delete SecurityTradingStatus;
    delete HaltReason;
    delete SecurityTradingEvent;
  }
};
class MDSecurityStatus : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint32_t> *SecurityID;
  ConstantFieldOptional<uint32_t> *SecurityIDSource;
  // decimal fields HighPx
  DefaultField<int32_t> *HighPx_exponent;
  NoOpField<int64_t> *HighPx_mantissa;
  // decimal fields LowPx
  DefaultField<int32_t> *LowPx_exponent;
  NoOpField<int64_t> *LowPx_mantissa;
  NoOpField<FFUtils::ByteArr> *Symbol;
  NoOpField<uint32_t> *SecurityTradingStatus;

  // Constructor
  MDSecurityStatus() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("f"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new NoOpField<uint32_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldOptional<uint32_t>(true, 8);
    // decimal fields HighPx
    HighPx_exponent = new DefaultField<int32_t>(false, true, -2);
    HighPx_mantissa = new NoOpField<int64_t>(true, false, 0);
    // decimal fields LowPx
    LowPx_exponent = new DefaultField<int32_t>(false, true, -2);
    LowPx_mantissa = new NoOpField<int64_t>(true, false, 0);
    Symbol = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new NoOpField<uint32_t>(false, false, 0);
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
    SecurityID->decode(input);
    SecurityIDSource->decode(pmap0);
    // decimal fields HighPx
    HighPx_exponent->decode(input, pmap0);
    if (HighPx_exponent->isMandatory || HighPx_exponent->isValueSet) HighPx_mantissa->decode(input);
    // decimal fields LowPx
    LowPx_exponent->decode(input, pmap0);
    if (LowPx_exponent->isMandatory || LowPx_exponent->isValueSet) LowPx_mantissa->decode(input);
    Symbol->decode(input);
    SecurityTradingStatus->decode(input);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    // decimal fields HighPx
    HighPx_exponent->reset();
    HighPx_mantissa->reset();
    // decimal fields LowPx
    LowPx_exponent->reset();
    LowPx_mantissa->reset();
    Symbol->reset();
    SecurityTradingStatus->reset();
  }

  // Destructor
  virtual ~MDSecurityStatus() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
    delete SecurityID;
    delete SecurityIDSource;
    // decimal fields HighPx
    delete HighPx_exponent;
    delete HighPx_mantissa;
    // decimal fields LowPx
    delete LowPx_exponent;
    delete LowPx_mantissa;
    delete Symbol;
    delete SecurityTradingStatus;
  }
};
class MDSnapshotFullRefresh_127 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *LastMsgSeqNumProcessed;
  NoOpField<uint32_t> *TotNumReports;
  NoOpField<uint32_t> *TradeDate;
  CopyField<FFUtils::ByteArr> *MDReqID;
  NoOpField<int32_t> *MarketDepth;
  NoOpField<uint32_t> *RptSeq;
  NoOpField<uint64_t> *SecurityID;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  DefaultField<FFUtils::ByteArr> *MDEntryType;
  DefaultField<FFUtils::ByteArr> *Curerncy;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int64_t> *MDEntrySize;
  CopyField<uint32_t> *MDInsertDate;
  CopyField<uint32_t> *MDInsertTime;
  DeltaField<uint32_t> *TradeVolume;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<FFUtils::ByteArr> *MDEntryTime;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  DefaultField<FFUtils::ByteArr> *PriceType;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  DefaultField<uint32_t> *SecurityTradingStatus;
  DefaultField<uint32_t> *TradSesOpenTime;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *OpenCloseSettleFlag;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  DefaultField<FFUtils::ByteArr> *MDEntryBuyer;
  DefaultField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  CopyField<uint32_t> *NumberOfOrders;
  DefaultField<uint32_t> *MDEntryPositionNo;
  DefaultField<uint32_t> *DayCumQty;
  DefaultField<uint32_t> *SellerDays;
  DefaultField<uint32_t> *SettlePriceType;
  DefaultField<uint32_t> *PriceLimitType;
  // decimal fields LowLimitPrice
  DefaultField<int32_t> *LowLimitPrice_exponent;
  DeltaField<int64_t> *LowLimitPrice_mantissa;
  // decimal fields HighLimitPrice
  DefaultField<int32_t> *HighLimitPrice_exponent;
  DeltaField<int64_t> *HighLimitPrice_mantissa;
  // decimal fields TradingRefPrice
  DefaultField<int32_t> *TradingRefPrice_exponent;
  DeltaField<int64_t> *TradingRefPrice_mantissa;
  // fields inside sequence MDEntries ends

  // Constructor
  MDSnapshotFullRefresh_127() {
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    LastMsgSeqNumProcessed = new NoOpField<uint32_t>(true, false, 0);
    TotNumReports = new NoOpField<uint32_t>(false, false, 0);
    TradeDate = new NoOpField<uint32_t>(false, false, 0);
    MDReqID = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MarketDepth = new NoOpField<int32_t>(false, false, 0);
    RptSeq = new NoOpField<uint32_t>(true, false, 0);
    SecurityID = new NoOpField<uint64_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDEntryType = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    Curerncy = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int64_t>(false, false, 0);
    MDInsertDate = new CopyField<uint32_t>(false, false, 0);
    MDInsertTime = new CopyField<uint32_t>(false, false, 0);
    TradeVolume = new DeltaField<uint32_t>(false, false, 0);
    MDEntryDate = new CopyField<uint32_t>(false, false, 0);
    MDEntryTime = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("E"));
    PriceType = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new DefaultField<uint32_t>(false, false, 0);
    TradSesOpenTime = new DefaultField<uint32_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettleFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new CopyField<uint32_t>(false, false, 0);
    MDEntryPositionNo = new DefaultField<uint32_t>(false, false, 0);
    DayCumQty = new DefaultField<uint32_t>(false, false, 0);
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    SettlePriceType = new DefaultField<uint32_t>(false, false, 0);
    PriceLimitType = new DefaultField<uint32_t>(false, false, 0);
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    LowLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    HighLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields TradingRefPrice
    TradingRefPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    TradingRefPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MessageType->decode();
    MsgSeqNum->decode(input);
    ApplVerID->decode();
    SendingTime->decode(input);
    LastMsgSeqNumProcessed->decode(input);
    TotNumReports->decode(input);
    TradeDate->decode(input);
    MDReqID->decode(input, pmap0);
    MarketDepth->decode(input);
    RptSeq->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode();
    SecurityExchange->decode();
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDEntryType->decode(input, pmap1);
      Curerncy->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      MDInsertDate->decode(input, pmap1);
      MDInsertTime->decode(input, pmap1);
      TradeVolume->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      MDStreamID->decode(input, pmap1);
      PriceType->decode(input, pmap1);
      TradingSessionSubID->decode(input);
      SecurityTradingStatus->decode(input, pmap1);
      TradSesOpenTime->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input, pmap1);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      DayCumQty->decode(input, pmap1);
      SellerDays->decode(input, pmap1);
      SettlePriceType->decode(input, pmap1);
      PriceLimitType->decode(input, pmap1);
      // decimal fields LowLimitPrice
      LowLimitPrice_exponent->decode(input, pmap1);
      if (LowLimitPrice_exponent->isMandatory || LowLimitPrice_exponent->isValueSet)
        LowLimitPrice_mantissa->decode(input);
      // decimal fields HighLimitPrice
      HighLimitPrice_exponent->decode(input, pmap1);
      if (HighLimitPrice_exponent->isMandatory || HighLimitPrice_exponent->isValueSet)
        HighLimitPrice_mantissa->decode(input);
      // decimal fields TradingRefPrice
      TradingRefPrice_exponent->decode(input, pmap1);
      if (TradingRefPrice_exponent->isMandatory || TradingRefPrice_exponent->isValueSet)
        TradingRefPrice_mantissa->decode(input);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MessageType->reset();
    MsgSeqNum->reset();
    ApplVerID->reset();
    SendingTime->reset();
    LastMsgSeqNumProcessed->reset();
    TotNumReports->reset();
    TradeDate->reset();
    MDReqID->reset();
    MarketDepth->reset();
    RptSeq->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDEntryType->reset();
    Curerncy->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    MDInsertDate->reset();
    MDInsertTime->reset();
    TradeVolume->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    TickDirection->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    MDStreamID->reset();
    PriceType->reset();
    TradingSessionSubID->reset();
    SecurityTradingStatus->reset();
    TradSesOpenTime->reset();
    TradeCondition->reset();
    OpenCloseSettleFlag->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    QuoteCondition->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    DayCumQty->reset();
    SellerDays->reset();
    SettlePriceType->reset();
    PriceLimitType->reset();
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent->reset();
    LowLimitPrice_mantissa->reset();
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent->reset();
    HighLimitPrice_mantissa->reset();
    // decimal fields TradingRefPrice
    TradingRefPrice_exponent->reset();
    TradingRefPrice_mantissa->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDSnapshotFullRefresh_127() {
    delete MessageType;
    delete MsgSeqNum;
    delete ApplVerID;
    delete SendingTime;
    delete LastMsgSeqNumProcessed;
    delete TotNumReports;
    delete TradeDate;
    delete MDReqID;
    delete MarketDepth;
    delete RptSeq;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDEntryType;
    delete Curerncy;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete MDInsertDate;
    delete MDInsertTime;
    delete TradeVolume;
    delete MDEntryDate;
    delete MDEntryTime;
    delete TickDirection;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete MDStreamID;
    delete PriceType;
    delete TradingSessionSubID;
    delete SecurityTradingStatus;
    delete TradSesOpenTime;
    delete TradeCondition;
    delete OpenCloseSettleFlag;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete QuoteCondition;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete DayCumQty;
    delete SellerDays;
    delete SettlePriceType;
    delete PriceLimitType;
    // decimal fields LowLimitPrice
    delete LowLimitPrice_exponent;
    delete LowLimitPrice_mantissa;
    // decimal fields HighLimitPrice
    delete HighLimitPrice_exponent;
    delete HighLimitPrice_mantissa;
    // decimal fields TradingRefPrice
    delete TradingRefPrice_exponent;
    delete TradingRefPrice_mantissa;
    // destructor for sequence MDEntries ends
  }
};
class MDSnapshotFullRefresh_124 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *LastMsgSeqNumProcessed;
  NoOpField<uint32_t> *TotNumReports;
  NoOpField<uint32_t> *TradeDate;
  CopyField<FFUtils::ByteArr> *MDReqID;
  NoOpField<int32_t> *MarketDepth;
  NoOpField<uint32_t> *RptSeq;
  NoOpField<uint64_t> *SecurityID;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  DefaultField<FFUtils::ByteArr> *MDEntryType;
  DefaultField<FFUtils::ByteArr> *Curerncy;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  CopyField<uint32_t> *MDInsertDate;
  CopyField<uint32_t> *MDInsertTime;
  DeltaField<uint32_t> *TradeVolume;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<FFUtils::ByteArr> *MDEntryTime;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  DefaultField<FFUtils::ByteArr> *PriceType;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  DefaultField<uint32_t> *SecurityTradingStatus;
  DefaultField<uint32_t> *TradSesOpenTime;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *OpenCloseSettleFlag;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  DefaultField<FFUtils::ByteArr> *MDEntryBuyer;
  DefaultField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  CopyField<uint32_t> *NumberOfOrders;
  DefaultField<uint32_t> *MDEntryPositionNo;
  DefaultField<uint32_t> *DayCumQty;
  DefaultField<uint32_t> *SellerDays;
  DefaultField<uint32_t> *SettlePriceType;
  DefaultField<uint32_t> *PriceLimitType;
  // decimal fields LowLimitPrice
  DefaultField<int32_t> *LowLimitPrice_exponent;
  DeltaField<int64_t> *LowLimitPrice_mantissa;
  // decimal fields HighLimitPrice
  DefaultField<int32_t> *HighLimitPrice_exponent;
  DeltaField<int64_t> *HighLimitPrice_mantissa;
  // decimal fields TradingRefPrice
  DefaultField<int32_t> *TradingRefPrice_exponent;
  DeltaField<int64_t> *TradingRefPrice_mantissa;
  // fields inside sequence MDEntries ends

  // Constructor
  MDSnapshotFullRefresh_124() {
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    LastMsgSeqNumProcessed = new NoOpField<uint32_t>(true, false, 0);
    TotNumReports = new NoOpField<uint32_t>(false, false, 0);
    TradeDate = new NoOpField<uint32_t>(false, false, 0);
    MDReqID = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MarketDepth = new NoOpField<int32_t>(false, false, 0);
    RptSeq = new NoOpField<uint32_t>(true, false, 0);
    SecurityID = new NoOpField<uint64_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDEntryType = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    Curerncy = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    MDInsertDate = new CopyField<uint32_t>(false, false, 0);
    MDInsertTime = new CopyField<uint32_t>(false, false, 0);
    TradeVolume = new DeltaField<uint32_t>(false, false, 0);
    MDEntryDate = new CopyField<uint32_t>(false, false, 0);
    MDEntryTime = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("E"));
    PriceType = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new DefaultField<uint32_t>(false, false, 0);
    TradSesOpenTime = new DefaultField<uint32_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettleFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new CopyField<uint32_t>(false, false, 0);
    MDEntryPositionNo = new DefaultField<uint32_t>(false, false, 0);
    DayCumQty = new DefaultField<uint32_t>(false, false, 0);
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    SettlePriceType = new DefaultField<uint32_t>(false, false, 0);
    PriceLimitType = new DefaultField<uint32_t>(false, false, 0);
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    LowLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    HighLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields TradingRefPrice
    TradingRefPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    TradingRefPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MessageType->decode();
    MsgSeqNum->decode(input);
    ApplVerID->decode();
    SendingTime->decode(input);
    LastMsgSeqNumProcessed->decode(input);
    TotNumReports->decode(input);
    TradeDate->decode(input);
    MDReqID->decode(input, pmap0);
    MarketDepth->decode(input);
    RptSeq->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode();
    SecurityExchange->decode();
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    // to be implemented in corrosponding cpp file
    bool sec_found_ = findSecurity();

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();

    if (sec_found_ && NoMDEntries_len > 0) {
      process_start();
    }

    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDEntryType->decode(input, pmap1);
      Curerncy->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      MDInsertDate->decode(input, pmap1);
      MDInsertTime->decode(input, pmap1);
      TradeVolume->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      MDStreamID->decode(input, pmap1);
      PriceType->decode(input, pmap1);
      TradingSessionSubID->decode(input);
      SecurityTradingStatus->decode(input, pmap1);
      TradSesOpenTime->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input, pmap1);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      DayCumQty->decode(input, pmap1);
      SellerDays->decode(input, pmap1);
      SettlePriceType->decode(input, pmap1);
      PriceLimitType->decode(input, pmap1);
      // decimal fields LowLimitPrice
      LowLimitPrice_exponent->decode(input, pmap1);
      if (LowLimitPrice_exponent->isMandatory || LowLimitPrice_exponent->isValueSet)
        LowLimitPrice_mantissa->decode(input);
      // decimal fields HighLimitPrice
      HighLimitPrice_exponent->decode(input, pmap1);
      if (HighLimitPrice_exponent->isMandatory || HighLimitPrice_exponent->isValueSet)
        HighLimitPrice_mantissa->decode(input);
      // decimal fields TradingRefPrice
      TradingRefPrice_exponent->decode(input, pmap1);
      if (TradingRefPrice_exponent->isMandatory || TradingRefPrice_exponent->isValueSet)
        TradingRefPrice_mantissa->decode(input);

      // will eventually process MdMsg
      if (sec_found_) process();
    }

    // decode for sequence MDEntries ends

    // signifies end of sequences should process flush based stuff
    if (sec_found_) process_end();
  }

  // process : TO be done in corresponding cpp file manually
  void process_start();
  void process();
  void process_end();
  bool findSecurity();

  // Reset
  void reset() {
    MessageType->reset();
    MsgSeqNum->reset();
    ApplVerID->reset();
    SendingTime->reset();
    LastMsgSeqNumProcessed->reset();
    TotNumReports->reset();
    TradeDate->reset();
    MDReqID->reset();
    MarketDepth->reset();
    RptSeq->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDEntryType->reset();
    Curerncy->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    MDInsertDate->reset();
    MDInsertTime->reset();
    TradeVolume->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    TickDirection->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    MDStreamID->reset();
    PriceType->reset();
    TradingSessionSubID->reset();
    SecurityTradingStatus->reset();
    TradSesOpenTime->reset();
    TradeCondition->reset();
    OpenCloseSettleFlag->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    QuoteCondition->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    DayCumQty->reset();
    SellerDays->reset();
    SettlePriceType->reset();
    PriceLimitType->reset();
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent->reset();
    LowLimitPrice_mantissa->reset();
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent->reset();
    HighLimitPrice_mantissa->reset();
    // decimal fields TradingRefPrice
    TradingRefPrice_exponent->reset();
    TradingRefPrice_mantissa->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDSnapshotFullRefresh_124() {
    delete MessageType;
    delete MsgSeqNum;
    delete ApplVerID;
    delete SendingTime;
    delete LastMsgSeqNumProcessed;
    delete TotNumReports;
    delete TradeDate;
    delete MDReqID;
    delete MarketDepth;
    delete RptSeq;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDEntryType;
    delete Curerncy;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete MDInsertDate;
    delete MDInsertTime;
    delete TradeVolume;
    delete MDEntryDate;
    delete MDEntryTime;
    delete TickDirection;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete MDStreamID;
    delete PriceType;
    delete TradingSessionSubID;
    delete SecurityTradingStatus;
    delete TradSesOpenTime;
    delete TradeCondition;
    delete OpenCloseSettleFlag;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete QuoteCondition;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete DayCumQty;
    delete SellerDays;
    delete SettlePriceType;
    delete PriceLimitType;
    // decimal fields LowLimitPrice
    delete LowLimitPrice_exponent;
    delete LowLimitPrice_mantissa;
    // decimal fields HighLimitPrice
    delete HighLimitPrice_exponent;
    delete HighLimitPrice_mantissa;
    // decimal fields TradingRefPrice
    delete TradingRefPrice_exponent;
    delete TradingRefPrice_mantissa;
    // destructor for sequence MDEntries ends
  }
};
class MDSnapshotFullRefresh_112 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *LastMsgSeqNumProcessed;
  NoOpField<uint32_t> *TotNumReports;
  NoOpField<uint32_t> *TradeDate;
  CopyField<FFUtils::ByteArr> *MDReqID;
  NoOpField<int32_t> *MarketDepth;
  NoOpField<uint32_t> *RptSeq;
  NoOpField<uint64_t> *SecurityID;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  DefaultField<FFUtils::ByteArr> *MDEntryType;
  DefaultField<FFUtils::ByteArr> *Curerncy;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  DeltaField<uint32_t> *TradeVolume;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<FFUtils::ByteArr> *MDEntryTime;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  DefaultField<FFUtils::ByteArr> *PriceType;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  DefaultField<uint32_t> *SecurityTradingStatus;
  DefaultField<uint32_t> *TradSesOpenTime;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *OpenCloseSettleFlag;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  DefaultField<FFUtils::ByteArr> *MDEntryBuyer;
  DefaultField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  CopyField<uint32_t> *NumberOfOrders;
  DefaultField<uint32_t> *MDEntryPositionNo;
  DefaultField<uint32_t> *DayCumQty;
  DefaultField<uint32_t> *SellerDays;
  DefaultField<uint32_t> *SettlePriceType;
  DefaultField<uint32_t> *PriceLimitType;
  // decimal fields LowLimitPrice
  DefaultField<int32_t> *LowLimitPrice_exponent;
  DeltaField<int64_t> *LowLimitPrice_mantissa;
  // decimal fields HighLimitPrice
  DefaultField<int32_t> *HighLimitPrice_exponent;
  DeltaField<int64_t> *HighLimitPrice_mantissa;
  // decimal fields TradingRefPrice
  DefaultField<int32_t> *TradingRefPrice_exponent;
  DeltaField<int64_t> *TradingRefPrice_mantissa;
  // fields inside sequence MDEntries ends

  // Constructor
  MDSnapshotFullRefresh_112() {
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    LastMsgSeqNumProcessed = new NoOpField<uint32_t>(true, false, 0);
    TotNumReports = new NoOpField<uint32_t>(false, false, 0);
    TradeDate = new NoOpField<uint32_t>(false, false, 0);
    MDReqID = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MarketDepth = new NoOpField<int32_t>(false, false, 0);
    RptSeq = new NoOpField<uint32_t>(true, false, 0);
    SecurityID = new NoOpField<uint64_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDEntryType = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    Curerncy = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    TradeVolume = new DeltaField<uint32_t>(false, false, 0);
    MDEntryDate = new CopyField<uint32_t>(false, false, 0);
    MDEntryTime = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("E"));
    PriceType = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new DefaultField<uint32_t>(false, false, 0);
    TradSesOpenTime = new DefaultField<uint32_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettleFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new CopyField<uint32_t>(false, false, 0);
    MDEntryPositionNo = new DefaultField<uint32_t>(false, false, 0);
    DayCumQty = new DefaultField<uint32_t>(false, false, 0);
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    SettlePriceType = new DefaultField<uint32_t>(false, false, 0);
    PriceLimitType = new DefaultField<uint32_t>(false, false, 0);
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    LowLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    HighLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields TradingRefPrice
    TradingRefPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    TradingRefPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MessageType->decode();
    MsgSeqNum->decode(input);
    ApplVerID->decode();
    PosDupFlag->decode(input, pmap0);
    SendingTime->decode(input);
    LastMsgSeqNumProcessed->decode(input);
    TotNumReports->decode(input);
    TradeDate->decode(input);
    MDReqID->decode(input, pmap0);
    MarketDepth->decode(input);
    RptSeq->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode();
    SecurityExchange->decode();
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDEntryType->decode(input, pmap1);
      Curerncy->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      TradeVolume->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      MDStreamID->decode(input, pmap1);
      PriceType->decode(input, pmap1);
      TradingSessionSubID->decode(input);
      SecurityTradingStatus->decode(input, pmap1);
      TradSesOpenTime->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettleFlag->decode(input, pmap1);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      DayCumQty->decode(input, pmap1);
      SellerDays->decode(input, pmap1);
      SettlePriceType->decode(input, pmap1);
      PriceLimitType->decode(input, pmap1);
      // decimal fields LowLimitPrice
      LowLimitPrice_exponent->decode(input, pmap1);
      if (LowLimitPrice_exponent->isMandatory || LowLimitPrice_exponent->isValueSet)
        LowLimitPrice_mantissa->decode(input);
      // decimal fields HighLimitPrice
      HighLimitPrice_exponent->decode(input, pmap1);
      if (HighLimitPrice_exponent->isMandatory || HighLimitPrice_exponent->isValueSet)
        HighLimitPrice_mantissa->decode(input);
      // decimal fields TradingRefPrice
      TradingRefPrice_exponent->decode(input, pmap1);
      if (TradingRefPrice_exponent->isMandatory || TradingRefPrice_exponent->isValueSet)
        TradingRefPrice_mantissa->decode(input);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MessageType->reset();
    MsgSeqNum->reset();
    ApplVerID->reset();
    PosDupFlag->reset();
    SendingTime->reset();
    LastMsgSeqNumProcessed->reset();
    TotNumReports->reset();
    TradeDate->reset();
    MDReqID->reset();
    MarketDepth->reset();
    RptSeq->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDEntryType->reset();
    Curerncy->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    TradeVolume->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    TickDirection->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    MDStreamID->reset();
    PriceType->reset();
    TradingSessionSubID->reset();
    SecurityTradingStatus->reset();
    TradSesOpenTime->reset();
    TradeCondition->reset();
    OpenCloseSettleFlag->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    QuoteCondition->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    DayCumQty->reset();
    SellerDays->reset();
    SettlePriceType->reset();
    PriceLimitType->reset();
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent->reset();
    LowLimitPrice_mantissa->reset();
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent->reset();
    HighLimitPrice_mantissa->reset();
    // decimal fields TradingRefPrice
    TradingRefPrice_exponent->reset();
    TradingRefPrice_mantissa->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDSnapshotFullRefresh_112() {
    delete MessageType;
    delete MsgSeqNum;
    delete ApplVerID;
    delete PosDupFlag;
    delete SendingTime;
    delete LastMsgSeqNumProcessed;
    delete TotNumReports;
    delete TradeDate;
    delete MDReqID;
    delete MarketDepth;
    delete RptSeq;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDEntryType;
    delete Curerncy;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete TradeVolume;
    delete MDEntryDate;
    delete MDEntryTime;
    delete TickDirection;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete MDStreamID;
    delete PriceType;
    delete TradingSessionSubID;
    delete SecurityTradingStatus;
    delete TradSesOpenTime;
    delete TradeCondition;
    delete OpenCloseSettleFlag;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete QuoteCondition;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete DayCumQty;
    delete SellerDays;
    delete SettlePriceType;
    delete PriceLimitType;
    // decimal fields LowLimitPrice
    delete LowLimitPrice_exponent;
    delete LowLimitPrice_mantissa;
    // decimal fields HighLimitPrice
    delete HighLimitPrice_exponent;
    delete HighLimitPrice_mantissa;
    // decimal fields TradingRefPrice
    delete TradingRefPrice_exponent;
    delete TradingRefPrice_mantissa;
    // destructor for sequence MDEntries ends
  }
};
class MDSnapshotFullRefresh_80 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *LastMsgSeqNumProcessed;
  NoOpField<uint32_t> *TotNumReports;
  NoOpField<uint32_t> *RptSeq;
  NoOpField<uint32_t> *MDBookType;
  NoOpField<uint32_t> *SecurityID;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  NoOpField<uint32_t> *MDSecurityTradingStatus;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  DefaultField<FFUtils::ByteArr> *MDEntryType;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  CopyField<uint32_t> *MDPriceLevel;
  CopyField<uint32_t> *NumberOfOrders;
  DeltaField<uint32_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  // fields inside sequence MDEntries ends

  // Constructor
  MDSnapshotFullRefresh_80() {
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"));
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    LastMsgSeqNumProcessed = new NoOpField<uint32_t>(true, false, 0);
    TotNumReports = new NoOpField<uint32_t>(true, false, 0);
    RptSeq = new NoOpField<uint32_t>(true, false, 0);
    MDBookType = new NoOpField<uint32_t>(true, false, 0);
    SecurityID = new NoOpField<uint32_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    MDSecurityTradingStatus = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDEntryType = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("K"));
    MDPriceLevel = new CopyField<uint32_t>(false, true, 1);
    NumberOfOrders = new CopyField<uint32_t>(false, false, 0);
    TradeVolume = new DeltaField<uint32_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MessageType->decode();
    ApplVerID->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    PosDupFlag->decode(input, pmap0);
    SendingTime->decode(input);
    LastMsgSeqNumProcessed->decode(input);
    TotNumReports->decode(input);
    RptSeq->decode(input);
    MDBookType->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode();
    MDSecurityTradingStatus->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDEntryType->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      QuoteCondition->decode(input, pmap1);
      MDPriceLevel->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      TradeVolume->decode(input);
      TickDirection->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MessageType->reset();
    ApplVerID->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    PosDupFlag->reset();
    SendingTime->reset();
    LastMsgSeqNumProcessed->reset();
    TotNumReports->reset();
    RptSeq->reset();
    MDBookType->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    MDSecurityTradingStatus->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDEntryType->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    QuoteCondition->reset();
    MDPriceLevel->reset();
    NumberOfOrders->reset();
    TradeVolume->reset();
    TickDirection->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDSnapshotFullRefresh_80() {
    delete MessageType;
    delete ApplVerID;
    delete SenderCompID;
    delete MsgSeqNum;
    delete PosDupFlag;
    delete SendingTime;
    delete LastMsgSeqNumProcessed;
    delete TotNumReports;
    delete RptSeq;
    delete MDBookType;
    delete SecurityID;
    delete SecurityIDSource;
    delete MDSecurityTradingStatus;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDEntryType;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete QuoteCondition;
    delete MDPriceLevel;
    delete NumberOfOrders;
    delete TradeVolume;
    delete TickDirection;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    // destructor for sequence MDEntries ends
  }
};
class MDSnapshotFullRefresh_53 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *LastMsgSeqNumProcessed;
  NoOpField<uint32_t> *TotNumReports;
  NoOpField<uint32_t> *RptSeq;
  NoOpField<uint32_t> *MDBookType;
  DeltaField<uint32_t> *SecurityID;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  DefaultField<FFUtils::ByteArr> *MDEntryType;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  DeltaField<int32_t> *MDEntrySize;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  ConstantFieldOptional<FFUtils::ByteArr> *TradeCondition;
  CopyField<uint32_t> *MDPriceLevel;
  CopyField<uint32_t> *NumberOfOrders;
  DefaultField<FFUtils::ByteArr> *TradingSessionID;
  DeltaField<uint32_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  // fields inside sequence MDEntries ends

  // Constructor
  MDSnapshotFullRefresh_53() {
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"));
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    LastMsgSeqNumProcessed = new NoOpField<uint32_t>(true, false, 0);
    TotNumReports = new NoOpField<uint32_t>(true, false, 0);
    RptSeq = new NoOpField<uint32_t>(true, false, 0);
    MDBookType = new NoOpField<uint32_t>(true, false, 0);
    SecurityID = new DeltaField<uint32_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDEntryType = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("2"));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("K"));
    TradeCondition = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("U"));
    MDPriceLevel = new CopyField<uint32_t>(false, true, 1);
    NumberOfOrders = new CopyField<uint32_t>(false, false, 0);
    TradingSessionID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    TradeVolume = new DeltaField<uint32_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MessageType->decode();
    ApplVerID->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    PosDupFlag->decode(input, pmap0);
    SendingTime->decode(input);
    LastMsgSeqNumProcessed->decode(input);
    TotNumReports->decode(input);
    RptSeq->decode(input);
    MDBookType->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode();
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDEntryType->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      MDEntrySize->decode(input);
      QuoteCondition->decode(input, pmap1);
      TradeCondition->decode(pmap1);
      MDPriceLevel->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      TradingSessionID->decode(input, pmap1);
      TradeVolume->decode(input);
      TickDirection->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MessageType->reset();
    ApplVerID->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    PosDupFlag->reset();
    SendingTime->reset();
    LastMsgSeqNumProcessed->reset();
    TotNumReports->reset();
    RptSeq->reset();
    MDBookType->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDEntryType->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    MDEntrySize->reset();
    QuoteCondition->reset();
    TradeCondition->reset();
    MDPriceLevel->reset();
    NumberOfOrders->reset();
    TradingSessionID->reset();
    TradeVolume->reset();
    TickDirection->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MDSnapshotFullRefresh_53() {
    delete MessageType;
    delete ApplVerID;
    delete SenderCompID;
    delete MsgSeqNum;
    delete PosDupFlag;
    delete SendingTime;
    delete LastMsgSeqNumProcessed;
    delete TotNumReports;
    delete RptSeq;
    delete MDBookType;
    delete SecurityID;
    delete SecurityIDSource;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDEntryType;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    delete MDEntrySize;
    delete QuoteCondition;
    delete TradeCondition;
    delete MDPriceLevel;
    delete NumberOfOrders;
    delete TradingSessionID;
    delete TradeVolume;
    delete TickDirection;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    // destructor for sequence MDEntries ends
  }
};
class MDNewsMessage_120 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint64_t> *OrigTime;
  CopyField<FFUtils::ByteArr> *NewsSource;
  CopyField<FFUtils::ByteArr> *LanguageCode;
  CopyField<FFUtils::ByteArr> *Headline;
  CopyField<FFUtils::ByteArr> *URLLink;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t> *NoRelatedSym;
  CopyField<uint64_t> *SecurityID;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  NoOpField<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence RelatedSym ends
  // fields inside sequence RoutingIds
  NoOpField<uint32_t> *NoRoutingIDs;
  DefaultField<uint32_t> *RoutingType;
  NoOpField<FFUtils::ByteArr> *RoutingID;
  // fields inside sequence RoutingIds ends
  // fields inside sequence LinesOfText
  NoOpField<uint32_t> *NoLinesOfText;
  CopyField<FFUtils::ByteArr> *text;
  NoOpField<uint32_t> *EncodedTextLen;
  NoOpField<FFUtils::ByteArr> *EncodedText;
  // fields inside sequence LinesOfText ends

  // Constructor
  MDNewsMessage_120() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("B"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    OrigTime = new NoOpField<uint64_t>(false, false, 0);
    NewsSource = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LanguageCode = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Headline = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    URLLink = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t>(false, false, 0);
    SecurityID = new CopyField<uint64_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityExchange = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence RelatedSym ends
    // constructor for sequence RoutingIds
    NoRoutingIDs = new NoOpField<uint32_t>(false, false, 0);
    RoutingType = new DefaultField<uint32_t>(false, true, 2);
    RoutingID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence RoutingIds ends
    // constructor for sequence LinesOfText
    NoLinesOfText = new NoOpField<uint32_t>(false, false, 0);
    text = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    EncodedTextLen = new NoOpField<uint32_t>(false, false, 0);
    EncodedText = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence LinesOfText ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    OrigTime->decode(input);
    NewsSource->decode(input, pmap0);
    LanguageCode->decode(input, pmap0);
    Headline->decode(input, pmap0);
    URLLink->decode(input, pmap0);
    // decode for sequence RelatedSym
    NoRelatedSym->decode(input);

    int NoRelatedSym_len = NoRelatedSym->previousValue.getValue();
    if (NoRelatedSym->previousValue.isAssigned()) {
      for (int i = 0; i < NoRelatedSym_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap1 = input.extractPmap();

        SecurityID->decode(input, pmap1);
        SecurityIDSource->decode();
        SecurityExchange->decode(input);
      }
    }
    // decode for sequence RelatedSym ends
    // decode for sequence RoutingIds
    NoRoutingIDs->decode(input);

    int NoRoutingIDs_len = NoRoutingIDs->previousValue.getValue();
    if (NoRoutingIDs->previousValue.isAssigned()) {
      for (int i = 0; i < NoRoutingIDs_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap1 = input.extractPmap();

        RoutingType->decode(input, pmap1);
        RoutingID->decode(input);
      }
    }
    // decode for sequence RoutingIds ends
    // decode for sequence LinesOfText
    NoLinesOfText->decode(input);

    int NoLinesOfText_len = NoLinesOfText->previousValue.getValue();
    if (NoLinesOfText->previousValue.isAssigned()) {
      for (int i = 0; i < NoLinesOfText_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap1 = input.extractPmap();

        text->decode(input, pmap1);
        EncodedTextLen->decode(input);
        EncodedText->decode(input);
      }
    }
    // decode for sequence LinesOfText ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    OrigTime->reset();
    NewsSource->reset();
    LanguageCode->reset();
    Headline->reset();
    URLLink->reset();
    // reset for sequence RelatedSym
    NoRelatedSym->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence RelatedSym ends
    // reset for sequence RoutingIds
    NoRoutingIDs->reset();
    RoutingType->reset();
    RoutingID->reset();
    // reset for sequence RoutingIds ends
    // reset for sequence LinesOfText
    NoLinesOfText->reset();
    text->reset();
    EncodedTextLen->reset();
    EncodedText->reset();
    // reset for sequence LinesOfText ends
  }

  // Destructor
  virtual ~MDNewsMessage_120() {
    delete ApplVerID;
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
    delete OrigTime;
    delete NewsSource;
    delete LanguageCode;
    delete Headline;
    delete URLLink;
    // destructor for sequence RelatedSym
    delete NoRelatedSym;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence RelatedSym ends
    // destructor for sequence RoutingIds
    delete NoRoutingIDs;
    delete RoutingType;
    delete RoutingID;
    // destructor for sequence RoutingIds ends
    // destructor for sequence LinesOfText
    delete NoLinesOfText;
    delete text;
    delete EncodedTextLen;
    delete EncodedText;
    // destructor for sequence LinesOfText ends
  }
};
class MDNewsMessage_100 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<FFUtils::ByteArr> *Headline;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  // fields inside sequence LinesOfText
  NoOpField<uint32_t> *NoLinesOfText;
  CopyField<FFUtils::ByteArr> *text;
  // fields inside sequence LinesOfText ends

  // Constructor
  MDNewsMessage_100() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("B"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    Headline = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence LinesOfText
    NoLinesOfText = new NoOpField<uint32_t>(true, false, 0);
    text = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence LinesOfText ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    Headline->decode(input);
    PosDupFlag->decode(input, pmap0);
    // decode for sequence LinesOfText
    NoLinesOfText->decode(input);

    int NoLinesOfText_len = NoLinesOfText->previousValue.getValue();
    for (int i = 0; i < NoLinesOfText_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      text->decode(input, pmap1);
    }
    // decode for sequence LinesOfText ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    Headline->reset();
    PosDupFlag->reset();
    // reset for sequence LinesOfText
    NoLinesOfText->reset();
    text->reset();
    // reset for sequence LinesOfText ends
  }

  // Destructor
  virtual ~MDNewsMessage_100() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete Headline;
    delete PosDupFlag;
    // destructor for sequence LinesOfText
    delete NoLinesOfText;
    delete text;
    // destructor for sequence LinesOfText ends
  }
};
class MDNewsMessage : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<FFUtils::ByteArr> *Headline;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;
  // fields inside sequence LinesOfText
  NoOpField<uint32_t> *NoLinesOfText;
  CopyField<FFUtils::ByteArr> *text;
  // fields inside sequence LinesOfText ends

  // Constructor
  MDNewsMessage() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("B"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    Headline = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence LinesOfText
    NoLinesOfText = new NoOpField<uint32_t>(true, false, 0);
    text = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence LinesOfText ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    Headline->decode(input);
    PosDupFlag->decode(input, pmap0);
    // decode for sequence LinesOfText
    NoLinesOfText->decode(input);

    int NoLinesOfText_len = NoLinesOfText->previousValue.getValue();
    for (int i = 0; i < NoLinesOfText_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      text->decode(input, pmap1);
    }
    // decode for sequence LinesOfText ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    Headline->reset();
    PosDupFlag->reset();
    // reset for sequence LinesOfText
    NoLinesOfText->reset();
    text->reset();
    // reset for sequence LinesOfText ends
  }

  // Destructor
  virtual ~MDNewsMessage() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete Headline;
    delete PosDupFlag;
    // destructor for sequence LinesOfText
    delete NoLinesOfText;
    delete text;
    // destructor for sequence LinesOfText ends
  }
};
class MDHeartbeat_101 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;

  // Constructor
  MDHeartbeat_101() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("0"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
  }

  // Destructor
  virtual ~MDHeartbeat_101() {
    delete ApplVerID;
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
  }
};
class MDHeartbeat : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  DefaultField<FFUtils::ByteArr> *PosDupFlag;

  // Constructor
  MDHeartbeat() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("0"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    PosDupFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    PosDupFlag->decode(input, pmap0);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    PosDupFlag->reset();
  }

  // Destructor
  virtual ~MDHeartbeat() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete PosDupFlag;
  }
};
class MDLogon_118 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplID;
  ConstantFieldMandatory<uint32_t> *EncryptMethod;
  NoOpField<uint32_t> *HeartbeatInt;
  ConstantFieldMandatory<FFUtils::ByteArr> *DefaultApplVerID;

  // Constructor
  MDLogon_118() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("A"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("REPLAY"));
    EncryptMethod = new ConstantFieldMandatory<uint32_t>(true, 0);
    HeartbeatInt = new NoOpField<uint32_t>(true, false, 0);
    DefaultApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplID->decode();
    EncryptMethod->decode();
    HeartbeatInt->decode(input);
    DefaultApplVerID->decode();
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplID->reset();
    EncryptMethod->reset();
    HeartbeatInt->reset();
    DefaultApplVerID->reset();
  }

  // Destructor
  virtual ~MDLogon_118() {
    delete ApplVerID;
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplID;
    delete EncryptMethod;
    delete HeartbeatInt;
    delete DefaultApplVerID;
  }
};
class MDLogon_3 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplID;
  ConstantFieldMandatory<uint32_t> *EncryptMethod;
  NoOpField<uint32_t> *HeartbeatInt;
  ConstantFieldMandatory<FFUtils::ByteArr> *DefaultApplVerID;

  // Constructor
  MDLogon_3() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("A"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("REPLAY"));
    EncryptMethod = new ConstantFieldMandatory<uint32_t>(true, 0);
    HeartbeatInt = new NoOpField<uint32_t>(true, false, 0);
    DefaultApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplID->decode();
    EncryptMethod->decode();
    HeartbeatInt->decode(input);
    DefaultApplVerID->decode();
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplID->reset();
    EncryptMethod->reset();
    HeartbeatInt->reset();
    DefaultApplVerID->reset();
  }

  // Destructor
  virtual ~MDLogon_3() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplID;
    delete EncryptMethod;
    delete HeartbeatInt;
    delete DefaultApplVerID;
  }
};
class MDLogon : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplFeedId;
  ConstantFieldMandatory<uint32_t> *EncryptMethod;
  NoOpField<uint32_t> *HeartbeatInt;
  ConstantFieldMandatory<FFUtils::ByteArr> *DefaultApplVerID;

  // Constructor
  MDLogon() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("A"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplFeedId = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("REPLAY"));
    EncryptMethod = new ConstantFieldMandatory<uint32_t>(true, 0);
    HeartbeatInt = new NoOpField<uint32_t>(true, false, 0);
    DefaultApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplFeedId->decode();
    EncryptMethod->decode();
    HeartbeatInt->decode(input);
    DefaultApplVerID->decode();
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplFeedId->reset();
    EncryptMethod->reset();
    HeartbeatInt->reset();
    DefaultApplVerID->reset();
  }

  // Destructor
  virtual ~MDLogon() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplFeedId;
    delete EncryptMethod;
    delete HeartbeatInt;
    delete DefaultApplVerID;
  }
};
class MDLogout_4 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplID;
  NoOpField<FFUtils::ByteArr> *Text;

  // Constructor
  MDLogout_4() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("5"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("REPLAY"));
    Text = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplID->decode();
    Text->decode(input);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplID->reset();
    Text->reset();
  }

  // Destructor
  virtual ~MDLogout_4() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplID;
    delete Text;
  }
};
class MDLogout_119 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplID;
  NoOpField<FFUtils::ByteArr> *Text;

  // Constructor
  MDLogout_119() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("5"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("REPLAY"));
    Text = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplID->decode();
    Text->decode(input);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplID->reset();
    Text->reset();
  }

  // Destructor
  virtual ~MDLogout_119() {
    delete ApplVerID;
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplID;
    delete Text;
  }
};
class MDLogout : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  ConstantFieldMandatory<FFUtils::ByteArr> *SenderCompID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplFeedId;
  NoOpField<FFUtils::ByteArr> *Text;

  // Constructor
  MDLogout() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("5"));
    SenderCompID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("CME"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplFeedId = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("REPLAY"));
    Text = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    SenderCompID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplFeedId->decode();
    Text->decode(input);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    SenderCompID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplFeedId->reset();
    Text->reset();
  }

  // Destructor
  virtual ~MDLogout() {
    delete ApplVerID;
    delete MessageType;
    delete SenderCompID;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplFeedId;
    delete Text;
  }
};
class MDSequenceReset : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t> *NewSeqNo;

  // Constructor
  MDSequenceReset() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("4"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    NewSeqNo = new NoOpField<uint32_t>(true, false, 0);
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode(pmap0);
    NewSeqNo->decode(input);
    process();
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
    NewSeqNo->reset();
  }

  // Destructor
  virtual ~MDSequenceReset() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete NewSeqNo;
  }
};
class DecoderMap {
 public:
  static void initilize(std::map<int, FastDecoder *> &t_map) {
    t_map[117] = new MDTcpRequestReject_117();
    t_map[111] = new MDSecurityList_111();
    t_map[81] = new MDIncRefresh_81();
    t_map[126] = new MDIncRefresh_126();
    t_map[123] = new MDIncRefresh_123();
    t_map[110] = new MDIncRefresh_110();
    t_map[109] = new MDIncRefresh_109();
    t_map[108] = new MDIncRefresh_108();
    t_map[97] = new MDIncRefresh_97();
    t_map[77] = new MDIncRefresh_77();
    t_map[103] = new MDIncRefresh_103();
    t_map[69] = new MDIncRefresh_69();
    t_map[83] = new MDIncRefresh_83();
    t_map[32] = new MDIncRefresh_32();
    t_map[84] = new MDIncRefresh_84();
    t_map[68] = new MDIncRefresh_68();
    t_map[104] = new MDIncRefresh_104();
    t_map[71] = new MDIncRefresh_71();
    t_map[59] = new MDIncRefresh_59();
    t_map[86] = new MDIncRefresh_86();
    t_map[35] = new MDIncRefresh_35();
    t_map[87] = new MDIncRefresh_87();
    t_map[36] = new MDIncRefresh_36();
    t_map[88] = new MDIncRefresh_88();
    t_map[72] = new MDIncRefresh_72();
    t_map[105] = new MDIncRefresh_105();
    t_map[67] = new MDIncRefresh_67();
    t_map[90] = new MDIncRefresh_90();
    t_map[39] = new MDIncRefresh_39();
    t_map[91] = new MDIncRefresh_91();
    t_map[70] = new MDIncRefresh_70();
    t_map[93] = new MDIncRefresh_93();
    t_map[41] = new MDIncRefresh_41();
    t_map[106] = new MDIncRefresh_106();
    t_map[73] = new MDIncRefresh_73();
    t_map[95] = new MDIncRefresh_95();
    t_map[43] = new MDIncRefresh_43();
    t_map[96] = new MDIncRefresh_96();
    t_map[44] = new MDIncRefresh_44();
    t_map[107] = new MDIncRefresh_107();
    t_map[74] = new MDIncRefresh_74();
    t_map[79] = new MDSecurityDefinition_79();
    t_map[78] = new MDSecurityDefinition_78();
    t_map[52] = new MDSecurityDefinition_52();
    t_map[98] = new MDQuoteRequest_98();
    t_map[54] = new MDQuoteRequest_54();
    t_map[125] = new MDSecurityStatus_125();
    t_map[113] = new MDSecurityStatus_113();
    t_map[102] = new MDSecurityStatus_102();
    t_map[99] = new MDSecurityStatus_99();
    t_map[76] = new MDSecurityStatus_76();
    t_map[48] = new MDSecurityStatus();
    t_map[127] = new MDSnapshotFullRefresh_127();
    t_map[124] = new MDSnapshotFullRefresh_124();
    t_map[112] = new MDSnapshotFullRefresh_112();
    t_map[80] = new MDSnapshotFullRefresh_80();
    t_map[53] = new MDSnapshotFullRefresh_53();
    t_map[120] = new MDNewsMessage_120();
    t_map[100] = new MDNewsMessage_100();
    t_map[49] = new MDNewsMessage();
    t_map[101] = new MDHeartbeat_101();
    t_map[50] = new MDHeartbeat();
    t_map[118] = new MDLogon_118();
    t_map[3] = new MDLogon_3();
    t_map[1] = new MDLogon();
    t_map[4] = new MDLogout_4();
    t_map[119] = new MDLogout_119();
    t_map[2] = new MDLogout();
    t_map[122] = new MDSequenceReset();
  }
  static void cleanUpMem(std::map<int, FastDecoder *> &t_map) {
    delete t_map[117];
    delete t_map[111];
    delete t_map[81];
    delete t_map[126];
    delete t_map[123];
    delete t_map[110];
    delete t_map[109];
    delete t_map[108];
    delete t_map[97];
    delete t_map[77];
    delete t_map[103];
    delete t_map[69];
    delete t_map[83];
    delete t_map[32];
    delete t_map[84];
    delete t_map[68];
    delete t_map[104];
    delete t_map[71];
    delete t_map[59];
    delete t_map[86];
    delete t_map[35];
    delete t_map[87];
    delete t_map[36];
    delete t_map[88];
    delete t_map[72];
    delete t_map[105];
    delete t_map[67];
    delete t_map[90];
    delete t_map[39];
    delete t_map[91];
    delete t_map[70];
    delete t_map[93];
    delete t_map[41];
    delete t_map[106];
    delete t_map[73];
    delete t_map[95];
    delete t_map[43];
    delete t_map[96];
    delete t_map[44];
    delete t_map[107];
    delete t_map[74];
    delete t_map[79];
    delete t_map[78];
    delete t_map[52];
    delete t_map[98];
    delete t_map[54];
    delete t_map[125];
    delete t_map[113];
    delete t_map[102];
    delete t_map[99];
    delete t_map[76];
    delete t_map[48];
    delete t_map[127];
    delete t_map[124];
    delete t_map[112];
    delete t_map[80];
    delete t_map[53];
    delete t_map[120];
    delete t_map[100];
    delete t_map[49];
    delete t_map[101];
    delete t_map[50];
    delete t_map[118];
    delete t_map[3];
    delete t_map[1];
    delete t_map[4];
    delete t_map[119];
    delete t_map[2];
    delete t_map[122];
  }
};
};
