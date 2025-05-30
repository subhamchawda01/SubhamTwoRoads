// AUTO GENERATED CLASS FOR TEMPLATE files/BMF/templates-UMDF-GTS.xml
// DO NOT MODIFY

#pragma once
#include "infracore/lwfixfast/FastDecoder.hpp"

namespace FF_GLOB {
namespace BMF_TEMPLATE_DECODER {
class SequenceReset_10 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t, true, false> *NewSeqNo;

  // Constructor
  SequenceReset_10() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("4"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    NewSeqNo = new NoOpField<uint32_t, true, false>(0, "glob_NewSeqNo");
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
  virtual ~SequenceReset_10() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete NewSeqNo;
  }
};
class Heartbeat_11 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;

  // Constructor
  Heartbeat_11() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("0"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode(pmap0);
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
  }

  // Destructor
  virtual ~Heartbeat_11() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
  }
};
class SecurityList_12 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t, false, false> *TotNoRelatedSym;
  NoOpField<uint32_t, false, false> *LastFragment;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t, true, false> *NoRelatedSym;
  CopyField<FFUtils::ByteArr, true, false> *Symbol;
  CopyField<FFUtils::ByteArr, true, false> *SecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityIDSource;
  CopyField<FFUtils::ByteArr, true, true> *SecurityExchange;
  // fields inside sequence SecurityAltID
  NoOpField<uint32_t, true, false> *NoSecurityAltID;
  CopyField<FFUtils::ByteArr, false, false> *SecurityAltID;
  CopyField<FFUtils::ByteArr, false, false> *SecurityAltIDSource;
  // fields inside sequence SecurityAltID ends
  // fields inside sequence Underlyings
  NoOpField<uint32_t, true, false> *NoUnderlyings;
  CopyField<FFUtils::ByteArr, false, false> *UnderlyingSymbol;
  CopyField<FFUtils::ByteArr, false, false> *UnderlyingSecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *UnderlyingSecurityIDSource;
  CopyField<FFUtils::ByteArr, false, true> *UnderlyingSecurityExchange;
  CopyField<FFUtils::Decimal, false, false> *IndexPct;
  // fields inside sequence Underlyings ends
  // fields inside sequence Legs
  NoOpField<uint32_t, true, false> *NoLegs;
  CopyField<FFUtils::ByteArr, false, false> *LegSymbol;
  CopyField<FFUtils::ByteArr, false, false> *LegSecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *LegSecurityIDSource;
  CopyField<FFUtils::ByteArr, false, true> *LegSecurityExchange;
  // fields inside sequence Legs ends
  CopyField<FFUtils::ByteArr, false, false> *SecurityUpdateAction;
  CopyField<FFUtils::Decimal, false, false> *RoundLot;
  CopyField<FFUtils::Decimal, false, false> *MinTradeVol;
  CopyField<FFUtils::Decimal, false, false> *MinPriceIncrement;
  CopyField<uint32_t, false, false> *TickSizeDenominator;
  CopyField<FFUtils::Decimal, false, false> *MinOrderQty;
  CopyField<FFUtils::Decimal, false, false> *MaxOrderQty;
  CopyField<int32_t, false, false> *InstrumentId;
  CopyField<FFUtils::ByteArr, false, false> *Currency;
  CopyField<FFUtils::ByteArr, false, false> *SecurityType;
  CopyField<FFUtils::ByteArr, false, false> *SecuritySubType;
  CopyField<int32_t, false, false> *Product;
  CopyField<FFUtils::ByteArr, false, false> *Asset;
  CopyField<FFUtils::ByteArr, false, false> *SecurityDesc;
  CopyField<uint32_t, false, false> *MaturityDate;
  CopyField<FFUtils::ByteArr, false, false> *MaturityMonthYear;
  CopyField<FFUtils::Decimal, false, false> *StrikePrice;
  CopyField<FFUtils::ByteArr, false, false> *StrikeCurrency;
  CopyField<FFUtils::Decimal, false, false> *ContractMultiplier;
  CopyField<FFUtils::ByteArr, false, false> *ContractSettlMonth;
  CopyField<FFUtils::ByteArr, false, false> *CFICode;
  CopyField<FFUtils::ByteArr, false, false> *CountryOfIssue;
  CopyField<uint32_t, false, false> *IssueDate;
  CopyField<uint32_t, false, false> *DatedDate;
  CopyField<uint32_t, false, false> *StartDate;
  CopyField<uint32_t, false, false> *EndDate;
  CopyField<FFUtils::ByteArr, false, false> *SettlType;
  CopyField<uint32_t, false, false> *SettlDate;
  CopyField<int32_t, false, false> *PriceType;
  CopyField<uint64_t, false, false> *SecurityValidityTimestamp;
  CopyField<FFUtils::ByteArr, false, false> *SecurityGroup;
  // fields inside sequence RelatedSym ends

  // Constructor
  SecurityList_12() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("y"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    TotNoRelatedSym = new NoOpField<uint32_t, false, false>(0, "glob_TotNoRelatedSym");
    LastFragment = new NoOpField<uint32_t, false, false>(0, "glob_LastFragment");
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t, true, false>(0, "glob_NoRelatedSym");
    Symbol = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_Symbol");
    SecurityID = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_SecurityID");
    SecurityIDSource =
        new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange = new CopyField<FFUtils::ByteArr, true, true>(FFUtils::ByteArr("XBSP"), "glob_SecurityExchange");
    // constructor for sequence SecurityAltID
    NoSecurityAltID = new NoOpField<uint32_t, true, false>(0, "glob_NoSecurityAltID");
    SecurityAltID = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityAltID");
    SecurityAltIDSource =
        new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityAltIDSource");
    // constructor for sequence SecurityAltID ends
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t, true, false>(0, "glob_NoUnderlyings");
    UnderlyingSymbol = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_UnderlyingSymbol");
    UnderlyingSecurityID =
        new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_UnderlyingSecurityID");
    UnderlyingSecurityIDSource =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_UnderlyingSecurityIDSource");
    UnderlyingSecurityExchange =
        new CopyField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("XBSP"), "glob_UnderlyingSecurityExchange");
    IndexPct = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_IndexPct");
    // constructor for sequence Underlyings ends
    // constructor for sequence Legs
    NoLegs = new NoOpField<uint32_t, true, false>(0, "glob_NoLegs");
    LegSymbol = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_LegSymbol");
    LegSecurityID = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_LegSecurityID");
    LegSecurityIDSource =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_LegSecurityIDSource");
    LegSecurityExchange =
        new CopyField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("XBSP"), "glob_LegSecurityExchange");
    // constructor for sequence Legs ends
    SecurityUpdateAction =
        new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityUpdateAction");
    RoundLot = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_RoundLot");
    MinTradeVol = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_MinTradeVol");
    MinPriceIncrement = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_MinPriceIncrement");
    TickSizeDenominator = new CopyField<uint32_t, false, false>(0, "glob_TickSizeDenominator");
    MinOrderQty = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_MinOrderQty");
    MaxOrderQty = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_MaxOrderQty");
    InstrumentId = new CopyField<int32_t, false, false>(0, "glob_InstrumentId");
    Currency = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Currency");
    SecurityType = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityType");
    SecuritySubType = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecuritySubType");
    Product = new CopyField<int32_t, false, false>(0, "glob_Product");
    Asset = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Asset");
    SecurityDesc = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityDesc");
    MaturityDate = new CopyField<uint32_t, false, false>(0, "glob_MaturityDate");
    MaturityMonthYear = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MaturityMonthYear");
    StrikePrice = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_StrikePrice");
    StrikeCurrency = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_StrikeCurrency");
    ContractMultiplier = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_ContractMultiplier");
    ContractSettlMonth = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_ContractSettlMonth");
    CFICode = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_CFICode");
    CountryOfIssue = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_CountryOfIssue");
    IssueDate = new CopyField<uint32_t, false, false>(0, "glob_IssueDate");
    DatedDate = new CopyField<uint32_t, false, false>(0, "glob_DatedDate");
    StartDate = new CopyField<uint32_t, false, false>(0, "glob_StartDate");
    EndDate = new CopyField<uint32_t, false, false>(0, "glob_EndDate");
    SettlType = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SettlType");
    SettlDate = new CopyField<uint32_t, false, false>(0, "glob_SettlDate");
    PriceType = new CopyField<int32_t, false, false>(0, "glob_PriceType");
    SecurityValidityTimestamp = new CopyField<uint64_t, false, false>(0, "glob_SecurityValidityTimestamp");
    SecurityGroup = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityGroup");
    // constructor for sequence RelatedSym ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode(pmap0);
    TotNoRelatedSym->decode(input);
    LastFragment->decode(input);
    // decode for sequence RelatedSym
    NoRelatedSym->decode(input);

    int NoRelatedSym_len = NoRelatedSym->hasValue() ? NoRelatedSym->getValue() : 0;
    for (int i = 0; i < NoRelatedSym_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      Symbol->decode(input, pmap1);
      SecurityID->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityExchange->decode(input, pmap1);
      // decode for sequence SecurityAltID
      NoSecurityAltID->decode(input);

      int NoSecurityAltID_len = NoSecurityAltID->hasValue() ? NoSecurityAltID->getValue() : 0;
      for (int i = 0; i < NoSecurityAltID_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap2 = input.extractPmap();

        SecurityAltID->decode(input, pmap2);
        SecurityAltIDSource->decode(input, pmap2);
      }
      // decode for sequence SecurityAltID ends
      // decode for sequence Underlyings
      NoUnderlyings->decode(input);

      int NoUnderlyings_len = NoUnderlyings->hasValue() ? NoUnderlyings->getValue() : 0;
      for (int i = 0; i < NoUnderlyings_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap2 = input.extractPmap();

        UnderlyingSymbol->decode(input, pmap2);
        UnderlyingSecurityID->decode(input, pmap2);
        UnderlyingSecurityIDSource->decode(input, pmap2);
        UnderlyingSecurityExchange->decode(input, pmap2);
        IndexPct->decode(input, pmap2);
      }
      // decode for sequence Underlyings ends
      // decode for sequence Legs
      NoLegs->decode(input);

      int NoLegs_len = NoLegs->hasValue() ? NoLegs->getValue() : 0;
      for (int i = 0; i < NoLegs_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap2 = input.extractPmap();

        LegSymbol->decode(input, pmap2);
        LegSecurityID->decode(input, pmap2);
        LegSecurityIDSource->decode(input, pmap2);
        LegSecurityExchange->decode(input, pmap2);
      }
      // decode for sequence Legs ends
      SecurityUpdateAction->decode(input, pmap1);
      RoundLot->decode(input, pmap1);
      MinTradeVol->decode(input, pmap1);
      MinPriceIncrement->decode(input, pmap1);
      TickSizeDenominator->decode(input, pmap1);
      MinOrderQty->decode(input, pmap1);
      MaxOrderQty->decode(input, pmap1);
      InstrumentId->decode(input, pmap1);
      Currency->decode(input, pmap1);
      SecurityType->decode(input, pmap1);
      SecuritySubType->decode(input, pmap1);
      Product->decode(input, pmap1);
      Asset->decode(input, pmap1);
      SecurityDesc->decode(input, pmap1);
      MaturityDate->decode(input, pmap1);
      MaturityMonthYear->decode(input, pmap1);
      StrikePrice->decode(input, pmap1);
      StrikeCurrency->decode(input, pmap1);
      ContractMultiplier->decode(input, pmap1);
      ContractSettlMonth->decode(input, pmap1);
      CFICode->decode(input, pmap1);
      CountryOfIssue->decode(input, pmap1);
      IssueDate->decode(input, pmap1);
      DatedDate->decode(input, pmap1);
      StartDate->decode(input, pmap1);
      EndDate->decode(input, pmap1);
      SettlType->decode(input, pmap1);
      SettlDate->decode(input, pmap1);
      PriceType->decode(input, pmap1);
      SecurityValidityTimestamp->decode(input, pmap1);
      SecurityGroup->decode(input, pmap1);
    }
    // decode for sequence RelatedSym ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
    TotNoRelatedSym->reset();
    LastFragment->reset();
    // reset for sequence RelatedSym
    NoRelatedSym->reset();
    Symbol->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence SecurityAltID
    NoSecurityAltID->reset();
    SecurityAltID->reset();
    SecurityAltIDSource->reset();
    // reset for sequence SecurityAltID ends
    // reset for sequence Underlyings
    NoUnderlyings->reset();
    UnderlyingSymbol->reset();
    UnderlyingSecurityID->reset();
    UnderlyingSecurityIDSource->reset();
    UnderlyingSecurityExchange->reset();
    IndexPct->reset();
    // reset for sequence Underlyings ends
    // reset for sequence Legs
    NoLegs->reset();
    LegSymbol->reset();
    LegSecurityID->reset();
    LegSecurityIDSource->reset();
    LegSecurityExchange->reset();
    // reset for sequence Legs ends
    SecurityUpdateAction->reset();
    RoundLot->reset();
    MinTradeVol->reset();
    MinPriceIncrement->reset();
    TickSizeDenominator->reset();
    MinOrderQty->reset();
    MaxOrderQty->reset();
    InstrumentId->reset();
    Currency->reset();
    SecurityType->reset();
    SecuritySubType->reset();
    Product->reset();
    Asset->reset();
    SecurityDesc->reset();
    MaturityDate->reset();
    MaturityMonthYear->reset();
    StrikePrice->reset();
    StrikeCurrency->reset();
    ContractMultiplier->reset();
    ContractSettlMonth->reset();
    CFICode->reset();
    CountryOfIssue->reset();
    IssueDate->reset();
    DatedDate->reset();
    StartDate->reset();
    EndDate->reset();
    SettlType->reset();
    SettlDate->reset();
    PriceType->reset();
    SecurityValidityTimestamp->reset();
    SecurityGroup->reset();
    // reset for sequence RelatedSym ends
  }

  // Destructor
  virtual ~SecurityList_12() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete TotNoRelatedSym;
    delete LastFragment;
    // destructor for sequence RelatedSym
    delete NoRelatedSym;
    delete Symbol;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence SecurityAltID
    delete NoSecurityAltID;
    delete SecurityAltID;
    delete SecurityAltIDSource;
    // destructor for sequence SecurityAltID ends
    // destructor for sequence Underlyings
    delete NoUnderlyings;
    delete UnderlyingSymbol;
    delete UnderlyingSecurityID;
    delete UnderlyingSecurityIDSource;
    delete UnderlyingSecurityExchange;
    delete IndexPct;
    // destructor for sequence Underlyings ends
    // destructor for sequence Legs
    delete NoLegs;
    delete LegSymbol;
    delete LegSecurityID;
    delete LegSecurityIDSource;
    delete LegSecurityExchange;
    // destructor for sequence Legs ends
    delete SecurityUpdateAction;
    delete RoundLot;
    delete MinTradeVol;
    delete MinPriceIncrement;
    delete TickSizeDenominator;
    delete MinOrderQty;
    delete MaxOrderQty;
    delete InstrumentId;
    delete Currency;
    delete SecurityType;
    delete SecuritySubType;
    delete Product;
    delete Asset;
    delete SecurityDesc;
    delete MaturityDate;
    delete MaturityMonthYear;
    delete StrikePrice;
    delete StrikeCurrency;
    delete ContractMultiplier;
    delete ContractSettlMonth;
    delete CFICode;
    delete CountryOfIssue;
    delete IssueDate;
    delete DatedDate;
    delete StartDate;
    delete EndDate;
    delete SettlType;
    delete SettlDate;
    delete PriceType;
    delete SecurityValidityTimestamp;
    delete SecurityGroup;
    // destructor for sequence RelatedSym ends
  }
};
class SecurityList_30 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  DefaultField<uint32_t, false, false> *TotNoRelatedSym;
  DefaultField<uint32_t, false, false> *LastFragment;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t, true, false> *NoRelatedSym;
  DeltaField<FFUtils::ByteArr, true> *SecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityExchange;
  DeltaField<FFUtils::ByteArr, false> *Symbol;
  // fields inside sequence SecurityAltID
  NoOpField<uint32_t, true, false> *NoSecurityAltID;
  DeltaField<FFUtils::ByteArr, false> *SecurityAltID;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityAltIDSource;
  // fields inside sequence SecurityAltID ends
  // fields inside sequence Underlyings
  NoOpField<uint32_t, true, false> *NoUnderlyings;
  DeltaField<FFUtils::ByteArr, false> *UnderlyingSymbol;
  DeltaField<FFUtils::ByteArr, false> *UnderlyingSecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *UnderlyingSecurityIDSource;
  DefaultField<FFUtils::ByteArr, false, true> *UnderlyingSecurityExchange;
  CopyField<FFUtils::Decimal, false, false> *IndexPct;
  // fields inside sequence Underlyings ends
  // fields inside sequence Legs
  NoOpField<uint32_t, true, false> *NoLegs;
  DeltaField<FFUtils::ByteArr, false> *LegSymbol;
  DeltaField<FFUtils::ByteArr, false> *LegSecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *LegSecurityIDSource;
  DefaultField<FFUtils::ByteArr, false, true> *LegSecurityExchange;
  DefaultField<FFUtils::ByteArr, false, false> *LegSide;
  DefaultField<FFUtils::Decimal, false, false> *LegRatioQty;
  // fields inside sequence Legs ends
  CopyField<FFUtils::ByteArr, false, false> *SecurityUpdateAction;
  CopyField<FFUtils::Decimal, false, false> *RoundLot;
  CopyField<FFUtils::Decimal, false, false> *MinTradeVol;
  CopyField<FFUtils::Decimal, false, false> *MinPriceIncrement;
  CopyField<uint32_t, false, false> *TickSizeDenominator;
  CopyField<FFUtils::Decimal, false, false> *MinOrderQty;
  CopyField<FFUtils::Decimal, false, false> *MaxOrderQty;
  DefaultField<int32_t, false, false> *InstrumentId;
  CopyField<FFUtils::ByteArr, false, true> *Currency;
  CopyField<FFUtils::ByteArr, false, false> *SecurityType;
  CopyField<FFUtils::ByteArr, false, false> *SecuritySubType;
  CopyField<FFUtils::ByteArr, false, false> *Asset;
  DeltaField<FFUtils::ByteArr, false> *SecurityDesc;
  DeltaField<uint32_t, false> *MaturityDate;
  CopyField<FFUtils::ByteArr, false, false> *MaturityMonthYear;
  CopyField<FFUtils::Decimal, false, false> *StrikePrice;
  CopyField<FFUtils::ByteArr, false, false> *StrikeCurrency;
  DefaultField<uint32_t, false, false> *ExerciseStyle;
  DefaultField<uint32_t, false, false> *PutOrCall;
  CopyField<FFUtils::Decimal, false, false> *ContractMultiplier;
  CopyField<uint32_t, false, false> *PriceDivisor;
  CopyField<FFUtils::ByteArr, false, false> *ContractSettlMonth;
  CopyField<FFUtils::ByteArr, false, false> *CFICode;
  CopyField<FFUtils::ByteArr, false, false> *CountryOfIssue;
  CopyField<uint32_t, false, false> *IssueDate;
  DefaultField<uint32_t, false, false> *DatedDate;
  CopyField<uint32_t, false, false> *StartDate;
  CopyField<uint32_t, false, false> *EndDate;
  CopyField<FFUtils::ByteArr, false, false> *SettlType;
  CopyField<uint32_t, false, false> *SettlDate;
  CopyField<int32_t, false, false> *PriceType;
  CopyField<uint64_t, false, false> *SecurityValidityTimestamp;
  CopyField<FFUtils::ByteArr, false, false> *SecurityGroup;
  // fields inside sequence RelatedSym ends

  // Constructor
  SecurityList_30() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("y"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    TotNoRelatedSym = new DefaultField<uint32_t, false, false>(0, "glob_TotNoRelatedSym");
    LastFragment = new DefaultField<uint32_t, false, false>(0, "glob_LastFragment");
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t, true, false>(0, "glob_NoRelatedSym");
    SecurityID = new DeltaField<FFUtils::ByteArr, true>(FFUtils::ByteArr(""), "glob_SecurityID");
    SecurityIDSource = new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_SecurityExchange");
    Symbol = new DeltaField<FFUtils::ByteArr, false>(FFUtils::ByteArr(""), "glob_Symbol");
    // constructor for sequence SecurityAltID
    NoSecurityAltID = new NoOpField<uint32_t, true, false>(0, "glob_NoSecurityAltID");
    SecurityAltID = new DeltaField<FFUtils::ByteArr, false>(FFUtils::ByteArr(""), "glob_SecurityAltID");
    SecurityAltIDSource =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("4"), "glob_SecurityAltIDSource");
    // constructor for sequence SecurityAltID ends
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t, true, false>(0, "glob_NoUnderlyings");
    UnderlyingSymbol = new DeltaField<FFUtils::ByteArr, false>(FFUtils::ByteArr(""), "glob_UnderlyingSymbol");
    UnderlyingSecurityID = new DeltaField<FFUtils::ByteArr, false>(FFUtils::ByteArr(""), "glob_UnderlyingSecurityID");
    UnderlyingSecurityIDSource =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_UnderlyingSecurityIDSource");
    UnderlyingSecurityExchange =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_UnderlyingSecurityExchange");
    IndexPct = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_IndexPct");
    // constructor for sequence Underlyings ends
    // constructor for sequence Legs
    NoLegs = new NoOpField<uint32_t, true, false>(0, "glob_NoLegs");
    LegSymbol = new DeltaField<FFUtils::ByteArr, false>(FFUtils::ByteArr(""), "glob_LegSymbol");
    LegSecurityID = new DeltaField<FFUtils::ByteArr, false>(FFUtils::ByteArr(""), "glob_LegSecurityID");
    LegSecurityIDSource =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_LegSecurityIDSource");
    LegSecurityExchange =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_LegSecurityExchange");
    LegSide = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_LegSide");
    LegRatioQty = new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_LegRatioQty");
    // constructor for sequence Legs ends
    SecurityUpdateAction =
        new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityUpdateAction");
    RoundLot = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_RoundLot");
    MinTradeVol = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_MinTradeVol");
    MinPriceIncrement = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_MinPriceIncrement");
    TickSizeDenominator = new CopyField<uint32_t, false, false>(0, "glob_TickSizeDenominator");
    MinOrderQty = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_MinOrderQty");
    MaxOrderQty = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_MaxOrderQty");
    InstrumentId = new DefaultField<int32_t, false, false>(0, "glob_InstrumentId");
    Currency = new CopyField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BRL"), "glob_Currency");
    SecurityType = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityType");
    SecuritySubType = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecuritySubType");
    Asset = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Asset");
    SecurityDesc = new DeltaField<FFUtils::ByteArr, false>(FFUtils::ByteArr(""), "glob_SecurityDesc");
    MaturityDate = new DeltaField<uint32_t, false>(0, "glob_MaturityDate");
    MaturityMonthYear = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MaturityMonthYear");
    StrikePrice = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_StrikePrice");
    StrikeCurrency = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_StrikeCurrency");
    ExerciseStyle = new DefaultField<uint32_t, false, false>(0, "glob_ExerciseStyle");
    PutOrCall = new DefaultField<uint32_t, false, false>(0, "glob_PutOrCall");
    ContractMultiplier = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_ContractMultiplier");
    PriceDivisor = new CopyField<uint32_t, false, false>(0, "glob_PriceDivisor");
    ContractSettlMonth = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_ContractSettlMonth");
    CFICode = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_CFICode");
    CountryOfIssue = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_CountryOfIssue");
    IssueDate = new CopyField<uint32_t, false, false>(0, "glob_IssueDate");
    DatedDate = new DefaultField<uint32_t, false, false>(0, "glob_DatedDate");
    StartDate = new CopyField<uint32_t, false, false>(0, "glob_StartDate");
    EndDate = new CopyField<uint32_t, false, false>(0, "glob_EndDate");
    SettlType = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SettlType");
    SettlDate = new CopyField<uint32_t, false, false>(0, "glob_SettlDate");
    PriceType = new CopyField<int32_t, false, false>(0, "glob_PriceType");
    SecurityValidityTimestamp = new CopyField<uint64_t, false, false>(0, "glob_SecurityValidityTimestamp");
    SecurityGroup = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityGroup");
    // constructor for sequence RelatedSym ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode(pmap0);
    TotNoRelatedSym->decode(input, pmap0);
    LastFragment->decode(input, pmap0);
    // decode for sequence RelatedSym
    NoRelatedSym->decode(input);

    int NoRelatedSym_len = NoRelatedSym->hasValue() ? NoRelatedSym->getValue() : 0;
    for (int i = 0; i < NoRelatedSym_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      SecurityID->decodeString(input);
      SecurityIDSource->decode(input, pmap1);
      SecurityExchange->decode(input, pmap1);
      Symbol->decodeString(input);
      // decode for sequence SecurityAltID
      NoSecurityAltID->decode(input);

      int NoSecurityAltID_len = NoSecurityAltID->hasValue() ? NoSecurityAltID->getValue() : 0;
      for (int i = 0; i < NoSecurityAltID_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap2 = input.extractPmap();

        SecurityAltID->decodeString(input);
        SecurityAltIDSource->decode(input, pmap2);
      }
      // decode for sequence SecurityAltID ends
      // decode for sequence Underlyings
      NoUnderlyings->decode(input);

      int NoUnderlyings_len = NoUnderlyings->hasValue() ? NoUnderlyings->getValue() : 0;
      for (int i = 0; i < NoUnderlyings_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap2 = input.extractPmap();

        UnderlyingSymbol->decodeString(input);
        UnderlyingSecurityID->decodeString(input);
        UnderlyingSecurityIDSource->decode(input, pmap2);
        UnderlyingSecurityExchange->decode(input, pmap2);
        IndexPct->decode(input, pmap2);
      }
      // decode for sequence Underlyings ends
      // decode for sequence Legs
      NoLegs->decode(input);

      int NoLegs_len = NoLegs->hasValue() ? NoLegs->getValue() : 0;
      for (int i = 0; i < NoLegs_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap2 = input.extractPmap();

        LegSymbol->decodeString(input);
        LegSecurityID->decodeString(input);
        LegSecurityIDSource->decode(input, pmap2);
        LegSecurityExchange->decode(input, pmap2);
        LegSide->decode(input, pmap2);
        LegRatioQty->decode(input, pmap2);
      }
      // decode for sequence Legs ends
      SecurityUpdateAction->decode(input, pmap1);
      RoundLot->decode(input, pmap1);
      MinTradeVol->decode(input, pmap1);
      MinPriceIncrement->decode(input, pmap1);
      TickSizeDenominator->decode(input, pmap1);
      MinOrderQty->decode(input, pmap1);
      MaxOrderQty->decode(input, pmap1);
      InstrumentId->decode(input, pmap1);
      Currency->decode(input, pmap1);
      SecurityType->decode(input, pmap1);
      SecuritySubType->decode(input, pmap1);
      Asset->decode(input, pmap1);
      SecurityDesc->decodeString(input);
      MaturityDate->decode(input);
      MaturityMonthYear->decode(input, pmap1);
      StrikePrice->decode(input, pmap1);
      StrikeCurrency->decode(input, pmap1);
      ExerciseStyle->decode(input, pmap1);
      PutOrCall->decode(input, pmap1);
      ContractMultiplier->decode(input, pmap1);
      PriceDivisor->decode(input, pmap1);
      ContractSettlMonth->decode(input, pmap1);
      CFICode->decode(input, pmap1);
      CountryOfIssue->decode(input, pmap1);
      IssueDate->decode(input, pmap1);
      DatedDate->decode(input, pmap1);
      StartDate->decode(input, pmap1);
      EndDate->decode(input, pmap1);
      SettlType->decode(input, pmap1);
      SettlDate->decode(input, pmap1);
      PriceType->decode(input, pmap1);
      SecurityValidityTimestamp->decode(input, pmap1);
      SecurityGroup->decode(input, pmap1);
      process();
    }
    // decode for sequence RelatedSym ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
    TotNoRelatedSym->reset();
    LastFragment->reset();
    // reset for sequence RelatedSym
    NoRelatedSym->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    Symbol->reset();
    // reset for sequence SecurityAltID
    NoSecurityAltID->reset();
    SecurityAltID->reset();
    SecurityAltIDSource->reset();
    // reset for sequence SecurityAltID ends
    // reset for sequence Underlyings
    NoUnderlyings->reset();
    UnderlyingSymbol->reset();
    UnderlyingSecurityID->reset();
    UnderlyingSecurityIDSource->reset();
    UnderlyingSecurityExchange->reset();
    IndexPct->reset();
    // reset for sequence Underlyings ends
    // reset for sequence Legs
    NoLegs->reset();
    LegSymbol->reset();
    LegSecurityID->reset();
    LegSecurityIDSource->reset();
    LegSecurityExchange->reset();
    LegSide->reset();
    LegRatioQty->reset();
    // reset for sequence Legs ends
    SecurityUpdateAction->reset();
    RoundLot->reset();
    MinTradeVol->reset();
    MinPriceIncrement->reset();
    TickSizeDenominator->reset();
    MinOrderQty->reset();
    MaxOrderQty->reset();
    InstrumentId->reset();
    Currency->reset();
    SecurityType->reset();
    SecuritySubType->reset();
    Asset->reset();
    SecurityDesc->reset();
    MaturityDate->reset();
    MaturityMonthYear->reset();
    StrikePrice->reset();
    StrikeCurrency->reset();
    ExerciseStyle->reset();
    PutOrCall->reset();
    ContractMultiplier->reset();
    PriceDivisor->reset();
    ContractSettlMonth->reset();
    CFICode->reset();
    CountryOfIssue->reset();
    IssueDate->reset();
    DatedDate->reset();
    StartDate->reset();
    EndDate->reset();
    SettlType->reset();
    SettlDate->reset();
    PriceType->reset();
    SecurityValidityTimestamp->reset();
    SecurityGroup->reset();
    // reset for sequence RelatedSym ends
  }

  // Destructor
  virtual ~SecurityList_30() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete TotNoRelatedSym;
    delete LastFragment;
    // destructor for sequence RelatedSym
    delete NoRelatedSym;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    delete Symbol;
    // destructor for sequence SecurityAltID
    delete NoSecurityAltID;
    delete SecurityAltID;
    delete SecurityAltIDSource;
    // destructor for sequence SecurityAltID ends
    // destructor for sequence Underlyings
    delete NoUnderlyings;
    delete UnderlyingSymbol;
    delete UnderlyingSecurityID;
    delete UnderlyingSecurityIDSource;
    delete UnderlyingSecurityExchange;
    delete IndexPct;
    // destructor for sequence Underlyings ends
    // destructor for sequence Legs
    delete NoLegs;
    delete LegSymbol;
    delete LegSecurityID;
    delete LegSecurityIDSource;
    delete LegSecurityExchange;
    delete LegSide;
    delete LegRatioQty;
    // destructor for sequence Legs ends
    delete SecurityUpdateAction;
    delete RoundLot;
    delete MinTradeVol;
    delete MinPriceIncrement;
    delete TickSizeDenominator;
    delete MinOrderQty;
    delete MaxOrderQty;
    delete InstrumentId;
    delete Currency;
    delete SecurityType;
    delete SecuritySubType;
    delete Asset;
    delete SecurityDesc;
    delete MaturityDate;
    delete MaturityMonthYear;
    delete StrikePrice;
    delete StrikeCurrency;
    delete ExerciseStyle;
    delete PutOrCall;
    delete ContractMultiplier;
    delete PriceDivisor;
    delete ContractSettlMonth;
    delete CFICode;
    delete CountryOfIssue;
    delete IssueDate;
    delete DatedDate;
    delete StartDate;
    delete EndDate;
    delete SettlType;
    delete SettlDate;
    delete PriceType;
    delete SecurityValidityTimestamp;
    delete SecurityGroup;
    // destructor for sequence RelatedSym ends
  }
};
class SecurityList_23 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t, false, false> *TotNoRelatedSym;
  NoOpField<uint32_t, false, false> *LastFragment;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t, true, false> *NoRelatedSym;
  CopyField<FFUtils::ByteArr, false, false> *Symbol;
  CopyField<FFUtils::ByteArr, true, false> *SecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityIDSource;
  CopyField<FFUtils::ByteArr, false, true> *SecurityExchange;
  // fields inside sequence SecurityAltID
  NoOpField<uint32_t, true, false> *NoSecurityAltID;
  CopyField<FFUtils::ByteArr, false, false> *SecurityAltID;
  CopyField<FFUtils::ByteArr, false, false> *SecurityAltIDSource;
  // fields inside sequence SecurityAltID ends
  // fields inside sequence Underlyings
  NoOpField<uint32_t, true, false> *NoUnderlyings;
  CopyField<FFUtils::ByteArr, false, false> *UnderlyingSymbol;
  CopyField<FFUtils::ByteArr, false, false> *UnderlyingSecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *UnderlyingSecurityIDSource;
  CopyField<FFUtils::ByteArr, false, true> *UnderlyingSecurityExchange;
  CopyField<FFUtils::Decimal, false, false> *IndexPct;
  // fields inside sequence Underlyings ends
  // fields inside sequence Legs
  NoOpField<uint32_t, true, false> *NoLegs;
  CopyField<FFUtils::ByteArr, false, false> *LegSymbol;
  CopyField<FFUtils::ByteArr, false, false> *LegSecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *LegSecurityIDSource;
  CopyField<FFUtils::ByteArr, false, true> *LegSecurityExchange;
  // fields inside sequence Legs ends
  CopyField<FFUtils::ByteArr, false, false> *SecurityUpdateAction;
  CopyField<FFUtils::Decimal, false, false> *RoundLot;
  CopyField<FFUtils::Decimal, false, false> *MinTradeVol;
  CopyField<FFUtils::Decimal, false, false> *MinPriceIncrement;
  CopyField<uint32_t, false, false> *TickSizeDenominator;
  CopyField<FFUtils::Decimal, false, false> *MinOrderQty;
  CopyField<FFUtils::Decimal, false, false> *MaxOrderQty;
  CopyField<int32_t, false, false> *InstrumentId;
  CopyField<FFUtils::ByteArr, false, false> *Currency;
  CopyField<FFUtils::ByteArr, false, false> *SecurityType;
  CopyField<FFUtils::ByteArr, false, false> *SecuritySubType;
  CopyField<FFUtils::ByteArr, false, false> *Asset;
  CopyField<FFUtils::ByteArr, false, false> *SecurityDesc;
  CopyField<uint32_t, false, false> *MaturityDate;
  CopyField<FFUtils::ByteArr, false, false> *MaturityMonthYear;
  CopyField<FFUtils::Decimal, false, false> *StrikePrice;
  CopyField<FFUtils::ByteArr, false, false> *StrikeCurrency;
  CopyField<FFUtils::Decimal, false, false> *ContractMultiplier;
  CopyField<FFUtils::ByteArr, false, false> *ContractSettlMonth;
  CopyField<FFUtils::ByteArr, false, false> *CFICode;
  CopyField<FFUtils::ByteArr, false, false> *CountryOfIssue;
  CopyField<uint32_t, false, false> *IssueDate;
  CopyField<uint32_t, false, false> *DatedDate;
  CopyField<uint32_t, false, false> *StartDate;
  CopyField<uint32_t, false, false> *EndDate;
  CopyField<FFUtils::ByteArr, false, false> *SettlType;
  CopyField<uint32_t, false, false> *SettlDate;
  CopyField<int32_t, false, false> *PriceType;
  CopyField<uint64_t, false, false> *SecurityValidityTimestamp;
  CopyField<FFUtils::ByteArr, false, false> *SecurityGroup;
  // fields inside sequence RelatedSym ends

  // Constructor
  SecurityList_23() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("y"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    TotNoRelatedSym = new NoOpField<uint32_t, false, false>(0, "glob_TotNoRelatedSym");
    LastFragment = new NoOpField<uint32_t, false, false>(0, "glob_LastFragment");
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t, true, false>(0, "glob_NoRelatedSym");
    Symbol = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Symbol");
    SecurityID = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_SecurityID");
    SecurityIDSource = new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange = new CopyField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_SecurityExchange");
    // constructor for sequence SecurityAltID
    NoSecurityAltID = new NoOpField<uint32_t, true, false>(0, "glob_NoSecurityAltID");
    SecurityAltID = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityAltID");
    SecurityAltIDSource =
        new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityAltIDSource");
    // constructor for sequence SecurityAltID ends
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t, true, false>(0, "glob_NoUnderlyings");
    UnderlyingSymbol = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_UnderlyingSymbol");
    UnderlyingSecurityID =
        new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_UnderlyingSecurityID");
    UnderlyingSecurityIDSource =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_UnderlyingSecurityIDSource");
    UnderlyingSecurityExchange =
        new CopyField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_UnderlyingSecurityExchange");
    IndexPct = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_IndexPct");
    // constructor for sequence Underlyings ends
    // constructor for sequence Legs
    NoLegs = new NoOpField<uint32_t, true, false>(0, "glob_NoLegs");
    LegSymbol = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_LegSymbol");
    LegSecurityID = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_LegSecurityID");
    LegSecurityIDSource =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_LegSecurityIDSource");
    LegSecurityExchange =
        new CopyField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_LegSecurityExchange");
    // constructor for sequence Legs ends
    SecurityUpdateAction =
        new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityUpdateAction");
    RoundLot = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_RoundLot");
    MinTradeVol = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_MinTradeVol");
    MinPriceIncrement = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_MinPriceIncrement");
    TickSizeDenominator = new CopyField<uint32_t, false, false>(0, "glob_TickSizeDenominator");
    MinOrderQty = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_MinOrderQty");
    MaxOrderQty = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_MaxOrderQty");
    InstrumentId = new CopyField<int32_t, false, false>(0, "glob_InstrumentId");
    Currency = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Currency");
    SecurityType = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityType");
    SecuritySubType = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecuritySubType");
    Asset = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Asset");
    SecurityDesc = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityDesc");
    MaturityDate = new CopyField<uint32_t, false, false>(0, "glob_MaturityDate");
    MaturityMonthYear = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MaturityMonthYear");
    StrikePrice = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_StrikePrice");
    StrikeCurrency = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_StrikeCurrency");
    ContractMultiplier = new CopyField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_ContractMultiplier");
    ContractSettlMonth = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_ContractSettlMonth");
    CFICode = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_CFICode");
    CountryOfIssue = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_CountryOfIssue");
    IssueDate = new CopyField<uint32_t, false, false>(0, "glob_IssueDate");
    DatedDate = new CopyField<uint32_t, false, false>(0, "glob_DatedDate");
    StartDate = new CopyField<uint32_t, false, false>(0, "glob_StartDate");
    EndDate = new CopyField<uint32_t, false, false>(0, "glob_EndDate");
    SettlType = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SettlType");
    SettlDate = new CopyField<uint32_t, false, false>(0, "glob_SettlDate");
    PriceType = new CopyField<int32_t, false, false>(0, "glob_PriceType");
    SecurityValidityTimestamp = new CopyField<uint64_t, false, false>(0, "glob_SecurityValidityTimestamp");
    SecurityGroup = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityGroup");
    // constructor for sequence RelatedSym ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode(pmap0);
    TotNoRelatedSym->decode(input);
    LastFragment->decode(input);
    // decode for sequence RelatedSym
    NoRelatedSym->decode(input);

    int NoRelatedSym_len = NoRelatedSym->hasValue() ? NoRelatedSym->getValue() : 0;
    for (int i = 0; i < NoRelatedSym_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      Symbol->decode(input, pmap1);
      SecurityID->decode(input, pmap1);
      SecurityIDSource->decode(input, pmap1);
      SecurityExchange->decode(input, pmap1);
      // decode for sequence SecurityAltID
      NoSecurityAltID->decode(input);

      int NoSecurityAltID_len = NoSecurityAltID->hasValue() ? NoSecurityAltID->getValue() : 0;
      for (int i = 0; i < NoSecurityAltID_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap2 = input.extractPmap();

        SecurityAltID->decode(input, pmap2);
        SecurityAltIDSource->decode(input, pmap2);
      }
      // decode for sequence SecurityAltID ends
      // decode for sequence Underlyings
      NoUnderlyings->decode(input);

      int NoUnderlyings_len = NoUnderlyings->hasValue() ? NoUnderlyings->getValue() : 0;
      for (int i = 0; i < NoUnderlyings_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap2 = input.extractPmap();

        UnderlyingSymbol->decode(input, pmap2);
        UnderlyingSecurityID->decode(input, pmap2);
        UnderlyingSecurityIDSource->decode(input, pmap2);
        UnderlyingSecurityExchange->decode(input, pmap2);
        IndexPct->decode(input, pmap2);
      }
      // decode for sequence Underlyings ends
      // decode for sequence Legs
      NoLegs->decode(input);

      int NoLegs_len = NoLegs->hasValue() ? NoLegs->getValue() : 0;
      for (int i = 0; i < NoLegs_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap2 = input.extractPmap();

        LegSymbol->decode(input, pmap2);
        LegSecurityID->decode(input, pmap2);
        LegSecurityIDSource->decode(input, pmap2);
        LegSecurityExchange->decode(input, pmap2);
      }
      // decode for sequence Legs ends
      SecurityUpdateAction->decode(input, pmap1);
      RoundLot->decode(input, pmap1);
      MinTradeVol->decode(input, pmap1);
      MinPriceIncrement->decode(input, pmap1);
      TickSizeDenominator->decode(input, pmap1);
      MinOrderQty->decode(input, pmap1);
      MaxOrderQty->decode(input, pmap1);
      InstrumentId->decode(input, pmap1);
      Currency->decode(input, pmap1);
      SecurityType->decode(input, pmap1);
      SecuritySubType->decode(input, pmap1);
      Asset->decode(input, pmap1);
      SecurityDesc->decode(input, pmap1);
      MaturityDate->decode(input, pmap1);
      MaturityMonthYear->decode(input, pmap1);
      StrikePrice->decode(input, pmap1);
      StrikeCurrency->decode(input, pmap1);
      ContractMultiplier->decode(input, pmap1);
      ContractSettlMonth->decode(input, pmap1);
      CFICode->decode(input, pmap1);
      CountryOfIssue->decode(input, pmap1);
      IssueDate->decode(input, pmap1);
      DatedDate->decode(input, pmap1);
      StartDate->decode(input, pmap1);
      EndDate->decode(input, pmap1);
      SettlType->decode(input, pmap1);
      SettlDate->decode(input, pmap1);
      PriceType->decode(input, pmap1);
      SecurityValidityTimestamp->decode(input, pmap1);
      SecurityGroup->decode(input, pmap1);
    }
    // decode for sequence RelatedSym ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
    TotNoRelatedSym->reset();
    LastFragment->reset();
    // reset for sequence RelatedSym
    NoRelatedSym->reset();
    Symbol->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence SecurityAltID
    NoSecurityAltID->reset();
    SecurityAltID->reset();
    SecurityAltIDSource->reset();
    // reset for sequence SecurityAltID ends
    // reset for sequence Underlyings
    NoUnderlyings->reset();
    UnderlyingSymbol->reset();
    UnderlyingSecurityID->reset();
    UnderlyingSecurityIDSource->reset();
    UnderlyingSecurityExchange->reset();
    IndexPct->reset();
    // reset for sequence Underlyings ends
    // reset for sequence Legs
    NoLegs->reset();
    LegSymbol->reset();
    LegSecurityID->reset();
    LegSecurityIDSource->reset();
    LegSecurityExchange->reset();
    // reset for sequence Legs ends
    SecurityUpdateAction->reset();
    RoundLot->reset();
    MinTradeVol->reset();
    MinPriceIncrement->reset();
    TickSizeDenominator->reset();
    MinOrderQty->reset();
    MaxOrderQty->reset();
    InstrumentId->reset();
    Currency->reset();
    SecurityType->reset();
    SecuritySubType->reset();
    Asset->reset();
    SecurityDesc->reset();
    MaturityDate->reset();
    MaturityMonthYear->reset();
    StrikePrice->reset();
    StrikeCurrency->reset();
    ContractMultiplier->reset();
    ContractSettlMonth->reset();
    CFICode->reset();
    CountryOfIssue->reset();
    IssueDate->reset();
    DatedDate->reset();
    StartDate->reset();
    EndDate->reset();
    SettlType->reset();
    SettlDate->reset();
    PriceType->reset();
    SecurityValidityTimestamp->reset();
    SecurityGroup->reset();
    // reset for sequence RelatedSym ends
  }

  // Destructor
  virtual ~SecurityList_23() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete TotNoRelatedSym;
    delete LastFragment;
    // destructor for sequence RelatedSym
    delete NoRelatedSym;
    delete Symbol;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence SecurityAltID
    delete NoSecurityAltID;
    delete SecurityAltID;
    delete SecurityAltIDSource;
    // destructor for sequence SecurityAltID ends
    // destructor for sequence Underlyings
    delete NoUnderlyings;
    delete UnderlyingSymbol;
    delete UnderlyingSecurityID;
    delete UnderlyingSecurityIDSource;
    delete UnderlyingSecurityExchange;
    delete IndexPct;
    // destructor for sequence Underlyings ends
    // destructor for sequence Legs
    delete NoLegs;
    delete LegSymbol;
    delete LegSecurityID;
    delete LegSecurityIDSource;
    delete LegSecurityExchange;
    // destructor for sequence Legs ends
    delete SecurityUpdateAction;
    delete RoundLot;
    delete MinTradeVol;
    delete MinPriceIncrement;
    delete TickSizeDenominator;
    delete MinOrderQty;
    delete MaxOrderQty;
    delete InstrumentId;
    delete Currency;
    delete SecurityType;
    delete SecuritySubType;
    delete Asset;
    delete SecurityDesc;
    delete MaturityDate;
    delete MaturityMonthYear;
    delete StrikePrice;
    delete StrikeCurrency;
    delete ContractMultiplier;
    delete ContractSettlMonth;
    delete CFICode;
    delete CountryOfIssue;
    delete IssueDate;
    delete DatedDate;
    delete StartDate;
    delete EndDate;
    delete SettlType;
    delete SettlDate;
    delete PriceType;
    delete SecurityValidityTimestamp;
    delete SecurityGroup;
    // destructor for sequence RelatedSym ends
  }
};
class MarketDataIncrementalRefresh_25 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  DefaultField<uint32_t, false, false> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t, true, false> *NoMDEntries;
  CopyField<FFUtils::ByteArr, true, false> *MDUpdateAction;
  CopyField<FFUtils::ByteArr, true, false> *MDEntryType;
  IncrementField<uint32_t, false, false> *RptSeq;
  DefaultField<uint32_t, false, false> *PriceBandType;
  CopyField<int64_t, false, false> *SecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityExchange;
  DefaultField<FFUtils::ByteArr, false, false> *MDStreamID;
  DeltaField<FFUtils::Decimal, false> *MDEntryPx;
  DeltaField<FFUtils::Decimal, false> *MDEntrySize;
  CopyField<uint32_t, true, false> *MDEntryDate;
  CopyField<uint32_t, true, false> *MDEntryTime;
  DefaultField<FFUtils::ByteArr, false, false> *TickDirection;
  DefaultField<FFUtils::ByteArr, false, false> *QuoteCondition;
  DefaultField<FFUtils::ByteArr, false, false> *TradeCondition;
  DefaultField<FFUtils::ByteArr, false, false> *OpenCloseSettlFlag;
  DefaultField<uint64_t, false, false> *NoSharesIssued;
  DefaultField<FFUtils::ByteArr, false, false> *Currency;
  DefaultField<FFUtils::ByteArr, false, false> *OrderID;
  DefaultField<FFUtils::ByteArr, false, false> *TradeID;
  CopyField<FFUtils::ByteArr, false, false> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr, false, false> *MDEntrySeller;
  DefaultField<uint32_t, false, false> *NumberOfOrders;
  IncrementField<uint32_t, false, false> *MDEntryPositionNo;
  DefaultField<uint32_t, false, false> *PriceType;
  DefaultField<FFUtils::Decimal, false, false> *NetChgPrevDay;
  DefaultField<uint32_t, false, false> *SellerDays;
  DefaultField<uint32_t, false, false> *SettlPriceType;
  DefaultField<FFUtils::Decimal, false, false> *TradeVolume;
  DefaultField<uint32_t, false, false> *PriceLimitType;
  DefaultField<FFUtils::Decimal, false, false> *LowLimitPrice;
  DefaultField<FFUtils::Decimal, false, false> *HighLimitPrice;
  DefaultField<FFUtils::Decimal, false, false> *TradingReferencePrice;
  DefaultField<uint64_t, false, false> *MDEntryID;
  DefaultField<uint32_t, false, false> *MDInsertDate;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataIncrementalRefresh_25() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    TradeDate = new DefaultField<uint32_t, false, false>(0, "glob_TradeDate");
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t, true, false>(0, "glob_NoMDEntries");
    MDUpdateAction = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_MDUpdateAction");
    MDEntryType = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_MDEntryType");
    RptSeq = new IncrementField<uint32_t, false, false>(0, "glob_RptSeq");
    PriceBandType = new DefaultField<uint32_t, false, false>(0, "glob_PriceBandType");
    SecurityID = new CopyField<int64_t, false, false>(0, "glob_SecurityID");
    SecurityIDSource = new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_SecurityExchange");
    MDStreamID = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDStreamID");
    MDEntryPx = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntryPx");
    MDEntrySize = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntrySize");
    MDEntryDate = new CopyField<uint32_t, true, false>(0, "glob_MDEntryDate");
    MDEntryTime = new CopyField<uint32_t, true, false>(0, "glob_MDEntryTime");
    TickDirection = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TickDirection");
    QuoteCondition = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_QuoteCondition");
    TradeCondition = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeCondition");
    OpenCloseSettlFlag =
        new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OpenCloseSettlFlag");
    NoSharesIssued = new DefaultField<uint64_t, false, false>(0, "glob_NoSharesIssued");
    Currency = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Currency");
    OrderID = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OrderID");
    TradeID = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeID");
    MDEntryBuyer = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntryBuyer");
    MDEntrySeller = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntrySeller");
    NumberOfOrders = new DefaultField<uint32_t, false, false>(0, "glob_NumberOfOrders");
    MDEntryPositionNo = new IncrementField<uint32_t, false, false>(0, "glob_MDEntryPositionNo");
    PriceType = new DefaultField<uint32_t, false, false>(0, "glob_PriceType");
    NetChgPrevDay = new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_NetChgPrevDay");
    SellerDays = new DefaultField<uint32_t, false, false>(0, "glob_SellerDays");
    SettlPriceType = new DefaultField<uint32_t, false, false>(0, "glob_SettlPriceType");
    TradeVolume = new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_TradeVolume");
    PriceLimitType = new DefaultField<uint32_t, false, false>(0, "glob_PriceLimitType");
    LowLimitPrice = new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_LowLimitPrice");
    HighLimitPrice = new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_HighLimitPrice");
    TradingReferencePrice =
        new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_TradingReferencePrice");
    MDEntryID = new DefaultField<uint64_t, false, false>(0, "glob_MDEntryID");
    MDInsertDate = new DefaultField<uint32_t, false, false>(0, "glob_MDInsertDate");
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode(pmap0);
    TradeDate->decode(input, pmap0);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->hasValue() ? NoMDEntries->getValue() : 0;
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      PriceBandType->decode(input, pmap1);
      SecurityID->decode(input, pmap1);
      SecurityIDSource->decode(input, pmap1);
      SecurityExchange->decode(input, pmap1);
      MDStreamID->decode(input, pmap1);
      MDEntryPx->decode(input);
      MDEntrySize->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettlFlag->decode(input, pmap1);
      NoSharesIssued->decode(input, pmap1);
      Currency->decode(input, pmap1);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      PriceType->decode(input, pmap1);
      NetChgPrevDay->decode(input, pmap1);
      SellerDays->decode(input, pmap1);
      SettlPriceType->decode(input, pmap1);
      TradeVolume->decode(input, pmap1);
      PriceLimitType->decode(input, pmap1);
      LowLimitPrice->decode(input, pmap1);
      HighLimitPrice->decode(input, pmap1);
      TradingReferencePrice->decode(input, pmap1);
      MDEntryID->decode(input, pmap1);
      MDInsertDate->decode(input, pmap1);
      //        process();//manually added
    }
    // decode for sequence MDEntries ends
    //      process_end();//manually added
  }

  // process : TO be done in corresponding cpp file manually
  void process();
  void process_end();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDEntryType->reset();
    RptSeq->reset();
    PriceBandType->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    MDStreamID->reset();
    MDEntryPx->reset();
    MDEntrySize->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    TickDirection->reset();
    QuoteCondition->reset();
    TradeCondition->reset();
    OpenCloseSettlFlag->reset();
    NoSharesIssued->reset();
    Currency->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    PriceType->reset();
    NetChgPrevDay->reset();
    SellerDays->reset();
    SettlPriceType->reset();
    TradeVolume->reset();
    PriceLimitType->reset();
    LowLimitPrice->reset();
    HighLimitPrice->reset();
    TradingReferencePrice->reset();
    MDEntryID->reset();
    MDInsertDate->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MarketDataIncrementalRefresh_25() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDEntryType;
    delete RptSeq;
    delete PriceBandType;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    delete MDStreamID;
    delete MDEntryPx;
    delete MDEntrySize;
    delete MDEntryDate;
    delete MDEntryTime;
    delete TickDirection;
    delete QuoteCondition;
    delete TradeCondition;
    delete OpenCloseSettlFlag;
    delete NoSharesIssued;
    delete Currency;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete PriceType;
    delete NetChgPrevDay;
    delete SellerDays;
    delete SettlPriceType;
    delete TradeVolume;
    delete PriceLimitType;
    delete LowLimitPrice;
    delete HighLimitPrice;
    delete TradingReferencePrice;
    delete MDEntryID;
    delete MDInsertDate;
    // destructor for sequence MDEntries ends
  }
};
class MarketDataIncrementalRefresh_13 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t, false, false> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t, true, false> *NoMDEntries;
  CopyField<FFUtils::ByteArr, true, false> *MDUpdateAction;
  CopyField<FFUtils::ByteArr, true, false> *MDEntryType;
  CopyField<FFUtils::ByteArr, true, false> *Symbol;
  CopyField<FFUtils::ByteArr, true, false> *SecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityIDSource;
  CopyField<FFUtils::ByteArr, true, true> *SecurityExchange;
  DeltaField<FFUtils::Decimal, false> *MDEntryPx;
  DeltaField<FFUtils::Decimal, false> *MDEntrySize;
  CopyField<uint32_t, true, false> *MDEntryDate;
  CopyField<uint32_t, true, false> *MDEntryTime;
  CopyField<FFUtils::ByteArr, false, false> *TickDirection;
  CopyField<FFUtils::ByteArr, false, false> *TradeCondition;
  CopyField<FFUtils::ByteArr, false, true> *OpenCloseSettlFlag;
  NoOpField<FFUtils::ByteArr, false, false> *OrderID;
  NoOpField<FFUtils::ByteArr, false, false> *TradeID;
  CopyField<FFUtils::ByteArr, false, false> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr, false, false> *MDEntrySeller;
  DefaultField<uint32_t, false, true> *NumberOfOrders;
  DefaultField<uint32_t, false, true> *MDEntryPositionNo;
  DeltaField<FFUtils::Decimal, false> *NetChgPrevDay;
  DeltaField<FFUtils::Decimal, false> *TradeVolume;
  CopyField<FFUtils::ByteArr, false, false> *SettlType;
  CopyField<uint32_t, false, false> *SettlDate;
  CopyField<int32_t, false, false> *SettlPriceType;
  IncrementField<uint32_t, true, false> *RptSeq;
  NoOpField<int32_t, false, false> *PriceBandType;
  NoOpField<int32_t, false, false> *PriceLimitType;
  NoOpField<FFUtils::Decimal, false, false> *LowLimitPrice;
  NoOpField<FFUtils::Decimal, false, false> *HighLimitPrice;
  NoOpField<FFUtils::Decimal, false, false> *TradingReferencePrice;
  NoOpField<FFUtils::Decimal, false, false> *PercentageVar;
  NoOpField<uint32_t, false, false> *NoUnchangedSecurities;
  NoOpField<uint32_t, false, false> *NoNotTradedSecurities;
  NoOpField<uint32_t, false, false> *TotTradedSecurities;
  NoOpField<FFUtils::Decimal, false, false> *CapitalPct;
  NoOpField<FFUtils::Decimal, false, false> *PrevYearVariation;
  NoOpField<uint32_t, false, false> *NoFallingSecurities;
  NoOpField<uint32_t, false, false> *NoRisingSecurities;
  NoOpField<FFUtils::Decimal, false, false> *PercThresholdNormalTrade;
  NoOpField<FFUtils::Decimal, false, false> *PercThresholdCrossTrade;
  NoOpField<uint64_t, false, false> *DailyAvgShares30D;
  NoOpField<FFUtils::Decimal, false, false> *MaximumNormalSharesPerDailyAvgShares30DRatio;
  NoOpField<FFUtils::Decimal, false, false> *MaximumCrossSharesPerDailyAvgShares30DRatio;
  NoOpField<FFUtils::Decimal, false, false> *NormalSharesPerOutstandingSharesRatio;
  NoOpField<FFUtils::Decimal, false, false> *CrossSharesPerOutstandingSharesRatio;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataIncrementalRefresh_13() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    TradeDate = new NoOpField<uint32_t, false, false>(0, "glob_TradeDate");
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t, true, false>(0, "glob_NoMDEntries");
    MDUpdateAction = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_MDUpdateAction");
    MDEntryType = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_MDEntryType");
    Symbol = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_Symbol");
    SecurityID = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_SecurityID");
    SecurityIDSource =
        new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange = new CopyField<FFUtils::ByteArr, true, true>(FFUtils::ByteArr("XBSP"), "glob_SecurityExchange");
    MDEntryPx = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntryPx");
    MDEntrySize = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntrySize");
    MDEntryDate = new CopyField<uint32_t, true, false>(0, "glob_MDEntryDate");
    MDEntryTime = new CopyField<uint32_t, true, false>(0, "glob_MDEntryTime");
    TickDirection = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TickDirection");
    TradeCondition = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeCondition");
    OpenCloseSettlFlag = new CopyField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("5"), "glob_OpenCloseSettlFlag");
    OrderID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OrderID");
    TradeID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeID");
    MDEntryBuyer = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntryBuyer");
    MDEntrySeller = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntrySeller");
    NumberOfOrders = new DefaultField<uint32_t, false, true>(1, "glob_NumberOfOrders");
    MDEntryPositionNo = new DefaultField<uint32_t, false, true>(1, "glob_MDEntryPositionNo");
    NetChgPrevDay = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_NetChgPrevDay");
    TradeVolume = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_TradeVolume");
    SettlType = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SettlType");
    SettlDate = new CopyField<uint32_t, false, false>(0, "glob_SettlDate");
    SettlPriceType = new CopyField<int32_t, false, false>(0, "glob_SettlPriceType");
    RptSeq = new IncrementField<uint32_t, true, false>(0, "glob_RptSeq");
    PriceBandType = new NoOpField<int32_t, false, false>(0, "glob_PriceBandType");
    PriceLimitType = new NoOpField<int32_t, false, false>(0, "glob_PriceLimitType");
    LowLimitPrice = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_LowLimitPrice");
    HighLimitPrice = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_HighLimitPrice");
    TradingReferencePrice =
        new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_TradingReferencePrice");
    PercentageVar = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercentageVar");
    NoUnchangedSecurities = new NoOpField<uint32_t, false, false>(0, "glob_NoUnchangedSecurities");
    NoNotTradedSecurities = new NoOpField<uint32_t, false, false>(0, "glob_NoNotTradedSecurities");
    TotTradedSecurities = new NoOpField<uint32_t, false, false>(0, "glob_TotTradedSecurities");
    CapitalPct = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_CapitalPct");
    PrevYearVariation = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PrevYearVariation");
    NoFallingSecurities = new NoOpField<uint32_t, false, false>(0, "glob_NoFallingSecurities");
    NoRisingSecurities = new NoOpField<uint32_t, false, false>(0, "glob_NoRisingSecurities");
    PercThresholdNormalTrade =
        new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercThresholdNormalTrade");
    PercThresholdCrossTrade =
        new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercThresholdCrossTrade");
    DailyAvgShares30D = new NoOpField<uint64_t, false, false>(0, "glob_DailyAvgShares30D");
    MaximumNormalSharesPerDailyAvgShares30DRatio = new NoOpField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_MaximumNormalSharesPerDailyAvgShares30DRatio");
    MaximumCrossSharesPerDailyAvgShares30DRatio = new NoOpField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_MaximumCrossSharesPerDailyAvgShares30DRatio");
    NormalSharesPerOutstandingSharesRatio = new NoOpField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_NormalSharesPerOutstandingSharesRatio");
    CrossSharesPerOutstandingSharesRatio =
        new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_CrossSharesPerOutstandingSharesRatio");
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode(pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->hasValue() ? NoMDEntries->getValue() : 0;
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      Symbol->decode(input, pmap1);
      SecurityID->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityExchange->decode(input, pmap1);
      MDEntryPx->decode(input);
      MDEntrySize->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettlFlag->decode(input, pmap1);
      OrderID->decode(input);
      TradeID->decode(input);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      NetChgPrevDay->decode(input);
      TradeVolume->decode(input);
      SettlType->decode(input, pmap1);
      SettlDate->decode(input, pmap1);
      SettlPriceType->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      PriceBandType->decode(input);
      PriceLimitType->decode(input);
      LowLimitPrice->decode(input);
      HighLimitPrice->decode(input);
      TradingReferencePrice->decode(input);
      PercentageVar->decode(input);
      NoUnchangedSecurities->decode(input);
      NoNotTradedSecurities->decode(input);
      TotTradedSecurities->decode(input);
      CapitalPct->decode(input);
      PrevYearVariation->decode(input);
      NoFallingSecurities->decode(input);
      NoRisingSecurities->decode(input);
      PercThresholdNormalTrade->decode(input);
      PercThresholdCrossTrade->decode(input);
      DailyAvgShares30D->decode(input);
      MaximumNormalSharesPerDailyAvgShares30DRatio->decode(input);
      MaximumCrossSharesPerDailyAvgShares30DRatio->decode(input);
      NormalSharesPerOutstandingSharesRatio->decode(input);
      CrossSharesPerOutstandingSharesRatio->decode(input);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDEntryType->reset();
    Symbol->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    MDEntryPx->reset();
    MDEntrySize->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    TickDirection->reset();
    TradeCondition->reset();
    OpenCloseSettlFlag->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    NetChgPrevDay->reset();
    TradeVolume->reset();
    SettlType->reset();
    SettlDate->reset();
    SettlPriceType->reset();
    RptSeq->reset();
    PriceBandType->reset();
    PriceLimitType->reset();
    LowLimitPrice->reset();
    HighLimitPrice->reset();
    TradingReferencePrice->reset();
    PercentageVar->reset();
    NoUnchangedSecurities->reset();
    NoNotTradedSecurities->reset();
    TotTradedSecurities->reset();
    CapitalPct->reset();
    PrevYearVariation->reset();
    NoFallingSecurities->reset();
    NoRisingSecurities->reset();
    PercThresholdNormalTrade->reset();
    PercThresholdCrossTrade->reset();
    DailyAvgShares30D->reset();
    MaximumNormalSharesPerDailyAvgShares30DRatio->reset();
    MaximumCrossSharesPerDailyAvgShares30DRatio->reset();
    NormalSharesPerOutstandingSharesRatio->reset();
    CrossSharesPerOutstandingSharesRatio->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MarketDataIncrementalRefresh_13() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDEntryType;
    delete Symbol;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    delete MDEntryPx;
    delete MDEntrySize;
    delete MDEntryDate;
    delete MDEntryTime;
    delete TickDirection;
    delete TradeCondition;
    delete OpenCloseSettlFlag;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete NetChgPrevDay;
    delete TradeVolume;
    delete SettlType;
    delete SettlDate;
    delete SettlPriceType;
    delete RptSeq;
    delete PriceBandType;
    delete PriceLimitType;
    delete LowLimitPrice;
    delete HighLimitPrice;
    delete TradingReferencePrice;
    delete PercentageVar;
    delete NoUnchangedSecurities;
    delete NoNotTradedSecurities;
    delete TotTradedSecurities;
    delete CapitalPct;
    delete PrevYearVariation;
    delete NoFallingSecurities;
    delete NoRisingSecurities;
    delete PercThresholdNormalTrade;
    delete PercThresholdCrossTrade;
    delete DailyAvgShares30D;
    delete MaximumNormalSharesPerDailyAvgShares30DRatio;
    delete MaximumCrossSharesPerDailyAvgShares30DRatio;
    delete NormalSharesPerOutstandingSharesRatio;
    delete CrossSharesPerOutstandingSharesRatio;
    // destructor for sequence MDEntries ends
  }
};
class MarketDataIncrementalRefresh_17 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t, false, false> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t, true, false> *NoMDEntries;
  CopyField<FFUtils::ByteArr, true, false> *MDUpdateAction;
  CopyField<FFUtils::ByteArr, true, false> *MDEntryType;
  IncrementField<uint32_t, true, false> *RptSeq;
  CopyField<int64_t, true, false> *SecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityExchange;
  NoOpField<FFUtils::ByteArr, false, false> *MDStreamID;
  DeltaField<FFUtils::Decimal, false> *MDEntryPx;
  DeltaField<FFUtils::Decimal, false> *MDEntrySize;
  CopyField<uint32_t, true, false> *MDEntryDate;
  CopyField<uint32_t, true, false> *MDEntryTime;
  CopyField<FFUtils::ByteArr, false, false> *TickDirection;
  CopyField<FFUtils::ByteArr, false, false> *QuoteCondition;
  CopyField<FFUtils::ByteArr, false, false> *TradeCondition;
  NoOpField<FFUtils::ByteArr, false, false> *OpenCloseSettlFlag;
  NoOpField<FFUtils::ByteArr, false, false> *Currency;
  NoOpField<FFUtils::ByteArr, false, false> *OrderID;
  NoOpField<FFUtils::ByteArr, false, false> *TradeID;
  CopyField<FFUtils::ByteArr, false, false> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr, false, false> *MDEntrySeller;
  DefaultField<uint32_t, false, true> *NumberOfOrders;
  IncrementField<uint32_t, false, false> *MDEntryPositionNo;
  NoOpField<uint32_t, false, false> *PriceType;
  DeltaField<FFUtils::Decimal, false> *NetChgPrevDay;
  NoOpField<uint32_t, false, false> *SellerDays;
  CopyField<int32_t, false, false> *SettlPriceType;
  DeltaField<FFUtils::Decimal, false> *TradeVolume;
  NoOpField<int32_t, false, false> *PriceLimitType;
  NoOpField<FFUtils::Decimal, false, false> *LowLimitPrice;
  NoOpField<FFUtils::Decimal, false, false> *HighLimitPrice;
  NoOpField<FFUtils::Decimal, false, false> *TradingReferencePrice;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataIncrementalRefresh_17() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    TradeDate = new NoOpField<uint32_t, false, false>(0, "glob_TradeDate");
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t, true, false>(0, "glob_NoMDEntries");
    MDUpdateAction = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_MDUpdateAction");
    MDEntryType = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_MDEntryType");
    RptSeq = new IncrementField<uint32_t, true, false>(0, "glob_RptSeq");
    SecurityID = new CopyField<int64_t, true, false>(0, "glob_SecurityID");
    SecurityIDSource = new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_SecurityExchange");
    MDStreamID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDStreamID");
    MDEntryPx = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntryPx");
    MDEntrySize = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntrySize");
    MDEntryDate = new CopyField<uint32_t, true, false>(0, "glob_MDEntryDate");
    MDEntryTime = new CopyField<uint32_t, true, false>(0, "glob_MDEntryTime");
    TickDirection = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TickDirection");
    QuoteCondition = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_QuoteCondition");
    TradeCondition = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeCondition");
    OpenCloseSettlFlag = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OpenCloseSettlFlag");
    Currency = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Currency");
    OrderID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OrderID");
    TradeID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeID");
    MDEntryBuyer = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntryBuyer");
    MDEntrySeller = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntrySeller");
    NumberOfOrders = new DefaultField<uint32_t, false, true>(1, "glob_NumberOfOrders");
    MDEntryPositionNo = new IncrementField<uint32_t, false, false>(0, "glob_MDEntryPositionNo");
    PriceType = new NoOpField<uint32_t, false, false>(0, "glob_PriceType");
    NetChgPrevDay = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_NetChgPrevDay");
    SellerDays = new NoOpField<uint32_t, false, false>(0, "glob_SellerDays");
    SettlPriceType = new CopyField<int32_t, false, false>(0, "glob_SettlPriceType");
    TradeVolume = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_TradeVolume");
    PriceLimitType = new NoOpField<int32_t, false, false>(0, "glob_PriceLimitType");
    LowLimitPrice = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_LowLimitPrice");
    HighLimitPrice = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_HighLimitPrice");
    TradingReferencePrice =
        new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_TradingReferencePrice");
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode(pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->hasValue() ? NoMDEntries->getValue() : 0;
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      SecurityID->decode(input, pmap1);
      SecurityIDSource->decode(input, pmap1);
      SecurityExchange->decode(input, pmap1);
      MDStreamID->decode(input);
      MDEntryPx->decode(input);
      MDEntrySize->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettlFlag->decode(input);
      Currency->decode(input);
      OrderID->decode(input);
      TradeID->decode(input);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      PriceType->decode(input);
      NetChgPrevDay->decode(input);
      SellerDays->decode(input);
      SettlPriceType->decode(input, pmap1);
      TradeVolume->decode(input);
      PriceLimitType->decode(input);
      LowLimitPrice->decode(input);
      HighLimitPrice->decode(input);
      TradingReferencePrice->decode(input);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDEntryType->reset();
    RptSeq->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    MDStreamID->reset();
    MDEntryPx->reset();
    MDEntrySize->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    TickDirection->reset();
    QuoteCondition->reset();
    TradeCondition->reset();
    OpenCloseSettlFlag->reset();
    Currency->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    PriceType->reset();
    NetChgPrevDay->reset();
    SellerDays->reset();
    SettlPriceType->reset();
    TradeVolume->reset();
    PriceLimitType->reset();
    LowLimitPrice->reset();
    HighLimitPrice->reset();
    TradingReferencePrice->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MarketDataIncrementalRefresh_17() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDEntryType;
    delete RptSeq;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    delete MDStreamID;
    delete MDEntryPx;
    delete MDEntrySize;
    delete MDEntryDate;
    delete MDEntryTime;
    delete TickDirection;
    delete QuoteCondition;
    delete TradeCondition;
    delete OpenCloseSettlFlag;
    delete Currency;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete PriceType;
    delete NetChgPrevDay;
    delete SellerDays;
    delete SettlPriceType;
    delete TradeVolume;
    delete PriceLimitType;
    delete LowLimitPrice;
    delete HighLimitPrice;
    delete TradingReferencePrice;
    // destructor for sequence MDEntries ends
  }
};
class MarketDataIncrementalRefresh_18 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t, false, false> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t, true, false> *NoMDEntries;
  CopyField<FFUtils::ByteArr, true, false> *MDUpdateAction;
  CopyField<FFUtils::ByteArr, true, false> *MDEntryType;
  IncrementField<uint32_t, true, false> *RptSeq;
  CopyField<FFUtils::ByteArr, true, false> *SecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityExchange;
  NoOpField<FFUtils::ByteArr, false, false> *MDStreamID;
  DeltaField<FFUtils::Decimal, false> *MDEntryPx;
  DeltaField<FFUtils::Decimal, false> *MDEntrySize;
  CopyField<uint32_t, true, false> *MDEntryDate;
  CopyField<uint32_t, true, false> *MDEntryTime;
  CopyField<FFUtils::ByteArr, false, false> *TickDirection;
  CopyField<FFUtils::ByteArr, false, false> *QuoteCondition;
  CopyField<FFUtils::ByteArr, false, false> *TradeCondition;
  NoOpField<FFUtils::ByteArr, false, false> *OpenCloseSettlFlag;
  NoOpField<FFUtils::ByteArr, false, false> *Currency;
  NoOpField<FFUtils::ByteArr, false, false> *OrderID;
  NoOpField<FFUtils::ByteArr, false, false> *TradeID;
  CopyField<FFUtils::ByteArr, false, false> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr, false, false> *MDEntrySeller;
  DefaultField<uint32_t, false, true> *NumberOfOrders;
  DefaultField<uint32_t, false, true> *MDEntryPositionNo;
  DeltaField<FFUtils::Decimal, false> *NetChgPrevDay;
  NoOpField<uint32_t, false, false> *SellerDays;
  DeltaField<FFUtils::Decimal, false> *TradeVolume;
  NoOpField<FFUtils::Decimal, false, false> *PercentageVar;
  NoOpField<uint32_t, false, false> *NoUnchangedSecurities;
  NoOpField<uint32_t, false, false> *NoNotTradedSecurities;
  NoOpField<uint32_t, false, false> *TotTradedSecurities;
  NoOpField<FFUtils::Decimal, false, false> *CapitalPct;
  NoOpField<FFUtils::Decimal, false, false> *PrevYearVariation;
  NoOpField<uint32_t, false, false> *NoFallingSecurities;
  NoOpField<uint32_t, false, false> *NoRisingSecurities;
  NoOpField<FFUtils::Decimal, false, false> *PercThresholdNormalTrade;
  NoOpField<FFUtils::Decimal, false, false> *PercThresholdCrossTrade;
  NoOpField<uint64_t, false, false> *DailyAvgShares30D;
  NoOpField<FFUtils::Decimal, false, false> *MaximumNormalSharesPerDailyAvgShares30DRatio;
  NoOpField<FFUtils::Decimal, false, false> *MaximumCrossSharesPerDailyAvgShares30DRatio;
  NoOpField<FFUtils::Decimal, false, false> *NormalSharesPerOutstandingSharesRatio;
  NoOpField<FFUtils::Decimal, false, false> *CrossSharesPerOutstandingSharesRatio;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataIncrementalRefresh_18() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    TradeDate = new NoOpField<uint32_t, false, false>(0, "glob_TradeDate");
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t, true, false>(0, "glob_NoMDEntries");
    MDUpdateAction = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_MDUpdateAction");
    MDEntryType = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_MDEntryType");
    RptSeq = new IncrementField<uint32_t, true, false>(0, "glob_RptSeq");
    SecurityID = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_SecurityID");
    SecurityIDSource = new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_SecurityExchange");
    MDStreamID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDStreamID");
    MDEntryPx = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntryPx");
    MDEntrySize = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntrySize");
    MDEntryDate = new CopyField<uint32_t, true, false>(0, "glob_MDEntryDate");
    MDEntryTime = new CopyField<uint32_t, true, false>(0, "glob_MDEntryTime");
    TickDirection = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TickDirection");
    QuoteCondition = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_QuoteCondition");
    TradeCondition = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeCondition");
    OpenCloseSettlFlag = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OpenCloseSettlFlag");
    Currency = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Currency");
    OrderID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OrderID");
    TradeID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeID");
    MDEntryBuyer = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntryBuyer");
    MDEntrySeller = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntrySeller");
    NumberOfOrders = new DefaultField<uint32_t, false, true>(1, "glob_NumberOfOrders");
    MDEntryPositionNo = new DefaultField<uint32_t, false, true>(1, "glob_MDEntryPositionNo");
    NetChgPrevDay = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_NetChgPrevDay");
    SellerDays = new NoOpField<uint32_t, false, false>(0, "glob_SellerDays");
    TradeVolume = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_TradeVolume");
    PercentageVar = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercentageVar");
    NoUnchangedSecurities = new NoOpField<uint32_t, false, false>(0, "glob_NoUnchangedSecurities");
    NoNotTradedSecurities = new NoOpField<uint32_t, false, false>(0, "glob_NoNotTradedSecurities");
    TotTradedSecurities = new NoOpField<uint32_t, false, false>(0, "glob_TotTradedSecurities");
    CapitalPct = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_CapitalPct");
    PrevYearVariation = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PrevYearVariation");
    NoFallingSecurities = new NoOpField<uint32_t, false, false>(0, "glob_NoFallingSecurities");
    NoRisingSecurities = new NoOpField<uint32_t, false, false>(0, "glob_NoRisingSecurities");
    PercThresholdNormalTrade =
        new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercThresholdNormalTrade");
    PercThresholdCrossTrade =
        new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercThresholdCrossTrade");
    DailyAvgShares30D = new NoOpField<uint64_t, false, false>(0, "glob_DailyAvgShares30D");
    MaximumNormalSharesPerDailyAvgShares30DRatio = new NoOpField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_MaximumNormalSharesPerDailyAvgShares30DRatio");
    MaximumCrossSharesPerDailyAvgShares30DRatio = new NoOpField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_MaximumCrossSharesPerDailyAvgShares30DRatio");
    NormalSharesPerOutstandingSharesRatio = new NoOpField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_NormalSharesPerOutstandingSharesRatio");
    CrossSharesPerOutstandingSharesRatio =
        new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_CrossSharesPerOutstandingSharesRatio");
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode(pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->hasValue() ? NoMDEntries->getValue() : 0;
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      SecurityID->decode(input, pmap1);
      SecurityIDSource->decode(input, pmap1);
      SecurityExchange->decode(input, pmap1);
      MDStreamID->decode(input);
      MDEntryPx->decode(input);
      MDEntrySize->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettlFlag->decode(input);
      Currency->decode(input);
      OrderID->decode(input);
      TradeID->decode(input);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      NetChgPrevDay->decode(input);
      SellerDays->decode(input);
      TradeVolume->decode(input);
      PercentageVar->decode(input);
      NoUnchangedSecurities->decode(input);
      NoNotTradedSecurities->decode(input);
      TotTradedSecurities->decode(input);
      CapitalPct->decode(input);
      PrevYearVariation->decode(input);
      NoFallingSecurities->decode(input);
      NoRisingSecurities->decode(input);
      PercThresholdNormalTrade->decode(input);
      PercThresholdCrossTrade->decode(input);
      DailyAvgShares30D->decode(input);
      MaximumNormalSharesPerDailyAvgShares30DRatio->decode(input);
      MaximumCrossSharesPerDailyAvgShares30DRatio->decode(input);
      NormalSharesPerOutstandingSharesRatio->decode(input);
      CrossSharesPerOutstandingSharesRatio->decode(input);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDEntryType->reset();
    RptSeq->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    MDStreamID->reset();
    MDEntryPx->reset();
    MDEntrySize->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    TickDirection->reset();
    QuoteCondition->reset();
    TradeCondition->reset();
    OpenCloseSettlFlag->reset();
    Currency->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    NetChgPrevDay->reset();
    SellerDays->reset();
    TradeVolume->reset();
    PercentageVar->reset();
    NoUnchangedSecurities->reset();
    NoNotTradedSecurities->reset();
    TotTradedSecurities->reset();
    CapitalPct->reset();
    PrevYearVariation->reset();
    NoFallingSecurities->reset();
    NoRisingSecurities->reset();
    PercThresholdNormalTrade->reset();
    PercThresholdCrossTrade->reset();
    DailyAvgShares30D->reset();
    MaximumNormalSharesPerDailyAvgShares30DRatio->reset();
    MaximumCrossSharesPerDailyAvgShares30DRatio->reset();
    NormalSharesPerOutstandingSharesRatio->reset();
    CrossSharesPerOutstandingSharesRatio->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MarketDataIncrementalRefresh_18() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDEntryType;
    delete RptSeq;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    delete MDStreamID;
    delete MDEntryPx;
    delete MDEntrySize;
    delete MDEntryDate;
    delete MDEntryTime;
    delete TickDirection;
    delete QuoteCondition;
    delete TradeCondition;
    delete OpenCloseSettlFlag;
    delete Currency;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete NetChgPrevDay;
    delete SellerDays;
    delete TradeVolume;
    delete PercentageVar;
    delete NoUnchangedSecurities;
    delete NoNotTradedSecurities;
    delete TotTradedSecurities;
    delete CapitalPct;
    delete PrevYearVariation;
    delete NoFallingSecurities;
    delete NoRisingSecurities;
    delete PercThresholdNormalTrade;
    delete PercThresholdCrossTrade;
    delete DailyAvgShares30D;
    delete MaximumNormalSharesPerDailyAvgShares30DRatio;
    delete MaximumCrossSharesPerDailyAvgShares30DRatio;
    delete NormalSharesPerOutstandingSharesRatio;
    delete CrossSharesPerOutstandingSharesRatio;
    // destructor for sequence MDEntries ends
  }
};
class MarketDataIncrementalRefresh_26 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t, false, false> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t, true, false> *NoMDEntries;
  CopyField<FFUtils::ByteArr, true, false> *MDUpdateAction;
  CopyField<FFUtils::ByteArr, true, false> *MDEntryType;
  IncrementField<uint32_t, false, false> *RptSeq;
  DefaultField<uint32_t, false, false> *PriceBandType;
  CopyField<FFUtils::ByteArr, false, false> *SecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityExchange;
  DefaultField<FFUtils::ByteArr, false, false> *MDStreamID;
  DeltaField<FFUtils::Decimal, false> *MDEntryPx;
  DeltaField<FFUtils::Decimal, false> *MDEntrySize;
  CopyField<uint32_t, true, false> *MDEntryDate;
  CopyField<uint32_t, true, false> *MDEntryTime;
  DefaultField<FFUtils::ByteArr, false, false> *TickDirection;
  DefaultField<FFUtils::ByteArr, false, false> *QuoteCondition;
  DefaultField<FFUtils::ByteArr, false, false> *TradeCondition;
  DefaultField<FFUtils::ByteArr, false, false> *OpenCloseSettlFlag;
  DefaultField<uint64_t, false, false> *NoSharesIssued;
  DefaultField<FFUtils::ByteArr, false, false> *Currency;
  DefaultField<FFUtils::ByteArr, false, false> *OrderID;
  DefaultField<FFUtils::ByteArr, false, false> *TradeID;
  CopyField<FFUtils::ByteArr, false, false> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr, false, false> *MDEntrySeller;
  DefaultField<uint32_t, false, false> *NumberOfOrders;
  DeltaField<uint32_t, false> *MDEntryPositionNo;
  DefaultField<FFUtils::Decimal, false, false> *NetChgPrevDay;
  DefaultField<uint32_t, false, false> *SellerDays;
  DefaultField<uint64_t, false, false> *TradeVolume;
  DefaultField<FFUtils::Decimal, false, false> *PercentageVar;
  DefaultField<uint32_t, false, false> *NoUnchangedSecurities;
  DefaultField<uint32_t, false, false> *NoNotTradedSecurities;
  DefaultField<uint32_t, false, false> *TotTradedSecurities;
  DefaultField<FFUtils::Decimal, false, false> *CapitalPct;
  DefaultField<FFUtils::Decimal, false, false> *PrevYearVariation;
  DefaultField<uint32_t, false, false> *NoFallingSecurities;
  DefaultField<uint32_t, false, false> *NoRisingSecurities;
  DefaultField<FFUtils::Decimal, false, false> *PercThresholdNormalTrade;
  DefaultField<FFUtils::Decimal, false, false> *PercThresholdCrossTrade;
  DefaultField<uint64_t, false, false> *DailyAvgShares30D;
  DefaultField<FFUtils::Decimal, false, false> *MaximumNormalSharesPerDailyAvgShares30DRatio;
  DefaultField<FFUtils::Decimal, false, false> *MaximumCrossSharesPerDailyAvgShares30DRatio;
  DefaultField<FFUtils::Decimal, false, false> *NormalSharesPerOutstandingSharesRatio;
  DefaultField<FFUtils::Decimal, false, false> *CrossSharesPerOutstandingSharesRatio;
  DefaultField<uint64_t, false, false> *MDEntryID;
  DefaultField<uint32_t, false, false> *MDInsertDate;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataIncrementalRefresh_26() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    TradeDate = new NoOpField<uint32_t, false, false>(0, "glob_TradeDate");
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t, true, false>(0, "glob_NoMDEntries");
    MDUpdateAction = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_MDUpdateAction");
    MDEntryType = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_MDEntryType");
    RptSeq = new IncrementField<uint32_t, false, false>(0, "glob_RptSeq");
    PriceBandType = new DefaultField<uint32_t, false, false>(0, "glob_PriceBandType");
    SecurityID = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityID");
    SecurityIDSource = new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_SecurityExchange");
    MDStreamID = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDStreamID");
    MDEntryPx = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntryPx");
    MDEntrySize = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntrySize");
    MDEntryDate = new CopyField<uint32_t, true, false>(0, "glob_MDEntryDate");
    MDEntryTime = new CopyField<uint32_t, true, false>(0, "glob_MDEntryTime");
    TickDirection = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TickDirection");
    QuoteCondition = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_QuoteCondition");
    TradeCondition = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeCondition");
    OpenCloseSettlFlag =
        new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OpenCloseSettlFlag");
    NoSharesIssued = new DefaultField<uint64_t, false, false>(0, "glob_NoSharesIssued");
    Currency = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Currency");
    OrderID = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OrderID");
    TradeID = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeID");
    MDEntryBuyer = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntryBuyer");
    MDEntrySeller = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntrySeller");
    NumberOfOrders = new DefaultField<uint32_t, false, false>(0, "glob_NumberOfOrders");
    MDEntryPositionNo = new DeltaField<uint32_t, false>(0, "glob_MDEntryPositionNo");
    NetChgPrevDay = new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_NetChgPrevDay");
    SellerDays = new DefaultField<uint32_t, false, false>(0, "glob_SellerDays");
    TradeVolume = new DefaultField<uint64_t, false, false>(0, "glob_TradeVolume");
    PercentageVar = new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercentageVar");
    NoUnchangedSecurities = new DefaultField<uint32_t, false, false>(0, "glob_NoUnchangedSecurities");
    NoNotTradedSecurities = new DefaultField<uint32_t, false, false>(0, "glob_NoNotTradedSecurities");
    TotTradedSecurities = new DefaultField<uint32_t, false, false>(0, "glob_TotTradedSecurities");
    CapitalPct = new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_CapitalPct");
    PrevYearVariation = new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PrevYearVariation");
    NoFallingSecurities = new DefaultField<uint32_t, false, false>(0, "glob_NoFallingSecurities");
    NoRisingSecurities = new DefaultField<uint32_t, false, false>(0, "glob_NoRisingSecurities");
    PercThresholdNormalTrade =
        new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercThresholdNormalTrade");
    PercThresholdCrossTrade =
        new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercThresholdCrossTrade");
    DailyAvgShares30D = new DefaultField<uint64_t, false, false>(0, "glob_DailyAvgShares30D");
    MaximumNormalSharesPerDailyAvgShares30DRatio = new DefaultField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_MaximumNormalSharesPerDailyAvgShares30DRatio");
    MaximumCrossSharesPerDailyAvgShares30DRatio = new DefaultField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_MaximumCrossSharesPerDailyAvgShares30DRatio");
    NormalSharesPerOutstandingSharesRatio = new DefaultField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_NormalSharesPerOutstandingSharesRatio");
    CrossSharesPerOutstandingSharesRatio = new DefaultField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_CrossSharesPerOutstandingSharesRatio");
    MDEntryID = new DefaultField<uint64_t, false, false>(0, "glob_MDEntryID");
    MDInsertDate = new DefaultField<uint32_t, false, false>(0, "glob_MDInsertDate");
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode(pmap0);
    TradeDate->decode(input);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->hasValue() ? NoMDEntries->getValue() : 0;
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDUpdateAction->decode(input, pmap1);
      MDEntryType->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      PriceBandType->decode(input, pmap1);
      SecurityID->decode(input, pmap1);
      SecurityIDSource->decode(input, pmap1);
      SecurityExchange->decode(input, pmap1);
      MDStreamID->decode(input, pmap1);
      MDEntryPx->decode(input);
      MDEntrySize->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettlFlag->decode(input, pmap1);
      NoSharesIssued->decode(input, pmap1);
      Currency->decode(input, pmap1);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input);
      NetChgPrevDay->decode(input, pmap1);
      SellerDays->decode(input, pmap1);
      TradeVolume->decode(input, pmap1);
      PercentageVar->decode(input, pmap1);
      NoUnchangedSecurities->decode(input, pmap1);
      NoNotTradedSecurities->decode(input, pmap1);
      TotTradedSecurities->decode(input, pmap1);
      CapitalPct->decode(input, pmap1);
      PrevYearVariation->decode(input, pmap1);
      NoFallingSecurities->decode(input, pmap1);
      NoRisingSecurities->decode(input, pmap1);
      PercThresholdNormalTrade->decode(input, pmap1);
      PercThresholdCrossTrade->decode(input, pmap1);
      DailyAvgShares30D->decode(input, pmap1);
      MaximumNormalSharesPerDailyAvgShares30DRatio->decode(input, pmap1);
      MaximumCrossSharesPerDailyAvgShares30DRatio->decode(input, pmap1);
      NormalSharesPerOutstandingSharesRatio->decode(input, pmap1);
      CrossSharesPerOutstandingSharesRatio->decode(input, pmap1);
      MDEntryID->decode(input, pmap1);
      MDInsertDate->decode(input, pmap1);

      process();  // manually added
    }
    // decode for sequence MDEntries ends
    process_end();  // manually added
  }

  // process : TO be done in corresponding cpp file manually
  void process();
  void process_end();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDEntryType->reset();
    RptSeq->reset();
    PriceBandType->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    MDStreamID->reset();
    MDEntryPx->reset();
    MDEntrySize->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    TickDirection->reset();
    QuoteCondition->reset();
    TradeCondition->reset();
    OpenCloseSettlFlag->reset();
    NoSharesIssued->reset();
    Currency->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    NetChgPrevDay->reset();
    SellerDays->reset();
    TradeVolume->reset();
    PercentageVar->reset();
    NoUnchangedSecurities->reset();
    NoNotTradedSecurities->reset();
    TotTradedSecurities->reset();
    CapitalPct->reset();
    PrevYearVariation->reset();
    NoFallingSecurities->reset();
    NoRisingSecurities->reset();
    PercThresholdNormalTrade->reset();
    PercThresholdCrossTrade->reset();
    DailyAvgShares30D->reset();
    MaximumNormalSharesPerDailyAvgShares30DRatio->reset();
    MaximumCrossSharesPerDailyAvgShares30DRatio->reset();
    NormalSharesPerOutstandingSharesRatio->reset();
    CrossSharesPerOutstandingSharesRatio->reset();
    MDEntryID->reset();
    MDInsertDate->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MarketDataIncrementalRefresh_26() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDEntryType;
    delete RptSeq;
    delete PriceBandType;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    delete MDStreamID;
    delete MDEntryPx;
    delete MDEntrySize;
    delete MDEntryDate;
    delete MDEntryTime;
    delete TickDirection;
    delete QuoteCondition;
    delete TradeCondition;
    delete OpenCloseSettlFlag;
    delete NoSharesIssued;
    delete Currency;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete NetChgPrevDay;
    delete SellerDays;
    delete TradeVolume;
    delete PercentageVar;
    delete NoUnchangedSecurities;
    delete NoNotTradedSecurities;
    delete TotTradedSecurities;
    delete CapitalPct;
    delete PrevYearVariation;
    delete NoFallingSecurities;
    delete NoRisingSecurities;
    delete PercThresholdNormalTrade;
    delete PercThresholdCrossTrade;
    delete DailyAvgShares30D;
    delete MaximumNormalSharesPerDailyAvgShares30DRatio;
    delete MaximumCrossSharesPerDailyAvgShares30DRatio;
    delete NormalSharesPerOutstandingSharesRatio;
    delete CrossSharesPerOutstandingSharesRatio;
    delete MDEntryID;
    delete MDInsertDate;
    // destructor for sequence MDEntries ends
  }
};
class MarketDataSnapshotFullRefresh_14 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  NoOpField<uint32_t, false, false> *LastMsgSeqNumProcessed;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<int32_t, true, false> *TotNumReports;
  NoOpField<uint32_t, false, false> *TradeDate;
  NoOpField<FFUtils::Decimal, false, false> *NetChgPrevDay;
  NoOpField<int32_t, false, false> *MarketDepth;
  NoOpField<FFUtils::ByteArr, true, false> *Symbol;
  NoOpField<FFUtils::ByteArr, true, false> *SecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr, true, true> *SecurityExchange;
  // fields inside sequence MDEntries
  NoOpField<uint32_t, true, false> *NoMDEntries;
  CopyField<FFUtils::ByteArr, true, false> *MDEntryType;
  DeltaField<FFUtils::Decimal, false> *MDEntryPx;
  DeltaField<FFUtils::Decimal, false> *MDEntrySize;
  CopyField<uint32_t, true, false> *MDEntryDate;
  CopyField<uint32_t, true, false> *MDEntryTime;
  CopyField<FFUtils::ByteArr, false, false> *TickDirection;
  NoOpField<FFUtils::ByteArr, false, false> *TradingSessionSubID;
  NoOpField<int32_t, false, false> *SecurityTradingStatus;
  NoOpField<uint64_t, false, false> *TradSesOpenTime;
  CopyField<FFUtils::ByteArr, false, false> *TradeCondition;
  CopyField<FFUtils::ByteArr, false, true> *OpenCloseSettlFlag;
  NoOpField<FFUtils::ByteArr, false, false> *OrderID;
  NoOpField<FFUtils::ByteArr, false, false> *TradeID;
  CopyField<FFUtils::ByteArr, false, false> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr, false, false> *MDEntrySeller;
  DefaultField<uint32_t, false, true> *NumberOfOrders;
  DefaultField<uint32_t, false, true> *MDEntryPositionNo;
  DeltaField<FFUtils::Decimal, false> *TradeVolume;
  CopyField<FFUtils::ByteArr, false, false> *SettlType;
  CopyField<uint32_t, false, false> *SettlDate;
  CopyField<int32_t, false, false> *SettlPriceType;
  CopyField<uint32_t, false, false> *RptSeq;
  NoOpField<int32_t, false, false> *PriceBandType;
  NoOpField<int32_t, false, false> *PriceLimitType;
  NoOpField<FFUtils::Decimal, false, false> *LowLimitPrice;
  NoOpField<FFUtils::Decimal, false, false> *HighLimitPrice;
  NoOpField<FFUtils::Decimal, false, false> *TradingReferencePrice;
  NoOpField<FFUtils::Decimal, false, false> *PercentageVar;
  NoOpField<uint32_t, false, false> *NoUnchangedSecurities;
  NoOpField<uint32_t, false, false> *NoNotTradedSecurities;
  NoOpField<uint32_t, false, false> *TotTradedSecurities;
  NoOpField<FFUtils::Decimal, false, false> *CapitalPct;
  NoOpField<FFUtils::Decimal, false, false> *PrevYearVariation;
  NoOpField<uint32_t, false, false> *NoFallingSecurities;
  NoOpField<uint32_t, false, false> *NoRisingSecurities;
  NoOpField<FFUtils::Decimal, false, false> *PercThresholdNormalTrade;
  NoOpField<FFUtils::Decimal, false, false> *PercThresholdCrossTrade;
  NoOpField<uint64_t, false, false> *DailyAvgShares30D;
  NoOpField<FFUtils::Decimal, false, false> *MaximumNormalSharesPerDailyAvgShares30DRatio;
  NoOpField<FFUtils::Decimal, false, false> *MaximumCrossSharesPerDailyAvgShares30DRatio;
  NoOpField<FFUtils::Decimal, false, false> *NormalSharesPerOutstandingSharesRatio;
  NoOpField<FFUtils::Decimal, false, false> *CrossSharesPerOutstandingSharesRatio;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataSnapshotFullRefresh_14() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    LastMsgSeqNumProcessed = new NoOpField<uint32_t, false, false>(0, "glob_LastMsgSeqNumProcessed");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    TotNumReports = new NoOpField<int32_t, true, false>(0, "glob_TotNumReports");
    TradeDate = new NoOpField<uint32_t, false, false>(0, "glob_TradeDate");
    NetChgPrevDay = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_NetChgPrevDay");
    MarketDepth = new NoOpField<int32_t, false, false>(0, "glob_MarketDepth");
    Symbol = new NoOpField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_Symbol");
    SecurityID = new NoOpField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_SecurityID");
    SecurityIDSource =
        new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange =
        new DefaultField<FFUtils::ByteArr, true, true>(FFUtils::ByteArr("XBSP"), "glob_SecurityExchange");
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t, true, false>(0, "glob_NoMDEntries");
    MDEntryType = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_MDEntryType");
    MDEntryPx = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntryPx");
    MDEntrySize = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntrySize");
    MDEntryDate = new CopyField<uint32_t, true, false>(0, "glob_MDEntryDate");
    MDEntryTime = new CopyField<uint32_t, true, false>(0, "glob_MDEntryTime");
    TickDirection = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TickDirection");
    TradingSessionSubID =
        new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradingSessionSubID");
    SecurityTradingStatus = new NoOpField<int32_t, false, false>(0, "glob_SecurityTradingStatus");
    TradSesOpenTime = new NoOpField<uint64_t, false, false>(0, "glob_TradSesOpenTime");
    TradeCondition = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeCondition");
    OpenCloseSettlFlag = new CopyField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("5"), "glob_OpenCloseSettlFlag");
    OrderID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OrderID");
    TradeID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeID");
    MDEntryBuyer = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntryBuyer");
    MDEntrySeller = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntrySeller");
    NumberOfOrders = new DefaultField<uint32_t, false, true>(1, "glob_NumberOfOrders");
    MDEntryPositionNo = new DefaultField<uint32_t, false, true>(1, "glob_MDEntryPositionNo");
    TradeVolume = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_TradeVolume");
    SettlType = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SettlType");
    SettlDate = new CopyField<uint32_t, false, false>(0, "glob_SettlDate");
    SettlPriceType = new CopyField<int32_t, false, false>(0, "glob_SettlPriceType");
    RptSeq = new CopyField<uint32_t, false, false>(0, "glob_RptSeq");
    PriceBandType = new NoOpField<int32_t, false, false>(0, "glob_PriceBandType");
    PriceLimitType = new NoOpField<int32_t, false, false>(0, "glob_PriceLimitType");
    LowLimitPrice = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_LowLimitPrice");
    HighLimitPrice = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_HighLimitPrice");
    TradingReferencePrice =
        new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_TradingReferencePrice");
    PercentageVar = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercentageVar");
    NoUnchangedSecurities = new NoOpField<uint32_t, false, false>(0, "glob_NoUnchangedSecurities");
    NoNotTradedSecurities = new NoOpField<uint32_t, false, false>(0, "glob_NoNotTradedSecurities");
    TotTradedSecurities = new NoOpField<uint32_t, false, false>(0, "glob_TotTradedSecurities");
    CapitalPct = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_CapitalPct");
    PrevYearVariation = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PrevYearVariation");
    NoFallingSecurities = new NoOpField<uint32_t, false, false>(0, "glob_NoFallingSecurities");
    NoRisingSecurities = new NoOpField<uint32_t, false, false>(0, "glob_NoRisingSecurities");
    PercThresholdNormalTrade =
        new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercThresholdNormalTrade");
    PercThresholdCrossTrade =
        new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercThresholdCrossTrade");
    DailyAvgShares30D = new NoOpField<uint64_t, false, false>(0, "glob_DailyAvgShares30D");
    MaximumNormalSharesPerDailyAvgShares30DRatio = new NoOpField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_MaximumNormalSharesPerDailyAvgShares30DRatio");
    MaximumCrossSharesPerDailyAvgShares30DRatio = new NoOpField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_MaximumCrossSharesPerDailyAvgShares30DRatio");
    NormalSharesPerOutstandingSharesRatio = new NoOpField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_NormalSharesPerOutstandingSharesRatio");
    CrossSharesPerOutstandingSharesRatio =
        new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_CrossSharesPerOutstandingSharesRatio");
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    LastMsgSeqNumProcessed->decode(input);
    ApplVerID->decode(pmap0);
    TotNumReports->decode(input);
    TradeDate->decode(input);
    NetChgPrevDay->decode(input);
    MarketDepth->decode(input);
    Symbol->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode();
    SecurityExchange->decode(input, pmap0);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->hasValue() ? NoMDEntries->getValue() : 0;
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDEntryType->decode(input, pmap1);
      MDEntryPx->decode(input);
      MDEntrySize->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      TradingSessionSubID->decode(input);
      SecurityTradingStatus->decode(input);
      TradSesOpenTime->decode(input);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettlFlag->decode(input, pmap1);
      OrderID->decode(input);
      TradeID->decode(input);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      TradeVolume->decode(input);
      SettlType->decode(input, pmap1);
      SettlDate->decode(input, pmap1);
      SettlPriceType->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      PriceBandType->decode(input);
      PriceLimitType->decode(input);
      LowLimitPrice->decode(input);
      HighLimitPrice->decode(input);
      TradingReferencePrice->decode(input);
      PercentageVar->decode(input);
      NoUnchangedSecurities->decode(input);
      NoNotTradedSecurities->decode(input);
      TotTradedSecurities->decode(input);
      CapitalPct->decode(input);
      PrevYearVariation->decode(input);
      NoFallingSecurities->decode(input);
      NoRisingSecurities->decode(input);
      PercThresholdNormalTrade->decode(input);
      PercThresholdCrossTrade->decode(input);
      DailyAvgShares30D->decode(input);
      MaximumNormalSharesPerDailyAvgShares30DRatio->decode(input);
      MaximumCrossSharesPerDailyAvgShares30DRatio->decode(input);
      NormalSharesPerOutstandingSharesRatio->decode(input);
      CrossSharesPerOutstandingSharesRatio->decode(input);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    LastMsgSeqNumProcessed->reset();
    ApplVerID->reset();
    TotNumReports->reset();
    TradeDate->reset();
    NetChgPrevDay->reset();
    MarketDepth->reset();
    Symbol->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDEntryType->reset();
    MDEntryPx->reset();
    MDEntrySize->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    TickDirection->reset();
    TradingSessionSubID->reset();
    SecurityTradingStatus->reset();
    TradSesOpenTime->reset();
    TradeCondition->reset();
    OpenCloseSettlFlag->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    TradeVolume->reset();
    SettlType->reset();
    SettlDate->reset();
    SettlPriceType->reset();
    RptSeq->reset();
    PriceBandType->reset();
    PriceLimitType->reset();
    LowLimitPrice->reset();
    HighLimitPrice->reset();
    TradingReferencePrice->reset();
    PercentageVar->reset();
    NoUnchangedSecurities->reset();
    NoNotTradedSecurities->reset();
    TotTradedSecurities->reset();
    CapitalPct->reset();
    PrevYearVariation->reset();
    NoFallingSecurities->reset();
    NoRisingSecurities->reset();
    PercThresholdNormalTrade->reset();
    PercThresholdCrossTrade->reset();
    DailyAvgShares30D->reset();
    MaximumNormalSharesPerDailyAvgShares30DRatio->reset();
    MaximumCrossSharesPerDailyAvgShares30DRatio->reset();
    NormalSharesPerOutstandingSharesRatio->reset();
    CrossSharesPerOutstandingSharesRatio->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MarketDataSnapshotFullRefresh_14() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete LastMsgSeqNumProcessed;
    delete ApplVerID;
    delete TotNumReports;
    delete TradeDate;
    delete NetChgPrevDay;
    delete MarketDepth;
    delete Symbol;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDEntryType;
    delete MDEntryPx;
    delete MDEntrySize;
    delete MDEntryDate;
    delete MDEntryTime;
    delete TickDirection;
    delete TradingSessionSubID;
    delete SecurityTradingStatus;
    delete TradSesOpenTime;
    delete TradeCondition;
    delete OpenCloseSettlFlag;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete TradeVolume;
    delete SettlType;
    delete SettlDate;
    delete SettlPriceType;
    delete RptSeq;
    delete PriceBandType;
    delete PriceLimitType;
    delete LowLimitPrice;
    delete HighLimitPrice;
    delete TradingReferencePrice;
    delete PercentageVar;
    delete NoUnchangedSecurities;
    delete NoNotTradedSecurities;
    delete TotTradedSecurities;
    delete CapitalPct;
    delete PrevYearVariation;
    delete NoFallingSecurities;
    delete NoRisingSecurities;
    delete PercThresholdNormalTrade;
    delete PercThresholdCrossTrade;
    delete DailyAvgShares30D;
    delete MaximumNormalSharesPerDailyAvgShares30DRatio;
    delete MaximumCrossSharesPerDailyAvgShares30DRatio;
    delete NormalSharesPerOutstandingSharesRatio;
    delete CrossSharesPerOutstandingSharesRatio;
    // destructor for sequence MDEntries ends
  }
};
class MarketDataSnapshotFullRefresh_27 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  NoOpField<uint32_t, false, false> *LastMsgSeqNumProcessed;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<int32_t, true, false> *TotNumReports;
  DefaultField<uint32_t, false, false> *TradeDate;
  DefaultField<FFUtils::Decimal, false, false> *NetChgPrevDay;
  DefaultField<int32_t, false, true> *MarketDepth;
  NoOpField<int64_t, true, false> *SecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityExchange;
  // fields inside sequence MDEntries
  NoOpField<uint32_t, true, false> *NoMDEntries;
  CopyField<FFUtils::ByteArr, true, false> *MDEntryType;
  IncrementField<uint32_t, false, false> *RptSeq;
  DefaultField<uint32_t, false, false> *PriceBandType;
  DefaultField<FFUtils::ByteArr, false, false> *MDStreamID;
  DeltaField<FFUtils::Decimal, false> *MDEntryPx;
  DeltaField<FFUtils::Decimal, false> *MDEntrySize;
  CopyField<uint32_t, true, false> *MDEntryDate;
  CopyField<uint32_t, true, false> *MDEntryTime;
  DefaultField<FFUtils::ByteArr, false, false> *TickDirection;
  DefaultField<uint32_t, false, false> *SecurityTradingStatus;
  DefaultField<FFUtils::ByteArr, false, false> *TradingSessionSubID;
  DefaultField<uint64_t, false, false> *TradSesOpenTime;
  DefaultField<FFUtils::ByteArr, false, false> *QuoteCondition;
  DefaultField<FFUtils::ByteArr, false, false> *TradeCondition;
  DefaultField<FFUtils::ByteArr, false, false> *OpenCloseSettlFlag;
  DefaultField<uint64_t, false, false> *NoSharesIssued;
  CopyField<FFUtils::ByteArr, false, false> *Currency;
  DefaultField<FFUtils::ByteArr, false, false> *OrderID;
  DefaultField<FFUtils::ByteArr, false, false> *TradeID;
  CopyField<FFUtils::ByteArr, false, false> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr, false, false> *MDEntrySeller;
  DefaultField<uint32_t, false, false> *NumberOfOrders;
  IncrementField<uint32_t, false, false> *MDEntryPositionNo;
  DefaultField<uint32_t, false, false> *PriceType;
  DefaultField<uint32_t, false, false> *SellerDays;
  DefaultField<uint32_t, false, false> *SettlPriceType;
  DefaultField<uint64_t, false, false> *TradeVolume;
  DefaultField<uint32_t, false, false> *PriceLimitType;
  DefaultField<FFUtils::Decimal, false, false> *LowLimitPrice;
  DefaultField<FFUtils::Decimal, false, false> *HighLimitPrice;
  DefaultField<FFUtils::Decimal, false, false> *TradingReferencePrice;
  DefaultField<uint64_t, false, false> *MDEntryID;
  DefaultField<uint32_t, false, false> *MDInsertDate;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataSnapshotFullRefresh_27() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    LastMsgSeqNumProcessed = new NoOpField<uint32_t, false, false>(0, "glob_LastMsgSeqNumProcessed");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    TotNumReports = new NoOpField<int32_t, true, false>(0, "glob_TotNumReports");
    TradeDate = new DefaultField<uint32_t, false, false>(0, "glob_TradeDate");
    NetChgPrevDay = new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_NetChgPrevDay");
    MarketDepth = new DefaultField<int32_t, false, true>(0, "glob_MarketDepth");
    SecurityID = new NoOpField<int64_t, true, false>(0, "glob_SecurityID");
    SecurityIDSource = new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_SecurityExchange");
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t, true, false>(0, "glob_NoMDEntries");
    MDEntryType = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_MDEntryType");
    RptSeq = new IncrementField<uint32_t, false, false>(0, "glob_RptSeq");
    PriceBandType = new DefaultField<uint32_t, false, false>(0, "glob_PriceBandType");
    MDStreamID = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDStreamID");
    MDEntryPx = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntryPx");
    MDEntrySize = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntrySize");
    MDEntryDate = new CopyField<uint32_t, true, false>(0, "glob_MDEntryDate");
    MDEntryTime = new CopyField<uint32_t, true, false>(0, "glob_MDEntryTime");
    TickDirection = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TickDirection");
    SecurityTradingStatus = new DefaultField<uint32_t, false, false>(0, "glob_SecurityTradingStatus");
    TradingSessionSubID =
        new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradingSessionSubID");
    TradSesOpenTime = new DefaultField<uint64_t, false, false>(0, "glob_TradSesOpenTime");
    QuoteCondition = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_QuoteCondition");
    TradeCondition = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeCondition");
    OpenCloseSettlFlag =
        new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OpenCloseSettlFlag");
    NoSharesIssued = new DefaultField<uint64_t, false, false>(0, "glob_NoSharesIssued");
    Currency = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Currency");
    OrderID = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OrderID");
    TradeID = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeID");
    MDEntryBuyer = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntryBuyer");
    MDEntrySeller = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntrySeller");
    NumberOfOrders = new DefaultField<uint32_t, false, false>(0, "glob_NumberOfOrders");
    MDEntryPositionNo = new IncrementField<uint32_t, false, false>(0, "glob_MDEntryPositionNo");
    PriceType = new DefaultField<uint32_t, false, false>(0, "glob_PriceType");
    SellerDays = new DefaultField<uint32_t, false, false>(0, "glob_SellerDays");
    SettlPriceType = new DefaultField<uint32_t, false, false>(0, "glob_SettlPriceType");
    TradeVolume = new DefaultField<uint64_t, false, false>(0, "glob_TradeVolume");
    PriceLimitType = new DefaultField<uint32_t, false, false>(0, "glob_PriceLimitType");
    LowLimitPrice = new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_LowLimitPrice");
    HighLimitPrice = new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_HighLimitPrice");
    TradingReferencePrice =
        new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_TradingReferencePrice");
    MDEntryID = new DefaultField<uint64_t, false, false>(0, "glob_MDEntryID");
    MDInsertDate = new DefaultField<uint32_t, false, false>(0, "glob_MDInsertDate");
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    LastMsgSeqNumProcessed->decode(input);
    ApplVerID->decode(pmap0);
    TotNumReports->decode(input);
    TradeDate->decode(input, pmap0);
    NetChgPrevDay->decode(input, pmap0);
    MarketDepth->decode(input, pmap0);
    SecurityID->decode(input);
    SecurityIDSource->decode(input, pmap0);
    SecurityExchange->decode(input, pmap0);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    // manually added
    //      bool sec_found_ = findSecurity( );

    int NoMDEntries_len = NoMDEntries->hasValue() ? NoMDEntries->getValue() : 0;
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDEntryType->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      PriceBandType->decode(input, pmap1);
      MDStreamID->decode(input, pmap1);
      MDEntryPx->decode(input);
      MDEntrySize->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      SecurityTradingStatus->decode(input, pmap1);
      TradingSessionSubID->decode(input, pmap1);
      TradSesOpenTime->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettlFlag->decode(input, pmap1);
      NoSharesIssued->decode(input, pmap1);
      Currency->decode(input, pmap1);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      PriceType->decode(input, pmap1);
      SellerDays->decode(input, pmap1);
      SettlPriceType->decode(input, pmap1);
      TradeVolume->decode(input, pmap1);
      PriceLimitType->decode(input, pmap1);
      LowLimitPrice->decode(input, pmap1);
      HighLimitPrice->decode(input, pmap1);
      TradingReferencePrice->decode(input, pmap1);
      MDEntryID->decode(input, pmap1);
      MDInsertDate->decode(input, pmap1);

      //        if( sec_found_ ) //manually added
      //          process( );
    }
    // decode for sequence MDEntries ends
    //      if( sec_found_ )//manually added
    //        process_end( );
  }

  // process : TO be done in corresponding cpp file manually
  void process();
  void process_end();
  bool findSecurity();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    LastMsgSeqNumProcessed->reset();
    ApplVerID->reset();
    TotNumReports->reset();
    TradeDate->reset();
    NetChgPrevDay->reset();
    MarketDepth->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDEntryType->reset();
    RptSeq->reset();
    PriceBandType->reset();
    MDStreamID->reset();
    MDEntryPx->reset();
    MDEntrySize->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    TickDirection->reset();
    SecurityTradingStatus->reset();
    TradingSessionSubID->reset();
    TradSesOpenTime->reset();
    QuoteCondition->reset();
    TradeCondition->reset();
    OpenCloseSettlFlag->reset();
    NoSharesIssued->reset();
    Currency->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    PriceType->reset();
    SellerDays->reset();
    SettlPriceType->reset();
    TradeVolume->reset();
    PriceLimitType->reset();
    LowLimitPrice->reset();
    HighLimitPrice->reset();
    TradingReferencePrice->reset();
    MDEntryID->reset();
    MDInsertDate->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MarketDataSnapshotFullRefresh_27() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete LastMsgSeqNumProcessed;
    delete ApplVerID;
    delete TotNumReports;
    delete TradeDate;
    delete NetChgPrevDay;
    delete MarketDepth;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDEntryType;
    delete RptSeq;
    delete PriceBandType;
    delete MDStreamID;
    delete MDEntryPx;
    delete MDEntrySize;
    delete MDEntryDate;
    delete MDEntryTime;
    delete TickDirection;
    delete SecurityTradingStatus;
    delete TradingSessionSubID;
    delete TradSesOpenTime;
    delete QuoteCondition;
    delete TradeCondition;
    delete OpenCloseSettlFlag;
    delete NoSharesIssued;
    delete Currency;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete PriceType;
    delete SellerDays;
    delete SettlPriceType;
    delete TradeVolume;
    delete PriceLimitType;
    delete LowLimitPrice;
    delete HighLimitPrice;
    delete TradingReferencePrice;
    delete MDEntryID;
    delete MDInsertDate;
    // destructor for sequence MDEntries ends
  }
};
class MarketDataSnapshotFullRefresh_19 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  NoOpField<uint32_t, false, false> *LastMsgSeqNumProcessed;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<int32_t, true, false> *TotNumReports;
  NoOpField<uint32_t, false, false> *TradeDate;
  NoOpField<FFUtils::Decimal, false, false> *NetChgPrevDay;
  NoOpField<int32_t, false, false> *MarketDepth;
  NoOpField<int64_t, true, false> *SecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityExchange;
  // fields inside sequence MDEntries
  NoOpField<uint32_t, true, false> *NoMDEntries;
  CopyField<FFUtils::ByteArr, true, false> *MDEntryType;
  CopyField<uint32_t, false, false> *RptSeq;
  NoOpField<FFUtils::ByteArr, false, false> *MDStreamID;
  DeltaField<FFUtils::Decimal, false> *MDEntryPx;
  DeltaField<FFUtils::Decimal, false> *MDEntrySize;
  CopyField<uint32_t, true, false> *MDEntryDate;
  CopyField<uint32_t, true, false> *MDEntryTime;
  CopyField<FFUtils::ByteArr, false, false> *TickDirection;
  NoOpField<int32_t, false, false> *SecurityTradingStatus;
  NoOpField<FFUtils::ByteArr, false, false> *TradingSessionSubID;
  NoOpField<uint64_t, false, false> *TradSesOpenTime;
  CopyField<FFUtils::ByteArr, false, false> *QuoteCondition;
  CopyField<FFUtils::ByteArr, false, false> *TradeCondition;
  NoOpField<FFUtils::ByteArr, false, false> *OpenCloseSettlFlag;
  CopyField<FFUtils::ByteArr, false, false> *Currency;
  NoOpField<FFUtils::ByteArr, false, false> *OrderID;
  NoOpField<FFUtils::ByteArr, false, false> *TradeID;
  CopyField<FFUtils::ByteArr, false, false> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr, false, false> *MDEntrySeller;
  DefaultField<uint32_t, false, true> *NumberOfOrders;
  IncrementField<uint32_t, false, false> *MDEntryPositionNo;
  NoOpField<uint32_t, false, false> *PriceType;
  NoOpField<uint32_t, false, false> *SellerDays;
  CopyField<uint32_t, false, false> *SettlPriceType;
  DeltaField<FFUtils::Decimal, false> *TradeVolume;
  NoOpField<int32_t, false, false> *PriceLimitType;
  NoOpField<FFUtils::Decimal, false, false> *LowLimitPrice;
  NoOpField<FFUtils::Decimal, false, false> *HighLimitPrice;
  NoOpField<FFUtils::Decimal, false, false> *TradingReferencePrice;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataSnapshotFullRefresh_19() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    LastMsgSeqNumProcessed = new NoOpField<uint32_t, false, false>(0, "glob_LastMsgSeqNumProcessed");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    TotNumReports = new NoOpField<int32_t, true, false>(0, "glob_TotNumReports");
    TradeDate = new NoOpField<uint32_t, false, false>(0, "glob_TradeDate");
    NetChgPrevDay = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_NetChgPrevDay");
    MarketDepth = new NoOpField<int32_t, false, false>(0, "glob_MarketDepth");
    SecurityID = new NoOpField<int64_t, true, false>(0, "glob_SecurityID");
    SecurityIDSource = new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_SecurityExchange");
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t, true, false>(0, "glob_NoMDEntries");
    MDEntryType = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_MDEntryType");
    RptSeq = new CopyField<uint32_t, false, false>(0, "glob_RptSeq");
    MDStreamID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDStreamID");
    MDEntryPx = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntryPx");
    MDEntrySize = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntrySize");
    MDEntryDate = new CopyField<uint32_t, true, false>(0, "glob_MDEntryDate");
    MDEntryTime = new CopyField<uint32_t, true, false>(0, "glob_MDEntryTime");
    TickDirection = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TickDirection");
    SecurityTradingStatus = new NoOpField<int32_t, false, false>(0, "glob_SecurityTradingStatus");
    TradingSessionSubID =
        new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradingSessionSubID");
    TradSesOpenTime = new NoOpField<uint64_t, false, false>(0, "glob_TradSesOpenTime");
    QuoteCondition = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_QuoteCondition");
    TradeCondition = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeCondition");
    OpenCloseSettlFlag = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OpenCloseSettlFlag");
    Currency = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Currency");
    OrderID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OrderID");
    TradeID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeID");
    MDEntryBuyer = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntryBuyer");
    MDEntrySeller = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntrySeller");
    NumberOfOrders = new DefaultField<uint32_t, false, true>(1, "glob_NumberOfOrders");
    MDEntryPositionNo = new IncrementField<uint32_t, false, false>(0, "glob_MDEntryPositionNo");
    PriceType = new NoOpField<uint32_t, false, false>(0, "glob_PriceType");
    SellerDays = new NoOpField<uint32_t, false, false>(0, "glob_SellerDays");
    SettlPriceType = new CopyField<uint32_t, false, false>(0, "glob_SettlPriceType");
    TradeVolume = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_TradeVolume");
    PriceLimitType = new NoOpField<int32_t, false, false>(0, "glob_PriceLimitType");
    LowLimitPrice = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_LowLimitPrice");
    HighLimitPrice = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_HighLimitPrice");
    TradingReferencePrice =
        new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_TradingReferencePrice");
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    LastMsgSeqNumProcessed->decode(input);
    ApplVerID->decode(pmap0);
    TotNumReports->decode(input);
    TradeDate->decode(input);
    NetChgPrevDay->decode(input);
    MarketDepth->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode(input, pmap0);
    SecurityExchange->decode(input, pmap0);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->hasValue() ? NoMDEntries->getValue() : 0;
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDEntryType->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      MDStreamID->decode(input);
      MDEntryPx->decode(input);
      MDEntrySize->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      SecurityTradingStatus->decode(input);
      TradingSessionSubID->decode(input);
      TradSesOpenTime->decode(input);
      QuoteCondition->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettlFlag->decode(input);
      Currency->decode(input, pmap1);
      OrderID->decode(input);
      TradeID->decode(input);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      PriceType->decode(input);
      SellerDays->decode(input);
      SettlPriceType->decode(input, pmap1);
      TradeVolume->decode(input);
      PriceLimitType->decode(input);
      LowLimitPrice->decode(input);
      HighLimitPrice->decode(input);
      TradingReferencePrice->decode(input);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    LastMsgSeqNumProcessed->reset();
    ApplVerID->reset();
    TotNumReports->reset();
    TradeDate->reset();
    NetChgPrevDay->reset();
    MarketDepth->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDEntryType->reset();
    RptSeq->reset();
    MDStreamID->reset();
    MDEntryPx->reset();
    MDEntrySize->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    TickDirection->reset();
    SecurityTradingStatus->reset();
    TradingSessionSubID->reset();
    TradSesOpenTime->reset();
    QuoteCondition->reset();
    TradeCondition->reset();
    OpenCloseSettlFlag->reset();
    Currency->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    PriceType->reset();
    SellerDays->reset();
    SettlPriceType->reset();
    TradeVolume->reset();
    PriceLimitType->reset();
    LowLimitPrice->reset();
    HighLimitPrice->reset();
    TradingReferencePrice->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MarketDataSnapshotFullRefresh_19() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete LastMsgSeqNumProcessed;
    delete ApplVerID;
    delete TotNumReports;
    delete TradeDate;
    delete NetChgPrevDay;
    delete MarketDepth;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDEntryType;
    delete RptSeq;
    delete MDStreamID;
    delete MDEntryPx;
    delete MDEntrySize;
    delete MDEntryDate;
    delete MDEntryTime;
    delete TickDirection;
    delete SecurityTradingStatus;
    delete TradingSessionSubID;
    delete TradSesOpenTime;
    delete QuoteCondition;
    delete TradeCondition;
    delete OpenCloseSettlFlag;
    delete Currency;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete PriceType;
    delete SellerDays;
    delete SettlPriceType;
    delete TradeVolume;
    delete PriceLimitType;
    delete LowLimitPrice;
    delete HighLimitPrice;
    delete TradingReferencePrice;
    // destructor for sequence MDEntries ends
  }
};
class MarketDataSnapshotFullRefresh_20 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  NoOpField<uint32_t, false, false> *LastMsgSeqNumProcessed;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<int32_t, true, false> *TotNumReports;
  NoOpField<uint32_t, false, false> *TradeDate;
  NoOpField<FFUtils::Decimal, false, false> *NetChgPrevDay;
  NoOpField<int32_t, false, false> *MarketDepth;
  NoOpField<FFUtils::ByteArr, true, false> *SecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityExchange;
  // fields inside sequence MDEntries
  NoOpField<uint32_t, true, false> *NoMDEntries;
  CopyField<FFUtils::ByteArr, true, false> *MDEntryType;
  CopyField<uint32_t, false, false> *RptSeq;
  NoOpField<FFUtils::ByteArr, false, false> *MDStreamID;
  DeltaField<FFUtils::Decimal, false> *MDEntryPx;
  DeltaField<FFUtils::Decimal, false> *MDEntrySize;
  CopyField<uint32_t, true, false> *MDEntryDate;
  CopyField<uint32_t, true, false> *MDEntryTime;
  CopyField<FFUtils::ByteArr, false, false> *TickDirection;
  NoOpField<int32_t, false, false> *SecurityTradingStatus;
  NoOpField<FFUtils::ByteArr, false, false> *TradingSessionSubID;
  NoOpField<uint64_t, false, false> *TradSesOpenTime;
  CopyField<FFUtils::ByteArr, false, false> *QuoteCondition;
  CopyField<FFUtils::ByteArr, false, false> *TradeCondition;
  NoOpField<FFUtils::ByteArr, false, false> *OpenCloseSettlFlag;
  CopyField<FFUtils::ByteArr, false, false> *Currency;
  NoOpField<FFUtils::ByteArr, false, false> *OrderID;
  NoOpField<FFUtils::ByteArr, false, false> *TradeID;
  CopyField<FFUtils::ByteArr, false, false> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr, false, false> *MDEntrySeller;
  DefaultField<uint32_t, false, true> *NumberOfOrders;
  IncrementField<uint32_t, false, false> *MDEntryPositionNo;
  NoOpField<uint32_t, false, false> *SellerDays;
  DeltaField<FFUtils::Decimal, false> *TradeVolume;
  NoOpField<FFUtils::Decimal, false, false> *PercentageVar;
  NoOpField<uint32_t, false, false> *NoUnchangedSecurities;
  NoOpField<uint32_t, false, false> *NoNotTradedSecurities;
  NoOpField<uint32_t, false, false> *TotTradedSecurities;
  NoOpField<FFUtils::Decimal, false, false> *CapitalPct;
  NoOpField<FFUtils::Decimal, false, false> *PrevYearVariation;
  NoOpField<uint32_t, false, false> *NoFallingSecurities;
  NoOpField<uint32_t, false, false> *NoRisingSecurities;
  NoOpField<FFUtils::Decimal, false, false> *PercThresholdNormalTrade;
  NoOpField<FFUtils::Decimal, false, false> *PercThresholdCrossTrade;
  NoOpField<uint64_t, false, false> *DailyAvgShares30D;
  NoOpField<FFUtils::Decimal, false, false> *MaximumNormalSharesPerDailyAvgShares30DRatio;
  NoOpField<FFUtils::Decimal, false, false> *MaximumCrossSharesPerDailyAvgShares30DRatio;
  NoOpField<FFUtils::Decimal, false, false> *NormalSharesPerOutstandingSharesRatio;
  NoOpField<FFUtils::Decimal, false, false> *CrossSharesPerOutstandingSharesRatio;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataSnapshotFullRefresh_20() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    LastMsgSeqNumProcessed = new NoOpField<uint32_t, false, false>(0, "glob_LastMsgSeqNumProcessed");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    TotNumReports = new NoOpField<int32_t, true, false>(0, "glob_TotNumReports");
    TradeDate = new NoOpField<uint32_t, false, false>(0, "glob_TradeDate");
    NetChgPrevDay = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_NetChgPrevDay");
    MarketDepth = new NoOpField<int32_t, false, false>(0, "glob_MarketDepth");
    SecurityID = new NoOpField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_SecurityID");
    SecurityIDSource = new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_SecurityExchange");
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t, true, false>(0, "glob_NoMDEntries");
    MDEntryType = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_MDEntryType");
    RptSeq = new CopyField<uint32_t, false, false>(0, "glob_RptSeq");
    MDStreamID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDStreamID");
    MDEntryPx = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntryPx");
    MDEntrySize = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntrySize");
    MDEntryDate = new CopyField<uint32_t, true, false>(0, "glob_MDEntryDate");
    MDEntryTime = new CopyField<uint32_t, true, false>(0, "glob_MDEntryTime");
    TickDirection = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TickDirection");
    SecurityTradingStatus = new NoOpField<int32_t, false, false>(0, "glob_SecurityTradingStatus");
    TradingSessionSubID =
        new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradingSessionSubID");
    TradSesOpenTime = new NoOpField<uint64_t, false, false>(0, "glob_TradSesOpenTime");
    QuoteCondition = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_QuoteCondition");
    TradeCondition = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeCondition");
    OpenCloseSettlFlag = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OpenCloseSettlFlag");
    Currency = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Currency");
    OrderID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OrderID");
    TradeID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeID");
    MDEntryBuyer = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntryBuyer");
    MDEntrySeller = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntrySeller");
    NumberOfOrders = new DefaultField<uint32_t, false, true>(1, "glob_NumberOfOrders");
    MDEntryPositionNo = new IncrementField<uint32_t, false, false>(0, "glob_MDEntryPositionNo");
    SellerDays = new NoOpField<uint32_t, false, false>(0, "glob_SellerDays");
    TradeVolume = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_TradeVolume");
    PercentageVar = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercentageVar");
    NoUnchangedSecurities = new NoOpField<uint32_t, false, false>(0, "glob_NoUnchangedSecurities");
    NoNotTradedSecurities = new NoOpField<uint32_t, false, false>(0, "glob_NoNotTradedSecurities");
    TotTradedSecurities = new NoOpField<uint32_t, false, false>(0, "glob_TotTradedSecurities");
    CapitalPct = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_CapitalPct");
    PrevYearVariation = new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PrevYearVariation");
    NoFallingSecurities = new NoOpField<uint32_t, false, false>(0, "glob_NoFallingSecurities");
    NoRisingSecurities = new NoOpField<uint32_t, false, false>(0, "glob_NoRisingSecurities");
    PercThresholdNormalTrade =
        new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercThresholdNormalTrade");
    PercThresholdCrossTrade =
        new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercThresholdCrossTrade");
    DailyAvgShares30D = new NoOpField<uint64_t, false, false>(0, "glob_DailyAvgShares30D");
    MaximumNormalSharesPerDailyAvgShares30DRatio = new NoOpField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_MaximumNormalSharesPerDailyAvgShares30DRatio");
    MaximumCrossSharesPerDailyAvgShares30DRatio = new NoOpField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_MaximumCrossSharesPerDailyAvgShares30DRatio");
    NormalSharesPerOutstandingSharesRatio = new NoOpField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_NormalSharesPerOutstandingSharesRatio");
    CrossSharesPerOutstandingSharesRatio =
        new NoOpField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_CrossSharesPerOutstandingSharesRatio");
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    LastMsgSeqNumProcessed->decode(input);
    ApplVerID->decode(pmap0);
    TotNumReports->decode(input);
    TradeDate->decode(input);
    NetChgPrevDay->decode(input);
    MarketDepth->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode(input, pmap0);
    SecurityExchange->decode(input, pmap0);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    int NoMDEntries_len = NoMDEntries->hasValue() ? NoMDEntries->getValue() : 0;
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDEntryType->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      MDStreamID->decode(input);
      MDEntryPx->decode(input);
      MDEntrySize->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      SecurityTradingStatus->decode(input);
      TradingSessionSubID->decode(input);
      TradSesOpenTime->decode(input);
      QuoteCondition->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettlFlag->decode(input);
      Currency->decode(input, pmap1);
      OrderID->decode(input);
      TradeID->decode(input);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      SellerDays->decode(input);
      TradeVolume->decode(input);
      PercentageVar->decode(input);
      NoUnchangedSecurities->decode(input);
      NoNotTradedSecurities->decode(input);
      TotTradedSecurities->decode(input);
      CapitalPct->decode(input);
      PrevYearVariation->decode(input);
      NoFallingSecurities->decode(input);
      NoRisingSecurities->decode(input);
      PercThresholdNormalTrade->decode(input);
      PercThresholdCrossTrade->decode(input);
      DailyAvgShares30D->decode(input);
      MaximumNormalSharesPerDailyAvgShares30DRatio->decode(input);
      MaximumCrossSharesPerDailyAvgShares30DRatio->decode(input);
      NormalSharesPerOutstandingSharesRatio->decode(input);
      CrossSharesPerOutstandingSharesRatio->decode(input);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    LastMsgSeqNumProcessed->reset();
    ApplVerID->reset();
    TotNumReports->reset();
    TradeDate->reset();
    NetChgPrevDay->reset();
    MarketDepth->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDEntryType->reset();
    RptSeq->reset();
    MDStreamID->reset();
    MDEntryPx->reset();
    MDEntrySize->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    TickDirection->reset();
    SecurityTradingStatus->reset();
    TradingSessionSubID->reset();
    TradSesOpenTime->reset();
    QuoteCondition->reset();
    TradeCondition->reset();
    OpenCloseSettlFlag->reset();
    Currency->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    SellerDays->reset();
    TradeVolume->reset();
    PercentageVar->reset();
    NoUnchangedSecurities->reset();
    NoNotTradedSecurities->reset();
    TotTradedSecurities->reset();
    CapitalPct->reset();
    PrevYearVariation->reset();
    NoFallingSecurities->reset();
    NoRisingSecurities->reset();
    PercThresholdNormalTrade->reset();
    PercThresholdCrossTrade->reset();
    DailyAvgShares30D->reset();
    MaximumNormalSharesPerDailyAvgShares30DRatio->reset();
    MaximumCrossSharesPerDailyAvgShares30DRatio->reset();
    NormalSharesPerOutstandingSharesRatio->reset();
    CrossSharesPerOutstandingSharesRatio->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MarketDataSnapshotFullRefresh_20() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete LastMsgSeqNumProcessed;
    delete ApplVerID;
    delete TotNumReports;
    delete TradeDate;
    delete NetChgPrevDay;
    delete MarketDepth;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDEntryType;
    delete RptSeq;
    delete MDStreamID;
    delete MDEntryPx;
    delete MDEntrySize;
    delete MDEntryDate;
    delete MDEntryTime;
    delete TickDirection;
    delete SecurityTradingStatus;
    delete TradingSessionSubID;
    delete TradSesOpenTime;
    delete QuoteCondition;
    delete TradeCondition;
    delete OpenCloseSettlFlag;
    delete Currency;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete SellerDays;
    delete TradeVolume;
    delete PercentageVar;
    delete NoUnchangedSecurities;
    delete NoNotTradedSecurities;
    delete TotTradedSecurities;
    delete CapitalPct;
    delete PrevYearVariation;
    delete NoFallingSecurities;
    delete NoRisingSecurities;
    delete PercThresholdNormalTrade;
    delete PercThresholdCrossTrade;
    delete DailyAvgShares30D;
    delete MaximumNormalSharesPerDailyAvgShares30DRatio;
    delete MaximumCrossSharesPerDailyAvgShares30DRatio;
    delete NormalSharesPerOutstandingSharesRatio;
    delete CrossSharesPerOutstandingSharesRatio;
    // destructor for sequence MDEntries ends
  }
};
class MarketDataSnapshotFullRefresh_28 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  NoOpField<uint32_t, false, false> *LastMsgSeqNumProcessed;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<int32_t, true, false> *TotNumReports;
  DefaultField<uint32_t, false, false> *TradeDate;
  DefaultField<FFUtils::Decimal, false, false> *NetChgPrevDay;
  DefaultField<int32_t, false, true> *MarketDepth;
  NoOpField<FFUtils::ByteArr, true, false> *SecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityExchange;
  // fields inside sequence MDEntries
  NoOpField<uint32_t, true, false> *NoMDEntries;
  CopyField<FFUtils::ByteArr, true, false> *MDEntryType;
  IncrementField<uint32_t, false, false> *RptSeq;
  DefaultField<uint32_t, false, false> *PriceBandType;
  DefaultField<FFUtils::ByteArr, false, false> *MDStreamID;
  DeltaField<FFUtils::Decimal, false> *MDEntryPx;
  DeltaField<FFUtils::Decimal, false> *MDEntrySize;
  CopyField<uint32_t, true, false> *MDEntryDate;
  CopyField<uint32_t, true, false> *MDEntryTime;
  DefaultField<FFUtils::ByteArr, false, false> *TickDirection;
  DefaultField<uint32_t, false, false> *SecurityTradingStatus;
  DefaultField<FFUtils::ByteArr, false, false> *TradingSessionSubID;
  DefaultField<uint64_t, false, false> *TradSesOpenTime;
  DefaultField<FFUtils::ByteArr, false, false> *QuoteCondition;
  DefaultField<FFUtils::ByteArr, false, false> *TradeCondition;
  DefaultField<FFUtils::ByteArr, false, false> *OpenCloseSettlFlag;
  DefaultField<uint64_t, false, false> *NoSharesIssued;
  CopyField<FFUtils::ByteArr, false, false> *Currency;
  DefaultField<FFUtils::ByteArr, false, false> *OrderID;
  DefaultField<FFUtils::ByteArr, false, false> *TradeID;
  CopyField<FFUtils::ByteArr, false, false> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr, false, false> *MDEntrySeller;
  DefaultField<uint32_t, false, false> *NumberOfOrders;
  IncrementField<uint32_t, false, false> *MDEntryPositionNo;
  DefaultField<uint32_t, false, false> *SellerDays;
  DefaultField<uint64_t, false, false> *TradeVolume;
  DefaultField<FFUtils::Decimal, false, false> *PercentageVar;
  DefaultField<uint32_t, false, false> *NoUnchangedSecurities;
  DefaultField<uint32_t, false, false> *NoNotTradedSecurities;
  DefaultField<uint32_t, false, false> *TotTradedSecurities;
  DefaultField<FFUtils::Decimal, false, false> *CapitalPct;
  DefaultField<FFUtils::Decimal, false, false> *PrevYearVariation;
  DefaultField<uint32_t, false, false> *NoFallingSecurities;
  DefaultField<uint32_t, false, false> *NoRisingSecurities;
  DefaultField<FFUtils::Decimal, false, false> *PercThresholdNormalTrade;
  DefaultField<FFUtils::Decimal, false, false> *PercThresholdCrossTrade;
  DefaultField<uint64_t, false, false> *DailyAvgShares30D;
  DefaultField<FFUtils::Decimal, false, false> *MaximumNormalSharesPerDailyAvgShares30DRatio;
  DefaultField<FFUtils::Decimal, false, false> *MaximumCrossSharesPerDailyAvgShares30DRatio;
  DefaultField<FFUtils::Decimal, false, false> *NormalSharesPerOutstandingSharesRatio;
  DefaultField<FFUtils::Decimal, false, false> *CrossSharesPerOutstandingSharesRatio;
  DefaultField<uint64_t, false, false> *MDEntryID;
  DefaultField<uint32_t, false, false> *MDInsertDate;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataSnapshotFullRefresh_28() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    LastMsgSeqNumProcessed = new NoOpField<uint32_t, false, false>(0, "glob_LastMsgSeqNumProcessed");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    TotNumReports = new NoOpField<int32_t, true, false>(0, "glob_TotNumReports");
    TradeDate = new DefaultField<uint32_t, false, false>(0, "glob_TradeDate");
    NetChgPrevDay = new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_NetChgPrevDay");
    MarketDepth = new DefaultField<int32_t, false, true>(0, "glob_MarketDepth");
    SecurityID = new NoOpField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_SecurityID");
    SecurityIDSource = new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_SecurityExchange");
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t, true, false>(0, "glob_NoMDEntries");
    MDEntryType = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_MDEntryType");
    RptSeq = new IncrementField<uint32_t, false, false>(0, "glob_RptSeq");
    PriceBandType = new DefaultField<uint32_t, false, false>(0, "glob_PriceBandType");
    MDStreamID = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDStreamID");
    MDEntryPx = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntryPx");
    MDEntrySize = new DeltaField<FFUtils::Decimal, false>(FFUtils::Decimal(0), "glob_MDEntrySize");
    MDEntryDate = new CopyField<uint32_t, true, false>(0, "glob_MDEntryDate");
    MDEntryTime = new CopyField<uint32_t, true, false>(0, "glob_MDEntryTime");
    TickDirection = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TickDirection");
    SecurityTradingStatus = new DefaultField<uint32_t, false, false>(0, "glob_SecurityTradingStatus");
    TradingSessionSubID =
        new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradingSessionSubID");
    TradSesOpenTime = new DefaultField<uint64_t, false, false>(0, "glob_TradSesOpenTime");
    QuoteCondition = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_QuoteCondition");
    TradeCondition = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeCondition");
    OpenCloseSettlFlag =
        new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OpenCloseSettlFlag");
    NoSharesIssued = new DefaultField<uint64_t, false, false>(0, "glob_NoSharesIssued");
    Currency = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Currency");
    OrderID = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_OrderID");
    TradeID = new DefaultField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradeID");
    MDEntryBuyer = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntryBuyer");
    MDEntrySeller = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_MDEntrySeller");
    NumberOfOrders = new DefaultField<uint32_t, false, false>(0, "glob_NumberOfOrders");
    MDEntryPositionNo = new IncrementField<uint32_t, false, false>(0, "glob_MDEntryPositionNo");
    SellerDays = new DefaultField<uint32_t, false, false>(0, "glob_SellerDays");
    TradeVolume = new DefaultField<uint64_t, false, false>(0, "glob_TradeVolume");
    PercentageVar = new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercentageVar");
    NoUnchangedSecurities = new DefaultField<uint32_t, false, false>(0, "glob_NoUnchangedSecurities");
    NoNotTradedSecurities = new DefaultField<uint32_t, false, false>(0, "glob_NoNotTradedSecurities");
    TotTradedSecurities = new DefaultField<uint32_t, false, false>(0, "glob_TotTradedSecurities");
    CapitalPct = new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_CapitalPct");
    PrevYearVariation = new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PrevYearVariation");
    NoFallingSecurities = new DefaultField<uint32_t, false, false>(0, "glob_NoFallingSecurities");
    NoRisingSecurities = new DefaultField<uint32_t, false, false>(0, "glob_NoRisingSecurities");
    PercThresholdNormalTrade =
        new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercThresholdNormalTrade");
    PercThresholdCrossTrade =
        new DefaultField<FFUtils::Decimal, false, false>(FFUtils::Decimal(0), "glob_PercThresholdCrossTrade");
    DailyAvgShares30D = new DefaultField<uint64_t, false, false>(0, "glob_DailyAvgShares30D");
    MaximumNormalSharesPerDailyAvgShares30DRatio = new DefaultField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_MaximumNormalSharesPerDailyAvgShares30DRatio");
    MaximumCrossSharesPerDailyAvgShares30DRatio = new DefaultField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_MaximumCrossSharesPerDailyAvgShares30DRatio");
    NormalSharesPerOutstandingSharesRatio = new DefaultField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_NormalSharesPerOutstandingSharesRatio");
    CrossSharesPerOutstandingSharesRatio = new DefaultField<FFUtils::Decimal, false, false>(
        FFUtils::Decimal(0), "glob_CrossSharesPerOutstandingSharesRatio");
    MDEntryID = new DefaultField<uint64_t, false, false>(0, "glob_MDEntryID");
    MDInsertDate = new DefaultField<uint32_t, false, false>(0, "glob_MDInsertDate");
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    LastMsgSeqNumProcessed->decode(input);
    ApplVerID->decode(pmap0);
    TotNumReports->decode(input);
    TradeDate->decode(input, pmap0);
    NetChgPrevDay->decode(input, pmap0);
    MarketDepth->decode(input, pmap0);
    SecurityID->decode(input);
    SecurityIDSource->decode(input, pmap0);
    SecurityExchange->decode(input, pmap0);
    // decode for sequence MDEntries
    NoMDEntries->decode(input);

    // manually added
    uint64_t secId = findSecurity();
    bool process_sec = ((int64_t)secId != -1);

    int NoMDEntries_len = NoMDEntries->hasValue() ? NoMDEntries->getValue() : 0;
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDEntryType->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      PriceBandType->decode(input, pmap1);
      MDStreamID->decode(input, pmap1);
      MDEntryPx->decode(input);
      MDEntrySize->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      TickDirection->decode(input, pmap1);
      SecurityTradingStatus->decode(input, pmap1);
      TradingSessionSubID->decode(input, pmap1);
      TradSesOpenTime->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettlFlag->decode(input, pmap1);
      NoSharesIssued->decode(input, pmap1);
      Currency->decode(input, pmap1);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      SellerDays->decode(input, pmap1);
      TradeVolume->decode(input, pmap1);
      PercentageVar->decode(input, pmap1);
      NoUnchangedSecurities->decode(input, pmap1);
      NoNotTradedSecurities->decode(input, pmap1);
      TotTradedSecurities->decode(input, pmap1);
      CapitalPct->decode(input, pmap1);
      PrevYearVariation->decode(input, pmap1);
      NoFallingSecurities->decode(input, pmap1);
      NoRisingSecurities->decode(input, pmap1);
      PercThresholdNormalTrade->decode(input, pmap1);
      PercThresholdCrossTrade->decode(input, pmap1);
      DailyAvgShares30D->decode(input, pmap1);
      MaximumNormalSharesPerDailyAvgShares30DRatio->decode(input, pmap1);
      MaximumCrossSharesPerDailyAvgShares30DRatio->decode(input, pmap1);
      NormalSharesPerOutstandingSharesRatio->decode(input, pmap1);
      CrossSharesPerOutstandingSharesRatio->decode(input, pmap1);
      MDEntryID->decode(input, pmap1);
      MDInsertDate->decode(input, pmap1);
      if (process_sec) process(secId);
    }
    if (process_sec) process_end(secId);
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();
  void process(uint64_t secId);
  void process_end(uint64_t secId);
  uint64_t findSecurity();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    LastMsgSeqNumProcessed->reset();
    ApplVerID->reset();
    TotNumReports->reset();
    TradeDate->reset();
    NetChgPrevDay->reset();
    MarketDepth->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDEntryType->reset();
    RptSeq->reset();
    PriceBandType->reset();
    MDStreamID->reset();
    MDEntryPx->reset();
    MDEntrySize->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    TickDirection->reset();
    SecurityTradingStatus->reset();
    TradingSessionSubID->reset();
    TradSesOpenTime->reset();
    QuoteCondition->reset();
    TradeCondition->reset();
    OpenCloseSettlFlag->reset();
    NoSharesIssued->reset();
    Currency->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    SellerDays->reset();
    TradeVolume->reset();
    PercentageVar->reset();
    NoUnchangedSecurities->reset();
    NoNotTradedSecurities->reset();
    TotTradedSecurities->reset();
    CapitalPct->reset();
    PrevYearVariation->reset();
    NoFallingSecurities->reset();
    NoRisingSecurities->reset();
    PercThresholdNormalTrade->reset();
    PercThresholdCrossTrade->reset();
    DailyAvgShares30D->reset();
    MaximumNormalSharesPerDailyAvgShares30DRatio->reset();
    MaximumCrossSharesPerDailyAvgShares30DRatio->reset();
    NormalSharesPerOutstandingSharesRatio->reset();
    CrossSharesPerOutstandingSharesRatio->reset();
    MDEntryID->reset();
    MDInsertDate->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  virtual ~MarketDataSnapshotFullRefresh_28() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete LastMsgSeqNumProcessed;
    delete ApplVerID;
    delete TotNumReports;
    delete TradeDate;
    delete NetChgPrevDay;
    delete MarketDepth;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDEntryType;
    delete RptSeq;
    delete PriceBandType;
    delete MDStreamID;
    delete MDEntryPx;
    delete MDEntrySize;
    delete MDEntryDate;
    delete MDEntryTime;
    delete TickDirection;
    delete SecurityTradingStatus;
    delete TradingSessionSubID;
    delete TradSesOpenTime;
    delete QuoteCondition;
    delete TradeCondition;
    delete OpenCloseSettlFlag;
    delete NoSharesIssued;
    delete Currency;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete SellerDays;
    delete TradeVolume;
    delete PercentageVar;
    delete NoUnchangedSecurities;
    delete NoNotTradedSecurities;
    delete TotTradedSecurities;
    delete CapitalPct;
    delete PrevYearVariation;
    delete NoFallingSecurities;
    delete NoRisingSecurities;
    delete PercThresholdNormalTrade;
    delete PercThresholdCrossTrade;
    delete DailyAvgShares30D;
    delete MaximumNormalSharesPerDailyAvgShares30DRatio;
    delete MaximumCrossSharesPerDailyAvgShares30DRatio;
    delete NormalSharesPerOutstandingSharesRatio;
    delete CrossSharesPerOutstandingSharesRatio;
    delete MDEntryID;
    delete MDInsertDate;
    // destructor for sequence MDEntries ends
  }
};
class SecurityStatus_15 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<FFUtils::ByteArr, false, false> *SecurityGroup;
  NoOpField<FFUtils::ByteArr, false, false> *Symbol;
  NoOpField<FFUtils::ByteArr, false, false> *SecurityID;
  ConstantFieldOptional<FFUtils::ByteArr> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityExchange;
  NoOpField<FFUtils::ByteArr, false, false> *TradingSessionSubID;
  NoOpField<int32_t, false, false> *SecurityTradingStatus;
  NoOpField<uint64_t, false, false> *TradSesOpenTime;
  NoOpField<uint64_t, false, false> *TransactTime;
  NoOpField<int32_t, false, false> *SecurityTradingEvent;
  // fields inside sequence NoMDEntryTypes
  NoOpField<uint32_t, true, false> *NoMDEntryTypes;
  NoOpField<FFUtils::ByteArr, true, false> *MDEntryType;
  // fields inside sequence NoMDEntryTypes ends

  // Constructor
  SecurityStatus_15() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("f"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    SecurityGroup = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityGroup");
    Symbol = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Symbol");
    SecurityID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityID");
    SecurityIDSource =
        new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("XBSP"), "glob_SecurityExchange");
    TradingSessionSubID =
        new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradingSessionSubID");
    SecurityTradingStatus = new NoOpField<int32_t, false, false>(0, "glob_SecurityTradingStatus");
    TradSesOpenTime = new NoOpField<uint64_t, false, false>(0, "glob_TradSesOpenTime");
    TransactTime = new NoOpField<uint64_t, false, false>(0, "glob_TransactTime");
    SecurityTradingEvent = new NoOpField<int32_t, false, false>(0, "glob_SecurityTradingEvent");
    // constructor for sequence NoMDEntryTypes
    NoMDEntryTypes = new NoOpField<uint32_t, true, false>(0, "glob_NoMDEntryTypes");
    MDEntryType = new NoOpField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_MDEntryType");
    // constructor for sequence NoMDEntryTypes ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode(pmap0);
    SecurityGroup->decode(input);
    Symbol->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode(pmap0);
    SecurityExchange->decode(input, pmap0);
    TradingSessionSubID->decode(input);
    SecurityTradingStatus->decode(input);
    TradSesOpenTime->decode(input);
    TransactTime->decode(input);
    SecurityTradingEvent->decode(input);
    // decode for sequence NoMDEntryTypes
    NoMDEntryTypes->decode(input);

    int NoMDEntryTypes_len = NoMDEntryTypes->hasValue() ? NoMDEntryTypes->getValue() : 0;
    for (int i = 0; i < NoMDEntryTypes_len; ++i) {
      MDEntryType->decode(input);
    }
    // decode for sequence NoMDEntryTypes ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
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
    // reset for sequence NoMDEntryTypes
    NoMDEntryTypes->reset();
    MDEntryType->reset();
    // reset for sequence NoMDEntryTypes ends
  }

  // Destructor
  virtual ~SecurityStatus_15() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
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
    // destructor for sequence NoMDEntryTypes
    delete NoMDEntryTypes;
    delete MDEntryType;
    // destructor for sequence NoMDEntryTypes ends
  }
};
class SecurityStatus_21 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<FFUtils::ByteArr, false, false> *SecurityGroup;
  NoOpField<int64_t, false, false> *SecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityExchange;
  NoOpField<FFUtils::ByteArr, false, false> *TradingSessionSubID;
  NoOpField<int32_t, false, false> *SecurityTradingStatus;
  NoOpField<uint64_t, false, false> *TradSesOpenTime;
  NoOpField<uint64_t, false, false> *TransactTime;
  NoOpField<int32_t, false, false> *SecurityTradingEvent;

  // Constructor
  SecurityStatus_21() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("f"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    SecurityGroup = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityGroup");
    SecurityID = new NoOpField<int64_t, false, false>(0, "glob_SecurityID");
    SecurityIDSource = new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_SecurityExchange");
    TradingSessionSubID =
        new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradingSessionSubID");
    SecurityTradingStatus = new NoOpField<int32_t, false, false>(0, "glob_SecurityTradingStatus");
    TradSesOpenTime = new NoOpField<uint64_t, false, false>(0, "glob_TradSesOpenTime");
    TransactTime = new NoOpField<uint64_t, false, false>(0, "glob_TransactTime");
    SecurityTradingEvent = new NoOpField<int32_t, false, false>(0, "glob_SecurityTradingEvent");
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode(pmap0);
    SecurityGroup->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode(input, pmap0);
    SecurityExchange->decode(input, pmap0);
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
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
    SecurityGroup->reset();
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
  virtual ~SecurityStatus_21() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete SecurityGroup;
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
class SecurityStatus_22 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<FFUtils::ByteArr, false, false> *SecurityGroup;
  NoOpField<FFUtils::ByteArr, false, false> *SecurityID;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr, false, true> *SecurityExchange;
  NoOpField<FFUtils::ByteArr, false, false> *TradingSessionSubID;
  NoOpField<int32_t, false, false> *SecurityTradingStatus;
  NoOpField<uint64_t, false, false> *TradSesOpenTime;
  NoOpField<uint64_t, false, false> *TransactTime;
  NoOpField<int32_t, false, false> *SecurityTradingEvent;

  // Constructor
  SecurityStatus_22() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("f"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    SecurityGroup = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityGroup");
    SecurityID = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_SecurityID");
    SecurityIDSource = new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange =
        new DefaultField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_SecurityExchange");
    TradingSessionSubID =
        new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_TradingSessionSubID");
    SecurityTradingStatus = new NoOpField<int32_t, false, false>(0, "glob_SecurityTradingStatus");
    TradSesOpenTime = new NoOpField<uint64_t, false, false>(0, "glob_TradSesOpenTime");
    TransactTime = new NoOpField<uint64_t, false, false>(0, "glob_TransactTime");
    SecurityTradingEvent = new NoOpField<int32_t, false, false>(0, "glob_SecurityTradingEvent");
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode(pmap0);
    SecurityGroup->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode(input, pmap0);
    SecurityExchange->decode(input, pmap0);
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
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
    SecurityGroup->reset();
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
  virtual ~SecurityStatus_22() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete SecurityGroup;
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
class News_16 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint64_t, false, false> *OrigTime;
  NoOpField<FFUtils::ByteArr, false, false> *NewsSource;
  NoOpField<FFUtils::ByteArr, false, false> *LanguageCode;
  NoOpField<FFUtils::ByteArr, true, false> *Headline;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t, true, false> *NoRelatedSym;
  CopyField<FFUtils::ByteArr, true, false> *Symbol;
  CopyField<FFUtils::ByteArr, true, false> *SecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityIDSource;
  CopyField<FFUtils::ByteArr, true, true> *SecurityExchange;
  // fields inside sequence RelatedSym ends
  // fields inside sequence RoutingIDs
  NoOpField<uint32_t, true, false> *NoRoutingIDs;
  CopyField<int32_t, true, true> *RoutingType;
  CopyField<FFUtils::ByteArr, false, false> *RoutingID;
  // fields inside sequence RoutingIDs ends
  // fields inside sequence LinesOfText
  NoOpField<uint32_t, true, false> *NoLinesOfText;
  CopyField<FFUtils::ByteArr, true, false> *Text;
  NoOpField<int32_t, false, false> *EncodedTextLen;
  NoOpField<FFUtils::ByteArr, false, false> *EncodedText;
  // fields inside sequence LinesOfText ends
  NoOpField<FFUtils::ByteArr, false, false> *URLLink;

  // Constructor
  News_16() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("B"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    OrigTime = new NoOpField<uint64_t, false, false>(0, "glob_OrigTime");
    NewsSource = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_NewsSource");
    LanguageCode = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_LanguageCode");
    Headline = new NoOpField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_Headline");
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t, true, false>(0, "glob_NoRelatedSym");
    Symbol = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_Symbol");
    SecurityID = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_SecurityID");
    SecurityIDSource =
        new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange = new CopyField<FFUtils::ByteArr, true, true>(FFUtils::ByteArr("XBSP"), "glob_SecurityExchange");
    // constructor for sequence RelatedSym ends
    // constructor for sequence RoutingIDs
    NoRoutingIDs = new NoOpField<uint32_t, true, false>(0, "glob_NoRoutingIDs");
    RoutingType = new CopyField<int32_t, true, true>(2, "glob_RoutingType");
    RoutingID = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_RoutingID");
    // constructor for sequence RoutingIDs ends
    // constructor for sequence LinesOfText
    NoLinesOfText = new NoOpField<uint32_t, true, false>(0, "glob_NoLinesOfText");
    Text = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_Text");
    EncodedTextLen = new NoOpField<int32_t, false, false>(0, "glob_EncodedTextLen");
    EncodedText = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_EncodedText");
    // constructor for sequence LinesOfText ends
    URLLink = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_URLLink");
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode(pmap0);
    OrigTime->decode(input);
    NewsSource->decode(input);
    LanguageCode->decode(input);
    Headline->decode(input);
    // decode for sequence RelatedSym
    NoRelatedSym->decode(input);

    int NoRelatedSym_len = NoRelatedSym->hasValue() ? NoRelatedSym->getValue() : 0;
    for (int i = 0; i < NoRelatedSym_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      Symbol->decode(input, pmap1);
      SecurityID->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityExchange->decode(input, pmap1);
    }
    // decode for sequence RelatedSym ends
    // decode for sequence RoutingIDs
    NoRoutingIDs->decode(input);

    int NoRoutingIDs_len = NoRoutingIDs->hasValue() ? NoRoutingIDs->getValue() : 0;
    for (int i = 0; i < NoRoutingIDs_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      RoutingType->decode(input, pmap1);
      RoutingID->decode(input, pmap1);
    }
    // decode for sequence RoutingIDs ends
    // decode for sequence LinesOfText
    NoLinesOfText->decode(input);

    int NoLinesOfText_len = NoLinesOfText->hasValue() ? NoLinesOfText->getValue() : 0;
    for (int i = 0; i < NoLinesOfText_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      Text->decode(input, pmap1);
      EncodedTextLen->decode(input);
      EncodedText->decode(input);
    }
    // decode for sequence LinesOfText ends
    URLLink->decode(input);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
    OrigTime->reset();
    NewsSource->reset();
    LanguageCode->reset();
    Headline->reset();
    // reset for sequence RelatedSym
    NoRelatedSym->reset();
    Symbol->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence RelatedSym ends
    // reset for sequence RoutingIDs
    NoRoutingIDs->reset();
    RoutingType->reset();
    RoutingID->reset();
    // reset for sequence RoutingIDs ends
    // reset for sequence LinesOfText
    NoLinesOfText->reset();
    Text->reset();
    EncodedTextLen->reset();
    EncodedText->reset();
    // reset for sequence LinesOfText ends
    URLLink->reset();
  }

  // Destructor
  virtual ~News_16() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete OrigTime;
    delete NewsSource;
    delete LanguageCode;
    delete Headline;
    // destructor for sequence RelatedSym
    delete NoRelatedSym;
    delete Symbol;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence RelatedSym ends
    // destructor for sequence RoutingIDs
    delete NoRoutingIDs;
    delete RoutingType;
    delete RoutingID;
    // destructor for sequence RoutingIDs ends
    // destructor for sequence LinesOfText
    delete NoLinesOfText;
    delete Text;
    delete EncodedTextLen;
    delete EncodedText;
    // destructor for sequence LinesOfText ends
    delete URLLink;
  }
};
class News_29 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint64_t, false, false> *OrigTime;
  NoOpField<FFUtils::ByteArr, false, false> *NewsSource;
  NoOpField<FFUtils::ByteArr, false, false> *LanguageCode;
  NoOpField<FFUtils::ByteArr, true, false> *Headline;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t, true, false> *NoRelatedSym;
  CopyField<FFUtils::ByteArr, false, false> *Symbol;
  CopyField<FFUtils::ByteArr, true, false> *SecurityID;
  ConstantFieldOptional<FFUtils::ByteArr> *SecurityIDSource;
  CopyField<FFUtils::ByteArr, false, true> *SecurityExchange;
  // fields inside sequence RelatedSym ends
  // fields inside sequence RoutingIDs
  NoOpField<uint32_t, true, false> *NoRoutingIDs;
  CopyField<int32_t, true, true> *RoutingType;
  CopyField<FFUtils::ByteArr, false, false> *RoutingID;
  // fields inside sequence RoutingIDs ends
  // fields inside sequence LinesOfText
  NoOpField<uint32_t, true, false> *NoLinesOfText;
  CopyField<FFUtils::ByteArr, true, false> *Text;
  NoOpField<int32_t, false, false> *EncodedTextLen;
  NoOpField<FFUtils::ByteArr, false, false> *EncodedText;
  // fields inside sequence LinesOfText ends
  NoOpField<FFUtils::ByteArr, false, false> *URLLink;

  // Constructor
  News_29() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("B"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    OrigTime = new NoOpField<uint64_t, false, false>(0, "glob_OrigTime");
    NewsSource = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_NewsSource");
    LanguageCode = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_LanguageCode");
    Headline = new NoOpField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_Headline");
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t, true, false>(0, "glob_NoRelatedSym");
    Symbol = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Symbol");
    SecurityID = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_SecurityID");
    SecurityIDSource =
        new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange = new CopyField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_SecurityExchange");
    // constructor for sequence RelatedSym ends
    // constructor for sequence RoutingIDs
    NoRoutingIDs = new NoOpField<uint32_t, true, false>(0, "glob_NoRoutingIDs");
    RoutingType = new CopyField<int32_t, true, true>(2, "glob_RoutingType");
    RoutingID = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_RoutingID");
    // constructor for sequence RoutingIDs ends
    // constructor for sequence LinesOfText
    NoLinesOfText = new NoOpField<uint32_t, true, false>(0, "glob_NoLinesOfText");
    Text = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_Text");
    EncodedTextLen = new NoOpField<int32_t, false, false>(0, "glob_EncodedTextLen");
    EncodedText = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_EncodedText");
    // constructor for sequence LinesOfText ends
    URLLink = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_URLLink");
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode(pmap0);
    OrigTime->decode(input);
    NewsSource->decode(input);
    LanguageCode->decode(input);
    Headline->decode(input);
    // decode for sequence RelatedSym
    NoRelatedSym->decode(input);

    int NoRelatedSym_len = NoRelatedSym->hasValue() ? NoRelatedSym->getValue() : 0;
    for (int i = 0; i < NoRelatedSym_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      Symbol->decode(input, pmap1);
      SecurityID->decode(input, pmap1);
      SecurityIDSource->decode(pmap1);
      SecurityExchange->decode(input, pmap1);
    }
    // decode for sequence RelatedSym ends
    // decode for sequence RoutingIDs
    NoRoutingIDs->decode(input);

    int NoRoutingIDs_len = NoRoutingIDs->hasValue() ? NoRoutingIDs->getValue() : 0;
    for (int i = 0; i < NoRoutingIDs_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      RoutingType->decode(input, pmap1);
      RoutingID->decode(input, pmap1);
    }
    // decode for sequence RoutingIDs ends
    // decode for sequence LinesOfText
    NoLinesOfText->decode(input);

    int NoLinesOfText_len = NoLinesOfText->hasValue() ? NoLinesOfText->getValue() : 0;
    for (int i = 0; i < NoLinesOfText_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      Text->decode(input, pmap1);
      EncodedTextLen->decode(input);
      EncodedText->decode(input);
    }
    // decode for sequence LinesOfText ends
    URLLink->decode(input);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
    OrigTime->reset();
    NewsSource->reset();
    LanguageCode->reset();
    Headline->reset();
    // reset for sequence RelatedSym
    NoRelatedSym->reset();
    Symbol->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence RelatedSym ends
    // reset for sequence RoutingIDs
    NoRoutingIDs->reset();
    RoutingType->reset();
    RoutingID->reset();
    // reset for sequence RoutingIDs ends
    // reset for sequence LinesOfText
    NoLinesOfText->reset();
    Text->reset();
    EncodedTextLen->reset();
    EncodedText->reset();
    // reset for sequence LinesOfText ends
    URLLink->reset();
  }

  // Destructor
  virtual ~News_29() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete OrigTime;
    delete NewsSource;
    delete LanguageCode;
    delete Headline;
    // destructor for sequence RelatedSym
    delete NoRelatedSym;
    delete Symbol;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence RelatedSym ends
    // destructor for sequence RoutingIDs
    delete NoRoutingIDs;
    delete RoutingType;
    delete RoutingID;
    // destructor for sequence RoutingIDs ends
    // destructor for sequence LinesOfText
    delete NoLinesOfText;
    delete Text;
    delete EncodedTextLen;
    delete EncodedText;
    // destructor for sequence LinesOfText ends
    delete URLLink;
  }
};
class News_24 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t, true, false> *MsgSeqNum;
  NoOpField<uint64_t, true, false> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint64_t, false, false> *OrigTime;
  NoOpField<FFUtils::ByteArr, false, false> *NewsSource;
  NoOpField<FFUtils::ByteArr, false, false> *LanguageCode;
  NoOpField<FFUtils::ByteArr, true, false> *Headline;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t, true, false> *NoRelatedSym;
  CopyField<FFUtils::ByteArr, false, false> *Symbol;
  CopyField<FFUtils::ByteArr, true, false> *SecurityID;
  ConstantFieldOptional<FFUtils::ByteArr> *SecurityIDSource;
  CopyField<FFUtils::ByteArr, false, true> *SecurityExchange;
  // fields inside sequence RelatedSym ends
  // fields inside sequence RoutingIDs
  NoOpField<uint32_t, true, false> *NoRoutingIDs;
  CopyField<int32_t, true, true> *RoutingType;
  CopyField<FFUtils::ByteArr, false, false> *RoutingID;
  // fields inside sequence RoutingIDs ends
  // fields inside sequence LinesOfText
  NoOpField<uint32_t, true, false> *NoLinesOfText;
  CopyField<FFUtils::ByteArr, true, false> *Text;
  NoOpField<int32_t, false, false> *EncodedTextLen;
  NoOpField<FFUtils::ByteArr, false, false> *EncodedText;
  // fields inside sequence LinesOfText ends
  NoOpField<FFUtils::ByteArr, false, false> *URLLink;

  // Constructor
  News_24() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("B"), "glob_MsgType");
    MsgSeqNum = new NoOpField<uint32_t, true, false>(0, "glob_MsgSeqNum");
    SendingTime = new NoOpField<uint64_t, true, false>(0, "glob_SendingTime");
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"), "glob_ApplVerID");
    OrigTime = new NoOpField<uint64_t, false, false>(0, "glob_OrigTime");
    NewsSource = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_NewsSource");
    LanguageCode = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_LanguageCode");
    Headline = new NoOpField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_Headline");
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t, true, false>(0, "glob_NoRelatedSym");
    Symbol = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_Symbol");
    SecurityID = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_SecurityID");
    SecurityIDSource =
        new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"), "glob_SecurityIDSource");
    SecurityExchange = new CopyField<FFUtils::ByteArr, false, true>(FFUtils::ByteArr("BVMF"), "glob_SecurityExchange");
    // constructor for sequence RelatedSym ends
    // constructor for sequence RoutingIDs
    NoRoutingIDs = new NoOpField<uint32_t, true, false>(0, "glob_NoRoutingIDs");
    RoutingType = new CopyField<int32_t, true, true>(2, "glob_RoutingType");
    RoutingID = new CopyField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_RoutingID");
    // constructor for sequence RoutingIDs ends
    // constructor for sequence LinesOfText
    NoLinesOfText = new NoOpField<uint32_t, true, false>(0, "glob_NoLinesOfText");
    Text = new CopyField<FFUtils::ByteArr, true, false>(FFUtils::ByteArr(""), "glob_Text");
    EncodedTextLen = new NoOpField<int32_t, false, false>(0, "glob_EncodedTextLen");
    EncodedText = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_EncodedText");
    // constructor for sequence LinesOfText ends
    URLLink = new NoOpField<FFUtils::ByteArr, false, false>(FFUtils::ByteArr(""), "glob_URLLink");
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode(pmap0);
    OrigTime->decode(input);
    NewsSource->decode(input);
    LanguageCode->decode(input);
    Headline->decode(input);
    // decode for sequence RelatedSym
    NoRelatedSym->decode(input);

    int NoRelatedSym_len = NoRelatedSym->hasValue() ? NoRelatedSym->getValue() : 0;
    for (int i = 0; i < NoRelatedSym_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      Symbol->decode(input, pmap1);
      SecurityID->decode(input, pmap1);
      SecurityIDSource->decode(pmap1);
      SecurityExchange->decode(input, pmap1);
    }
    // decode for sequence RelatedSym ends
    // decode for sequence RoutingIDs
    NoRoutingIDs->decode(input);

    int NoRoutingIDs_len = NoRoutingIDs->hasValue() ? NoRoutingIDs->getValue() : 0;
    for (int i = 0; i < NoRoutingIDs_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      RoutingType->decode(input, pmap1);
      RoutingID->decode(input, pmap1);
    }
    // decode for sequence RoutingIDs ends
    // decode for sequence LinesOfText
    NoLinesOfText->decode(input);

    int NoLinesOfText_len = NoLinesOfText->hasValue() ? NoLinesOfText->getValue() : 0;
    for (int i = 0; i < NoLinesOfText_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      Text->decode(input, pmap1);
      EncodedTextLen->decode(input);
      EncodedText->decode(input);
    }
    // decode for sequence LinesOfText ends
    URLLink->decode(input);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
    OrigTime->reset();
    NewsSource->reset();
    LanguageCode->reset();
    Headline->reset();
    // reset for sequence RelatedSym
    NoRelatedSym->reset();
    Symbol->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence RelatedSym ends
    // reset for sequence RoutingIDs
    NoRoutingIDs->reset();
    RoutingType->reset();
    RoutingID->reset();
    // reset for sequence RoutingIDs ends
    // reset for sequence LinesOfText
    NoLinesOfText->reset();
    Text->reset();
    EncodedTextLen->reset();
    EncodedText->reset();
    // reset for sequence LinesOfText ends
    URLLink->reset();
  }

  // Destructor
  virtual ~News_24() {
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete OrigTime;
    delete NewsSource;
    delete LanguageCode;
    delete Headline;
    // destructor for sequence RelatedSym
    delete NoRelatedSym;
    delete Symbol;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence RelatedSym ends
    // destructor for sequence RoutingIDs
    delete NoRoutingIDs;
    delete RoutingType;
    delete RoutingID;
    // destructor for sequence RoutingIDs ends
    // destructor for sequence LinesOfText
    delete NoLinesOfText;
    delete Text;
    delete EncodedTextLen;
    delete EncodedText;
    // destructor for sequence LinesOfText ends
    delete URLLink;
  }
};
class DecoderMap {
 public:
  static void initilize(std::map<int, FastDecoder *> &t_map) {
    t_map[10] = new SequenceReset_10();
    t_map[11] = new Heartbeat_11();
    t_map[12] = new SecurityList_12();
    t_map[30] = new SecurityList_30();
    t_map[23] = new SecurityList_23();
    t_map[25] = new MarketDataIncrementalRefresh_25();
    t_map[13] = new MarketDataIncrementalRefresh_13();
    t_map[17] = new MarketDataIncrementalRefresh_17();
    t_map[18] = new MarketDataIncrementalRefresh_18();
    t_map[26] = new MarketDataIncrementalRefresh_26();
    t_map[14] = new MarketDataSnapshotFullRefresh_14();
    t_map[27] = new MarketDataSnapshotFullRefresh_27();
    t_map[19] = new MarketDataSnapshotFullRefresh_19();
    t_map[20] = new MarketDataSnapshotFullRefresh_20();
    t_map[28] = new MarketDataSnapshotFullRefresh_28();
    t_map[15] = new SecurityStatus_15();
    t_map[21] = new SecurityStatus_21();
    t_map[22] = new SecurityStatus_22();
    t_map[16] = new News_16();
    t_map[29] = new News_29();
    t_map[24] = new News_24();
  }
  static void cleanUpMem(std::map<int, FastDecoder *> &t_map) {
    delete t_map[10];
    delete t_map[11];
    delete t_map[12];
    delete t_map[30];
    delete t_map[23];
    delete t_map[25];
    delete t_map[13];
    delete t_map[17];
    delete t_map[18];
    delete t_map[26];
    delete t_map[14];
    delete t_map[27];
    delete t_map[19];
    delete t_map[20];
    delete t_map[28];
    delete t_map[15];
    delete t_map[21];
    delete t_map[22];
    delete t_map[16];
    delete t_map[29];
    delete t_map[24];
  }
};
};
}
