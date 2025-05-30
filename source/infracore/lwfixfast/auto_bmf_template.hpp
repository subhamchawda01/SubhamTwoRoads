// AUTO GENERATED CLASS FOR TEMPLATE files/BMF/templates-UMDF-GTS.xml
// DO NOT MODIFY

#pragma once

#include "infracore/lwfixfast/FastDecoder.hpp"
namespace BMF_TEMPLATE_DECODER {
class SequenceReset_10 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t> *NewSeqNo;

  // Constructor
  SequenceReset_10() {
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;

  // Constructor
  Heartbeat_11() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("0"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t> *TotNoRelatedSym;
  NoOpField<uint32_t> *LastFragment;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t> *NoRelatedSym;
  CopyField<FFUtils::ByteArr> *Symbol;
  CopyField<FFUtils::ByteArr> *SecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityIDSource;
  CopyField<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence SecurityAltID
  NoOpField<uint32_t> *NoSecurityAltID;
  CopyField<FFUtils::ByteArr> *SecurityAltID;
  CopyField<FFUtils::ByteArr> *SecurityAltIDSource;
  // fields inside sequence SecurityAltID ends
  // fields inside sequence Underlyings
  NoOpField<uint32_t> *NoUnderlyings;
  CopyField<FFUtils::ByteArr> *UnderlyingSymbol;
  CopyField<FFUtils::ByteArr> *UnderlyingSecurityID;
  DefaultField<FFUtils::ByteArr> *UnderlyingSecurityIDSource;
  CopyField<FFUtils::ByteArr> *UnderlyingSecurityExchange;
  CopyField<FFUtils::Decimal> *IndexPct;
  // fields inside sequence Underlyings ends
  // fields inside sequence Legs
  NoOpField<uint32_t> *NoLegs;
  CopyField<FFUtils::ByteArr> *LegSymbol;
  CopyField<FFUtils::ByteArr> *LegSecurityID;
  DefaultField<FFUtils::ByteArr> *LegSecurityIDSource;
  CopyField<FFUtils::ByteArr> *LegSecurityExchange;
  // fields inside sequence Legs ends
  CopyField<FFUtils::ByteArr> *SecurityUpdateAction;
  CopyField<FFUtils::Decimal> *RoundLot;
  CopyField<FFUtils::Decimal> *MinTradeVol;
  CopyField<FFUtils::Decimal> *MinPriceIncrement;
  CopyField<uint32_t> *TickSizeDenominator;
  CopyField<FFUtils::Decimal> *MinOrderQty;
  CopyField<FFUtils::Decimal> *MaxOrderQty;
  CopyField<int32_t> *InstrumentId;
  CopyField<FFUtils::ByteArr> *Currency;
  CopyField<FFUtils::ByteArr> *SecurityType;
  CopyField<FFUtils::ByteArr> *SecuritySubType;
  CopyField<int32_t> *Product;
  CopyField<FFUtils::ByteArr> *Asset;
  CopyField<FFUtils::ByteArr> *SecurityDesc;
  CopyField<uint32_t> *MaturityDate;
  CopyField<FFUtils::ByteArr> *MaturityMonthYear;
  CopyField<FFUtils::Decimal> *StrikePrice;
  CopyField<FFUtils::ByteArr> *StrikeCurrency;
  CopyField<FFUtils::Decimal> *ContractMultiplier;
  CopyField<FFUtils::ByteArr> *ContractSettlMonth;
  CopyField<FFUtils::ByteArr> *CFICode;
  CopyField<FFUtils::ByteArr> *CountryOfIssue;
  CopyField<uint32_t> *IssueDate;
  CopyField<uint32_t> *DatedDate;
  CopyField<uint32_t> *StartDate;
  CopyField<uint32_t> *EndDate;
  CopyField<FFUtils::ByteArr> *SettlType;
  CopyField<uint32_t> *SettlDate;
  CopyField<int32_t> *PriceType;
  CopyField<uint64_t> *SecurityValidityTimestamp;
  CopyField<FFUtils::ByteArr> *SecurityGroup;
  // fields inside sequence RelatedSym ends

  // Constructor
  SecurityList_12() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("y"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    TotNoRelatedSym = new NoOpField<uint32_t>(false, false, 0);
    LastFragment = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t>(true, false, 0);
    Symbol = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityID = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityIDSource = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    SecurityExchange = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("XBSP"));
    // constructor for sequence SecurityAltID
    NoSecurityAltID = new NoOpField<uint32_t>(true, false, 0);
    SecurityAltID = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityAltIDSource = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence SecurityAltID ends
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t>(true, false, 0);
    UnderlyingSymbol = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    UnderlyingSecurityID = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    UnderlyingSecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    UnderlyingSecurityExchange = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("XBSP"));
    IndexPct = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    // constructor for sequence Underlyings ends
    // constructor for sequence Legs
    NoLegs = new NoOpField<uint32_t>(true, false, 0);
    LegSymbol = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegSecurityID = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegSecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    LegSecurityExchange = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("XBSP"));
    // constructor for sequence Legs ends
    SecurityUpdateAction = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    RoundLot = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MinTradeVol = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MinPriceIncrement = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    TickSizeDenominator = new CopyField<uint32_t>(false, false, 0);
    MinOrderQty = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MaxOrderQty = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    InstrumentId = new CopyField<int32_t>(false, false, 0);
    Currency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityType = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecuritySubType = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Product = new CopyField<int32_t>(false, false, 0);
    Asset = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityDesc = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MaturityDate = new CopyField<uint32_t>(false, false, 0);
    MaturityMonthYear = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    StrikePrice = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    StrikeCurrency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    ContractMultiplier = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    ContractSettlMonth = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    CFICode = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    CountryOfIssue = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    IssueDate = new CopyField<uint32_t>(false, false, 0);
    DatedDate = new CopyField<uint32_t>(false, false, 0);
    StartDate = new CopyField<uint32_t>(false, false, 0);
    EndDate = new CopyField<uint32_t>(false, false, 0);
    SettlType = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlDate = new CopyField<uint32_t>(false, false, 0);
    PriceType = new CopyField<int32_t>(false, false, 0);
    SecurityValidityTimestamp = new CopyField<uint64_t>(false, false, 0);
    SecurityGroup = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
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

    int NoRelatedSym_len = NoRelatedSym->previousValue.getValue();
    for (int i = 0; i < NoRelatedSym_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      Symbol->decode(input, pmap1);
      SecurityID->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityExchange->decode(input, pmap1);
      // decode for sequence SecurityAltID
      NoSecurityAltID->decode(input);

      int NoSecurityAltID_len = NoSecurityAltID->previousValue.getValue();
      for (int i = 0; i < NoSecurityAltID_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap2 = input.extractPmap();

        SecurityAltID->decode(input, pmap2);
        SecurityAltIDSource->decode(input, pmap2);
      }
      // decode for sequence SecurityAltID ends
      // decode for sequence Underlyings
      NoUnderlyings->decode(input);

      int NoUnderlyings_len = NoUnderlyings->previousValue.getValue();
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

      int NoLegs_len = NoLegs->previousValue.getValue();
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  DefaultField<uint32_t> *TotNoRelatedSym;
  DefaultField<uint32_t> *LastFragment;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t> *NoRelatedSym;
  DeltaField<FFUtils::ByteArr> *SecurityID;
  DefaultField<FFUtils::ByteArr> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr> *SecurityExchange;
  DeltaField<FFUtils::ByteArr> *Symbol;
  // fields inside sequence SecurityAltID
  NoOpField<uint32_t> *NoSecurityAltID;
  DeltaField<FFUtils::ByteArr> *SecurityAltID;
  DefaultField<FFUtils::ByteArr> *SecurityAltIDSource;
  // fields inside sequence SecurityAltID ends
  // fields inside sequence Underlyings
  NoOpField<uint32_t> *NoUnderlyings;
  DeltaField<FFUtils::ByteArr> *UnderlyingSymbol;
  DeltaField<FFUtils::ByteArr> *UnderlyingSecurityID;
  DefaultField<FFUtils::ByteArr> *UnderlyingSecurityIDSource;
  DefaultField<FFUtils::ByteArr> *UnderlyingSecurityExchange;
  CopyField<FFUtils::Decimal> *IndexPct;
  // fields inside sequence Underlyings ends
  // fields inside sequence Legs
  NoOpField<uint32_t> *NoLegs;
  DeltaField<FFUtils::ByteArr> *LegSymbol;
  DeltaField<FFUtils::ByteArr> *LegSecurityID;
  DefaultField<FFUtils::ByteArr> *LegSecurityIDSource;
  DefaultField<FFUtils::ByteArr> *LegSecurityExchange;
  DefaultField<FFUtils::ByteArr> *LegSide;
  DefaultField<FFUtils::Decimal> *LegRatioQty;
  // fields inside sequence Legs ends
  CopyField<FFUtils::ByteArr> *SecurityUpdateAction;
  CopyField<FFUtils::Decimal> *RoundLot;
  CopyField<FFUtils::Decimal> *MinTradeVol;
  CopyField<FFUtils::Decimal> *MinPriceIncrement;
  CopyField<uint32_t> *TickSizeDenominator;
  CopyField<FFUtils::Decimal> *MinOrderQty;
  CopyField<FFUtils::Decimal> *MaxOrderQty;
  DefaultField<int32_t> *InstrumentId;
  CopyField<FFUtils::ByteArr> *Currency;
  CopyField<FFUtils::ByteArr> *SecurityType;
  CopyField<FFUtils::ByteArr> *SecuritySubType;
  CopyField<FFUtils::ByteArr> *Asset;
  DeltaField<FFUtils::ByteArr> *SecurityDesc;
  DeltaField<uint32_t> *MaturityDate;
  CopyField<FFUtils::ByteArr> *MaturityMonthYear;
  CopyField<FFUtils::Decimal> *StrikePrice;
  CopyField<FFUtils::ByteArr> *StrikeCurrency;
  DefaultField<uint32_t> *ExerciseStyle;
  DefaultField<uint32_t> *PutOrCall;
  CopyField<FFUtils::Decimal> *ContractMultiplier;
  CopyField<uint32_t> *PriceDivisor;
  CopyField<FFUtils::ByteArr> *ContractSettlMonth;
  CopyField<FFUtils::ByteArr> *CFICode;
  CopyField<FFUtils::ByteArr> *CountryOfIssue;
  CopyField<uint32_t> *IssueDate;
  DefaultField<uint32_t> *DatedDate;
  CopyField<uint32_t> *StartDate;
  CopyField<uint32_t> *EndDate;
  CopyField<FFUtils::ByteArr> *SettlType;
  CopyField<uint32_t> *SettlDate;
  CopyField<int32_t> *PriceType;
  CopyField<uint64_t> *SecurityValidityTimestamp;
  CopyField<FFUtils::ByteArr> *SecurityGroup;
  // fields inside sequence RelatedSym ends

  // Constructor
  SecurityList_30() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("y"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    TotNoRelatedSym = new DefaultField<uint32_t>(false, false, 0);
    LastFragment = new DefaultField<uint32_t>(false, false, 0);
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t>(true, false, 0);
    SecurityID = new DeltaField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    SecurityExchange = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    Symbol = new DeltaField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence SecurityAltID
    NoSecurityAltID = new NoOpField<uint32_t>(true, false, 0);
    SecurityAltID = new DeltaField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityAltIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("4"));
    // constructor for sequence SecurityAltID ends
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t>(true, false, 0);
    UnderlyingSymbol = new DeltaField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    UnderlyingSecurityID = new DeltaField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    UnderlyingSecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    UnderlyingSecurityExchange = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    IndexPct = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    // constructor for sequence Underlyings ends
    // constructor for sequence Legs
    NoLegs = new NoOpField<uint32_t>(true, false, 0);
    LegSymbol = new DeltaField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegSecurityID = new DeltaField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegSecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    LegSecurityExchange = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    LegSide = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegRatioQty = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    // constructor for sequence Legs ends
    SecurityUpdateAction = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    RoundLot = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MinTradeVol = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MinPriceIncrement = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    TickSizeDenominator = new CopyField<uint32_t>(false, false, 0);
    MinOrderQty = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MaxOrderQty = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    InstrumentId = new DefaultField<int32_t>(false, false, 0);
    Currency = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BRL"));
    SecurityType = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecuritySubType = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Asset = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityDesc = new DeltaField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MaturityDate = new DeltaField<uint32_t>(false, false, 0);
    MaturityMonthYear = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    StrikePrice = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    StrikeCurrency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    ExerciseStyle = new DefaultField<uint32_t>(false, false, 0);
    PutOrCall = new DefaultField<uint32_t>(false, false, 0);
    ContractMultiplier = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PriceDivisor = new CopyField<uint32_t>(false, false, 0);
    ContractSettlMonth = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    CFICode = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    CountryOfIssue = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    IssueDate = new CopyField<uint32_t>(false, false, 0);
    DatedDate = new DefaultField<uint32_t>(false, false, 0);
    StartDate = new CopyField<uint32_t>(false, false, 0);
    EndDate = new CopyField<uint32_t>(false, false, 0);
    SettlType = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlDate = new CopyField<uint32_t>(false, false, 0);
    PriceType = new CopyField<int32_t>(false, false, 0);
    SecurityValidityTimestamp = new CopyField<uint64_t>(false, false, 0);
    SecurityGroup = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
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

    int NoRelatedSym_len = NoRelatedSym->previousValue.getValue();
    for (int i = 0; i < NoRelatedSym_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      SecurityID->decodeString(input);
      SecurityIDSource->decode(input, pmap1);
      SecurityExchange->decode(input, pmap1);
      Symbol->decodeString(input);
      // decode for sequence SecurityAltID
      NoSecurityAltID->decode(input);

      int NoSecurityAltID_len = NoSecurityAltID->previousValue.getValue();
      for (int i = 0; i < NoSecurityAltID_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap2 = input.extractPmap();

        SecurityAltID->decodeString(input);
        SecurityAltIDSource->decode(input, pmap2);
      }
      // decode for sequence SecurityAltID ends
      // decode for sequence Underlyings
      NoUnderlyings->decode(input);

      int NoUnderlyings_len = NoUnderlyings->previousValue.getValue();
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

      int NoLegs_len = NoLegs->previousValue.getValue();
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t> *TotNoRelatedSym;
  NoOpField<uint32_t> *LastFragment;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t> *NoRelatedSym;
  CopyField<FFUtils::ByteArr> *Symbol;
  CopyField<FFUtils::ByteArr> *SecurityID;
  DefaultField<FFUtils::ByteArr> *SecurityIDSource;
  CopyField<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence SecurityAltID
  NoOpField<uint32_t> *NoSecurityAltID;
  CopyField<FFUtils::ByteArr> *SecurityAltID;
  CopyField<FFUtils::ByteArr> *SecurityAltIDSource;
  // fields inside sequence SecurityAltID ends
  // fields inside sequence Underlyings
  NoOpField<uint32_t> *NoUnderlyings;
  CopyField<FFUtils::ByteArr> *UnderlyingSymbol;
  CopyField<FFUtils::ByteArr> *UnderlyingSecurityID;
  DefaultField<FFUtils::ByteArr> *UnderlyingSecurityIDSource;
  CopyField<FFUtils::ByteArr> *UnderlyingSecurityExchange;
  CopyField<FFUtils::Decimal> *IndexPct;
  // fields inside sequence Underlyings ends
  // fields inside sequence Legs
  NoOpField<uint32_t> *NoLegs;
  CopyField<FFUtils::ByteArr> *LegSymbol;
  CopyField<FFUtils::ByteArr> *LegSecurityID;
  DefaultField<FFUtils::ByteArr> *LegSecurityIDSource;
  CopyField<FFUtils::ByteArr> *LegSecurityExchange;
  // fields inside sequence Legs ends
  CopyField<FFUtils::ByteArr> *SecurityUpdateAction;
  CopyField<FFUtils::Decimal> *RoundLot;
  CopyField<FFUtils::Decimal> *MinTradeVol;
  CopyField<FFUtils::Decimal> *MinPriceIncrement;
  CopyField<uint32_t> *TickSizeDenominator;
  CopyField<FFUtils::Decimal> *MinOrderQty;
  CopyField<FFUtils::Decimal> *MaxOrderQty;
  CopyField<int32_t> *InstrumentId;
  CopyField<FFUtils::ByteArr> *Currency;
  CopyField<FFUtils::ByteArr> *SecurityType;
  CopyField<FFUtils::ByteArr> *SecuritySubType;
  CopyField<FFUtils::ByteArr> *Asset;
  CopyField<FFUtils::ByteArr> *SecurityDesc;
  CopyField<uint32_t> *MaturityDate;
  CopyField<FFUtils::ByteArr> *MaturityMonthYear;
  CopyField<FFUtils::Decimal> *StrikePrice;
  CopyField<FFUtils::ByteArr> *StrikeCurrency;
  CopyField<FFUtils::Decimal> *ContractMultiplier;
  CopyField<FFUtils::ByteArr> *ContractSettlMonth;
  CopyField<FFUtils::ByteArr> *CFICode;
  CopyField<FFUtils::ByteArr> *CountryOfIssue;
  CopyField<uint32_t> *IssueDate;
  CopyField<uint32_t> *DatedDate;
  CopyField<uint32_t> *StartDate;
  CopyField<uint32_t> *EndDate;
  CopyField<FFUtils::ByteArr> *SettlType;
  CopyField<uint32_t> *SettlDate;
  CopyField<int32_t> *PriceType;
  CopyField<uint64_t> *SecurityValidityTimestamp;
  CopyField<FFUtils::ByteArr> *SecurityGroup;
  // fields inside sequence RelatedSym ends

  // Constructor
  SecurityList_23() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("y"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    TotNoRelatedSym = new NoOpField<uint32_t>(false, false, 0);
    LastFragment = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t>(true, false, 0);
    Symbol = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    SecurityExchange = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence SecurityAltID
    NoSecurityAltID = new NoOpField<uint32_t>(true, false, 0);
    SecurityAltID = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityAltIDSource = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence SecurityAltID ends
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t>(true, false, 0);
    UnderlyingSymbol = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    UnderlyingSecurityID = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    UnderlyingSecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    UnderlyingSecurityExchange = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    IndexPct = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    // constructor for sequence Underlyings ends
    // constructor for sequence Legs
    NoLegs = new NoOpField<uint32_t>(true, false, 0);
    LegSymbol = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegSecurityID = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LegSecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    LegSecurityExchange = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence Legs ends
    SecurityUpdateAction = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    RoundLot = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MinTradeVol = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MinPriceIncrement = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    TickSizeDenominator = new CopyField<uint32_t>(false, false, 0);
    MinOrderQty = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MaxOrderQty = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    InstrumentId = new CopyField<int32_t>(false, false, 0);
    Currency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityType = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecuritySubType = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Asset = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityDesc = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MaturityDate = new CopyField<uint32_t>(false, false, 0);
    MaturityMonthYear = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    StrikePrice = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    StrikeCurrency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    ContractMultiplier = new CopyField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    ContractSettlMonth = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    CFICode = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    CountryOfIssue = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    IssueDate = new CopyField<uint32_t>(false, false, 0);
    DatedDate = new CopyField<uint32_t>(false, false, 0);
    StartDate = new CopyField<uint32_t>(false, false, 0);
    EndDate = new CopyField<uint32_t>(false, false, 0);
    SettlType = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlDate = new CopyField<uint32_t>(false, false, 0);
    PriceType = new CopyField<int32_t>(false, false, 0);
    SecurityValidityTimestamp = new CopyField<uint64_t>(false, false, 0);
    SecurityGroup = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
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

    int NoRelatedSym_len = NoRelatedSym->previousValue.getValue();
    for (int i = 0; i < NoRelatedSym_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      Symbol->decode(input, pmap1);
      SecurityID->decode(input, pmap1);
      SecurityIDSource->decode(input, pmap1);
      SecurityExchange->decode(input, pmap1);
      // decode for sequence SecurityAltID
      NoSecurityAltID->decode(input);

      int NoSecurityAltID_len = NoSecurityAltID->previousValue.getValue();
      for (int i = 0; i < NoSecurityAltID_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap2 = input.extractPmap();

        SecurityAltID->decode(input, pmap2);
        SecurityAltIDSource->decode(input, pmap2);
      }
      // decode for sequence SecurityAltID ends
      // decode for sequence Underlyings
      NoUnderlyings->decode(input);

      int NoUnderlyings_len = NoUnderlyings->previousValue.getValue();
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

      int NoLegs_len = NoLegs->previousValue.getValue();
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  DefaultField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<FFUtils::ByteArr> *MDUpdateAction;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  IncrementField<uint32_t> *RptSeq;
  DefaultField<uint32_t> *PriceBandType;
  CopyField<int64_t> *SecurityID;
  DefaultField<FFUtils::ByteArr> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr> *SecurityExchange;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  DeltaField<FFUtils::Decimal> *MDEntryPx;
  DeltaField<FFUtils::Decimal> *MDEntrySize;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<uint32_t> *MDEntryTime;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *OpenCloseSettlFlag;
  DefaultField<uint64_t> *NoSharesIssued;
  DefaultField<FFUtils::ByteArr> *Currency;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  CopyField<FFUtils::ByteArr> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<uint32_t> *NumberOfOrders;
  IncrementField<uint32_t> *MDEntryPositionNo;
  DefaultField<uint32_t> *PriceType;
  DefaultField<FFUtils::Decimal> *NetChgPrevDay;
  DefaultField<uint32_t> *SellerDays;
  DefaultField<uint32_t> *SettlPriceType;
  DefaultField<FFUtils::Decimal> *TradeVolume;
  DefaultField<uint32_t> *PriceLimitType;
  DefaultField<FFUtils::Decimal> *LowLimitPrice;
  DefaultField<FFUtils::Decimal> *HighLimitPrice;
  DefaultField<FFUtils::Decimal> *TradingReferencePrice;
  DefaultField<uint64_t> *MDEntryID;
  DefaultField<uint32_t> *MDInsertDate;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataIncrementalRefresh_25() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    TradeDate = new DefaultField<uint32_t>(false, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    RptSeq = new IncrementField<uint32_t>(false, false, 0);
    PriceBandType = new DefaultField<uint32_t>(false, false, 0);
    SecurityID = new CopyField<int64_t>(false, false, 0);
    SecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    SecurityExchange = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryPx = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntrySize = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntryDate = new CopyField<uint32_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettlFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NoSharesIssued = new DefaultField<uint64_t>(false, false, 0);
    Currency = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    MDEntryPositionNo = new IncrementField<uint32_t>(false, false, 0);
    PriceType = new DefaultField<uint32_t>(false, false, 0);
    NetChgPrevDay = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    SettlPriceType = new DefaultField<uint32_t>(false, false, 0);
    TradeVolume = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PriceLimitType = new DefaultField<uint32_t>(false, false, 0);
    LowLimitPrice = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    HighLimitPrice = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    TradingReferencePrice = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntryID = new DefaultField<uint64_t>(false, false, 0);
    MDInsertDate = new DefaultField<uint32_t>(false, false, 0);
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

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<FFUtils::ByteArr> *MDUpdateAction;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  CopyField<FFUtils::ByteArr> *Symbol;
  CopyField<FFUtils::ByteArr> *SecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityIDSource;
  CopyField<FFUtils::ByteArr> *SecurityExchange;
  DeltaField<FFUtils::Decimal> *MDEntryPx;
  DeltaField<FFUtils::Decimal> *MDEntrySize;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<uint32_t> *MDEntryTime;
  CopyField<FFUtils::ByteArr> *TickDirection;
  CopyField<FFUtils::ByteArr> *TradeCondition;
  CopyField<FFUtils::ByteArr> *OpenCloseSettlFlag;
  NoOpField<FFUtils::ByteArr> *OrderID;
  NoOpField<FFUtils::ByteArr> *TradeID;
  CopyField<FFUtils::ByteArr> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<uint32_t> *NumberOfOrders;
  DefaultField<uint32_t> *MDEntryPositionNo;
  DeltaField<FFUtils::Decimal> *NetChgPrevDay;
  DeltaField<FFUtils::Decimal> *TradeVolume;
  CopyField<FFUtils::ByteArr> *SettlType;
  CopyField<uint32_t> *SettlDate;
  CopyField<int32_t> *SettlPriceType;
  IncrementField<uint32_t> *RptSeq;
  NoOpField<int32_t> *PriceBandType;
  NoOpField<int32_t> *PriceLimitType;
  NoOpField<FFUtils::Decimal> *LowLimitPrice;
  NoOpField<FFUtils::Decimal> *HighLimitPrice;
  NoOpField<FFUtils::Decimal> *TradingReferencePrice;
  NoOpField<FFUtils::Decimal> *PercentageVar;
  NoOpField<uint32_t> *NoUnchangedSecurities;
  NoOpField<uint32_t> *NoNotTradedSecurities;
  NoOpField<uint32_t> *TotTradedSecurities;
  NoOpField<FFUtils::Decimal> *CapitalPct;
  NoOpField<FFUtils::Decimal> *PrevYearVariation;
  NoOpField<uint32_t> *NoFallingSecurities;
  NoOpField<uint32_t> *NoRisingSecurities;
  NoOpField<FFUtils::Decimal> *PercThresholdNormalTrade;
  NoOpField<FFUtils::Decimal> *PercThresholdCrossTrade;
  NoOpField<uint64_t> *DailyAvgShares30D;
  NoOpField<FFUtils::Decimal> *MaximumNormalSharesPerDailyAvgShares30DRatio;
  NoOpField<FFUtils::Decimal> *MaximumCrossSharesPerDailyAvgShares30DRatio;
  NoOpField<FFUtils::Decimal> *NormalSharesPerOutstandingSharesRatio;
  NoOpField<FFUtils::Decimal> *CrossSharesPerOutstandingSharesRatio;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataIncrementalRefresh_13() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    TradeDate = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    Symbol = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityID = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityIDSource = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    SecurityExchange = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("XBSP"));
    MDEntryPx = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntrySize = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntryDate = new CopyField<uint32_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    TickDirection = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettlFlag = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("5"));
    OrderID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new DefaultField<uint32_t>(false, true, 1);
    MDEntryPositionNo = new DefaultField<uint32_t>(false, true, 1);
    NetChgPrevDay = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    TradeVolume = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    SettlType = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlDate = new CopyField<uint32_t>(false, false, 0);
    SettlPriceType = new CopyField<int32_t>(false, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    PriceBandType = new NoOpField<int32_t>(false, false, 0);
    PriceLimitType = new NoOpField<int32_t>(false, false, 0);
    LowLimitPrice = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    HighLimitPrice = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    TradingReferencePrice = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PercentageVar = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NoUnchangedSecurities = new NoOpField<uint32_t>(false, false, 0);
    NoNotTradedSecurities = new NoOpField<uint32_t>(false, false, 0);
    TotTradedSecurities = new NoOpField<uint32_t>(false, false, 0);
    CapitalPct = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PrevYearVariation = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NoFallingSecurities = new NoOpField<uint32_t>(false, false, 0);
    NoRisingSecurities = new NoOpField<uint32_t>(false, false, 0);
    PercThresholdNormalTrade = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PercThresholdCrossTrade = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    DailyAvgShares30D = new NoOpField<uint64_t>(false, false, 0);
    MaximumNormalSharesPerDailyAvgShares30DRatio = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MaximumCrossSharesPerDailyAvgShares30DRatio = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NormalSharesPerOutstandingSharesRatio = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    CrossSharesPerOutstandingSharesRatio = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
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

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<FFUtils::ByteArr> *MDUpdateAction;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  IncrementField<uint32_t> *RptSeq;
  CopyField<int64_t> *SecurityID;
  DefaultField<FFUtils::ByteArr> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr> *SecurityExchange;
  NoOpField<FFUtils::ByteArr> *MDStreamID;
  DeltaField<FFUtils::Decimal> *MDEntryPx;
  DeltaField<FFUtils::Decimal> *MDEntrySize;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<uint32_t> *MDEntryTime;
  CopyField<FFUtils::ByteArr> *TickDirection;
  CopyField<FFUtils::ByteArr> *QuoteCondition;
  CopyField<FFUtils::ByteArr> *TradeCondition;
  NoOpField<FFUtils::ByteArr> *OpenCloseSettlFlag;
  NoOpField<FFUtils::ByteArr> *Currency;
  NoOpField<FFUtils::ByteArr> *OrderID;
  NoOpField<FFUtils::ByteArr> *TradeID;
  CopyField<FFUtils::ByteArr> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<uint32_t> *NumberOfOrders;
  IncrementField<uint32_t> *MDEntryPositionNo;
  NoOpField<uint32_t> *PriceType;
  DeltaField<FFUtils::Decimal> *NetChgPrevDay;
  NoOpField<uint32_t> *SellerDays;
  CopyField<int32_t> *SettlPriceType;
  DeltaField<FFUtils::Decimal> *TradeVolume;
  NoOpField<int32_t> *PriceLimitType;
  NoOpField<FFUtils::Decimal> *LowLimitPrice;
  NoOpField<FFUtils::Decimal> *HighLimitPrice;
  NoOpField<FFUtils::Decimal> *TradingReferencePrice;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataIncrementalRefresh_17() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    TradeDate = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    SecurityID = new CopyField<int64_t>(true, false, 0);
    SecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    SecurityExchange = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    MDStreamID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryPx = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntrySize = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntryDate = new CopyField<uint32_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    TickDirection = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettlFlag = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Currency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OrderID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new DefaultField<uint32_t>(false, true, 1);
    MDEntryPositionNo = new IncrementField<uint32_t>(false, false, 0);
    PriceType = new NoOpField<uint32_t>(false, false, 0);
    NetChgPrevDay = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    SellerDays = new NoOpField<uint32_t>(false, false, 0);
    SettlPriceType = new CopyField<int32_t>(false, false, 0);
    TradeVolume = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PriceLimitType = new NoOpField<int32_t>(false, false, 0);
    LowLimitPrice = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    HighLimitPrice = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    TradingReferencePrice = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
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

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<FFUtils::ByteArr> *MDUpdateAction;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  IncrementField<uint32_t> *RptSeq;
  CopyField<FFUtils::ByteArr> *SecurityID;
  DefaultField<FFUtils::ByteArr> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr> *SecurityExchange;
  NoOpField<FFUtils::ByteArr> *MDStreamID;
  DeltaField<FFUtils::Decimal> *MDEntryPx;
  DeltaField<FFUtils::Decimal> *MDEntrySize;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<uint32_t> *MDEntryTime;
  CopyField<FFUtils::ByteArr> *TickDirection;
  CopyField<FFUtils::ByteArr> *QuoteCondition;
  CopyField<FFUtils::ByteArr> *TradeCondition;
  NoOpField<FFUtils::ByteArr> *OpenCloseSettlFlag;
  NoOpField<FFUtils::ByteArr> *Currency;
  NoOpField<FFUtils::ByteArr> *OrderID;
  NoOpField<FFUtils::ByteArr> *TradeID;
  CopyField<FFUtils::ByteArr> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<uint32_t> *NumberOfOrders;
  DefaultField<uint32_t> *MDEntryPositionNo;
  DeltaField<FFUtils::Decimal> *NetChgPrevDay;
  NoOpField<uint32_t> *SellerDays;
  DeltaField<FFUtils::Decimal> *TradeVolume;
  NoOpField<FFUtils::Decimal> *PercentageVar;
  NoOpField<uint32_t> *NoUnchangedSecurities;
  NoOpField<uint32_t> *NoNotTradedSecurities;
  NoOpField<uint32_t> *TotTradedSecurities;
  NoOpField<FFUtils::Decimal> *CapitalPct;
  NoOpField<FFUtils::Decimal> *PrevYearVariation;
  NoOpField<uint32_t> *NoFallingSecurities;
  NoOpField<uint32_t> *NoRisingSecurities;
  NoOpField<FFUtils::Decimal> *PercThresholdNormalTrade;
  NoOpField<FFUtils::Decimal> *PercThresholdCrossTrade;
  NoOpField<uint64_t> *DailyAvgShares30D;
  NoOpField<FFUtils::Decimal> *MaximumNormalSharesPerDailyAvgShares30DRatio;
  NoOpField<FFUtils::Decimal> *MaximumCrossSharesPerDailyAvgShares30DRatio;
  NoOpField<FFUtils::Decimal> *NormalSharesPerOutstandingSharesRatio;
  NoOpField<FFUtils::Decimal> *CrossSharesPerOutstandingSharesRatio;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataIncrementalRefresh_18() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    TradeDate = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    SecurityID = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    SecurityExchange = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    MDStreamID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryPx = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntrySize = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntryDate = new CopyField<uint32_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    TickDirection = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettlFlag = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Currency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OrderID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new DefaultField<uint32_t>(false, true, 1);
    MDEntryPositionNo = new DefaultField<uint32_t>(false, true, 1);
    NetChgPrevDay = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    SellerDays = new NoOpField<uint32_t>(false, false, 0);
    TradeVolume = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PercentageVar = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NoUnchangedSecurities = new NoOpField<uint32_t>(false, false, 0);
    NoNotTradedSecurities = new NoOpField<uint32_t>(false, false, 0);
    TotTradedSecurities = new NoOpField<uint32_t>(false, false, 0);
    CapitalPct = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PrevYearVariation = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NoFallingSecurities = new NoOpField<uint32_t>(false, false, 0);
    NoRisingSecurities = new NoOpField<uint32_t>(false, false, 0);
    PercThresholdNormalTrade = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PercThresholdCrossTrade = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    DailyAvgShares30D = new NoOpField<uint64_t>(false, false, 0);
    MaximumNormalSharesPerDailyAvgShares30DRatio = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MaximumCrossSharesPerDailyAvgShares30DRatio = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NormalSharesPerOutstandingSharesRatio = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    CrossSharesPerOutstandingSharesRatio = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
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

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<FFUtils::ByteArr> *MDUpdateAction;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  IncrementField<uint32_t> *RptSeq;
  DefaultField<uint32_t> *PriceBandType;
  CopyField<FFUtils::ByteArr> *SecurityID;
  DefaultField<FFUtils::ByteArr> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr> *SecurityExchange;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  DeltaField<FFUtils::Decimal> *MDEntryPx;
  DeltaField<FFUtils::Decimal> *MDEntrySize;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<uint32_t> *MDEntryTime;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *OpenCloseSettlFlag;
  DefaultField<uint64_t> *NoSharesIssued;
  DefaultField<FFUtils::ByteArr> *Currency;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  CopyField<FFUtils::ByteArr> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<uint32_t> *NumberOfOrders;
  DeltaField<uint32_t> *MDEntryPositionNo;
  DefaultField<FFUtils::Decimal> *NetChgPrevDay;
  DefaultField<uint32_t> *SellerDays;
  DefaultField<uint64_t> *TradeVolume;
  DefaultField<FFUtils::Decimal> *PercentageVar;
  DefaultField<uint32_t> *NoUnchangedSecurities;
  DefaultField<uint32_t> *NoNotTradedSecurities;
  DefaultField<uint32_t> *TotTradedSecurities;
  DefaultField<FFUtils::Decimal> *CapitalPct;
  DefaultField<FFUtils::Decimal> *PrevYearVariation;
  DefaultField<uint32_t> *NoFallingSecurities;
  DefaultField<uint32_t> *NoRisingSecurities;
  DefaultField<FFUtils::Decimal> *PercThresholdNormalTrade;
  DefaultField<FFUtils::Decimal> *PercThresholdCrossTrade;
  DefaultField<uint64_t> *DailyAvgShares30D;
  DefaultField<FFUtils::Decimal> *MaximumNormalSharesPerDailyAvgShares30DRatio;
  DefaultField<FFUtils::Decimal> *MaximumCrossSharesPerDailyAvgShares30DRatio;
  DefaultField<FFUtils::Decimal> *NormalSharesPerOutstandingSharesRatio;
  DefaultField<FFUtils::Decimal> *CrossSharesPerOutstandingSharesRatio;
  DefaultField<uint64_t> *MDEntryID;
  DefaultField<uint32_t> *MDInsertDate;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataIncrementalRefresh_26() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    TradeDate = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    RptSeq = new IncrementField<uint32_t>(false, false, 0);
    PriceBandType = new DefaultField<uint32_t>(false, false, 0);
    SecurityID = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    SecurityExchange = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryPx = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntrySize = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntryDate = new CopyField<uint32_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettlFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NoSharesIssued = new DefaultField<uint64_t>(false, false, 0);
    Currency = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    MDEntryPositionNo = new DeltaField<uint32_t>(false, false, 0);
    NetChgPrevDay = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    TradeVolume = new DefaultField<uint64_t>(false, false, 0);
    PercentageVar = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NoUnchangedSecurities = new DefaultField<uint32_t>(false, false, 0);
    NoNotTradedSecurities = new DefaultField<uint32_t>(false, false, 0);
    TotTradedSecurities = new DefaultField<uint32_t>(false, false, 0);
    CapitalPct = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PrevYearVariation = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NoFallingSecurities = new DefaultField<uint32_t>(false, false, 0);
    NoRisingSecurities = new DefaultField<uint32_t>(false, false, 0);
    PercThresholdNormalTrade = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PercThresholdCrossTrade = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    DailyAvgShares30D = new DefaultField<uint64_t>(false, false, 0);
    MaximumNormalSharesPerDailyAvgShares30DRatio =
        new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MaximumCrossSharesPerDailyAvgShares30DRatio = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NormalSharesPerOutstandingSharesRatio = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    CrossSharesPerOutstandingSharesRatio = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntryID = new DefaultField<uint64_t>(false, false, 0);
    MDInsertDate = new DefaultField<uint32_t>(false, false, 0);
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

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *LastMsgSeqNumProcessed;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<int32_t> *TotNumReports;
  NoOpField<uint32_t> *TradeDate;
  NoOpField<FFUtils::Decimal> *NetChgPrevDay;
  NoOpField<int32_t> *MarketDepth;
  NoOpField<FFUtils::ByteArr> *Symbol;
  NoOpField<FFUtils::ByteArr> *SecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  DeltaField<FFUtils::Decimal> *MDEntryPx;
  DeltaField<FFUtils::Decimal> *MDEntrySize;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<uint32_t> *MDEntryTime;
  CopyField<FFUtils::ByteArr> *TickDirection;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  NoOpField<int32_t> *SecurityTradingStatus;
  NoOpField<uint64_t> *TradSesOpenTime;
  CopyField<FFUtils::ByteArr> *TradeCondition;
  CopyField<FFUtils::ByteArr> *OpenCloseSettlFlag;
  NoOpField<FFUtils::ByteArr> *OrderID;
  NoOpField<FFUtils::ByteArr> *TradeID;
  CopyField<FFUtils::ByteArr> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<uint32_t> *NumberOfOrders;
  DefaultField<uint32_t> *MDEntryPositionNo;
  DeltaField<FFUtils::Decimal> *TradeVolume;
  CopyField<FFUtils::ByteArr> *SettlType;
  CopyField<uint32_t> *SettlDate;
  CopyField<int32_t> *SettlPriceType;
  CopyField<uint32_t> *RptSeq;
  NoOpField<int32_t> *PriceBandType;
  NoOpField<int32_t> *PriceLimitType;
  NoOpField<FFUtils::Decimal> *LowLimitPrice;
  NoOpField<FFUtils::Decimal> *HighLimitPrice;
  NoOpField<FFUtils::Decimal> *TradingReferencePrice;
  NoOpField<FFUtils::Decimal> *PercentageVar;
  NoOpField<uint32_t> *NoUnchangedSecurities;
  NoOpField<uint32_t> *NoNotTradedSecurities;
  NoOpField<uint32_t> *TotTradedSecurities;
  NoOpField<FFUtils::Decimal> *CapitalPct;
  NoOpField<FFUtils::Decimal> *PrevYearVariation;
  NoOpField<uint32_t> *NoFallingSecurities;
  NoOpField<uint32_t> *NoRisingSecurities;
  NoOpField<FFUtils::Decimal> *PercThresholdNormalTrade;
  NoOpField<FFUtils::Decimal> *PercThresholdCrossTrade;
  NoOpField<uint64_t> *DailyAvgShares30D;
  NoOpField<FFUtils::Decimal> *MaximumNormalSharesPerDailyAvgShares30DRatio;
  NoOpField<FFUtils::Decimal> *MaximumCrossSharesPerDailyAvgShares30DRatio;
  NoOpField<FFUtils::Decimal> *NormalSharesPerOutstandingSharesRatio;
  NoOpField<FFUtils::Decimal> *CrossSharesPerOutstandingSharesRatio;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataSnapshotFullRefresh_14() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    LastMsgSeqNumProcessed = new NoOpField<uint32_t>(false, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    TotNumReports = new NoOpField<int32_t>(true, false, 0);
    TradeDate = new NoOpField<uint32_t>(false, false, 0);
    NetChgPrevDay = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MarketDepth = new NoOpField<int32_t>(false, false, 0);
    Symbol = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityID = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityIDSource = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    SecurityExchange = new DefaultField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("XBSP"));
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    MDEntryPx = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntrySize = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntryDate = new CopyField<uint32_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    TickDirection = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new NoOpField<int32_t>(false, false, 0);
    TradSesOpenTime = new NoOpField<uint64_t>(false, false, 0);
    TradeCondition = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettlFlag = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("5"));
    OrderID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new DefaultField<uint32_t>(false, true, 1);
    MDEntryPositionNo = new DefaultField<uint32_t>(false, true, 1);
    TradeVolume = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    SettlType = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlDate = new CopyField<uint32_t>(false, false, 0);
    SettlPriceType = new CopyField<int32_t>(false, false, 0);
    RptSeq = new CopyField<uint32_t>(false, false, 0);
    PriceBandType = new NoOpField<int32_t>(false, false, 0);
    PriceLimitType = new NoOpField<int32_t>(false, false, 0);
    LowLimitPrice = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    HighLimitPrice = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    TradingReferencePrice = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PercentageVar = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NoUnchangedSecurities = new NoOpField<uint32_t>(false, false, 0);
    NoNotTradedSecurities = new NoOpField<uint32_t>(false, false, 0);
    TotTradedSecurities = new NoOpField<uint32_t>(false, false, 0);
    CapitalPct = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PrevYearVariation = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NoFallingSecurities = new NoOpField<uint32_t>(false, false, 0);
    NoRisingSecurities = new NoOpField<uint32_t>(false, false, 0);
    PercThresholdNormalTrade = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PercThresholdCrossTrade = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    DailyAvgShares30D = new NoOpField<uint64_t>(false, false, 0);
    MaximumNormalSharesPerDailyAvgShares30DRatio = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MaximumCrossSharesPerDailyAvgShares30DRatio = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NormalSharesPerOutstandingSharesRatio = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    CrossSharesPerOutstandingSharesRatio = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
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

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *LastMsgSeqNumProcessed;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<int32_t> *TotNumReports;
  DefaultField<uint32_t> *TradeDate;
  DefaultField<FFUtils::Decimal> *NetChgPrevDay;
  DefaultField<int32_t> *MarketDepth;
  NoOpField<int64_t> *SecurityID;
  DefaultField<FFUtils::ByteArr> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  IncrementField<uint32_t> *RptSeq;
  DefaultField<uint32_t> *PriceBandType;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  DeltaField<FFUtils::Decimal> *MDEntryPx;
  DeltaField<FFUtils::Decimal> *MDEntrySize;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<uint32_t> *MDEntryTime;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<uint32_t> *SecurityTradingStatus;
  DefaultField<FFUtils::ByteArr> *TradingSessionSubID;
  DefaultField<uint64_t> *TradSesOpenTime;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *OpenCloseSettlFlag;
  DefaultField<uint64_t> *NoSharesIssued;
  CopyField<FFUtils::ByteArr> *Currency;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  CopyField<FFUtils::ByteArr> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<uint32_t> *NumberOfOrders;
  IncrementField<uint32_t> *MDEntryPositionNo;
  DefaultField<uint32_t> *PriceType;
  DefaultField<uint32_t> *SellerDays;
  DefaultField<uint32_t> *SettlPriceType;
  DefaultField<uint64_t> *TradeVolume;
  DefaultField<uint32_t> *PriceLimitType;
  DefaultField<FFUtils::Decimal> *LowLimitPrice;
  DefaultField<FFUtils::Decimal> *HighLimitPrice;
  DefaultField<FFUtils::Decimal> *TradingReferencePrice;
  DefaultField<uint64_t> *MDEntryID;
  DefaultField<uint32_t> *MDInsertDate;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataSnapshotFullRefresh_27() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    LastMsgSeqNumProcessed = new NoOpField<uint32_t>(false, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    TotNumReports = new NoOpField<int32_t>(true, false, 0);
    TradeDate = new DefaultField<uint32_t>(false, false, 0);
    NetChgPrevDay = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MarketDepth = new DefaultField<int32_t>(false, true, 0);
    SecurityID = new NoOpField<int64_t>(true, false, 0);
    SecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    SecurityExchange = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    RptSeq = new IncrementField<uint32_t>(false, false, 0);
    PriceBandType = new DefaultField<uint32_t>(false, false, 0);
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryPx = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntrySize = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntryDate = new CopyField<uint32_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new DefaultField<uint32_t>(false, false, 0);
    TradingSessionSubID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradSesOpenTime = new DefaultField<uint64_t>(false, false, 0);
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettlFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NoSharesIssued = new DefaultField<uint64_t>(false, false, 0);
    Currency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    MDEntryPositionNo = new IncrementField<uint32_t>(false, false, 0);
    PriceType = new DefaultField<uint32_t>(false, false, 0);
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    SettlPriceType = new DefaultField<uint32_t>(false, false, 0);
    TradeVolume = new DefaultField<uint64_t>(false, false, 0);
    PriceLimitType = new DefaultField<uint32_t>(false, false, 0);
    LowLimitPrice = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    HighLimitPrice = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    TradingReferencePrice = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntryID = new DefaultField<uint64_t>(false, false, 0);
    MDInsertDate = new DefaultField<uint32_t>(false, false, 0);
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

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *LastMsgSeqNumProcessed;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<int32_t> *TotNumReports;
  NoOpField<uint32_t> *TradeDate;
  NoOpField<FFUtils::Decimal> *NetChgPrevDay;
  NoOpField<int32_t> *MarketDepth;
  NoOpField<int64_t> *SecurityID;
  DefaultField<FFUtils::ByteArr> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  CopyField<uint32_t> *RptSeq;
  NoOpField<FFUtils::ByteArr> *MDStreamID;
  DeltaField<FFUtils::Decimal> *MDEntryPx;
  DeltaField<FFUtils::Decimal> *MDEntrySize;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<uint32_t> *MDEntryTime;
  CopyField<FFUtils::ByteArr> *TickDirection;
  NoOpField<int32_t> *SecurityTradingStatus;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  NoOpField<uint64_t> *TradSesOpenTime;
  CopyField<FFUtils::ByteArr> *QuoteCondition;
  CopyField<FFUtils::ByteArr> *TradeCondition;
  NoOpField<FFUtils::ByteArr> *OpenCloseSettlFlag;
  CopyField<FFUtils::ByteArr> *Currency;
  NoOpField<FFUtils::ByteArr> *OrderID;
  NoOpField<FFUtils::ByteArr> *TradeID;
  CopyField<FFUtils::ByteArr> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<uint32_t> *NumberOfOrders;
  IncrementField<uint32_t> *MDEntryPositionNo;
  NoOpField<uint32_t> *PriceType;
  NoOpField<uint32_t> *SellerDays;
  CopyField<uint32_t> *SettlPriceType;
  DeltaField<FFUtils::Decimal> *TradeVolume;
  NoOpField<int32_t> *PriceLimitType;
  NoOpField<FFUtils::Decimal> *LowLimitPrice;
  NoOpField<FFUtils::Decimal> *HighLimitPrice;
  NoOpField<FFUtils::Decimal> *TradingReferencePrice;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataSnapshotFullRefresh_19() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    LastMsgSeqNumProcessed = new NoOpField<uint32_t>(false, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    TotNumReports = new NoOpField<int32_t>(true, false, 0);
    TradeDate = new NoOpField<uint32_t>(false, false, 0);
    NetChgPrevDay = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MarketDepth = new NoOpField<int32_t>(false, false, 0);
    SecurityID = new NoOpField<int64_t>(true, false, 0);
    SecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    SecurityExchange = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    RptSeq = new CopyField<uint32_t>(false, false, 0);
    MDStreamID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryPx = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntrySize = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntryDate = new CopyField<uint32_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    TickDirection = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new NoOpField<int32_t>(false, false, 0);
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradSesOpenTime = new NoOpField<uint64_t>(false, false, 0);
    QuoteCondition = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettlFlag = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Currency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OrderID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new DefaultField<uint32_t>(false, true, 1);
    MDEntryPositionNo = new IncrementField<uint32_t>(false, false, 0);
    PriceType = new NoOpField<uint32_t>(false, false, 0);
    SellerDays = new NoOpField<uint32_t>(false, false, 0);
    SettlPriceType = new CopyField<uint32_t>(false, false, 0);
    TradeVolume = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PriceLimitType = new NoOpField<int32_t>(false, false, 0);
    LowLimitPrice = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    HighLimitPrice = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    TradingReferencePrice = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
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

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *LastMsgSeqNumProcessed;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<int32_t> *TotNumReports;
  NoOpField<uint32_t> *TradeDate;
  NoOpField<FFUtils::Decimal> *NetChgPrevDay;
  NoOpField<int32_t> *MarketDepth;
  NoOpField<FFUtils::ByteArr> *SecurityID;
  DefaultField<FFUtils::ByteArr> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  CopyField<uint32_t> *RptSeq;
  NoOpField<FFUtils::ByteArr> *MDStreamID;
  DeltaField<FFUtils::Decimal> *MDEntryPx;
  DeltaField<FFUtils::Decimal> *MDEntrySize;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<uint32_t> *MDEntryTime;
  CopyField<FFUtils::ByteArr> *TickDirection;
  NoOpField<int32_t> *SecurityTradingStatus;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  NoOpField<uint64_t> *TradSesOpenTime;
  CopyField<FFUtils::ByteArr> *QuoteCondition;
  CopyField<FFUtils::ByteArr> *TradeCondition;
  NoOpField<FFUtils::ByteArr> *OpenCloseSettlFlag;
  CopyField<FFUtils::ByteArr> *Currency;
  NoOpField<FFUtils::ByteArr> *OrderID;
  NoOpField<FFUtils::ByteArr> *TradeID;
  CopyField<FFUtils::ByteArr> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<uint32_t> *NumberOfOrders;
  IncrementField<uint32_t> *MDEntryPositionNo;
  NoOpField<uint32_t> *SellerDays;
  DeltaField<FFUtils::Decimal> *TradeVolume;
  NoOpField<FFUtils::Decimal> *PercentageVar;
  NoOpField<uint32_t> *NoUnchangedSecurities;
  NoOpField<uint32_t> *NoNotTradedSecurities;
  NoOpField<uint32_t> *TotTradedSecurities;
  NoOpField<FFUtils::Decimal> *CapitalPct;
  NoOpField<FFUtils::Decimal> *PrevYearVariation;
  NoOpField<uint32_t> *NoFallingSecurities;
  NoOpField<uint32_t> *NoRisingSecurities;
  NoOpField<FFUtils::Decimal> *PercThresholdNormalTrade;
  NoOpField<FFUtils::Decimal> *PercThresholdCrossTrade;
  NoOpField<uint64_t> *DailyAvgShares30D;
  NoOpField<FFUtils::Decimal> *MaximumNormalSharesPerDailyAvgShares30DRatio;
  NoOpField<FFUtils::Decimal> *MaximumCrossSharesPerDailyAvgShares30DRatio;
  NoOpField<FFUtils::Decimal> *NormalSharesPerOutstandingSharesRatio;
  NoOpField<FFUtils::Decimal> *CrossSharesPerOutstandingSharesRatio;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataSnapshotFullRefresh_20() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    LastMsgSeqNumProcessed = new NoOpField<uint32_t>(false, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    TotNumReports = new NoOpField<int32_t>(true, false, 0);
    TradeDate = new NoOpField<uint32_t>(false, false, 0);
    NetChgPrevDay = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MarketDepth = new NoOpField<int32_t>(false, false, 0);
    SecurityID = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    SecurityExchange = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    RptSeq = new CopyField<uint32_t>(false, false, 0);
    MDStreamID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryPx = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntrySize = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntryDate = new CopyField<uint32_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    TickDirection = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new NoOpField<int32_t>(false, false, 0);
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradSesOpenTime = new NoOpField<uint64_t>(false, false, 0);
    QuoteCondition = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettlFlag = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Currency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OrderID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new DefaultField<uint32_t>(false, true, 1);
    MDEntryPositionNo = new IncrementField<uint32_t>(false, false, 0);
    SellerDays = new NoOpField<uint32_t>(false, false, 0);
    TradeVolume = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PercentageVar = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NoUnchangedSecurities = new NoOpField<uint32_t>(false, false, 0);
    NoNotTradedSecurities = new NoOpField<uint32_t>(false, false, 0);
    TotTradedSecurities = new NoOpField<uint32_t>(false, false, 0);
    CapitalPct = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PrevYearVariation = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NoFallingSecurities = new NoOpField<uint32_t>(false, false, 0);
    NoRisingSecurities = new NoOpField<uint32_t>(false, false, 0);
    PercThresholdNormalTrade = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PercThresholdCrossTrade = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    DailyAvgShares30D = new NoOpField<uint64_t>(false, false, 0);
    MaximumNormalSharesPerDailyAvgShares30DRatio = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MaximumCrossSharesPerDailyAvgShares30DRatio = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NormalSharesPerOutstandingSharesRatio = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    CrossSharesPerOutstandingSharesRatio = new NoOpField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
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

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *LastMsgSeqNumProcessed;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<int32_t> *TotNumReports;
  DefaultField<uint32_t> *TradeDate;
  DefaultField<FFUtils::Decimal> *NetChgPrevDay;
  DefaultField<int32_t> *MarketDepth;
  NoOpField<FFUtils::ByteArr> *SecurityID;
  DefaultField<FFUtils::ByteArr> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  IncrementField<uint32_t> *RptSeq;
  DefaultField<uint32_t> *PriceBandType;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  DeltaField<FFUtils::Decimal> *MDEntryPx;
  DeltaField<FFUtils::Decimal> *MDEntrySize;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<uint32_t> *MDEntryTime;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  DefaultField<uint32_t> *SecurityTradingStatus;
  DefaultField<FFUtils::ByteArr> *TradingSessionSubID;
  DefaultField<uint64_t> *TradSesOpenTime;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *OpenCloseSettlFlag;
  DefaultField<uint64_t> *NoSharesIssued;
  CopyField<FFUtils::ByteArr> *Currency;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  CopyField<FFUtils::ByteArr> *MDEntryBuyer;
  CopyField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<uint32_t> *NumberOfOrders;
  IncrementField<uint32_t> *MDEntryPositionNo;
  DefaultField<uint32_t> *SellerDays;
  DefaultField<uint64_t> *TradeVolume;
  DefaultField<FFUtils::Decimal> *PercentageVar;
  DefaultField<uint32_t> *NoUnchangedSecurities;
  DefaultField<uint32_t> *NoNotTradedSecurities;
  DefaultField<uint32_t> *TotTradedSecurities;
  DefaultField<FFUtils::Decimal> *CapitalPct;
  DefaultField<FFUtils::Decimal> *PrevYearVariation;
  DefaultField<uint32_t> *NoFallingSecurities;
  DefaultField<uint32_t> *NoRisingSecurities;
  DefaultField<FFUtils::Decimal> *PercThresholdNormalTrade;
  DefaultField<FFUtils::Decimal> *PercThresholdCrossTrade;
  DefaultField<uint64_t> *DailyAvgShares30D;
  DefaultField<FFUtils::Decimal> *MaximumNormalSharesPerDailyAvgShares30DRatio;
  DefaultField<FFUtils::Decimal> *MaximumCrossSharesPerDailyAvgShares30DRatio;
  DefaultField<FFUtils::Decimal> *NormalSharesPerOutstandingSharesRatio;
  DefaultField<FFUtils::Decimal> *CrossSharesPerOutstandingSharesRatio;
  DefaultField<uint64_t> *MDEntryID;
  DefaultField<uint32_t> *MDInsertDate;
  // fields inside sequence MDEntries ends

  // Constructor
  MarketDataSnapshotFullRefresh_28() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    LastMsgSeqNumProcessed = new NoOpField<uint32_t>(false, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    TotNumReports = new NoOpField<int32_t>(true, false, 0);
    TradeDate = new DefaultField<uint32_t>(false, false, 0);
    NetChgPrevDay = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MarketDepth = new DefaultField<int32_t>(false, true, 0);
    SecurityID = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    SecurityExchange = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    RptSeq = new IncrementField<uint32_t>(false, false, 0);
    PriceBandType = new DefaultField<uint32_t>(false, false, 0);
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryPx = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntrySize = new DeltaField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntryDate = new CopyField<uint32_t>(true, false, 0);
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new DefaultField<uint32_t>(false, false, 0);
    TradingSessionSubID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradSesOpenTime = new DefaultField<uint64_t>(false, false, 0);
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettlFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NoSharesIssued = new DefaultField<uint64_t>(false, false, 0);
    Currency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    MDEntryPositionNo = new IncrementField<uint32_t>(false, false, 0);
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    TradeVolume = new DefaultField<uint64_t>(false, false, 0);
    PercentageVar = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NoUnchangedSecurities = new DefaultField<uint32_t>(false, false, 0);
    NoNotTradedSecurities = new DefaultField<uint32_t>(false, false, 0);
    TotTradedSecurities = new DefaultField<uint32_t>(false, false, 0);
    CapitalPct = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PrevYearVariation = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NoFallingSecurities = new DefaultField<uint32_t>(false, false, 0);
    NoRisingSecurities = new DefaultField<uint32_t>(false, false, 0);
    PercThresholdNormalTrade = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    PercThresholdCrossTrade = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    DailyAvgShares30D = new DefaultField<uint64_t>(false, false, 0);
    MaximumNormalSharesPerDailyAvgShares30DRatio =
        new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MaximumCrossSharesPerDailyAvgShares30DRatio = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    NormalSharesPerOutstandingSharesRatio = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    CrossSharesPerOutstandingSharesRatio = new DefaultField<FFUtils::Decimal>(false, false, FFUtils::Decimal(0));
    MDEntryID = new DefaultField<uint64_t>(false, false, 0);
    MDInsertDate = new DefaultField<uint32_t>(false, false, 0);
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

    int NoMDEntries_len = NoMDEntries->previousValue.getValue();
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<FFUtils::ByteArr> *SecurityGroup;
  NoOpField<FFUtils::ByteArr> *Symbol;
  NoOpField<FFUtils::ByteArr> *SecurityID;
  ConstantFieldOptional<FFUtils::ByteArr> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr> *SecurityExchange;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  NoOpField<int32_t> *SecurityTradingStatus;
  NoOpField<uint64_t> *TradSesOpenTime;
  NoOpField<uint64_t> *TransactTime;
  NoOpField<int32_t> *SecurityTradingEvent;
  // fields inside sequence NoMDEntryTypes
  NoOpField<uint32_t> *NoMDEntryTypes;
  NoOpField<FFUtils::ByteArr> *MDEntryType;
  // fields inside sequence NoMDEntryTypes ends

  // Constructor
  SecurityStatus_15() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("f"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    SecurityGroup = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Symbol = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityIDSource = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    SecurityExchange = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("XBSP"));
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new NoOpField<int32_t>(false, false, 0);
    TradSesOpenTime = new NoOpField<uint64_t>(false, false, 0);
    TransactTime = new NoOpField<uint64_t>(false, false, 0);
    SecurityTradingEvent = new NoOpField<int32_t>(false, false, 0);
    // constructor for sequence NoMDEntryTypes
    NoMDEntryTypes = new NoOpField<uint32_t>(true, false, 0);
    MDEntryType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
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

    int NoMDEntryTypes_len = NoMDEntryTypes->previousValue.getValue();
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<FFUtils::ByteArr> *SecurityGroup;
  NoOpField<int64_t> *SecurityID;
  DefaultField<FFUtils::ByteArr> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr> *SecurityExchange;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  NoOpField<int32_t> *SecurityTradingStatus;
  NoOpField<uint64_t> *TradSesOpenTime;
  NoOpField<uint64_t> *TransactTime;
  NoOpField<int32_t> *SecurityTradingEvent;

  // Constructor
  SecurityStatus_21() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("f"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    SecurityGroup = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new NoOpField<int64_t>(false, false, 0);
    SecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    SecurityExchange = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new NoOpField<int32_t>(false, false, 0);
    TradSesOpenTime = new NoOpField<uint64_t>(false, false, 0);
    TransactTime = new NoOpField<uint64_t>(false, false, 0);
    SecurityTradingEvent = new NoOpField<int32_t>(false, false, 0);
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<FFUtils::ByteArr> *SecurityGroup;
  NoOpField<FFUtils::ByteArr> *SecurityID;
  DefaultField<FFUtils::ByteArr> *SecurityIDSource;
  DefaultField<FFUtils::ByteArr> *SecurityExchange;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  NoOpField<int32_t> *SecurityTradingStatus;
  NoOpField<uint64_t> *TradSesOpenTime;
  NoOpField<uint64_t> *TransactTime;
  NoOpField<int32_t> *SecurityTradingEvent;

  // Constructor
  SecurityStatus_22() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("f"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    SecurityGroup = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityIDSource = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("8"));
    SecurityExchange = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new NoOpField<int32_t>(false, false, 0);
    TradSesOpenTime = new NoOpField<uint64_t>(false, false, 0);
    TransactTime = new NoOpField<uint64_t>(false, false, 0);
    SecurityTradingEvent = new NoOpField<int32_t>(false, false, 0);
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint64_t> *OrigTime;
  NoOpField<FFUtils::ByteArr> *NewsSource;
  NoOpField<FFUtils::ByteArr> *LanguageCode;
  NoOpField<FFUtils::ByteArr> *Headline;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t> *NoRelatedSym;
  CopyField<FFUtils::ByteArr> *Symbol;
  CopyField<FFUtils::ByteArr> *SecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityIDSource;
  CopyField<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence RelatedSym ends
  // fields inside sequence RoutingIDs
  NoOpField<uint32_t> *NoRoutingIDs;
  CopyField<int32_t> *RoutingType;
  CopyField<FFUtils::ByteArr> *RoutingID;
  // fields inside sequence RoutingIDs ends
  // fields inside sequence LinesOfText
  NoOpField<uint32_t> *NoLinesOfText;
  CopyField<FFUtils::ByteArr> *Text;
  NoOpField<int32_t> *EncodedTextLen;
  NoOpField<FFUtils::ByteArr> *EncodedText;
  // fields inside sequence LinesOfText ends
  NoOpField<FFUtils::ByteArr> *URLLink;

  // Constructor
  News_16() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("B"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    OrigTime = new NoOpField<uint64_t>(false, false, 0);
    NewsSource = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LanguageCode = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Headline = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t>(true, false, 0);
    Symbol = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityID = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityIDSource = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    SecurityExchange = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("XBSP"));
    // constructor for sequence RelatedSym ends
    // constructor for sequence RoutingIDs
    NoRoutingIDs = new NoOpField<uint32_t>(true, false, 0);
    RoutingType = new CopyField<int32_t>(true, true, 2);
    RoutingID = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence RoutingIDs ends
    // constructor for sequence LinesOfText
    NoLinesOfText = new NoOpField<uint32_t>(true, false, 0);
    Text = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    EncodedTextLen = new NoOpField<int32_t>(false, false, 0);
    EncodedText = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence LinesOfText ends
    URLLink = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
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

    int NoRelatedSym_len = NoRelatedSym->previousValue.getValue();
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

    int NoRoutingIDs_len = NoRoutingIDs->previousValue.getValue();
    for (int i = 0; i < NoRoutingIDs_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      RoutingType->decode(input, pmap1);
      RoutingID->decode(input, pmap1);
    }
    // decode for sequence RoutingIDs ends
    // decode for sequence LinesOfText
    NoLinesOfText->decode(input);

    int NoLinesOfText_len = NoLinesOfText->previousValue.getValue();
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint64_t> *OrigTime;
  NoOpField<FFUtils::ByteArr> *NewsSource;
  NoOpField<FFUtils::ByteArr> *LanguageCode;
  NoOpField<FFUtils::ByteArr> *Headline;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t> *NoRelatedSym;
  CopyField<FFUtils::ByteArr> *Symbol;
  CopyField<FFUtils::ByteArr> *SecurityID;
  ConstantFieldOptional<FFUtils::ByteArr> *SecurityIDSource;
  CopyField<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence RelatedSym ends
  // fields inside sequence RoutingIDs
  NoOpField<uint32_t> *NoRoutingIDs;
  CopyField<int32_t> *RoutingType;
  CopyField<FFUtils::ByteArr> *RoutingID;
  // fields inside sequence RoutingIDs ends
  // fields inside sequence LinesOfText
  NoOpField<uint32_t> *NoLinesOfText;
  CopyField<FFUtils::ByteArr> *Text;
  NoOpField<int32_t> *EncodedTextLen;
  NoOpField<FFUtils::ByteArr> *EncodedText;
  // fields inside sequence LinesOfText ends
  NoOpField<FFUtils::ByteArr> *URLLink;

  // Constructor
  News_29() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("B"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    OrigTime = new NoOpField<uint64_t>(false, false, 0);
    NewsSource = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LanguageCode = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Headline = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t>(true, false, 0);
    Symbol = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityIDSource = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    SecurityExchange = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence RelatedSym ends
    // constructor for sequence RoutingIDs
    NoRoutingIDs = new NoOpField<uint32_t>(true, false, 0);
    RoutingType = new CopyField<int32_t>(true, true, 2);
    RoutingID = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence RoutingIDs ends
    // constructor for sequence LinesOfText
    NoLinesOfText = new NoOpField<uint32_t>(true, false, 0);
    Text = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    EncodedTextLen = new NoOpField<int32_t>(false, false, 0);
    EncodedText = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence LinesOfText ends
    URLLink = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
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

    int NoRelatedSym_len = NoRelatedSym->previousValue.getValue();
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

    int NoRoutingIDs_len = NoRoutingIDs->previousValue.getValue();
    for (int i = 0; i < NoRoutingIDs_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      RoutingType->decode(input, pmap1);
      RoutingID->decode(input, pmap1);
    }
    // decode for sequence RoutingIDs ends
    // decode for sequence LinesOfText
    NoLinesOfText->decode(input);

    int NoLinesOfText_len = NoLinesOfText->previousValue.getValue();
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
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldOptional<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint64_t> *OrigTime;
  NoOpField<FFUtils::ByteArr> *NewsSource;
  NoOpField<FFUtils::ByteArr> *LanguageCode;
  NoOpField<FFUtils::ByteArr> *Headline;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t> *NoRelatedSym;
  CopyField<FFUtils::ByteArr> *Symbol;
  CopyField<FFUtils::ByteArr> *SecurityID;
  ConstantFieldOptional<FFUtils::ByteArr> *SecurityIDSource;
  CopyField<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence RelatedSym ends
  // fields inside sequence RoutingIDs
  NoOpField<uint32_t> *NoRoutingIDs;
  CopyField<int32_t> *RoutingType;
  CopyField<FFUtils::ByteArr> *RoutingID;
  // fields inside sequence RoutingIDs ends
  // fields inside sequence LinesOfText
  NoOpField<uint32_t> *NoLinesOfText;
  CopyField<FFUtils::ByteArr> *Text;
  NoOpField<int32_t> *EncodedTextLen;
  NoOpField<FFUtils::ByteArr> *EncodedText;
  // fields inside sequence LinesOfText ends
  NoOpField<FFUtils::ByteArr> *URLLink;

  // Constructor
  News_24() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("B"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    OrigTime = new NoOpField<uint64_t>(false, false, 0);
    NewsSource = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LanguageCode = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Headline = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t>(true, false, 0);
    Symbol = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityIDSource = new ConstantFieldOptional<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    SecurityExchange = new CopyField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence RelatedSym ends
    // constructor for sequence RoutingIDs
    NoRoutingIDs = new NoOpField<uint32_t>(true, false, 0);
    RoutingType = new CopyField<int32_t>(true, true, 2);
    RoutingID = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence RoutingIDs ends
    // constructor for sequence LinesOfText
    NoLinesOfText = new NoOpField<uint32_t>(true, false, 0);
    Text = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    EncodedTextLen = new NoOpField<int32_t>(false, false, 0);
    EncodedText = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence LinesOfText ends
    URLLink = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
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

    int NoRelatedSym_len = NoRelatedSym->previousValue.getValue();
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

    int NoRoutingIDs_len = NoRoutingIDs->previousValue.getValue();
    for (int i = 0; i < NoRoutingIDs_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      RoutingType->decode(input, pmap1);
      RoutingID->decode(input, pmap1);
    }
    // decode for sequence RoutingIDs ends
    // decode for sequence LinesOfText
    NoLinesOfText->decode(input);

    int NoLinesOfText_len = NoLinesOfText->previousValue.getValue();
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
