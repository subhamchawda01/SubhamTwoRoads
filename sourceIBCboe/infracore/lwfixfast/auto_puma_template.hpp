// AUTO GENERATED CLASS FOR TEMPLATE infracore/files/BMF/templates_BMF_derivatives_UMDF2.0.xml
// DO NOT MODIFY

#pragma once

#include "infracore/lwfixfast/FastDecoder.hpp"
namespace PUMA_TEMPLATE_DECODER {
class MDNonFix : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t> *DataLen;
  NoOpField<FFUtils::ByteVec> *Data;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageEncoding;

  // Constructor
  MDNonFix() {
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("n"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    DataLen = new NoOpField<uint32_t>(true, false, 0);
    Data = new NoOpField<FFUtils::ByteVec>(true, false, FFUtils::ByteArr(""));
    MessageEncoding = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("RLC-Z5"));
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MessageType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    ApplVerID->decode();
    DataLen->decode(input);
    Data->decode(input);
    MessageEncoding->decode();
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MessageType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    ApplVerID->reset();
    DataLen->reset();
    Data->reset();
    MessageEncoding->reset();
  }

  // Destructor
  ~MDNonFix() {
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete DataLen;
    delete Data;
    delete MessageEncoding;
  }
};
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
  ~MDTcpRequestReject_117() {
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplVerID;
    delete MDReqID;
    delete MDReqRejReason;
    delete Text;
  }
};
class MDSecurityList_149 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<FFUtils::ByteArr> *SendingTime;
  NoOpField<uint32_t> *TotNoRelatedSym;
  NoOpField<FFUtils::ByteArr> *LastFragment;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t> *NoRelatedSym;
  NoOpField<FFUtils::ByteArr> *Symbol;
  CopyField<uint64_t> *SecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence ApplIDs
  NoOpField<uint32_t> *NoApplIDs;
  NoOpField<FFUtils::ByteArr> *ApplID;
  // fields inside sequence FeedTypes
  NoOpField<uint32_t> *NoMDFeedTypes;
  NoOpField<FFUtils::ByteArr> *MDFeedType;
  NoOpField<uint32_t> *MarketDepth;
  // fields inside sequence FeedTypes ends
  // fields inside sequence ApplIDs ends
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
  // decimal fields IndexPct
  CopyField<int32_t> *IndexPct_exponent;
  DeltaField<int64_t> *IndexPct_mantissa;
  // decimal fields IndexTheoreticalQty
  CopyField<int32_t> *IndexTheoreticalQty_exponent;
  DeltaField<int64_t> *IndexTheoreticalQty_mantissa;
  // fields inside sequence Underlyings ends
  NoOpField<int32_t> *ImpliedMarketIndicator;
  // fields inside sequence InstrAttrib
  NoOpField<uint32_t> *NoInstrAttrib;
  NoOpField<int32_t> *InstAttribType;
  NoOpField<FFUtils::ByteArr> *InstAttribValue;
  // fields inside sequence InstrAttrib ends
  // fields inside sequence TickRules
  NoOpField<uint32_t> *NoTickRules;
  // decimal fields StartTickPriceRange
  CopyField<int32_t> *StartTickPriceRange_exponent;
  DeltaField<int64_t> *StartTickPriceRange_mantissa;
  // decimal fields EndTickPriceRange
  CopyField<int32_t> *EndTickPriceRange_exponent;
  DeltaField<int64_t> *EndTickPriceRange_mantissa;
  // decimal fields TickIncrement
  CopyField<int32_t> *TickIncrement_exponent;
  DeltaField<int64_t> *TickIncrement_mantissa;
  NoOpField<int32_t> *TickRuleType;
  // fields inside sequence TickRules ends
  // fields inside sequence Legs
  NoOpField<uint32_t> *NoLegs;
  NoOpField<FFUtils::ByteArr> *LegSymbol;
  NoOpField<uint64_t> *LegSecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *LegSecurityIDSource;
  CopyField<int32_t> *LegRatioQty;
  NoOpField<FFUtils::ByteArr> *LegSecurityType;
  NoOpField<int32_t> *LegSide;
  ConstantFieldMandatory<FFUtils::ByteArr> *LegSecurityExchange;
  // fields inside sequence Legs ends
  NoOpField<FFUtils::ByteArr> *SecurityUpdateAction;
  // fields inside sequence Lots
  NoOpField<uint32_t> *NoLotTypeRules;
  NoOpField<int32_t> *LotType;
  NoOpField<uint32_t> *MinLotSize;
  // fields inside sequence Lots ends
  // decimal fields MinPriceIncrement
  CopyField<int32_t> *MinPriceIncrement_exponent;
  DeltaField<int64_t> *MinPriceIncrement_mantissa;
  NoOpField<uint32_t> *TickSizeDenominator;
  // decimal fields PriceDivisor
  CopyField<int32_t> *PriceDivisor_exponent;
  DeltaField<int64_t> *PriceDivisor_mantissa;
  NoOpField<uint32_t> *MinOrderQty;
  NoOpField<uint64_t> *MaxOrderQty;
  NoOpField<int32_t> *MultiLegModel;
  NoOpField<int32_t> *MultiLegPriceMethod;
  NoOpField<FFUtils::ByteArr> *Currency;
  NoOpField<FFUtils::ByteArr> *SettlCurrency;
  NoOpField<int32_t> *Product;
  NoOpField<FFUtils::ByteArr> *SecurityType;
  NoOpField<FFUtils::ByteArr> *SecuritySubType;
  NoOpField<FFUtils::ByteArr> *SecurityStrategyType;
  NoOpField<FFUtils::ByteArr> *Asset;
  NoOpField<FFUtils::ByteArr> *SecurityDesc;
  NoOpField<uint64_t> *NoShareIssued;
  NoOpField<uint32_t> *MaturityDate;
  NoOpField<uint32_t> *MaturityMonthYear;
  // decimal fields StrikePrice
  CopyField<int32_t> *StrikePrice_exponent;
  DeltaField<int64_t> *StrikePrice_mantissa;
  NoOpField<FFUtils::ByteArr> *StrikeCurrency;
  NoOpField<int32_t> *ExerciseStyle;
  NoOpField<int32_t> *PutOrCall;
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
  NoOpField<uint64_t> *SecurityValidityTimestamp;
  NoOpField<FFUtils::ByteArr> *MarketSegmentID;
  NoOpField<FFUtils::ByteArr> *GovernanceIndicator;
  NoOpField<int32_t> *CorporateActionEventID;
  NoOpField<FFUtils::ByteArr> *SecurityGroup;
  NoOpField<int32_t> *SecurityMatchType;
  // fields inside sequence RelatedSym ends

  // Constructor
  MDSecurityList_149() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("y"));
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    TotNoRelatedSym = new NoOpField<uint32_t>(true, false, 0);
    LastFragment = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t>(true, false, 0);
    Symbol = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityID = new CopyField<uint64_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    SecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence ApplIDs
    NoApplIDs = new NoOpField<uint32_t>(true, false, 0);
    ApplID = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence FeedTypes
    NoMDFeedTypes = new NoOpField<uint32_t>(false, false, 0);
    MDFeedType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    MarketDepth = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence FeedTypes ends
    // constructor for sequence ApplIDs ends
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
    // decimal fields IndexPct
    IndexPct_exponent = new CopyField<int32_t>(false, true, -2);
    IndexPct_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields IndexTheoreticalQty
    IndexTheoreticalQty_exponent = new CopyField<int32_t>(false, false, 0);
    IndexTheoreticalQty_mantissa = new DeltaField<int64_t>(true, false, 0);
    // constructor for sequence Underlyings ends
    ImpliedMarketIndicator = new NoOpField<int32_t>(false, false, 0);
    // constructor for sequence InstrAttrib
    NoInstrAttrib = new NoOpField<uint32_t>(false, false, 0);
    InstAttribType = new NoOpField<int32_t>(false, false, 0);
    InstAttribValue = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence InstrAttrib ends
    // constructor for sequence TickRules
    NoTickRules = new NoOpField<uint32_t>(false, false, 0);
    // decimal fields StartTickPriceRange
    StartTickPriceRange_exponent = new CopyField<int32_t>(false, true, -2);
    StartTickPriceRange_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields EndTickPriceRange
    EndTickPriceRange_exponent = new CopyField<int32_t>(false, true, -2);
    EndTickPriceRange_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields TickIncrement
    TickIncrement_exponent = new CopyField<int32_t>(false, true, -2);
    TickIncrement_mantissa = new DeltaField<int64_t>(true, false, 0);
    TickRuleType = new NoOpField<int32_t>(false, false, 0);
    // constructor for sequence TickRules ends
    // constructor for sequence Legs
    NoLegs = new NoOpField<uint32_t>(false, false, 0);
    LegSymbol = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    LegSecurityID = new NoOpField<uint64_t>(true, false, 0);
    LegSecurityIDSource = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    LegRatioQty = new CopyField<int32_t>(true, false, 0);
    LegSecurityType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    LegSide = new NoOpField<int32_t>(true, false, 0);
    LegSecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence Legs ends
    SecurityUpdateAction = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence Lots
    NoLotTypeRules = new NoOpField<uint32_t>(false, false, 0);
    LotType = new NoOpField<int32_t>(false, false, 0);
    MinLotSize = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence Lots ends
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent = new CopyField<int32_t>(false, true, -2);
    MinPriceIncrement_mantissa = new DeltaField<int64_t>(true, false, 0);
    TickSizeDenominator = new NoOpField<uint32_t>(false, false, 0);
    // decimal fields PriceDivisor
    PriceDivisor_exponent = new CopyField<int32_t>(false, true, -2);
    PriceDivisor_mantissa = new DeltaField<int64_t>(true, false, 0);
    MinOrderQty = new NoOpField<uint32_t>(false, false, 0);
    MaxOrderQty = new NoOpField<uint64_t>(false, false, 0);
    MultiLegModel = new NoOpField<int32_t>(false, false, 0);
    MultiLegPriceMethod = new NoOpField<int32_t>(false, false, 0);
    Currency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlCurrency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Product = new NoOpField<int32_t>(true, false, 0);
    SecurityType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecuritySubType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityStrategyType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Asset = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityDesc = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    NoShareIssued = new NoOpField<uint64_t>(false, false, 0);
    MaturityDate = new NoOpField<uint32_t>(false, false, 0);
    MaturityMonthYear = new NoOpField<uint32_t>(false, false, 0);
    // decimal fields StrikePrice
    StrikePrice_exponent = new CopyField<int32_t>(false, true, -2);
    StrikePrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    StrikeCurrency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    ExerciseStyle = new NoOpField<int32_t>(false, false, 0);
    PutOrCall = new NoOpField<int32_t>(false, false, 0);
    // decimal fields ContractMultiplier
    ContractMultiplier_exponent = new CopyField<int32_t>(false, true, -2);
    ContractMultiplier_mantissa = new DeltaField<int64_t>(true, false, 0);
    ContractSettlMonth = new NoOpField<uint32_t>(false, false, 0);
    CFICode = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    CountryOfIssue = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    IssueDate = new NoOpField<uint32_t>(true, false, 0);
    DatedDate = new NoOpField<uint32_t>(false, false, 0);
    StartDate = new NoOpField<uint32_t>(false, false, 0);
    EndDate = new NoOpField<uint32_t>(false, false, 0);
    SettlType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlDate = new NoOpField<uint32_t>(false, false, 0);
    SecurityValidityTimestamp = new NoOpField<uint64_t>(true, false, 0);
    MarketSegmentID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    GovernanceIndicator = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    CorporateActionEventID = new NoOpField<int32_t>(false, false, 0);
    SecurityGroup = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityMatchType = new NoOpField<int32_t>(false, false, 0);
    // constructor for sequence RelatedSym ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    ApplVerID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    TotNoRelatedSym->decode(input);
    LastFragment->decode(input);
    // decode for sequence RelatedSym
    NoRelatedSym->decode(input);

    int NoRelatedSym_len = NoRelatedSym->previousValue.getValue();
    for (int i = 0; i < NoRelatedSym_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      Symbol->decode(input);
      SecurityID->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityExchange->decode();
      // decode for sequence ApplIDs
      NoApplIDs->decode(input);

      int NoApplIDs_len = NoApplIDs->previousValue.getValue();
      for (int i = 0; i < NoApplIDs_len; ++i) {
        ApplID->decode(input);
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
      // decode for sequence ApplIDs ends
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
          // extract pmap
          FFUtils::PMap pmap2 = input.extractPmap();

          UnderlyingSymbol->decode(input);
          UnderlyingSecurityID->decode(input);
          UnderlyingSecurityIDSource->decode();
          UnderlyingSecurityExchange->decode();
          // decimal fields IndexPct
          IndexPct_exponent->decode(input, pmap2);
          if (IndexPct_exponent->isMandatory || IndexPct_exponent->previousValue.isAssigned())
            IndexPct_mantissa->decode(input);
          // decimal fields IndexTheoreticalQty
          IndexTheoreticalQty_exponent->decode(input, pmap2);
          if (IndexTheoreticalQty_exponent->isMandatory || IndexTheoreticalQty_exponent->previousValue.isAssigned())
            IndexTheoreticalQty_mantissa->decode(input);
        }
      }
      // decode for sequence Underlyings ends
      ImpliedMarketIndicator->decode(input);
      // decode for sequence InstrAttrib
      NoInstrAttrib->decode(input);

      int NoInstrAttrib_len = NoInstrAttrib->previousValue.getValue();
      if (NoInstrAttrib->previousValue.isAssigned()) {
        for (int i = 0; i < NoInstrAttrib_len; ++i) {
          InstAttribType->decode(input);
          InstAttribValue->decode(input);
        }
      }
      // decode for sequence InstrAttrib ends
      // decode for sequence TickRules
      NoTickRules->decode(input);

      int NoTickRules_len = NoTickRules->previousValue.getValue();
      if (NoTickRules->previousValue.isAssigned()) {
        for (int i = 0; i < NoTickRules_len; ++i) {
          // extract pmap
          FFUtils::PMap pmap2 = input.extractPmap();

          // decimal fields StartTickPriceRange
          StartTickPriceRange_exponent->decode(input, pmap2);
          if (StartTickPriceRange_exponent->isMandatory || StartTickPriceRange_exponent->previousValue.isAssigned())
            StartTickPriceRange_mantissa->decode(input);
          // decimal fields EndTickPriceRange
          EndTickPriceRange_exponent->decode(input, pmap2);
          if (EndTickPriceRange_exponent->isMandatory || EndTickPriceRange_exponent->previousValue.isAssigned())
            EndTickPriceRange_mantissa->decode(input);
          // decimal fields TickIncrement
          TickIncrement_exponent->decode(input, pmap2);
          if (TickIncrement_exponent->isMandatory || TickIncrement_exponent->previousValue.isAssigned())
            TickIncrement_mantissa->decode(input);
          TickRuleType->decode(input);
        }
      }
      // decode for sequence TickRules ends
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
          LegSecurityType->decode(input);
          LegSide->decode(input);
          LegSecurityExchange->decode();
        }
      }
      // decode for sequence Legs ends
      SecurityUpdateAction->decode(input);
      // decode for sequence Lots
      NoLotTypeRules->decode(input);

      int NoLotTypeRules_len = NoLotTypeRules->previousValue.getValue();
      if (NoLotTypeRules->previousValue.isAssigned()) {
        for (int i = 0; i < NoLotTypeRules_len; ++i) {
          LotType->decode(input);
          MinLotSize->decode(input);
        }
      }
      // decode for sequence Lots ends
      // decimal fields MinPriceIncrement
      MinPriceIncrement_exponent->decode(input, pmap1);
      if (MinPriceIncrement_exponent->isMandatory || MinPriceIncrement_exponent->previousValue.isAssigned())
        MinPriceIncrement_mantissa->decode(input);
      TickSizeDenominator->decode(input);
      // decimal fields PriceDivisor
      PriceDivisor_exponent->decode(input, pmap1);
      if (PriceDivisor_exponent->isMandatory || PriceDivisor_exponent->previousValue.isAssigned())
        PriceDivisor_mantissa->decode(input);
      MinOrderQty->decode(input);
      MaxOrderQty->decode(input);
      MultiLegModel->decode(input);
      MultiLegPriceMethod->decode(input);
      Currency->decode(input);
      SettlCurrency->decode(input);
      Product->decode(input);
      SecurityType->decode(input);
      SecuritySubType->decode(input);
      SecurityStrategyType->decode(input);
      Asset->decode(input);
      SecurityDesc->decode(input);
      NoShareIssued->decode(input);
      MaturityDate->decode(input);
      MaturityMonthYear->decode(input);
      // decimal fields StrikePrice
      StrikePrice_exponent->decode(input, pmap1);
      if (StrikePrice_exponent->isMandatory || StrikePrice_exponent->previousValue.isAssigned())
        StrikePrice_mantissa->decode(input);
      StrikeCurrency->decode(input);
      ExerciseStyle->decode(input);
      PutOrCall->decode(input);
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
      SecurityValidityTimestamp->decode(input);
      MarketSegmentID->decode(input);
      GovernanceIndicator->decode(input);
      CorporateActionEventID->decode(input);
      SecurityGroup->decode(input);
      SecurityMatchType->decode(input);
    }
    // decode for sequence RelatedSym ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    ApplVerID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    TotNoRelatedSym->reset();
    LastFragment->reset();
    // reset for sequence RelatedSym
    NoRelatedSym->reset();
    Symbol->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence ApplIDs
    NoApplIDs->reset();
    ApplID->reset();
    // reset for sequence FeedTypes
    NoMDFeedTypes->reset();
    MDFeedType->reset();
    MarketDepth->reset();
    // reset for sequence FeedTypes ends
    // reset for sequence ApplIDs ends
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
    // decimal fields IndexPct
    IndexPct_exponent->reset();
    IndexPct_mantissa->reset();
    // decimal fields IndexTheoreticalQty
    IndexTheoreticalQty_exponent->reset();
    IndexTheoreticalQty_mantissa->reset();
    // reset for sequence Underlyings ends
    ImpliedMarketIndicator->reset();
    // reset for sequence InstrAttrib
    NoInstrAttrib->reset();
    InstAttribType->reset();
    InstAttribValue->reset();
    // reset for sequence InstrAttrib ends
    // reset for sequence TickRules
    NoTickRules->reset();
    // decimal fields StartTickPriceRange
    StartTickPriceRange_exponent->reset();
    StartTickPriceRange_mantissa->reset();
    // decimal fields EndTickPriceRange
    EndTickPriceRange_exponent->reset();
    EndTickPriceRange_mantissa->reset();
    // decimal fields TickIncrement
    TickIncrement_exponent->reset();
    TickIncrement_mantissa->reset();
    TickRuleType->reset();
    // reset for sequence TickRules ends
    // reset for sequence Legs
    NoLegs->reset();
    LegSymbol->reset();
    LegSecurityID->reset();
    LegSecurityIDSource->reset();
    LegRatioQty->reset();
    LegSecurityType->reset();
    LegSide->reset();
    LegSecurityExchange->reset();
    // reset for sequence Legs ends
    SecurityUpdateAction->reset();
    // reset for sequence Lots
    NoLotTypeRules->reset();
    LotType->reset();
    MinLotSize->reset();
    // reset for sequence Lots ends
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent->reset();
    MinPriceIncrement_mantissa->reset();
    TickSizeDenominator->reset();
    // decimal fields PriceDivisor
    PriceDivisor_exponent->reset();
    PriceDivisor_mantissa->reset();
    MinOrderQty->reset();
    MaxOrderQty->reset();
    MultiLegModel->reset();
    MultiLegPriceMethod->reset();
    Currency->reset();
    SettlCurrency->reset();
    Product->reset();
    SecurityType->reset();
    SecuritySubType->reset();
    SecurityStrategyType->reset();
    Asset->reset();
    SecurityDesc->reset();
    NoShareIssued->reset();
    MaturityDate->reset();
    MaturityMonthYear->reset();
    // decimal fields StrikePrice
    StrikePrice_exponent->reset();
    StrikePrice_mantissa->reset();
    StrikeCurrency->reset();
    ExerciseStyle->reset();
    PutOrCall->reset();
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
    MarketSegmentID->reset();
    GovernanceIndicator->reset();
    CorporateActionEventID->reset();
    SecurityGroup->reset();
    SecurityMatchType->reset();
    // reset for sequence RelatedSym ends
  }

  // Destructor
  ~MDSecurityList_149() {
    delete MsgType;
    delete ApplVerID;
    delete MsgSeqNum;
    delete SendingTime;
    delete TotNoRelatedSym;
    delete LastFragment;
    // destructor for sequence RelatedSym
    delete NoRelatedSym;
    delete Symbol;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence ApplIDs
    delete NoApplIDs;
    delete ApplID;
    // destructor for sequence FeedTypes
    delete NoMDFeedTypes;
    delete MDFeedType;
    delete MarketDepth;
    // destructor for sequence FeedTypes ends
    // destructor for sequence ApplIDs ends
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
    // decimal fields IndexPct
    delete IndexPct_exponent;
    delete IndexPct_mantissa;
    // decimal fields IndexTheoreticalQty
    delete IndexTheoreticalQty_exponent;
    delete IndexTheoreticalQty_mantissa;
    // destructor for sequence Underlyings ends
    delete ImpliedMarketIndicator;
    // destructor for sequence InstrAttrib
    delete NoInstrAttrib;
    delete InstAttribType;
    delete InstAttribValue;
    // destructor for sequence InstrAttrib ends
    // destructor for sequence TickRules
    delete NoTickRules;
    // decimal fields StartTickPriceRange
    delete StartTickPriceRange_exponent;
    delete StartTickPriceRange_mantissa;
    // decimal fields EndTickPriceRange
    delete EndTickPriceRange_exponent;
    delete EndTickPriceRange_mantissa;
    // decimal fields TickIncrement
    delete TickIncrement_exponent;
    delete TickIncrement_mantissa;
    delete TickRuleType;
    // destructor for sequence TickRules ends
    // destructor for sequence Legs
    delete NoLegs;
    delete LegSymbol;
    delete LegSecurityID;
    delete LegSecurityIDSource;
    delete LegRatioQty;
    delete LegSecurityType;
    delete LegSide;
    delete LegSecurityExchange;
    // destructor for sequence Legs ends
    delete SecurityUpdateAction;
    // destructor for sequence Lots
    delete NoLotTypeRules;
    delete LotType;
    delete MinLotSize;
    // destructor for sequence Lots ends
    // decimal fields MinPriceIncrement
    delete MinPriceIncrement_exponent;
    delete MinPriceIncrement_mantissa;
    delete TickSizeDenominator;
    // decimal fields PriceDivisor
    delete PriceDivisor_exponent;
    delete PriceDivisor_mantissa;
    delete MinOrderQty;
    delete MaxOrderQty;
    delete MultiLegModel;
    delete MultiLegPriceMethod;
    delete Currency;
    delete SettlCurrency;
    delete Product;
    delete SecurityType;
    delete SecuritySubType;
    delete SecurityStrategyType;
    delete Asset;
    delete SecurityDesc;
    delete NoShareIssued;
    delete MaturityDate;
    delete MaturityMonthYear;
    // decimal fields StrikePrice
    delete StrikePrice_exponent;
    delete StrikePrice_mantissa;
    delete StrikeCurrency;
    delete ExerciseStyle;
    delete PutOrCall;
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
    delete MarketSegmentID;
    delete GovernanceIndicator;
    delete CorporateActionEventID;
    delete SecurityGroup;
    delete SecurityMatchType;
    // destructor for sequence RelatedSym ends
  }
};
class MDSecurityList_148 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<FFUtils::ByteArr> *SendingTime;
  NoOpField<uint32_t> *TotNoRelatedSym;
  NoOpField<FFUtils::ByteArr> *LastFragment;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t> *NoRelatedSym;
  NoOpField<FFUtils::ByteArr> *Symbol;
  CopyField<uint64_t> *SecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence ApplIDs
  NoOpField<uint32_t> *NoApplIDs;
  NoOpField<FFUtils::ByteArr> *ApplID;
  // fields inside sequence FeedTypes
  NoOpField<uint32_t> *NoMDFeedTypes;
  NoOpField<FFUtils::ByteArr> *MDFeedType;
  NoOpField<uint32_t> *MarketDepth;
  // fields inside sequence FeedTypes ends
  // fields inside sequence ApplIDs ends
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
  // decimal fields IndexPct
  CopyField<int32_t> *IndexPct_exponent;
  DeltaField<int64_t> *IndexPct_mantissa;
  // fields inside sequence Underlyings ends
  NoOpField<int32_t> *ImpliedMarketIndicator;
  // fields inside sequence InstrAttrib
  NoOpField<uint32_t> *NoInstrAttrib;
  NoOpField<int32_t> *InstAttribType;
  NoOpField<FFUtils::ByteArr> *InstAttribValue;
  // fields inside sequence InstrAttrib ends
  // fields inside sequence TickRules
  NoOpField<uint32_t> *NoTickRules;
  // decimal fields StartTickPriceRange
  CopyField<int32_t> *StartTickPriceRange_exponent;
  DeltaField<int64_t> *StartTickPriceRange_mantissa;
  // decimal fields EndTickPriceRange
  CopyField<int32_t> *EndTickPriceRange_exponent;
  DeltaField<int64_t> *EndTickPriceRange_mantissa;
  // decimal fields TickIncrement
  CopyField<int32_t> *TickIncrement_exponent;
  DeltaField<int64_t> *TickIncrement_mantissa;
  NoOpField<int32_t> *TickRuleType;
  // fields inside sequence TickRules ends
  // fields inside sequence Legs
  NoOpField<uint32_t> *NoLegs;
  NoOpField<FFUtils::ByteArr> *LegSymbol;
  NoOpField<uint64_t> *LegSecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *LegSecurityIDSource;
  CopyField<int32_t> *LegRatioQty;
  NoOpField<FFUtils::ByteArr> *LegSecurityType;
  NoOpField<int32_t> *LegSide;
  ConstantFieldMandatory<FFUtils::ByteArr> *LegSecurityExchange;
  // fields inside sequence Legs ends
  NoOpField<FFUtils::ByteArr> *SecurityUpdateAction;
  // fields inside sequence Lots
  NoOpField<uint32_t> *NoLotTypeRules;
  NoOpField<int32_t> *LotType;
  NoOpField<uint32_t> *MinLotSize;
  // fields inside sequence Lots ends
  // decimal fields MinPriceIncrement
  CopyField<int32_t> *MinPriceIncrement_exponent;
  DeltaField<int64_t> *MinPriceIncrement_mantissa;
  NoOpField<uint32_t> *TickSizeDenominator;
  // decimal fields PriceDivisor
  CopyField<int32_t> *PriceDivisor_exponent;
  DeltaField<int64_t> *PriceDivisor_mantissa;
  NoOpField<uint32_t> *MinOrderQty;
  NoOpField<uint64_t> *MaxOrderQty;
  NoOpField<int32_t> *MultiLegModel;
  NoOpField<int32_t> *MultiLegPriceMethod;
  NoOpField<FFUtils::ByteArr> *Currency;
  NoOpField<FFUtils::ByteArr> *SettlCurrency;
  NoOpField<int32_t> *Product;
  NoOpField<FFUtils::ByteArr> *SecurityType;
  NoOpField<FFUtils::ByteArr> *SecuritySubType;
  NoOpField<FFUtils::ByteArr> *SecurityStrategyType;
  NoOpField<FFUtils::ByteArr> *Asset;
  NoOpField<FFUtils::ByteArr> *SecurityDesc;
  NoOpField<uint64_t> *NoShareIssued;
  NoOpField<uint32_t> *MaturityDate;
  NoOpField<uint32_t> *MaturityMonthYear;
  // decimal fields StrikePrice
  CopyField<int32_t> *StrikePrice_exponent;
  DeltaField<int64_t> *StrikePrice_mantissa;
  NoOpField<FFUtils::ByteArr> *StrikeCurrency;
  NoOpField<int32_t> *ExerciseStyle;
  NoOpField<int32_t> *PutOrCall;
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
  NoOpField<uint64_t> *SecurityValidityTimestamp;
  NoOpField<FFUtils::ByteArr> *MarketSegmentID;
  NoOpField<FFUtils::ByteArr> *GovernanceIndicator;
  NoOpField<int32_t> *CorporateActionEventID;
  NoOpField<FFUtils::ByteArr> *SecurityGroup;
  NoOpField<int32_t> *SecurityMatchType;
  // fields inside sequence RelatedSym ends

  // Constructor
  MDSecurityList_148() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("y"));
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    TotNoRelatedSym = new NoOpField<uint32_t>(true, false, 0);
    LastFragment = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t>(true, false, 0);
    Symbol = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityID = new CopyField<uint64_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    SecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence ApplIDs
    NoApplIDs = new NoOpField<uint32_t>(true, false, 0);
    ApplID = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence FeedTypes
    NoMDFeedTypes = new NoOpField<uint32_t>(false, false, 0);
    MDFeedType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    MarketDepth = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence FeedTypes ends
    // constructor for sequence ApplIDs ends
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
    // decimal fields IndexPct
    IndexPct_exponent = new CopyField<int32_t>(false, true, -2);
    IndexPct_mantissa = new DeltaField<int64_t>(true, false, 0);
    // constructor for sequence Underlyings ends
    ImpliedMarketIndicator = new NoOpField<int32_t>(false, false, 0);
    // constructor for sequence InstrAttrib
    NoInstrAttrib = new NoOpField<uint32_t>(false, false, 0);
    InstAttribType = new NoOpField<int32_t>(false, false, 0);
    InstAttribValue = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence InstrAttrib ends
    // constructor for sequence TickRules
    NoTickRules = new NoOpField<uint32_t>(false, false, 0);
    // decimal fields StartTickPriceRange
    StartTickPriceRange_exponent = new CopyField<int32_t>(false, true, -2);
    StartTickPriceRange_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields EndTickPriceRange
    EndTickPriceRange_exponent = new CopyField<int32_t>(false, true, -2);
    EndTickPriceRange_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields TickIncrement
    TickIncrement_exponent = new CopyField<int32_t>(false, true, -2);
    TickIncrement_mantissa = new DeltaField<int64_t>(true, false, 0);
    TickRuleType = new NoOpField<int32_t>(false, false, 0);
    // constructor for sequence TickRules ends
    // constructor for sequence Legs
    NoLegs = new NoOpField<uint32_t>(false, false, 0);
    LegSymbol = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    LegSecurityID = new NoOpField<uint64_t>(true, false, 0);
    LegSecurityIDSource = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    LegRatioQty = new CopyField<int32_t>(true, false, 0);
    LegSecurityType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    LegSide = new NoOpField<int32_t>(true, false, 0);
    LegSecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence Legs ends
    SecurityUpdateAction = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence Lots
    NoLotTypeRules = new NoOpField<uint32_t>(false, false, 0);
    LotType = new NoOpField<int32_t>(false, false, 0);
    MinLotSize = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence Lots ends
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent = new CopyField<int32_t>(false, true, -2);
    MinPriceIncrement_mantissa = new DeltaField<int64_t>(true, false, 0);
    TickSizeDenominator = new NoOpField<uint32_t>(false, false, 0);
    // decimal fields PriceDivisor
    PriceDivisor_exponent = new CopyField<int32_t>(false, true, -2);
    PriceDivisor_mantissa = new DeltaField<int64_t>(true, false, 0);
    MinOrderQty = new NoOpField<uint32_t>(false, false, 0);
    MaxOrderQty = new NoOpField<uint64_t>(false, false, 0);
    MultiLegModel = new NoOpField<int32_t>(false, false, 0);
    MultiLegPriceMethod = new NoOpField<int32_t>(false, false, 0);
    Currency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlCurrency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Product = new NoOpField<int32_t>(true, false, 0);
    SecurityType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecuritySubType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityStrategyType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Asset = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityDesc = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    NoShareIssued = new NoOpField<uint64_t>(false, false, 0);
    MaturityDate = new NoOpField<uint32_t>(false, false, 0);
    MaturityMonthYear = new NoOpField<uint32_t>(false, false, 0);
    // decimal fields StrikePrice
    StrikePrice_exponent = new CopyField<int32_t>(false, true, -2);
    StrikePrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    StrikeCurrency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    ExerciseStyle = new NoOpField<int32_t>(false, false, 0);
    PutOrCall = new NoOpField<int32_t>(false, false, 0);
    // decimal fields ContractMultiplier
    ContractMultiplier_exponent = new CopyField<int32_t>(false, true, -2);
    ContractMultiplier_mantissa = new DeltaField<int64_t>(true, false, 0);
    ContractSettlMonth = new NoOpField<uint32_t>(false, false, 0);
    CFICode = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    CountryOfIssue = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    IssueDate = new NoOpField<uint32_t>(true, false, 0);
    DatedDate = new NoOpField<uint32_t>(false, false, 0);
    StartDate = new NoOpField<uint32_t>(false, false, 0);
    EndDate = new NoOpField<uint32_t>(false, false, 0);
    SettlType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlDate = new NoOpField<uint32_t>(false, false, 0);
    SecurityValidityTimestamp = new NoOpField<uint64_t>(true, false, 0);
    MarketSegmentID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    GovernanceIndicator = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    CorporateActionEventID = new NoOpField<int32_t>(false, false, 0);
    SecurityGroup = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityMatchType = new NoOpField<int32_t>(false, false, 0);
    // constructor for sequence RelatedSym ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    ApplVerID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    TotNoRelatedSym->decode(input);
    LastFragment->decode(input);
    // decode for sequence RelatedSym
    NoRelatedSym->decode(input);

    int NoRelatedSym_len = NoRelatedSym->previousValue.getValue();
    for (int i = 0; i < NoRelatedSym_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      Symbol->decode(input);
      SecurityID->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityExchange->decode();
      // decode for sequence ApplIDs
      NoApplIDs->decode(input);

      int NoApplIDs_len = NoApplIDs->previousValue.getValue();
      for (int i = 0; i < NoApplIDs_len; ++i) {
        ApplID->decode(input);
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
      // decode for sequence ApplIDs ends
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
          // extract pmap
          FFUtils::PMap pmap2 = input.extractPmap();

          UnderlyingSymbol->decode(input);
          UnderlyingSecurityID->decode(input);
          UnderlyingSecurityIDSource->decode();
          UnderlyingSecurityExchange->decode();
          // decimal fields IndexPct
          IndexPct_exponent->decode(input, pmap2);
          if (IndexPct_exponent->isMandatory || IndexPct_exponent->previousValue.isAssigned())
            IndexPct_mantissa->decode(input);
        }
      }
      // decode for sequence Underlyings ends
      ImpliedMarketIndicator->decode(input);
      // decode for sequence InstrAttrib
      NoInstrAttrib->decode(input);

      int NoInstrAttrib_len = NoInstrAttrib->previousValue.getValue();
      if (NoInstrAttrib->previousValue.isAssigned()) {
        for (int i = 0; i < NoInstrAttrib_len; ++i) {
          InstAttribType->decode(input);
          InstAttribValue->decode(input);
        }
      }
      // decode for sequence InstrAttrib ends
      // decode for sequence TickRules
      NoTickRules->decode(input);

      int NoTickRules_len = NoTickRules->previousValue.getValue();
      if (NoTickRules->previousValue.isAssigned()) {
        for (int i = 0; i < NoTickRules_len; ++i) {
          // extract pmap
          FFUtils::PMap pmap2 = input.extractPmap();

          // decimal fields StartTickPriceRange
          StartTickPriceRange_exponent->decode(input, pmap2);
          if (StartTickPriceRange_exponent->isMandatory || StartTickPriceRange_exponent->previousValue.isAssigned())
            StartTickPriceRange_mantissa->decode(input);
          // decimal fields EndTickPriceRange
          EndTickPriceRange_exponent->decode(input, pmap2);
          if (EndTickPriceRange_exponent->isMandatory || EndTickPriceRange_exponent->previousValue.isAssigned())
            EndTickPriceRange_mantissa->decode(input);
          // decimal fields TickIncrement
          TickIncrement_exponent->decode(input, pmap2);
          if (TickIncrement_exponent->isMandatory || TickIncrement_exponent->previousValue.isAssigned())
            TickIncrement_mantissa->decode(input);
          TickRuleType->decode(input);
        }
      }
      // decode for sequence TickRules ends
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
          LegSecurityType->decode(input);
          LegSide->decode(input);
          LegSecurityExchange->decode();
        }
      }
      // decode for sequence Legs ends
      SecurityUpdateAction->decode(input);
      // decode for sequence Lots
      NoLotTypeRules->decode(input);

      int NoLotTypeRules_len = NoLotTypeRules->previousValue.getValue();
      if (NoLotTypeRules->previousValue.isAssigned()) {
        for (int i = 0; i < NoLotTypeRules_len; ++i) {
          LotType->decode(input);
          MinLotSize->decode(input);
        }
      }
      // decode for sequence Lots ends
      // decimal fields MinPriceIncrement
      MinPriceIncrement_exponent->decode(input, pmap1);
      if (MinPriceIncrement_exponent->isMandatory || MinPriceIncrement_exponent->previousValue.isAssigned())
        MinPriceIncrement_mantissa->decode(input);
      TickSizeDenominator->decode(input);
      // decimal fields PriceDivisor
      PriceDivisor_exponent->decode(input, pmap1);
      if (PriceDivisor_exponent->isMandatory || PriceDivisor_exponent->previousValue.isAssigned())
        PriceDivisor_mantissa->decode(input);
      MinOrderQty->decode(input);
      MaxOrderQty->decode(input);
      MultiLegModel->decode(input);
      MultiLegPriceMethod->decode(input);
      Currency->decode(input);
      SettlCurrency->decode(input);
      Product->decode(input);
      SecurityType->decode(input);
      SecuritySubType->decode(input);
      SecurityStrategyType->decode(input);
      Asset->decode(input);
      SecurityDesc->decode(input);
      NoShareIssued->decode(input);
      MaturityDate->decode(input);
      MaturityMonthYear->decode(input);
      // decimal fields StrikePrice
      StrikePrice_exponent->decode(input, pmap1);
      if (StrikePrice_exponent->isMandatory || StrikePrice_exponent->previousValue.isAssigned())
        StrikePrice_mantissa->decode(input);
      StrikeCurrency->decode(input);
      ExerciseStyle->decode(input);
      PutOrCall->decode(input);
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
      SecurityValidityTimestamp->decode(input);
      MarketSegmentID->decode(input);
      GovernanceIndicator->decode(input);
      CorporateActionEventID->decode(input);
      SecurityGroup->decode(input);
      SecurityMatchType->decode(input);
    }
    // decode for sequence RelatedSym ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    ApplVerID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    TotNoRelatedSym->reset();
    LastFragment->reset();
    // reset for sequence RelatedSym
    NoRelatedSym->reset();
    Symbol->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence ApplIDs
    NoApplIDs->reset();
    ApplID->reset();
    // reset for sequence FeedTypes
    NoMDFeedTypes->reset();
    MDFeedType->reset();
    MarketDepth->reset();
    // reset for sequence FeedTypes ends
    // reset for sequence ApplIDs ends
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
    // decimal fields IndexPct
    IndexPct_exponent->reset();
    IndexPct_mantissa->reset();
    // reset for sequence Underlyings ends
    ImpliedMarketIndicator->reset();
    // reset for sequence InstrAttrib
    NoInstrAttrib->reset();
    InstAttribType->reset();
    InstAttribValue->reset();
    // reset for sequence InstrAttrib ends
    // reset for sequence TickRules
    NoTickRules->reset();
    // decimal fields StartTickPriceRange
    StartTickPriceRange_exponent->reset();
    StartTickPriceRange_mantissa->reset();
    // decimal fields EndTickPriceRange
    EndTickPriceRange_exponent->reset();
    EndTickPriceRange_mantissa->reset();
    // decimal fields TickIncrement
    TickIncrement_exponent->reset();
    TickIncrement_mantissa->reset();
    TickRuleType->reset();
    // reset for sequence TickRules ends
    // reset for sequence Legs
    NoLegs->reset();
    LegSymbol->reset();
    LegSecurityID->reset();
    LegSecurityIDSource->reset();
    LegRatioQty->reset();
    LegSecurityType->reset();
    LegSide->reset();
    LegSecurityExchange->reset();
    // reset for sequence Legs ends
    SecurityUpdateAction->reset();
    // reset for sequence Lots
    NoLotTypeRules->reset();
    LotType->reset();
    MinLotSize->reset();
    // reset for sequence Lots ends
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent->reset();
    MinPriceIncrement_mantissa->reset();
    TickSizeDenominator->reset();
    // decimal fields PriceDivisor
    PriceDivisor_exponent->reset();
    PriceDivisor_mantissa->reset();
    MinOrderQty->reset();
    MaxOrderQty->reset();
    MultiLegModel->reset();
    MultiLegPriceMethod->reset();
    Currency->reset();
    SettlCurrency->reset();
    Product->reset();
    SecurityType->reset();
    SecuritySubType->reset();
    SecurityStrategyType->reset();
    Asset->reset();
    SecurityDesc->reset();
    NoShareIssued->reset();
    MaturityDate->reset();
    MaturityMonthYear->reset();
    // decimal fields StrikePrice
    StrikePrice_exponent->reset();
    StrikePrice_mantissa->reset();
    StrikeCurrency->reset();
    ExerciseStyle->reset();
    PutOrCall->reset();
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
    MarketSegmentID->reset();
    GovernanceIndicator->reset();
    CorporateActionEventID->reset();
    SecurityGroup->reset();
    SecurityMatchType->reset();
    // reset for sequence RelatedSym ends
  }

  // Destructor
  ~MDSecurityList_148() {
    delete MsgType;
    delete ApplVerID;
    delete MsgSeqNum;
    delete SendingTime;
    delete TotNoRelatedSym;
    delete LastFragment;
    // destructor for sequence RelatedSym
    delete NoRelatedSym;
    delete Symbol;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence ApplIDs
    delete NoApplIDs;
    delete ApplID;
    // destructor for sequence FeedTypes
    delete NoMDFeedTypes;
    delete MDFeedType;
    delete MarketDepth;
    // destructor for sequence FeedTypes ends
    // destructor for sequence ApplIDs ends
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
    // decimal fields IndexPct
    delete IndexPct_exponent;
    delete IndexPct_mantissa;
    // destructor for sequence Underlyings ends
    delete ImpliedMarketIndicator;
    // destructor for sequence InstrAttrib
    delete NoInstrAttrib;
    delete InstAttribType;
    delete InstAttribValue;
    // destructor for sequence InstrAttrib ends
    // destructor for sequence TickRules
    delete NoTickRules;
    // decimal fields StartTickPriceRange
    delete StartTickPriceRange_exponent;
    delete StartTickPriceRange_mantissa;
    // decimal fields EndTickPriceRange
    delete EndTickPriceRange_exponent;
    delete EndTickPriceRange_mantissa;
    // decimal fields TickIncrement
    delete TickIncrement_exponent;
    delete TickIncrement_mantissa;
    delete TickRuleType;
    // destructor for sequence TickRules ends
    // destructor for sequence Legs
    delete NoLegs;
    delete LegSymbol;
    delete LegSecurityID;
    delete LegSecurityIDSource;
    delete LegRatioQty;
    delete LegSecurityType;
    delete LegSide;
    delete LegSecurityExchange;
    // destructor for sequence Legs ends
    delete SecurityUpdateAction;
    // destructor for sequence Lots
    delete NoLotTypeRules;
    delete LotType;
    delete MinLotSize;
    // destructor for sequence Lots ends
    // decimal fields MinPriceIncrement
    delete MinPriceIncrement_exponent;
    delete MinPriceIncrement_mantissa;
    delete TickSizeDenominator;
    // decimal fields PriceDivisor
    delete PriceDivisor_exponent;
    delete PriceDivisor_mantissa;
    delete MinOrderQty;
    delete MaxOrderQty;
    delete MultiLegModel;
    delete MultiLegPriceMethod;
    delete Currency;
    delete SettlCurrency;
    delete Product;
    delete SecurityType;
    delete SecuritySubType;
    delete SecurityStrategyType;
    delete Asset;
    delete SecurityDesc;
    delete NoShareIssued;
    delete MaturityDate;
    delete MaturityMonthYear;
    // decimal fields StrikePrice
    delete StrikePrice_exponent;
    delete StrikePrice_mantissa;
    delete StrikeCurrency;
    delete ExerciseStyle;
    delete PutOrCall;
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
    delete MarketSegmentID;
    delete GovernanceIndicator;
    delete CorporateActionEventID;
    delete SecurityGroup;
    delete SecurityMatchType;
    // destructor for sequence RelatedSym ends
  }
};
class MDSecurityList_141 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<FFUtils::ByteArr> *SendingTime;
  NoOpField<uint32_t> *TotNoRelatedSym;
  NoOpField<FFUtils::ByteArr> *LastFragment;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t> *NoRelatedSym;
  NoOpField<FFUtils::ByteArr> *Symbol;
  CopyField<uint64_t> *SecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence ApplIDs
  NoOpField<uint32_t> *NoApplIDs;
  NoOpField<FFUtils::ByteArr> *ApplID;
  // fields inside sequence FeedTypes
  NoOpField<uint32_t> *NoMDFeedTypes;
  NoOpField<FFUtils::ByteArr> *MDFeedType;
  NoOpField<uint32_t> *MarketDepth;
  // fields inside sequence FeedTypes ends
  // fields inside sequence ApplIDs ends
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
  // decimal fields IndexPct
  CopyField<int32_t> *IndexPct_exponent;
  DeltaField<int64_t> *IndexPct_mantissa;
  // fields inside sequence Underlyings ends
  NoOpField<int32_t> *ImpliedMarketIndicator;
  // fields inside sequence InstrAttrib
  NoOpField<uint32_t> *NoInstrAttrib;
  NoOpField<int32_t> *InstAttribType;
  NoOpField<FFUtils::ByteArr> *InstAttribValue;
  // fields inside sequence InstrAttrib ends
  // fields inside sequence TickRules
  NoOpField<uint32_t> *NoTickRules;
  // decimal fields StartTickPriceRange
  CopyField<int32_t> *StartTickPriceRange_exponent;
  DeltaField<int64_t> *StartTickPriceRange_mantissa;
  // decimal fields EndTickPriceRange
  CopyField<int32_t> *EndTickPriceRange_exponent;
  DeltaField<int64_t> *EndTickPriceRange_mantissa;
  // decimal fields TickIncrement
  CopyField<int32_t> *TickIncrement_exponent;
  DeltaField<int64_t> *TickIncrement_mantissa;
  NoOpField<int32_t> *TickRuleType;
  // fields inside sequence TickRules ends
  // fields inside sequence Legs
  NoOpField<uint32_t> *NoLegs;
  NoOpField<FFUtils::ByteArr> *LegSymbol;
  NoOpField<uint64_t> *LegSecurityID;
  ConstantFieldMandatory<FFUtils::ByteArr> *LegSecurityIDSource;
  CopyField<int32_t> *LegRatioQty;
  NoOpField<FFUtils::ByteArr> *LegSecurityType;
  NoOpField<int32_t> *LegSide;
  ConstantFieldMandatory<FFUtils::ByteArr> *LegSecurityExchange;
  // fields inside sequence Legs ends
  NoOpField<FFUtils::ByteArr> *SecurityUpdateAction;
  // fields inside sequence Lots
  NoOpField<uint32_t> *NoLotTypeRules;
  NoOpField<int32_t> *LotType;
  NoOpField<uint32_t> *MinLotSize;
  // fields inside sequence Lots ends
  // decimal fields MinPriceIncrement
  CopyField<int32_t> *MinPriceIncrement_exponent;
  DeltaField<int64_t> *MinPriceIncrement_mantissa;
  NoOpField<uint32_t> *TickSizeDenominator;
  // decimal fields PriceDivisor
  CopyField<int32_t> *PriceDivisor_exponent;
  DeltaField<int64_t> *PriceDivisor_mantissa;
  NoOpField<uint32_t> *MinOrderQty;
  NoOpField<uint64_t> *MaxOrderQty;
  NoOpField<int32_t> *MultiLegModel;
  NoOpField<int32_t> *MultiLegPriceMethod;
  NoOpField<FFUtils::ByteArr> *Currency;
  NoOpField<FFUtils::ByteArr> *SettlCurrency;
  NoOpField<int32_t> *Product;
  NoOpField<FFUtils::ByteArr> *SecurityType;
  NoOpField<FFUtils::ByteArr> *SecuritySubType;
  NoOpField<FFUtils::ByteArr> *SecurityStrategyType;
  NoOpField<FFUtils::ByteArr> *Asset;
  NoOpField<FFUtils::ByteArr> *SecurityDesc;
  NoOpField<uint64_t> *NoShareIssued;
  NoOpField<uint32_t> *MaturityDate;
  NoOpField<uint32_t> *MaturityMonthYear;
  // decimal fields StrikePrice
  CopyField<int32_t> *StrikePrice_exponent;
  DeltaField<int64_t> *StrikePrice_mantissa;
  NoOpField<FFUtils::ByteArr> *StrikeCurrency;
  NoOpField<int32_t> *ExerciseStyle;
  NoOpField<int32_t> *PutOrCall;
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
  NoOpField<uint64_t> *SecurityValidityTimestamp;
  NoOpField<FFUtils::ByteArr> *MarketSegmentID;
  NoOpField<FFUtils::ByteArr> *GovernanceIndicator;
  NoOpField<int32_t> *CorporateActionEventID;
  NoOpField<FFUtils::ByteArr> *SecurityGroup;
  NoOpField<int32_t> *SecurityMatchType;
  // fields inside sequence RelatedSym ends

  // Constructor
  MDSecurityList_141() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("y"));
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    TotNoRelatedSym = new NoOpField<uint32_t>(true, false, 0);
    LastFragment = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t>(true, false, 0);
    Symbol = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityID = new CopyField<uint64_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    SecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence ApplIDs
    NoApplIDs = new NoOpField<uint32_t>(true, false, 0);
    ApplID = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence FeedTypes
    NoMDFeedTypes = new NoOpField<uint32_t>(false, false, 0);
    MDFeedType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    MarketDepth = new NoOpField<uint32_t>(true, false, 0);
    // constructor for sequence FeedTypes ends
    // constructor for sequence ApplIDs ends
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
    // decimal fields IndexPct
    IndexPct_exponent = new CopyField<int32_t>(false, true, -2);
    IndexPct_mantissa = new DeltaField<int64_t>(true, false, 0);
    // constructor for sequence Underlyings ends
    ImpliedMarketIndicator = new NoOpField<int32_t>(false, false, 0);
    // constructor for sequence InstrAttrib
    NoInstrAttrib = new NoOpField<uint32_t>(false, false, 0);
    InstAttribType = new NoOpField<int32_t>(false, false, 0);
    InstAttribValue = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence InstrAttrib ends
    // constructor for sequence TickRules
    NoTickRules = new NoOpField<uint32_t>(false, false, 0);
    // decimal fields StartTickPriceRange
    StartTickPriceRange_exponent = new CopyField<int32_t>(false, true, -2);
    StartTickPriceRange_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields EndTickPriceRange
    EndTickPriceRange_exponent = new CopyField<int32_t>(false, true, -2);
    EndTickPriceRange_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields TickIncrement
    TickIncrement_exponent = new CopyField<int32_t>(false, true, -2);
    TickIncrement_mantissa = new DeltaField<int64_t>(true, false, 0);
    TickRuleType = new NoOpField<int32_t>(false, false, 0);
    // constructor for sequence TickRules ends
    // constructor for sequence Legs
    NoLegs = new NoOpField<uint32_t>(false, false, 0);
    LegSymbol = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    LegSecurityID = new NoOpField<uint64_t>(true, false, 0);
    LegSecurityIDSource = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("8"));
    LegRatioQty = new CopyField<int32_t>(true, false, 0);
    LegSecurityType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    LegSide = new NoOpField<int32_t>(true, false, 0);
    LegSecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // constructor for sequence Legs ends
    SecurityUpdateAction = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence Lots
    NoLotTypeRules = new NoOpField<uint32_t>(false, false, 0);
    LotType = new NoOpField<int32_t>(false, false, 0);
    MinLotSize = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence Lots ends
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent = new CopyField<int32_t>(false, true, -2);
    MinPriceIncrement_mantissa = new DeltaField<int64_t>(true, false, 0);
    TickSizeDenominator = new NoOpField<uint32_t>(false, false, 0);
    // decimal fields PriceDivisor
    PriceDivisor_exponent = new CopyField<int32_t>(false, true, -2);
    PriceDivisor_mantissa = new DeltaField<int64_t>(true, false, 0);
    MinOrderQty = new NoOpField<uint32_t>(false, false, 0);
    MaxOrderQty = new NoOpField<uint64_t>(false, false, 0);
    MultiLegModel = new NoOpField<int32_t>(false, false, 0);
    MultiLegPriceMethod = new NoOpField<int32_t>(false, false, 0);
    Currency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlCurrency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Product = new NoOpField<int32_t>(true, false, 0);
    SecurityType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecuritySubType = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityStrategyType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Asset = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityDesc = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    NoShareIssued = new NoOpField<uint64_t>(false, false, 0);
    MaturityDate = new NoOpField<uint32_t>(false, false, 0);
    MaturityMonthYear = new NoOpField<uint32_t>(false, false, 0);
    // decimal fields StrikePrice
    StrikePrice_exponent = new CopyField<int32_t>(false, true, -2);
    StrikePrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    StrikeCurrency = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    ExerciseStyle = new NoOpField<int32_t>(false, false, 0);
    PutOrCall = new NoOpField<int32_t>(false, false, 0);
    // decimal fields ContractMultiplier
    ContractMultiplier_exponent = new CopyField<int32_t>(false, true, -2);
    ContractMultiplier_mantissa = new DeltaField<int64_t>(true, false, 0);
    ContractSettlMonth = new NoOpField<uint32_t>(false, false, 0);
    CFICode = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    CountryOfIssue = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    IssueDate = new NoOpField<uint32_t>(true, false, 0);
    DatedDate = new NoOpField<uint32_t>(false, false, 0);
    StartDate = new NoOpField<uint32_t>(false, false, 0);
    EndDate = new NoOpField<uint32_t>(false, false, 0);
    SettlType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SettlDate = new NoOpField<uint32_t>(false, false, 0);
    SecurityValidityTimestamp = new NoOpField<uint64_t>(true, false, 0);
    MarketSegmentID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    GovernanceIndicator = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    CorporateActionEventID = new NoOpField<int32_t>(false, false, 0);
    SecurityGroup = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    SecurityMatchType = new NoOpField<int32_t>(false, false, 0);
    // constructor for sequence RelatedSym ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
    ApplVerID->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    TotNoRelatedSym->decode(input);
    LastFragment->decode(input);
    // decode for sequence RelatedSym
    NoRelatedSym->decode(input);

    int NoRelatedSym_len = NoRelatedSym->previousValue.getValue();
    for (int i = 0; i < NoRelatedSym_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      Symbol->decode(input);
      SecurityID->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityExchange->decode();
      // decode for sequence ApplIDs
      NoApplIDs->decode(input);

      int NoApplIDs_len = NoApplIDs->previousValue.getValue();
      for (int i = 0; i < NoApplIDs_len; ++i) {
        ApplID->decode(input);
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
      // decode for sequence ApplIDs ends
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
          // extract pmap
          FFUtils::PMap pmap2 = input.extractPmap();

          UnderlyingSymbol->decode(input);
          UnderlyingSecurityID->decode(input);
          UnderlyingSecurityIDSource->decode();
          UnderlyingSecurityExchange->decode();
          // decimal fields IndexPct
          IndexPct_exponent->decode(input, pmap2);
          if (IndexPct_exponent->isMandatory || IndexPct_exponent->previousValue.isAssigned())
            IndexPct_mantissa->decode(input);
        }
      }
      // decode for sequence Underlyings ends
      ImpliedMarketIndicator->decode(input);
      // decode for sequence InstrAttrib
      NoInstrAttrib->decode(input);

      int NoInstrAttrib_len = NoInstrAttrib->previousValue.getValue();
      if (NoInstrAttrib->previousValue.isAssigned()) {
        for (int i = 0; i < NoInstrAttrib_len; ++i) {
          InstAttribType->decode(input);
          InstAttribValue->decode(input);
        }
      }
      // decode for sequence InstrAttrib ends
      // decode for sequence TickRules
      NoTickRules->decode(input);

      int NoTickRules_len = NoTickRules->previousValue.getValue();
      if (NoTickRules->previousValue.isAssigned()) {
        for (int i = 0; i < NoTickRules_len; ++i) {
          // extract pmap
          FFUtils::PMap pmap2 = input.extractPmap();

          // decimal fields StartTickPriceRange
          StartTickPriceRange_exponent->decode(input, pmap2);
          if (StartTickPriceRange_exponent->isMandatory || StartTickPriceRange_exponent->previousValue.isAssigned())
            StartTickPriceRange_mantissa->decode(input);
          // decimal fields EndTickPriceRange
          EndTickPriceRange_exponent->decode(input, pmap2);
          if (EndTickPriceRange_exponent->isMandatory || EndTickPriceRange_exponent->previousValue.isAssigned())
            EndTickPriceRange_mantissa->decode(input);
          // decimal fields TickIncrement
          TickIncrement_exponent->decode(input, pmap2);
          if (TickIncrement_exponent->isMandatory || TickIncrement_exponent->previousValue.isAssigned())
            TickIncrement_mantissa->decode(input);
          TickRuleType->decode(input);
        }
      }
      // decode for sequence TickRules ends
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
          LegSecurityType->decode(input);
          LegSide->decode(input);
          LegSecurityExchange->decode();
        }
      }
      // decode for sequence Legs ends
      SecurityUpdateAction->decode(input);
      // decode for sequence Lots
      NoLotTypeRules->decode(input);

      int NoLotTypeRules_len = NoLotTypeRules->previousValue.getValue();
      if (NoLotTypeRules->previousValue.isAssigned()) {
        for (int i = 0; i < NoLotTypeRules_len; ++i) {
          LotType->decode(input);
          MinLotSize->decode(input);
        }
      }
      // decode for sequence Lots ends
      // decimal fields MinPriceIncrement
      MinPriceIncrement_exponent->decode(input, pmap1);
      if (MinPriceIncrement_exponent->isMandatory || MinPriceIncrement_exponent->previousValue.isAssigned())
        MinPriceIncrement_mantissa->decode(input);
      TickSizeDenominator->decode(input);
      // decimal fields PriceDivisor
      PriceDivisor_exponent->decode(input, pmap1);
      if (PriceDivisor_exponent->isMandatory || PriceDivisor_exponent->previousValue.isAssigned())
        PriceDivisor_mantissa->decode(input);
      MinOrderQty->decode(input);
      MaxOrderQty->decode(input);
      MultiLegModel->decode(input);
      MultiLegPriceMethod->decode(input);
      Currency->decode(input);
      SettlCurrency->decode(input);
      Product->decode(input);
      SecurityType->decode(input);
      SecuritySubType->decode(input);
      SecurityStrategyType->decode(input);
      Asset->decode(input);
      SecurityDesc->decode(input);
      NoShareIssued->decode(input);
      MaturityDate->decode(input);
      MaturityMonthYear->decode(input);
      // decimal fields StrikePrice
      StrikePrice_exponent->decode(input, pmap1);
      if (StrikePrice_exponent->isMandatory || StrikePrice_exponent->previousValue.isAssigned())
        StrikePrice_mantissa->decode(input);
      StrikeCurrency->decode(input);
      ExerciseStyle->decode(input);
      PutOrCall->decode(input);
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
      SecurityValidityTimestamp->decode(input);
      MarketSegmentID->decode(input);
      GovernanceIndicator->decode(input);
      CorporateActionEventID->decode(input);
      SecurityGroup->decode(input);
      SecurityMatchType->decode(input);

      process();
    }
    // decode for sequence RelatedSym ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
    ApplVerID->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    TotNoRelatedSym->reset();
    LastFragment->reset();
    // reset for sequence RelatedSym
    NoRelatedSym->reset();
    Symbol->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence ApplIDs
    NoApplIDs->reset();
    ApplID->reset();
    // reset for sequence FeedTypes
    NoMDFeedTypes->reset();
    MDFeedType->reset();
    MarketDepth->reset();
    // reset for sequence FeedTypes ends
    // reset for sequence ApplIDs ends
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
    // decimal fields IndexPct
    IndexPct_exponent->reset();
    IndexPct_mantissa->reset();
    // reset for sequence Underlyings ends
    ImpliedMarketIndicator->reset();
    // reset for sequence InstrAttrib
    NoInstrAttrib->reset();
    InstAttribType->reset();
    InstAttribValue->reset();
    // reset for sequence InstrAttrib ends
    // reset for sequence TickRules
    NoTickRules->reset();
    // decimal fields StartTickPriceRange
    StartTickPriceRange_exponent->reset();
    StartTickPriceRange_mantissa->reset();
    // decimal fields EndTickPriceRange
    EndTickPriceRange_exponent->reset();
    EndTickPriceRange_mantissa->reset();
    // decimal fields TickIncrement
    TickIncrement_exponent->reset();
    TickIncrement_mantissa->reset();
    TickRuleType->reset();
    // reset for sequence TickRules ends
    // reset for sequence Legs
    NoLegs->reset();
    LegSymbol->reset();
    LegSecurityID->reset();
    LegSecurityIDSource->reset();
    LegRatioQty->reset();
    LegSecurityType->reset();
    LegSide->reset();
    LegSecurityExchange->reset();
    // reset for sequence Legs ends
    SecurityUpdateAction->reset();
    // reset for sequence Lots
    NoLotTypeRules->reset();
    LotType->reset();
    MinLotSize->reset();
    // reset for sequence Lots ends
    // decimal fields MinPriceIncrement
    MinPriceIncrement_exponent->reset();
    MinPriceIncrement_mantissa->reset();
    TickSizeDenominator->reset();
    // decimal fields PriceDivisor
    PriceDivisor_exponent->reset();
    PriceDivisor_mantissa->reset();
    MinOrderQty->reset();
    MaxOrderQty->reset();
    MultiLegModel->reset();
    MultiLegPriceMethod->reset();
    Currency->reset();
    SettlCurrency->reset();
    Product->reset();
    SecurityType->reset();
    SecuritySubType->reset();
    SecurityStrategyType->reset();
    Asset->reset();
    SecurityDesc->reset();
    NoShareIssued->reset();
    MaturityDate->reset();
    MaturityMonthYear->reset();
    // decimal fields StrikePrice
    StrikePrice_exponent->reset();
    StrikePrice_mantissa->reset();
    StrikeCurrency->reset();
    ExerciseStyle->reset();
    PutOrCall->reset();
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
    MarketSegmentID->reset();
    GovernanceIndicator->reset();
    CorporateActionEventID->reset();
    SecurityGroup->reset();
    SecurityMatchType->reset();
    // reset for sequence RelatedSym ends
  }

  // Destructor
  ~MDSecurityList_141() {
    delete MsgType;
    delete ApplVerID;
    delete MsgSeqNum;
    delete SendingTime;
    delete TotNoRelatedSym;
    delete LastFragment;
    // destructor for sequence RelatedSym
    delete NoRelatedSym;
    delete Symbol;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence ApplIDs
    delete NoApplIDs;
    delete ApplID;
    // destructor for sequence FeedTypes
    delete NoMDFeedTypes;
    delete MDFeedType;
    delete MarketDepth;
    // destructor for sequence FeedTypes ends
    // destructor for sequence ApplIDs ends
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
    // decimal fields IndexPct
    delete IndexPct_exponent;
    delete IndexPct_mantissa;
    // destructor for sequence Underlyings ends
    delete ImpliedMarketIndicator;
    // destructor for sequence InstrAttrib
    delete NoInstrAttrib;
    delete InstAttribType;
    delete InstAttribValue;
    // destructor for sequence InstrAttrib ends
    // destructor for sequence TickRules
    delete NoTickRules;
    // decimal fields StartTickPriceRange
    delete StartTickPriceRange_exponent;
    delete StartTickPriceRange_mantissa;
    // decimal fields EndTickPriceRange
    delete EndTickPriceRange_exponent;
    delete EndTickPriceRange_mantissa;
    // decimal fields TickIncrement
    delete TickIncrement_exponent;
    delete TickIncrement_mantissa;
    delete TickRuleType;
    // destructor for sequence TickRules ends
    // destructor for sequence Legs
    delete NoLegs;
    delete LegSymbol;
    delete LegSecurityID;
    delete LegSecurityIDSource;
    delete LegRatioQty;
    delete LegSecurityType;
    delete LegSide;
    delete LegSecurityExchange;
    // destructor for sequence Legs ends
    delete SecurityUpdateAction;
    // destructor for sequence Lots
    delete NoLotTypeRules;
    delete LotType;
    delete MinLotSize;
    // destructor for sequence Lots ends
    // decimal fields MinPriceIncrement
    delete MinPriceIncrement_exponent;
    delete MinPriceIncrement_mantissa;
    delete TickSizeDenominator;
    // decimal fields PriceDivisor
    delete PriceDivisor_exponent;
    delete PriceDivisor_mantissa;
    delete MinOrderQty;
    delete MaxOrderQty;
    delete MultiLegModel;
    delete MultiLegPriceMethod;
    delete Currency;
    delete SettlCurrency;
    delete Product;
    delete SecurityType;
    delete SecuritySubType;
    delete SecurityStrategyType;
    delete Asset;
    delete SecurityDesc;
    delete NoShareIssued;
    delete MaturityDate;
    delete MaturityMonthYear;
    // decimal fields StrikePrice
    delete StrikePrice_exponent;
    delete StrikePrice_mantissa;
    delete StrikeCurrency;
    delete ExerciseStyle;
    delete PutOrCall;
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
    delete MarketSegmentID;
    delete GovernanceIndicator;
    delete CorporateActionEventID;
    delete SecurityGroup;
    delete SecurityMatchType;
    // destructor for sequence RelatedSym ends
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
  ~MDSecurityList_111() {
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
  CopyField<uint64_t> *SecurityID;
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
    SecurityID = new CopyField<uint64_t>(false, false, 0);
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
  ~MDIncRefresh_81() {
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
class MDIncRefresh_150 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityExchange;
  CopyField<uint64_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  // decimal fields MDEntryInterestRate
  DefaultField<int32_t> *MDEntryInterestRate_exponent;
  DeltaField<int64_t> *MDEntryInterestRate_mantissa;
  DefaultField<uint32_t> *NumberOfOrders;
  NoOpField<FFUtils::ByteArr> *PriceType;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int64_t> *MDEntrySize;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<uint32_t> *MDInsertDate;
  CopyField<uint32_t> *MDInsertTime;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  CopyField<FFUtils::ByteArr> *Currency;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *SellerDays;
  DeltaField<uint64_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  NoOpField<FFUtils::ByteArr> *TradeCondition;
  NoOpField<uint32_t> *TradingSessionID;
  NoOpField<uint32_t> *OpenCloseSettlFlag;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  DefaultField<FFUtils::ByteArr> *MDEntryBuyer;
  DefaultField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<uint32_t> *MDEntryPositionNo;
  NoOpField<uint32_t> *SettPriceType;
  NoOpField<uint32_t> *LastTradeDate;
  NoOpField<uint32_t> *PriceAdjustmentMethod;
  DefaultField<FFUtils::ByteArr> *PriceBandType;
  DefaultField<uint32_t> *PriceLimitType;
  // decimal fields LowLimitPrice
  DefaultField<int32_t> *LowLimitPrice_exponent;
  DeltaField<int64_t> *LowLimitPrice_mantissa;
  // decimal fields HighLimitPrice
  DefaultField<int32_t> *HighLimitPrice_exponent;
  DeltaField<int64_t> *HighLimitPrice_mantissa;
  // decimal fields TradingReferencePrice
  DefaultField<int32_t> *TradingReferencePrice_exponent;
  DeltaField<int64_t> *TradingReferencePrice_mantissa;
  NoOpField<uint32_t> *PriceBandMidpointPriceType;
  NoOpField<uint64_t> *AvgDailyTradedQty;
  NoOpField<uint64_t> *ExpireDate;
  NoOpField<uint64_t> *EarlyTermination;
  NoOpField<uint32_t> *BTBCertIndicator;
  NoOpField<uint32_t> *BTBContractInfo;
  NoOpField<uint32_t> *BTBGraceDate;
  NoOpField<uint64_t> *MaxTradeVol;
  // fields inside sequence Underlyings
  NoOpField<uint32_t> *NoUnderlyings;
  DeltaField<uint64_t> *UnderlyingSecurityID;
  ConstantFieldMandatory<uint32_t> *UnderlyingSecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *UnderlyingSecurityExchange;
  // decimal fields UnderlyingPx
  DefaultField<int32_t> *UnderlyingPx_exponent;
  DeltaField<int64_t> *UnderlyingPx_mantissa;
  NoOpField<uint32_t> *UnderlyingPxType;
  // fields inside sequence Underlyings ends
  NoOpField<uint64_t> *IndexSeq;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_150() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    TradeDate = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    SecurityID = new CopyField<uint64_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields MDEntryInterestRate
    MDEntryInterestRate_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryInterestRate_mantissa = new DeltaField<int64_t>(true, false, 0);
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    PriceType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int64_t>(false, false, 0);
    MDEntryDate = new CopyField<uint32_t>(false, false, 0);
    MDInsertDate = new CopyField<uint32_t>(false, false, 0);
    MDInsertTime = new CopyField<uint32_t>(false, false, 0);
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Currency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    TradeVolume = new DeltaField<uint64_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradingSessionID = new NoOpField<uint32_t>(false, false, 0);
    OpenCloseSettlFlag = new NoOpField<uint32_t>(false, false, 0);
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryPositionNo = new DefaultField<uint32_t>(false, false, 0);
    SettPriceType = new NoOpField<uint32_t>(false, false, 0);
    LastTradeDate = new NoOpField<uint32_t>(false, false, 0);
    PriceAdjustmentMethod = new NoOpField<uint32_t>(false, false, 0);
    PriceBandType = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    PriceLimitType = new DefaultField<uint32_t>(false, false, 0);
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    LowLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    HighLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent = new DefaultField<int32_t>(false, false, 0);
    TradingReferencePrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    PriceBandMidpointPriceType = new NoOpField<uint32_t>(false, false, 0);
    AvgDailyTradedQty = new NoOpField<uint64_t>(false, false, 0);
    ExpireDate = new NoOpField<uint64_t>(false, false, 0);
    EarlyTermination = new NoOpField<uint64_t>(false, false, 0);
    BTBCertIndicator = new NoOpField<uint32_t>(false, false, 0);
    BTBContractInfo = new NoOpField<uint32_t>(false, false, 0);
    BTBGraceDate = new NoOpField<uint32_t>(false, false, 0);
    MaxTradeVol = new NoOpField<uint64_t>(false, false, 0);
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t>(false, false, 0);
    UnderlyingSecurityID = new DeltaField<uint64_t>(true, false, 0);
    UnderlyingSecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    UnderlyingSecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // decimal fields UnderlyingPx
    UnderlyingPx_exponent = new DefaultField<int32_t>(true, true, -2);
    UnderlyingPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    UnderlyingPxType = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence Underlyings ends
    IndexSeq = new NoOpField<uint64_t>(false, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MsgType->decode();
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
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityExchange->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      // decimal fields MDEntryInterestRate
      MDEntryInterestRate_exponent->decode(input, pmap1);
      if (MDEntryInterestRate_exponent->isMandatory || MDEntryInterestRate_exponent->isValueSet)
        MDEntryInterestRate_mantissa->decode(input);
      NumberOfOrders->decode(input, pmap1);
      PriceType->decode(input);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDInsertDate->decode(input, pmap1);
      MDInsertTime->decode(input, pmap1);
      MDStreamID->decode(input, pmap1);
      Currency->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      SellerDays->decode(input, pmap1);
      TradeVolume->decode(input);
      TickDirection->decode(input, pmap1);
      TradeCondition->decode(input);
      TradingSessionID->decode(input);
      OpenCloseSettlFlag->decode(input);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      SettPriceType->decode(input);
      LastTradeDate->decode(input);
      PriceAdjustmentMethod->decode(input);
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
      // decimal fields TradingReferencePrice
      TradingReferencePrice_exponent->decode(input, pmap1);
      if (TradingReferencePrice_exponent->isMandatory || TradingReferencePrice_exponent->isValueSet)
        TradingReferencePrice_mantissa->decode(input);
      PriceBandMidpointPriceType->decode(input);
      AvgDailyTradedQty->decode(input);
      ExpireDate->decode(input);
      EarlyTermination->decode(input);
      BTBCertIndicator->decode(input);
      BTBContractInfo->decode(input);
      BTBGraceDate->decode(input);
      MaxTradeVol->decode(input);
      // decode for sequence Underlyings
      NoUnderlyings->decode(input);

      int NoUnderlyings_len = NoUnderlyings->previousValue.getValue();
      if (NoUnderlyings->previousValue.isAssigned()) {
        for (int i = 0; i < NoUnderlyings_len; ++i) {
          UnderlyingSecurityID->decode(input);
          UnderlyingSecurityIDSource->decode();
          UnderlyingSecurityExchange->decode();
          // decimal fields UnderlyingPx
          UnderlyingPx_exponent->decode(input, pmap1);
          if (UnderlyingPx_exponent->isMandatory || UnderlyingPx_exponent->isValueSet)
            UnderlyingPx_mantissa->decode(input);
          UnderlyingPxType->decode(input);
        }
      }
      // decode for sequence Underlyings ends
      IndexSeq->decode(input);

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

  void ProcessTrade(char entry_type, uint64_t sec_id);
  void ProcessBookDelta(char entry_type, uint64_t sec_id);
  void ProcessOpeninigPrice(char entry_type, uint64_t sec_id);
  void ProcessSettlementPrice(char entry_type, uint64_t sec_id);
  void ProcessImbalance(char entry_type, uint64_t sec_id);
  void ProcessOrderFeed();
  void ProcessPriceFeed();

  // Reset
  void reset() {
    ApplVerID->reset();
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    SecurityID->reset();
    RptSeq->reset();
    QuoteCondition->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    // decimal fields MDEntryInterestRate
    MDEntryInterestRate_exponent->reset();
    MDEntryInterestRate_mantissa->reset();
    NumberOfOrders->reset();
    PriceType->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    MDEntryDate->reset();
    MDInsertDate->reset();
    MDInsertTime->reset();
    MDStreamID->reset();
    Currency->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    SellerDays->reset();
    TradeVolume->reset();
    TickDirection->reset();
    TradeCondition->reset();
    TradingSessionID->reset();
    OpenCloseSettlFlag->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    MDEntryPositionNo->reset();
    SettPriceType->reset();
    LastTradeDate->reset();
    PriceAdjustmentMethod->reset();
    PriceBandType->reset();
    PriceLimitType->reset();
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent->reset();
    LowLimitPrice_mantissa->reset();
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent->reset();
    HighLimitPrice_mantissa->reset();
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent->reset();
    TradingReferencePrice_mantissa->reset();
    PriceBandMidpointPriceType->reset();
    AvgDailyTradedQty->reset();
    ExpireDate->reset();
    EarlyTermination->reset();
    BTBCertIndicator->reset();
    BTBContractInfo->reset();
    BTBGraceDate->reset();
    MaxTradeVol->reset();
    // reset for sequence Underlyings
    NoUnderlyings->reset();
    UnderlyingSecurityID->reset();
    UnderlyingSecurityIDSource->reset();
    UnderlyingSecurityExchange->reset();
    // decimal fields UnderlyingPx
    UnderlyingPx_exponent->reset();
    UnderlyingPx_mantissa->reset();
    UnderlyingPxType->reset();
    // reset for sequence Underlyings ends
    IndexSeq->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  ~MDIncRefresh_150() {
    delete ApplVerID;
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityExchange;
    delete SecurityID;
    delete RptSeq;
    delete QuoteCondition;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    // decimal fields MDEntryInterestRate
    delete MDEntryInterestRate_exponent;
    delete MDEntryInterestRate_mantissa;
    delete NumberOfOrders;
    delete PriceType;
    delete MDEntryTime;
    delete MDEntrySize;
    delete MDEntryDate;
    delete MDInsertDate;
    delete MDInsertTime;
    delete MDStreamID;
    delete Currency;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete SellerDays;
    delete TradeVolume;
    delete TickDirection;
    delete TradeCondition;
    delete TradingSessionID;
    delete OpenCloseSettlFlag;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete MDEntryPositionNo;
    delete SettPriceType;
    delete LastTradeDate;
    delete PriceAdjustmentMethod;
    delete PriceBandType;
    delete PriceLimitType;
    // decimal fields LowLimitPrice
    delete LowLimitPrice_exponent;
    delete LowLimitPrice_mantissa;
    // decimal fields HighLimitPrice
    delete HighLimitPrice_exponent;
    delete HighLimitPrice_mantissa;
    // decimal fields TradingReferencePrice
    delete TradingReferencePrice_exponent;
    delete TradingReferencePrice_mantissa;
    delete PriceBandMidpointPriceType;
    delete AvgDailyTradedQty;
    delete ExpireDate;
    delete EarlyTermination;
    delete BTBCertIndicator;
    delete BTBContractInfo;
    delete BTBGraceDate;
    delete MaxTradeVol;
    // destructor for sequence Underlyings
    delete NoUnderlyings;
    delete UnderlyingSecurityID;
    delete UnderlyingSecurityIDSource;
    delete UnderlyingSecurityExchange;
    // decimal fields UnderlyingPx
    delete UnderlyingPx_exponent;
    delete UnderlyingPx_mantissa;
    delete UnderlyingPxType;
    // destructor for sequence Underlyings ends
    delete IndexSeq;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_145 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityExchange;
  CopyField<uint64_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  // decimal fields MDEntryInterestRate
  DefaultField<int32_t> *MDEntryInterestRate_exponent;
  DeltaField<int64_t> *MDEntryInterestRate_mantissa;
  DefaultField<uint32_t> *NumberOfOrders;
  NoOpField<FFUtils::ByteArr> *PriceType;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int64_t> *MDEntrySize;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<uint32_t> *MDInsertDate;
  CopyField<uint32_t> *MDInsertTime;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  CopyField<FFUtils::ByteArr> *Currency;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *SellerDays;
  DeltaField<uint64_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  NoOpField<FFUtils::ByteArr> *TradeCondition;
  NoOpField<uint32_t> *TradingSessionID;
  NoOpField<uint32_t> *OpenCloseSettlFlag;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  DefaultField<FFUtils::ByteArr> *MDEntryBuyer;
  DefaultField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<uint32_t> *MDEntryPositionNo;
  NoOpField<uint32_t> *SettPriceType;
  NoOpField<uint32_t> *LastTradeDate;
  NoOpField<uint32_t> *PriceAdjustmentMethod;
  DefaultField<FFUtils::ByteArr> *PriceBandType;
  DefaultField<uint32_t> *PriceLimitType;
  // decimal fields LowLimitPrice
  DefaultField<int32_t> *LowLimitPrice_exponent;
  DeltaField<int64_t> *LowLimitPrice_mantissa;
  // decimal fields HighLimitPrice
  DefaultField<int32_t> *HighLimitPrice_exponent;
  DeltaField<int64_t> *HighLimitPrice_mantissa;
  // decimal fields TradingReferencePrice
  DefaultField<int32_t> *TradingReferencePrice_exponent;
  DeltaField<int64_t> *TradingReferencePrice_mantissa;
  NoOpField<uint32_t> *PriceBandMidpointPriceType;
  NoOpField<uint64_t> *AvgDailyTradedQty;
  NoOpField<uint64_t> *ExpireDate;
  NoOpField<uint64_t> *EarlyTermination;
  NoOpField<uint64_t> *MaxTradeVol;
  // fields inside sequence Underlyings
  NoOpField<uint32_t> *NoUnderlyings;
  DeltaField<uint64_t> *UnderlyingSecurityID;
  ConstantFieldMandatory<uint32_t> *UnderlyingSecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *UnderlyingSecurityExchange;
  // decimal fields UnderlyingPx
  DefaultField<int32_t> *UnderlyingPx_exponent;
  DeltaField<int64_t> *UnderlyingPx_mantissa;
  NoOpField<uint32_t> *UnderlyingPxType;
  // fields inside sequence Underlyings ends
  NoOpField<uint64_t> *IndexSeq;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_145() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    TradeDate = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    SecurityID = new CopyField<uint64_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields MDEntryInterestRate
    MDEntryInterestRate_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryInterestRate_mantissa = new DeltaField<int64_t>(true, false, 0);
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    PriceType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int64_t>(false, false, 0);
    MDEntryDate = new CopyField<uint32_t>(false, false, 0);
    MDInsertDate = new CopyField<uint32_t>(false, false, 0);
    MDInsertTime = new CopyField<uint32_t>(false, false, 0);
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Currency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    TradeVolume = new DeltaField<uint64_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradingSessionID = new NoOpField<uint32_t>(false, false, 0);
    OpenCloseSettlFlag = new NoOpField<uint32_t>(false, false, 0);
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryPositionNo = new DefaultField<uint32_t>(false, false, 0);
    SettPriceType = new NoOpField<uint32_t>(false, false, 0);
    LastTradeDate = new NoOpField<uint32_t>(false, false, 0);
    PriceAdjustmentMethod = new NoOpField<uint32_t>(false, false, 0);
    PriceBandType = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    PriceLimitType = new DefaultField<uint32_t>(false, false, 0);
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    LowLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    HighLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent = new DefaultField<int32_t>(false, false, 0);
    TradingReferencePrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    PriceBandMidpointPriceType = new NoOpField<uint32_t>(false, false, 0);
    AvgDailyTradedQty = new NoOpField<uint64_t>(false, false, 0);
    ExpireDate = new NoOpField<uint64_t>(false, false, 0);
    EarlyTermination = new NoOpField<uint64_t>(false, false, 0);
    MaxTradeVol = new NoOpField<uint64_t>(false, false, 0);
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t>(false, false, 0);
    UnderlyingSecurityID = new DeltaField<uint64_t>(true, false, 0);
    UnderlyingSecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    UnderlyingSecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // decimal fields UnderlyingPx
    UnderlyingPx_exponent = new DefaultField<int32_t>(true, true, -2);
    UnderlyingPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    UnderlyingPxType = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence Underlyings ends
    IndexSeq = new NoOpField<uint64_t>(false, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MsgType->decode();
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
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityExchange->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      // decimal fields MDEntryInterestRate
      MDEntryInterestRate_exponent->decode(input, pmap1);
      if (MDEntryInterestRate_exponent->isMandatory || MDEntryInterestRate_exponent->isValueSet)
        MDEntryInterestRate_mantissa->decode(input);
      NumberOfOrders->decode(input, pmap1);
      PriceType->decode(input);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDInsertDate->decode(input, pmap1);
      MDInsertTime->decode(input, pmap1);
      MDStreamID->decode(input, pmap1);
      Currency->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      SellerDays->decode(input, pmap1);
      TradeVolume->decode(input);
      TickDirection->decode(input, pmap1);
      TradeCondition->decode(input);
      TradingSessionID->decode(input);
      OpenCloseSettlFlag->decode(input);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      SettPriceType->decode(input);
      LastTradeDate->decode(input);
      PriceAdjustmentMethod->decode(input);
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
      // decimal fields TradingReferencePrice
      TradingReferencePrice_exponent->decode(input, pmap1);
      if (TradingReferencePrice_exponent->isMandatory || TradingReferencePrice_exponent->isValueSet)
        TradingReferencePrice_mantissa->decode(input);
      PriceBandMidpointPriceType->decode(input);
      AvgDailyTradedQty->decode(input);
      ExpireDate->decode(input);
      EarlyTermination->decode(input);
      MaxTradeVol->decode(input);
      // decode for sequence Underlyings
      NoUnderlyings->decode(input);

      int NoUnderlyings_len = NoUnderlyings->previousValue.getValue();
      if (NoUnderlyings->previousValue.isAssigned()) {
        for (int i = 0; i < NoUnderlyings_len; ++i) {
          UnderlyingSecurityID->decode(input);
          UnderlyingSecurityIDSource->decode();
          UnderlyingSecurityExchange->decode();
          // decimal fields UnderlyingPx
          UnderlyingPx_exponent->decode(input, pmap1);
          if (UnderlyingPx_exponent->isMandatory || UnderlyingPx_exponent->isValueSet)
            UnderlyingPx_mantissa->decode(input);
          UnderlyingPxType->decode(input);
        }
      }
      // decode for sequence Underlyings ends
      IndexSeq->decode(input);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    SecurityID->reset();
    RptSeq->reset();
    QuoteCondition->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    // decimal fields MDEntryInterestRate
    MDEntryInterestRate_exponent->reset();
    MDEntryInterestRate_mantissa->reset();
    NumberOfOrders->reset();
    PriceType->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    MDEntryDate->reset();
    MDInsertDate->reset();
    MDInsertTime->reset();
    MDStreamID->reset();
    Currency->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    SellerDays->reset();
    TradeVolume->reset();
    TickDirection->reset();
    TradeCondition->reset();
    TradingSessionID->reset();
    OpenCloseSettlFlag->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    MDEntryPositionNo->reset();
    SettPriceType->reset();
    LastTradeDate->reset();
    PriceAdjustmentMethod->reset();
    PriceBandType->reset();
    PriceLimitType->reset();
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent->reset();
    LowLimitPrice_mantissa->reset();
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent->reset();
    HighLimitPrice_mantissa->reset();
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent->reset();
    TradingReferencePrice_mantissa->reset();
    PriceBandMidpointPriceType->reset();
    AvgDailyTradedQty->reset();
    ExpireDate->reset();
    EarlyTermination->reset();
    MaxTradeVol->reset();
    // reset for sequence Underlyings
    NoUnderlyings->reset();
    UnderlyingSecurityID->reset();
    UnderlyingSecurityIDSource->reset();
    UnderlyingSecurityExchange->reset();
    // decimal fields UnderlyingPx
    UnderlyingPx_exponent->reset();
    UnderlyingPx_mantissa->reset();
    UnderlyingPxType->reset();
    // reset for sequence Underlyings ends
    IndexSeq->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  ~MDIncRefresh_145() {
    delete ApplVerID;
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityExchange;
    delete SecurityID;
    delete RptSeq;
    delete QuoteCondition;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    // decimal fields MDEntryInterestRate
    delete MDEntryInterestRate_exponent;
    delete MDEntryInterestRate_mantissa;
    delete NumberOfOrders;
    delete PriceType;
    delete MDEntryTime;
    delete MDEntrySize;
    delete MDEntryDate;
    delete MDInsertDate;
    delete MDInsertTime;
    delete MDStreamID;
    delete Currency;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete SellerDays;
    delete TradeVolume;
    delete TickDirection;
    delete TradeCondition;
    delete TradingSessionID;
    delete OpenCloseSettlFlag;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete MDEntryPositionNo;
    delete SettPriceType;
    delete LastTradeDate;
    delete PriceAdjustmentMethod;
    delete PriceBandType;
    delete PriceLimitType;
    // decimal fields LowLimitPrice
    delete LowLimitPrice_exponent;
    delete LowLimitPrice_mantissa;
    // decimal fields HighLimitPrice
    delete HighLimitPrice_exponent;
    delete HighLimitPrice_mantissa;
    // decimal fields TradingReferencePrice
    delete TradingReferencePrice_exponent;
    delete TradingReferencePrice_mantissa;
    delete PriceBandMidpointPriceType;
    delete AvgDailyTradedQty;
    delete ExpireDate;
    delete EarlyTermination;
    delete MaxTradeVol;
    // destructor for sequence Underlyings
    delete NoUnderlyings;
    delete UnderlyingSecurityID;
    delete UnderlyingSecurityIDSource;
    delete UnderlyingSecurityExchange;
    // decimal fields UnderlyingPx
    delete UnderlyingPx_exponent;
    delete UnderlyingPx_mantissa;
    delete UnderlyingPxType;
    // destructor for sequence Underlyings ends
    delete IndexSeq;
    // destructor for sequence MDEntries ends
  }
};
class MDIncRefresh_138 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<uint32_t> *TradeDate;
  // fields inside sequence MDEntries
  NoOpField<uint32_t> *NoMDEntries;
  CopyField<uint32_t> *MDUpdateAction;
  CopyField<FFUtils::ByteArr> *MDEntryType;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *SecurityExchange;
  CopyField<uint64_t> *SecurityID;
  IncrementField<uint32_t> *RptSeq;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  // decimal fields MDEntryInterestRate
  DefaultField<int32_t> *MDEntryInterestRate_exponent;
  DeltaField<int64_t> *MDEntryInterestRate_mantissa;
  DefaultField<uint32_t> *NumberOfOrders;
  NoOpField<FFUtils::ByteArr> *PriceType;
  CopyField<uint32_t> *MDEntryTime;
  DeltaField<int64_t> *MDEntrySize;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<uint32_t> *MDInsertDate;
  CopyField<uint32_t> *MDInsertTime;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  CopyField<FFUtils::ByteArr> *Currency;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<uint32_t> *SellerDays;
  DeltaField<uint64_t> *TradeVolume;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  NoOpField<FFUtils::ByteArr> *TradeCondition;
  NoOpField<uint32_t> *TradingSessionID;
  NoOpField<FFUtils::ByteArr> *OpenCloseSettlFlag;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  DefaultField<FFUtils::ByteArr> *MDEntryBuyer;
  DefaultField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<uint32_t> *MDEntryPositionNo;
  NoOpField<uint32_t> *SettPriceType;
  NoOpField<uint32_t> *LastTradeDate;
  NoOpField<uint32_t> *PriceAdjustmentMethod;
  DefaultField<FFUtils::ByteArr> *PriceBandType;
  DefaultField<uint32_t> *PriceLimitType;
  // decimal fields LowLimitPrice
  DefaultField<int32_t> *LowLimitPrice_exponent;
  DeltaField<int64_t> *LowLimitPrice_mantissa;
  // decimal fields HighLimitPrice
  DefaultField<int32_t> *HighLimitPrice_exponent;
  DeltaField<int64_t> *HighLimitPrice_mantissa;
  // decimal fields TradingReferencePrice
  DefaultField<int32_t> *TradingReferencePrice_exponent;
  DeltaField<int64_t> *TradingReferencePrice_mantissa;
  NoOpField<uint32_t> *PriceBandMidpointPriceType;
  NoOpField<uint64_t> *AvgDailyTradedQty;
  NoOpField<uint64_t> *ExpireDate;
  NoOpField<uint64_t> *EarlyTermination;
  NoOpField<uint64_t> *MaxTradeVol;
  // fields inside sequence Underlyings
  NoOpField<uint32_t> *NoUnderlyings;
  DeltaField<uint64_t> *UnderlyingSecurityID;
  ConstantFieldMandatory<uint32_t> *UnderlyingSecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *UnderlyingSecurityExchange;
  // decimal fields UnderlyingPx
  DefaultField<int32_t> *UnderlyingPx_exponent;
  DeltaField<int64_t> *UnderlyingPx_mantissa;
  NoOpField<uint32_t> *UnderlyingPxType;
  // fields inside sequence Underlyings ends
  NoOpField<uint64_t> *IndexSeq;
  // fields inside sequence MDEntries ends

  // Constructor
  MDIncRefresh_138() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("X"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    TradeDate = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence MDEntries
    NoMDEntries = new NoOpField<uint32_t>(true, false, 0);
    MDUpdateAction = new CopyField<uint32_t>(true, true, 1);
    MDEntryType = new CopyField<FFUtils::ByteArr>(true, true, FFUtils::ByteArr("0"));
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    SecurityID = new CopyField<uint64_t>(true, false, 0);
    RptSeq = new IncrementField<uint32_t>(true, false, 0);
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields MDEntryInterestRate
    MDEntryInterestRate_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryInterestRate_mantissa = new DeltaField<int64_t>(true, false, 0);
    NumberOfOrders = new DefaultField<uint32_t>(false, false, 0);
    PriceType = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryTime = new CopyField<uint32_t>(true, false, 0);
    MDEntrySize = new DeltaField<int64_t>(false, false, 0);
    MDEntryDate = new CopyField<uint32_t>(false, false, 0);
    MDInsertDate = new CopyField<uint32_t>(false, false, 0);
    MDInsertTime = new CopyField<uint32_t>(false, false, 0);
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Currency = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    TradeVolume = new DeltaField<uint64_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeCondition = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradingSessionID = new NoOpField<uint32_t>(false, false, 0);
    OpenCloseSettlFlag = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryPositionNo = new DefaultField<uint32_t>(false, false, 0);
    SettPriceType = new NoOpField<uint32_t>(false, false, 0);
    LastTradeDate = new NoOpField<uint32_t>(false, false, 0);
    PriceAdjustmentMethod = new NoOpField<uint32_t>(false, false, 0);
    PriceBandType = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    PriceLimitType = new DefaultField<uint32_t>(false, false, 0);
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    LowLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    HighLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent = new DefaultField<int32_t>(false, false, 0);
    TradingReferencePrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    PriceBandMidpointPriceType = new NoOpField<uint32_t>(false, false, 0);
    AvgDailyTradedQty = new NoOpField<uint64_t>(false, false, 0);
    ExpireDate = new NoOpField<uint64_t>(false, false, 0);
    EarlyTermination = new NoOpField<uint64_t>(false, false, 0);
    MaxTradeVol = new NoOpField<uint64_t>(false, false, 0);
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t>(false, false, 0);
    UnderlyingSecurityID = new DeltaField<uint64_t>(true, false, 0);
    UnderlyingSecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    UnderlyingSecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // decimal fields UnderlyingPx
    UnderlyingPx_exponent = new DefaultField<int32_t>(true, true, -2);
    UnderlyingPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    UnderlyingPxType = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence Underlyings ends
    IndexSeq = new NoOpField<uint64_t>(false, false, 0);
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MsgType->decode();
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
      MDEntryType->decode(input, pmap1);
      SecurityIDSource->decode();
      SecurityExchange->decode();
      SecurityID->decode(input, pmap1);
      RptSeq->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      // decimal fields MDEntryInterestRate
      MDEntryInterestRate_exponent->decode(input, pmap1);
      if (MDEntryInterestRate_exponent->isMandatory || MDEntryInterestRate_exponent->isValueSet)
        MDEntryInterestRate_mantissa->decode(input);
      NumberOfOrders->decode(input, pmap1);
      PriceType->decode(input);
      MDEntryTime->decode(input, pmap1);
      MDEntrySize->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDInsertDate->decode(input, pmap1);
      MDInsertTime->decode(input, pmap1);
      MDStreamID->decode(input, pmap1);
      Currency->decode(input, pmap1);
      // decimal fields NetChgPrevDay
      NetChgPrevDay_exponent->decode(input, pmap1);
      if (NetChgPrevDay_exponent->isMandatory || NetChgPrevDay_exponent->isValueSet)
        NetChgPrevDay_mantissa->decode(input);
      SellerDays->decode(input, pmap1);
      TradeVolume->decode(input);
      TickDirection->decode(input, pmap1);
      TradeCondition->decode(input);
      TradingSessionID->decode(input);
      OpenCloseSettlFlag->decode(input);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      SettPriceType->decode(input);
      LastTradeDate->decode(input);
      PriceAdjustmentMethod->decode(input);
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
      // decimal fields TradingReferencePrice
      TradingReferencePrice_exponent->decode(input, pmap1);
      if (TradingReferencePrice_exponent->isMandatory || TradingReferencePrice_exponent->isValueSet)
        TradingReferencePrice_mantissa->decode(input);
      PriceBandMidpointPriceType->decode(input);
      AvgDailyTradedQty->decode(input);
      ExpireDate->decode(input);
      EarlyTermination->decode(input);
      MaxTradeVol->decode(input);
      // decode for sequence Underlyings
      NoUnderlyings->decode(input);

      int NoUnderlyings_len = NoUnderlyings->previousValue.getValue();
      if (NoUnderlyings->previousValue.isAssigned()) {
        for (int i = 0; i < NoUnderlyings_len; ++i) {
          UnderlyingSecurityID->decode(input);
          UnderlyingSecurityIDSource->decode();
          UnderlyingSecurityExchange->decode();
          // decimal fields UnderlyingPx
          UnderlyingPx_exponent->decode(input, pmap1);
          if (UnderlyingPx_exponent->isMandatory || UnderlyingPx_exponent->isValueSet)
            UnderlyingPx_mantissa->decode(input);
          UnderlyingPxType->decode(input);
        }
      }
      // decode for sequence Underlyings ends
      IndexSeq->decode(input);
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    TradeDate->reset();
    // reset for sequence MDEntries
    NoMDEntries->reset();
    MDUpdateAction->reset();
    MDEntryType->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    SecurityID->reset();
    RptSeq->reset();
    QuoteCondition->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    // decimal fields MDEntryInterestRate
    MDEntryInterestRate_exponent->reset();
    MDEntryInterestRate_mantissa->reset();
    NumberOfOrders->reset();
    PriceType->reset();
    MDEntryTime->reset();
    MDEntrySize->reset();
    MDEntryDate->reset();
    MDInsertDate->reset();
    MDInsertTime->reset();
    MDStreamID->reset();
    Currency->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    SellerDays->reset();
    TradeVolume->reset();
    TickDirection->reset();
    TradeCondition->reset();
    TradingSessionID->reset();
    OpenCloseSettlFlag->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    MDEntryPositionNo->reset();
    SettPriceType->reset();
    LastTradeDate->reset();
    PriceAdjustmentMethod->reset();
    PriceBandType->reset();
    PriceLimitType->reset();
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent->reset();
    LowLimitPrice_mantissa->reset();
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent->reset();
    HighLimitPrice_mantissa->reset();
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent->reset();
    TradingReferencePrice_mantissa->reset();
    PriceBandMidpointPriceType->reset();
    AvgDailyTradedQty->reset();
    ExpireDate->reset();
    EarlyTermination->reset();
    MaxTradeVol->reset();
    // reset for sequence Underlyings
    NoUnderlyings->reset();
    UnderlyingSecurityID->reset();
    UnderlyingSecurityIDSource->reset();
    UnderlyingSecurityExchange->reset();
    // decimal fields UnderlyingPx
    UnderlyingPx_exponent->reset();
    UnderlyingPx_mantissa->reset();
    UnderlyingPxType->reset();
    // reset for sequence Underlyings ends
    IndexSeq->reset();
    // reset for sequence MDEntries ends
  }

  // Destructor
  ~MDIncRefresh_138() {
    delete ApplVerID;
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete TradeDate;
    // destructor for sequence MDEntries
    delete NoMDEntries;
    delete MDUpdateAction;
    delete MDEntryType;
    delete SecurityIDSource;
    delete SecurityExchange;
    delete SecurityID;
    delete RptSeq;
    delete QuoteCondition;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    // decimal fields MDEntryInterestRate
    delete MDEntryInterestRate_exponent;
    delete MDEntryInterestRate_mantissa;
    delete NumberOfOrders;
    delete PriceType;
    delete MDEntryTime;
    delete MDEntrySize;
    delete MDEntryDate;
    delete MDInsertDate;
    delete MDInsertTime;
    delete MDStreamID;
    delete Currency;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete SellerDays;
    delete TradeVolume;
    delete TickDirection;
    delete TradeCondition;
    delete TradingSessionID;
    delete OpenCloseSettlFlag;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete MDEntryPositionNo;
    delete SettPriceType;
    delete LastTradeDate;
    delete PriceAdjustmentMethod;
    delete PriceBandType;
    delete PriceLimitType;
    // decimal fields LowLimitPrice
    delete LowLimitPrice_exponent;
    delete LowLimitPrice_mantissa;
    // decimal fields HighLimitPrice
    delete HighLimitPrice_exponent;
    delete HighLimitPrice_mantissa;
    // decimal fields TradingReferencePrice
    delete TradingReferencePrice_exponent;
    delete TradingReferencePrice_mantissa;
    delete PriceBandMidpointPriceType;
    delete AvgDailyTradedQty;
    delete ExpireDate;
    delete EarlyTermination;
    delete MaxTradeVol;
    // destructor for sequence Underlyings
    delete NoUnderlyings;
    delete UnderlyingSecurityID;
    delete UnderlyingSecurityIDSource;
    delete UnderlyingSecurityExchange;
    // decimal fields UnderlyingPx
    delete UnderlyingPx_exponent;
    delete UnderlyingPx_mantissa;
    delete UnderlyingPxType;
    // destructor for sequence Underlyings ends
    delete IndexSeq;
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
    }
    // decode for sequence MDEntries ends

    // signifies end of sequences should process flush based stuff
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
  ~MDIncRefresh_123() {
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
  ~MDIncRefresh_126() {
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
class MDSecurityStatus_142 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<FFUtils::ByteArr> *SecurityGroup;
  NoOpField<uint64_t> *SecurityID;
  ConstantFieldOptional<uint32_t> *SecurityIDSource;
  NoOpField<FFUtils::ByteArr> *SecurityExchange;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  NoOpField<uint32_t> *SecurityTradingStatus;
  NoOpField<uint32_t> *TradingSessionID;
  NoOpField<uint64_t> *TradSesOpenTime;
  NoOpField<uint64_t> *TransactTime;
  NoOpField<uint32_t> *TradeDate;
  NoOpField<uint32_t> *SecurityTradingEvent;

  // Constructor
  MDSecurityStatus_142() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("f"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    SecurityGroup = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new NoOpField<uint64_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldOptional<uint32_t>(true, 8);
    SecurityExchange = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new NoOpField<uint32_t>(false, false, 0);
    TradingSessionID = new NoOpField<uint32_t>(false, false, 0);
    TradSesOpenTime = new NoOpField<uint64_t>(false, false, 0);
    TransactTime = new NoOpField<uint64_t>(true, false, 0);
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    SecurityTradingEvent = new NoOpField<uint32_t>(false, false, 0);
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    SecurityGroup->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode(pmap0);
    SecurityExchange->decode(input);
    TradingSessionSubID->decode(input);
    SecurityTradingStatus->decode(input);
    TradingSessionID->decode(input);
    TradSesOpenTime->decode(input);
    TransactTime->decode(input);
    TradeDate->decode(input);
    SecurityTradingEvent->decode(input);
    // update security trading status
    process();
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    SecurityGroup->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    TradingSessionSubID->reset();
    SecurityTradingStatus->reset();
    TradingSessionID->reset();
    TradSesOpenTime->reset();
    TransactTime->reset();
    TradeDate->reset();
    SecurityTradingEvent->reset();
  }

  // Destructor
  ~MDSecurityStatus_142() {
    delete ApplVerID;
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete SecurityGroup;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    delete TradingSessionSubID;
    delete SecurityTradingStatus;
    delete TradingSessionID;
    delete TradSesOpenTime;
    delete TransactTime;
    delete TradeDate;
    delete SecurityTradingEvent;
  }
};
class MDSecurityStatus_134 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  NoOpField<FFUtils::ByteArr> *SecurityGroup;
  NoOpField<FFUtils::ByteArr> *Symbol;
  NoOpField<uint64_t> *SecurityID;
  ConstantFieldOptional<uint32_t> *SecurityIDSource;
  NoOpField<FFUtils::ByteArr> *SecurityExchange;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  NoOpField<uint32_t> *SecurityTradingStatus;
  NoOpField<uint32_t> *TradingSessionID;
  NoOpField<uint64_t> *TradSesOpenTime;
  NoOpField<uint64_t> *TransactTime;
  NoOpField<uint32_t> *TradeDate;
  NoOpField<uint32_t> *SecurityTradingEvent;

  // Constructor
  MDSecurityStatus_134() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("f"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    SecurityGroup = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Symbol = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityID = new NoOpField<uint64_t>(false, false, 0);
    SecurityIDSource = new ConstantFieldOptional<uint32_t>(true, 8);
    SecurityExchange = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new NoOpField<uint32_t>(false, false, 0);
    TradingSessionID = new NoOpField<uint32_t>(false, false, 0);
    TradSesOpenTime = new NoOpField<uint64_t>(false, false, 0);
    TransactTime = new NoOpField<uint64_t>(true, false, 0);
    TradeDate = new NoOpField<uint32_t>(true, false, 0);
    SecurityTradingEvent = new NoOpField<uint32_t>(false, false, 0);
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    SecurityGroup->decode(input);
    Symbol->decode(input);
    SecurityID->decode(input);
    SecurityIDSource->decode(pmap0);
    SecurityExchange->decode(input);
    TradingSessionSubID->decode(input);
    SecurityTradingStatus->decode(input);
    TradingSessionID->decode(input);
    TradSesOpenTime->decode(input);
    TransactTime->decode(input);
    TradeDate->decode(input);
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
    SecurityGroup->reset();
    Symbol->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    TradingSessionSubID->reset();
    SecurityTradingStatus->reset();
    TradingSessionID->reset();
    TradSesOpenTime->reset();
    TransactTime->reset();
    TradeDate->reset();
    SecurityTradingEvent->reset();
  }

  // Destructor
  ~MDSecurityStatus_134() {
    delete ApplVerID;
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
    delete SecurityGroup;
    delete Symbol;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    delete TradingSessionSubID;
    delete SecurityTradingStatus;
    delete TradingSessionID;
    delete TradSesOpenTime;
    delete TransactTime;
    delete TradeDate;
    delete SecurityTradingEvent;
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
  ~MDSecurityStatus_125() {
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
  ~MDSnapshotFullRefresh_127() {
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
class MDSnapshotFullRefresh_151 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
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
  DefaultField<FFUtils::ByteArr> *Currency;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  // decimal fields MDEntryInterestRate
  DefaultField<int32_t> *MDEntryInterestRate_exponent;
  DeltaField<int64_t> *MDEntryInterestRate_mantissa;
  NoOpField<uint32_t> *IndexSeq;
  DeltaField<int64_t> *MDEntrySize;
  DeltaField<uint64_t> *TradeVolume;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<FFUtils::ByteArr> *MDEntryTime;
  CopyField<uint32_t> *MDInsertDate;
  CopyField<uint32_t> *MDInsertTime;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  DefaultField<FFUtils::ByteArr> *PriceType;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  DefaultField<uint32_t> *SecurityTradingStatus;
  DefaultField<uint64_t> *TradSesOpenTime;
  NoOpField<uint32_t> *TradingSessionID;
  NoOpField<uint32_t> *SecurityTradingEvent;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<uint32_t> *OpenCloseSettlFlag;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  DefaultField<FFUtils::ByteArr> *MDEntryBuyer;
  DefaultField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  CopyField<uint32_t> *NumberOfOrders;
  DefaultField<uint32_t> *MDEntryPositionNo;
  DefaultField<uint32_t> *SellerDays;
  DefaultField<uint32_t> *SettPriceType;
  NoOpField<uint32_t> *LastTradeDate;
  NoOpField<uint32_t> *PriceAdjustmentMethod;
  DefaultField<uint32_t> *PriceLimitType;
  // decimal fields LowLimitPrice
  DefaultField<int32_t> *LowLimitPrice_exponent;
  DeltaField<int64_t> *LowLimitPrice_mantissa;
  // decimal fields HighLimitPrice
  DefaultField<int32_t> *HighLimitPrice_exponent;
  DeltaField<int64_t> *HighLimitPrice_mantissa;
  // decimal fields TradingReferencePrice
  DefaultField<int32_t> *TradingReferencePrice_exponent;
  DeltaField<int64_t> *TradingReferencePrice_mantissa;
  NoOpField<uint32_t> *PriceBandMidpointPriceType;
  NoOpField<uint64_t> *AvgDailyTradedQty;
  NoOpField<uint64_t> *ExpireDate;
  NoOpField<uint64_t> *EarlyTermination;
  NoOpField<uint32_t> *BTBCertIndicator;
  NoOpField<uint32_t> *BTBContractInfo;
  NoOpField<uint32_t> *BTBGraceDate;
  NoOpField<uint64_t> *MaxTradeVol;
  DefaultField<FFUtils::ByteArr> *PriceBandType;
  // fields inside sequence Underlyings
  NoOpField<uint32_t> *NoUnderlyings;
  DeltaField<uint64_t> *UnderlyingSecurityID;
  ConstantFieldMandatory<uint32_t> *UnderlyingSecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *UnderlyingSecurityExchange;
  // decimal fields UnderlyingPx
  DefaultField<int32_t> *UnderlyingPx_exponent;
  DeltaField<int64_t> *UnderlyingPx_mantissa;
  NoOpField<uint32_t> *UnderlyingPxType;
  // fields inside sequence Underlyings ends
  // fields inside sequence MDEntries ends

  // Constructor
  MDSnapshotFullRefresh_151() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"));
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
    Currency = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields MDEntryInterestRate
    MDEntryInterestRate_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryInterestRate_mantissa = new DeltaField<int64_t>(true, false, 0);
    IndexSeq = new NoOpField<uint32_t>(false, false, 0);
    MDEntrySize = new DeltaField<int64_t>(false, false, 0);
    TradeVolume = new DeltaField<uint64_t>(false, false, 0);
    MDEntryDate = new CopyField<uint32_t>(false, false, 0);
    MDEntryTime = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDInsertDate = new CopyField<uint32_t>(false, false, 0);
    MDInsertTime = new CopyField<uint32_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("E"));
    PriceType = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new DefaultField<uint32_t>(false, false, 0);
    TradSesOpenTime = new DefaultField<uint64_t>(false, false, 0);
    TradingSessionID = new NoOpField<uint32_t>(false, false, 0);
    SecurityTradingEvent = new NoOpField<uint32_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettlFlag = new DefaultField<uint32_t>(false, false, 0);
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new CopyField<uint32_t>(false, false, 0);
    MDEntryPositionNo = new DefaultField<uint32_t>(false, false, 0);
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    SettPriceType = new DefaultField<uint32_t>(false, false, 0);
    LastTradeDate = new NoOpField<uint32_t>(false, false, 0);
    PriceAdjustmentMethod = new NoOpField<uint32_t>(false, false, 0);
    PriceLimitType = new DefaultField<uint32_t>(false, false, 0);
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    LowLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    HighLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent = new DefaultField<int32_t>(false, false, 0);
    TradingReferencePrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    PriceBandMidpointPriceType = new NoOpField<uint32_t>(false, false, 0);
    AvgDailyTradedQty = new NoOpField<uint64_t>(false, false, 0);
    ExpireDate = new NoOpField<uint64_t>(false, false, 0);
    EarlyTermination = new NoOpField<uint64_t>(false, false, 0);
    BTBCertIndicator = new NoOpField<uint32_t>(false, false, 0);
    BTBContractInfo = new NoOpField<uint32_t>(false, false, 0);
    BTBGraceDate = new NoOpField<uint32_t>(false, false, 0);
    MaxTradeVol = new NoOpField<uint64_t>(false, false, 0);
    PriceBandType = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t>(false, false, 0);
    UnderlyingSecurityID = new DeltaField<uint64_t>(true, false, 0);
    UnderlyingSecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    UnderlyingSecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // decimal fields UnderlyingPx
    UnderlyingPx_exponent = new DefaultField<int32_t>(true, true, -2);
    UnderlyingPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    UnderlyingPxType = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence Underlyings ends
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
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
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDEntryType->decode(input, pmap1);
      Currency->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      // decimal fields MDEntryInterestRate
      MDEntryInterestRate_exponent->decode(input, pmap1);
      if (MDEntryInterestRate_exponent->isMandatory || MDEntryInterestRate_exponent->isValueSet)
        MDEntryInterestRate_mantissa->decode(input);
      IndexSeq->decode(input);
      MDEntrySize->decode(input);
      TradeVolume->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      MDInsertDate->decode(input, pmap1);
      MDInsertTime->decode(input, pmap1);
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
      TradingSessionID->decode(input);
      SecurityTradingEvent->decode(input);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettlFlag->decode(input, pmap1);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      SellerDays->decode(input, pmap1);
      SettPriceType->decode(input, pmap1);
      LastTradeDate->decode(input);
      PriceAdjustmentMethod->decode(input);
      PriceLimitType->decode(input, pmap1);
      // decimal fields LowLimitPrice
      LowLimitPrice_exponent->decode(input, pmap1);
      if (LowLimitPrice_exponent->isMandatory || LowLimitPrice_exponent->isValueSet)
        LowLimitPrice_mantissa->decode(input);
      // decimal fields HighLimitPrice
      HighLimitPrice_exponent->decode(input, pmap1);
      if (HighLimitPrice_exponent->isMandatory || HighLimitPrice_exponent->isValueSet)
        HighLimitPrice_mantissa->decode(input);
      // decimal fields TradingReferencePrice
      TradingReferencePrice_exponent->decode(input, pmap1);
      if (TradingReferencePrice_exponent->isMandatory || TradingReferencePrice_exponent->isValueSet)
        TradingReferencePrice_mantissa->decode(input);
      PriceBandMidpointPriceType->decode(input);
      AvgDailyTradedQty->decode(input);
      ExpireDate->decode(input);
      EarlyTermination->decode(input);
      BTBCertIndicator->decode(input);
      BTBContractInfo->decode(input);
      BTBGraceDate->decode(input);
      MaxTradeVol->decode(input);
      PriceBandType->decode(input, pmap1);
      // decode for sequence Underlyings
      NoUnderlyings->decode(input);

      int NoUnderlyings_len = NoUnderlyings->previousValue.getValue();
      if (NoUnderlyings->previousValue.isAssigned()) {
        for (int i = 0; i < NoUnderlyings_len; ++i) {
          UnderlyingSecurityID->decode(input);
          UnderlyingSecurityIDSource->decode();
          UnderlyingSecurityExchange->decode();
          // decimal fields UnderlyingPx
          UnderlyingPx_exponent->decode(input, pmap1);
          if (UnderlyingPx_exponent->isMandatory || UnderlyingPx_exponent->isValueSet)
            UnderlyingPx_mantissa->decode(input);
          UnderlyingPxType->decode(input);
        }
      }
      // decode for sequence Underlyings ends
      // will eventually process MdMsg
      if (sec_found_) process();
    }
    // decode for sequence MDEntries ends

    // will eventually process MdMsg
    if (sec_found_) process_end();
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  bool findSecurity();

  void process_end();

  // Reset
  void reset() {
    MsgType->reset();
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
    Currency->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    // decimal fields MDEntryInterestRate
    MDEntryInterestRate_exponent->reset();
    MDEntryInterestRate_mantissa->reset();
    IndexSeq->reset();
    MDEntrySize->reset();
    TradeVolume->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    MDInsertDate->reset();
    MDInsertTime->reset();
    TickDirection->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    MDStreamID->reset();
    PriceType->reset();
    TradingSessionSubID->reset();
    SecurityTradingStatus->reset();
    TradSesOpenTime->reset();
    TradingSessionID->reset();
    SecurityTradingEvent->reset();
    TradeCondition->reset();
    OpenCloseSettlFlag->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    QuoteCondition->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    SellerDays->reset();
    SettPriceType->reset();
    LastTradeDate->reset();
    PriceAdjustmentMethod->reset();
    PriceLimitType->reset();
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent->reset();
    LowLimitPrice_mantissa->reset();
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent->reset();
    HighLimitPrice_mantissa->reset();
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent->reset();
    TradingReferencePrice_mantissa->reset();
    PriceBandMidpointPriceType->reset();
    AvgDailyTradedQty->reset();
    ExpireDate->reset();
    EarlyTermination->reset();
    BTBCertIndicator->reset();
    BTBContractInfo->reset();
    BTBGraceDate->reset();
    MaxTradeVol->reset();
    PriceBandType->reset();
    // reset for sequence Underlyings
    NoUnderlyings->reset();
    UnderlyingSecurityID->reset();
    UnderlyingSecurityIDSource->reset();
    UnderlyingSecurityExchange->reset();
    // decimal fields UnderlyingPx
    UnderlyingPx_exponent->reset();
    UnderlyingPx_mantissa->reset();
    UnderlyingPxType->reset();
    // reset for sequence Underlyings ends
    // reset for sequence MDEntries ends
  }

  // Destructor
  ~MDSnapshotFullRefresh_151() {
    delete MsgType;
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
    delete Currency;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    // decimal fields MDEntryInterestRate
    delete MDEntryInterestRate_exponent;
    delete MDEntryInterestRate_mantissa;
    delete IndexSeq;
    delete MDEntrySize;
    delete TradeVolume;
    delete MDEntryDate;
    delete MDEntryTime;
    delete MDInsertDate;
    delete MDInsertTime;
    delete TickDirection;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete MDStreamID;
    delete PriceType;
    delete TradingSessionSubID;
    delete SecurityTradingStatus;
    delete TradSesOpenTime;
    delete TradingSessionID;
    delete SecurityTradingEvent;
    delete TradeCondition;
    delete OpenCloseSettlFlag;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete QuoteCondition;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete SellerDays;
    delete SettPriceType;
    delete LastTradeDate;
    delete PriceAdjustmentMethod;
    delete PriceLimitType;
    // decimal fields LowLimitPrice
    delete LowLimitPrice_exponent;
    delete LowLimitPrice_mantissa;
    // decimal fields HighLimitPrice
    delete HighLimitPrice_exponent;
    delete HighLimitPrice_mantissa;
    // decimal fields TradingReferencePrice
    delete TradingReferencePrice_exponent;
    delete TradingReferencePrice_mantissa;
    delete PriceBandMidpointPriceType;
    delete AvgDailyTradedQty;
    delete ExpireDate;
    delete EarlyTermination;
    delete BTBCertIndicator;
    delete BTBContractInfo;
    delete BTBGraceDate;
    delete MaxTradeVol;
    delete PriceBandType;
    // destructor for sequence Underlyings
    delete NoUnderlyings;
    delete UnderlyingSecurityID;
    delete UnderlyingSecurityIDSource;
    delete UnderlyingSecurityExchange;
    // decimal fields UnderlyingPx
    delete UnderlyingPx_exponent;
    delete UnderlyingPx_mantissa;
    delete UnderlyingPxType;
    // destructor for sequence Underlyings ends
    // destructor for sequence MDEntries ends
  }
};
class MDSnapshotFullRefresh_147 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
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
  DefaultField<FFUtils::ByteArr> *Currency;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  // decimal fields MDEntryInterestRate
  DefaultField<int32_t> *MDEntryInterestRate_exponent;
  DeltaField<int64_t> *MDEntryInterestRate_mantissa;
  NoOpField<uint32_t> *IndexSeq;
  DeltaField<int64_t> *MDEntrySize;
  DeltaField<uint64_t> *TradeVolume;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<FFUtils::ByteArr> *MDEntryTime;
  CopyField<uint32_t> *MDInsertDate;
  CopyField<uint32_t> *MDInsertTime;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  DefaultField<FFUtils::ByteArr> *PriceType;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  DefaultField<uint32_t> *SecurityTradingStatus;
  DefaultField<uint64_t> *TradSesOpenTime;
  NoOpField<uint32_t> *TradingSessionID;
  NoOpField<uint32_t> *SecurityTradingEvent;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<uint32_t> *OpenCloseSettlFlag;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  DefaultField<FFUtils::ByteArr> *MDEntryBuyer;
  DefaultField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  CopyField<uint32_t> *NumberOfOrders;
  DefaultField<uint32_t> *MDEntryPositionNo;
  DefaultField<uint32_t> *SellerDays;
  DefaultField<uint32_t> *SettPriceType;
  NoOpField<uint32_t> *LastTradeDate;
  NoOpField<uint32_t> *PriceAdjustmentMethod;
  DefaultField<uint32_t> *PriceLimitType;
  // decimal fields LowLimitPrice
  DefaultField<int32_t> *LowLimitPrice_exponent;
  DeltaField<int64_t> *LowLimitPrice_mantissa;
  // decimal fields HighLimitPrice
  DefaultField<int32_t> *HighLimitPrice_exponent;
  DeltaField<int64_t> *HighLimitPrice_mantissa;
  // decimal fields TradingReferencePrice
  DefaultField<int32_t> *TradingReferencePrice_exponent;
  DeltaField<int64_t> *TradingReferencePrice_mantissa;
  NoOpField<uint32_t> *PriceBandMidpointPriceType;
  NoOpField<uint64_t> *AvgDailyTradedQty;
  NoOpField<uint64_t> *ExpireDate;
  NoOpField<uint64_t> *EarlyTermination;
  NoOpField<uint64_t> *MaxTradeVol;
  DefaultField<FFUtils::ByteArr> *PriceBandType;
  // fields inside sequence Underlyings
  NoOpField<uint32_t> *NoUnderlyings;
  DeltaField<uint64_t> *UnderlyingSecurityID;
  ConstantFieldMandatory<uint32_t> *UnderlyingSecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *UnderlyingSecurityExchange;
  // decimal fields UnderlyingPx
  DefaultField<int32_t> *UnderlyingPx_exponent;
  DeltaField<int64_t> *UnderlyingPx_mantissa;
  NoOpField<uint32_t> *UnderlyingPxType;
  // fields inside sequence Underlyings ends
  // fields inside sequence MDEntries ends

  // Constructor
  MDSnapshotFullRefresh_147() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"));
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
    Currency = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields MDEntryInterestRate
    MDEntryInterestRate_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryInterestRate_mantissa = new DeltaField<int64_t>(true, false, 0);
    IndexSeq = new NoOpField<uint32_t>(false, false, 0);
    MDEntrySize = new DeltaField<int64_t>(false, false, 0);
    TradeVolume = new DeltaField<uint64_t>(false, false, 0);
    MDEntryDate = new CopyField<uint32_t>(false, false, 0);
    MDEntryTime = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDInsertDate = new CopyField<uint32_t>(false, false, 0);
    MDInsertTime = new CopyField<uint32_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("E"));
    PriceType = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new DefaultField<uint32_t>(false, false, 0);
    TradSesOpenTime = new DefaultField<uint64_t>(false, false, 0);
    TradingSessionID = new NoOpField<uint32_t>(false, false, 0);
    SecurityTradingEvent = new NoOpField<uint32_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettlFlag = new DefaultField<uint32_t>(false, false, 0);
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new CopyField<uint32_t>(false, false, 0);
    MDEntryPositionNo = new DefaultField<uint32_t>(false, false, 0);
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    SettPriceType = new DefaultField<uint32_t>(false, false, 0);
    LastTradeDate = new NoOpField<uint32_t>(false, false, 0);
    PriceAdjustmentMethod = new NoOpField<uint32_t>(false, false, 0);
    PriceLimitType = new DefaultField<uint32_t>(false, false, 0);
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    LowLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    HighLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent = new DefaultField<int32_t>(false, false, 0);
    TradingReferencePrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    PriceBandMidpointPriceType = new NoOpField<uint32_t>(false, false, 0);
    AvgDailyTradedQty = new NoOpField<uint64_t>(false, false, 0);
    ExpireDate = new NoOpField<uint64_t>(false, false, 0);
    EarlyTermination = new NoOpField<uint64_t>(false, false, 0);
    MaxTradeVol = new NoOpField<uint64_t>(false, false, 0);
    PriceBandType = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t>(false, false, 0);
    UnderlyingSecurityID = new DeltaField<uint64_t>(true, false, 0);
    UnderlyingSecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    UnderlyingSecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // decimal fields UnderlyingPx
    UnderlyingPx_exponent = new DefaultField<int32_t>(true, true, -2);
    UnderlyingPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    UnderlyingPxType = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence Underlyings ends
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
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
    for (int i = 0; i < NoMDEntries_len; ++i) {
      // extract pmap
      FFUtils::PMap pmap1 = input.extractPmap();

      MDEntryType->decode(input, pmap1);
      Currency->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      // decimal fields MDEntryInterestRate
      MDEntryInterestRate_exponent->decode(input, pmap1);
      if (MDEntryInterestRate_exponent->isMandatory || MDEntryInterestRate_exponent->isValueSet)
        MDEntryInterestRate_mantissa->decode(input);
      IndexSeq->decode(input);
      MDEntrySize->decode(input);
      TradeVolume->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      MDInsertDate->decode(input, pmap1);
      MDInsertTime->decode(input, pmap1);
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
      TradingSessionID->decode(input);
      SecurityTradingEvent->decode(input);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettlFlag->decode(input, pmap1);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      SellerDays->decode(input, pmap1);
      SettPriceType->decode(input, pmap1);
      LastTradeDate->decode(input);
      PriceAdjustmentMethod->decode(input);
      PriceLimitType->decode(input, pmap1);
      // decimal fields LowLimitPrice
      LowLimitPrice_exponent->decode(input, pmap1);
      if (LowLimitPrice_exponent->isMandatory || LowLimitPrice_exponent->isValueSet)
        LowLimitPrice_mantissa->decode(input);
      // decimal fields HighLimitPrice
      HighLimitPrice_exponent->decode(input, pmap1);
      if (HighLimitPrice_exponent->isMandatory || HighLimitPrice_exponent->isValueSet)
        HighLimitPrice_mantissa->decode(input);
      // decimal fields TradingReferencePrice
      TradingReferencePrice_exponent->decode(input, pmap1);
      if (TradingReferencePrice_exponent->isMandatory || TradingReferencePrice_exponent->isValueSet)
        TradingReferencePrice_mantissa->decode(input);
      PriceBandMidpointPriceType->decode(input);
      AvgDailyTradedQty->decode(input);
      ExpireDate->decode(input);
      EarlyTermination->decode(input);
      MaxTradeVol->decode(input);
      PriceBandType->decode(input, pmap1);
      // decode for sequence Underlyings
      NoUnderlyings->decode(input);

      int NoUnderlyings_len = NoUnderlyings->previousValue.getValue();
      if (NoUnderlyings->previousValue.isAssigned()) {
        for (int i = 0; i < NoUnderlyings_len; ++i) {
          UnderlyingSecurityID->decode(input);
          UnderlyingSecurityIDSource->decode();
          UnderlyingSecurityExchange->decode();
          // decimal fields UnderlyingPx
          UnderlyingPx_exponent->decode(input, pmap1);
          if (UnderlyingPx_exponent->isMandatory || UnderlyingPx_exponent->isValueSet)
            UnderlyingPx_mantissa->decode(input);
          UnderlyingPxType->decode(input);
        }
      }
      // decode for sequence Underlyings ends

      // will eventually process MdMsg
      if (sec_found_) process();
    }
    // decode for sequence MDEntries ends

    // signifies end of sequences should process flush based stuff
    if (sec_found_) process_end();
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  bool findSecurity();

  void process_end();
  void process_start();
  void process_order_feed();

  // Reset
  void reset() {
    MsgType->reset();
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
    Currency->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    // decimal fields MDEntryInterestRate
    MDEntryInterestRate_exponent->reset();
    MDEntryInterestRate_mantissa->reset();
    IndexSeq->reset();
    MDEntrySize->reset();
    TradeVolume->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    MDInsertDate->reset();
    MDInsertTime->reset();
    TickDirection->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    MDStreamID->reset();
    PriceType->reset();
    TradingSessionSubID->reset();
    SecurityTradingStatus->reset();
    TradSesOpenTime->reset();
    TradingSessionID->reset();
    SecurityTradingEvent->reset();
    TradeCondition->reset();
    OpenCloseSettlFlag->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    QuoteCondition->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    SellerDays->reset();
    SettPriceType->reset();
    LastTradeDate->reset();
    PriceAdjustmentMethod->reset();
    PriceLimitType->reset();
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent->reset();
    LowLimitPrice_mantissa->reset();
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent->reset();
    HighLimitPrice_mantissa->reset();
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent->reset();
    TradingReferencePrice_mantissa->reset();
    PriceBandMidpointPriceType->reset();
    AvgDailyTradedQty->reset();
    ExpireDate->reset();
    EarlyTermination->reset();
    MaxTradeVol->reset();
    PriceBandType->reset();
    // reset for sequence Underlyings
    NoUnderlyings->reset();
    UnderlyingSecurityID->reset();
    UnderlyingSecurityIDSource->reset();
    UnderlyingSecurityExchange->reset();
    // decimal fields UnderlyingPx
    UnderlyingPx_exponent->reset();
    UnderlyingPx_mantissa->reset();
    UnderlyingPxType->reset();
    // reset for sequence Underlyings ends
    // reset for sequence MDEntries ends
  }

  // Destructor
  ~MDSnapshotFullRefresh_147() {
    delete MsgType;
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
    delete Currency;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    // decimal fields MDEntryInterestRate
    delete MDEntryInterestRate_exponent;
    delete MDEntryInterestRate_mantissa;
    delete IndexSeq;
    delete MDEntrySize;
    delete TradeVolume;
    delete MDEntryDate;
    delete MDEntryTime;
    delete MDInsertDate;
    delete MDInsertTime;
    delete TickDirection;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete MDStreamID;
    delete PriceType;
    delete TradingSessionSubID;
    delete SecurityTradingStatus;
    delete TradSesOpenTime;
    delete TradingSessionID;
    delete SecurityTradingEvent;
    delete TradeCondition;
    delete OpenCloseSettlFlag;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete QuoteCondition;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete SellerDays;
    delete SettPriceType;
    delete LastTradeDate;
    delete PriceAdjustmentMethod;
    delete PriceLimitType;
    // decimal fields LowLimitPrice
    delete LowLimitPrice_exponent;
    delete LowLimitPrice_mantissa;
    // decimal fields HighLimitPrice
    delete HighLimitPrice_exponent;
    delete HighLimitPrice_mantissa;
    // decimal fields TradingReferencePrice
    delete TradingReferencePrice_exponent;
    delete TradingReferencePrice_mantissa;
    delete PriceBandMidpointPriceType;
    delete AvgDailyTradedQty;
    delete ExpireDate;
    delete EarlyTermination;
    delete MaxTradeVol;
    delete PriceBandType;
    // destructor for sequence Underlyings
    delete NoUnderlyings;
    delete UnderlyingSecurityID;
    delete UnderlyingSecurityIDSource;
    delete UnderlyingSecurityExchange;
    // decimal fields UnderlyingPx
    delete UnderlyingPx_exponent;
    delete UnderlyingPx_mantissa;
    delete UnderlyingPxType;
    // destructor for sequence Underlyings ends
    // destructor for sequence MDEntries ends
  }
};
class MDSnapshotFullRefresh_146 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
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
  DefaultField<FFUtils::ByteArr> *Currency;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  // decimal fields MDEntryInterestRate
  DefaultField<int32_t> *MDEntryInterestRate_exponent;
  DeltaField<int64_t> *MDEntryInterestRate_mantissa;
  NoOpField<uint32_t> *IndexSeq;
  DeltaField<int32_t> *MDEntrySize;
  DeltaField<uint64_t> *TradeVolume;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<FFUtils::ByteArr> *MDEntryTime;
  CopyField<uint32_t> *MDInsertDate;
  CopyField<uint32_t> *MDInsertTime;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  DefaultField<FFUtils::ByteArr> *PriceType;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  DefaultField<uint32_t> *SecurityTradingStatus;
  DefaultField<uint64_t> *TradSesOpenTime;
  NoOpField<uint32_t> *TradingSessionID;
  NoOpField<uint32_t> *SecurityTradingEvent;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<uint32_t> *OpenCloseSettlFlag;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  DefaultField<FFUtils::ByteArr> *MDEntryBuyer;
  DefaultField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  CopyField<uint32_t> *NumberOfOrders;
  DefaultField<uint32_t> *MDEntryPositionNo;
  DefaultField<uint32_t> *SellerDays;
  DefaultField<uint32_t> *SettPriceType;
  NoOpField<uint32_t> *LastTradeDate;
  NoOpField<uint32_t> *PriceAdjustmentMethod;
  DefaultField<uint32_t> *PriceLimitType;
  // decimal fields LowLimitPrice
  DefaultField<int32_t> *LowLimitPrice_exponent;
  DeltaField<int64_t> *LowLimitPrice_mantissa;
  // decimal fields HighLimitPrice
  DefaultField<int32_t> *HighLimitPrice_exponent;
  DeltaField<int64_t> *HighLimitPrice_mantissa;
  // decimal fields TradingReferencePrice
  DefaultField<int32_t> *TradingReferencePrice_exponent;
  DeltaField<int64_t> *TradingReferencePrice_mantissa;
  NoOpField<uint32_t> *PriceBandMidpointPriceType;
  NoOpField<uint64_t> *AvgDailyTradedQty;
  NoOpField<uint64_t> *ExpireDate;
  NoOpField<uint64_t> *EarlyTermination;
  NoOpField<uint64_t> *MaxTradeVol;
  DefaultField<FFUtils::ByteArr> *PriceBandType;
  // fields inside sequence Underlyings
  NoOpField<uint32_t> *NoUnderlyings;
  DeltaField<uint64_t> *UnderlyingSecurityID;
  ConstantFieldMandatory<uint32_t> *UnderlyingSecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *UnderlyingSecurityExchange;
  // decimal fields UnderlyingPx
  DefaultField<int32_t> *UnderlyingPx_exponent;
  DeltaField<int64_t> *UnderlyingPx_mantissa;
  NoOpField<uint32_t> *UnderlyingPxType;
  // fields inside sequence Underlyings ends
  // fields inside sequence MDEntries ends

  // Constructor
  MDSnapshotFullRefresh_146() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"));
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
    Currency = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields MDEntryInterestRate
    MDEntryInterestRate_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryInterestRate_mantissa = new DeltaField<int64_t>(true, false, 0);
    IndexSeq = new NoOpField<uint32_t>(false, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    TradeVolume = new DeltaField<uint64_t>(false, false, 0);
    MDEntryDate = new CopyField<uint32_t>(false, false, 0);
    MDEntryTime = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDInsertDate = new CopyField<uint32_t>(false, false, 0);
    MDInsertTime = new CopyField<uint32_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("E"));
    PriceType = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new DefaultField<uint32_t>(false, false, 0);
    TradSesOpenTime = new DefaultField<uint64_t>(false, false, 0);
    TradingSessionID = new NoOpField<uint32_t>(false, false, 0);
    SecurityTradingEvent = new NoOpField<uint32_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettlFlag = new DefaultField<uint32_t>(false, false, 0);
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new CopyField<uint32_t>(false, false, 0);
    MDEntryPositionNo = new DefaultField<uint32_t>(false, false, 0);
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    SettPriceType = new DefaultField<uint32_t>(false, false, 0);
    LastTradeDate = new NoOpField<uint32_t>(false, false, 0);
    PriceAdjustmentMethod = new NoOpField<uint32_t>(false, false, 0);
    PriceLimitType = new DefaultField<uint32_t>(false, false, 0);
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    LowLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    HighLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent = new DefaultField<int32_t>(false, false, 0);
    TradingReferencePrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    PriceBandMidpointPriceType = new NoOpField<uint32_t>(false, false, 0);
    AvgDailyTradedQty = new NoOpField<uint64_t>(false, false, 0);
    ExpireDate = new NoOpField<uint64_t>(false, false, 0);
    EarlyTermination = new NoOpField<uint64_t>(false, false, 0);
    MaxTradeVol = new NoOpField<uint64_t>(false, false, 0);
    PriceBandType = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t>(false, false, 0);
    UnderlyingSecurityID = new DeltaField<uint64_t>(true, false, 0);
    UnderlyingSecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    UnderlyingSecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // decimal fields UnderlyingPx
    UnderlyingPx_exponent = new DefaultField<int32_t>(true, true, -2);
    UnderlyingPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    UnderlyingPxType = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence Underlyings ends
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
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
      Currency->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      // decimal fields MDEntryInterestRate
      MDEntryInterestRate_exponent->decode(input, pmap1);
      if (MDEntryInterestRate_exponent->isMandatory || MDEntryInterestRate_exponent->isValueSet)
        MDEntryInterestRate_mantissa->decode(input);
      IndexSeq->decode(input);
      MDEntrySize->decode(input);
      TradeVolume->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      MDInsertDate->decode(input, pmap1);
      MDInsertTime->decode(input, pmap1);
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
      TradingSessionID->decode(input);
      SecurityTradingEvent->decode(input);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettlFlag->decode(input, pmap1);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      SellerDays->decode(input, pmap1);
      SettPriceType->decode(input, pmap1);
      LastTradeDate->decode(input);
      PriceAdjustmentMethod->decode(input);
      PriceLimitType->decode(input, pmap1);
      // decimal fields LowLimitPrice
      LowLimitPrice_exponent->decode(input, pmap1);
      if (LowLimitPrice_exponent->isMandatory || LowLimitPrice_exponent->isValueSet)
        LowLimitPrice_mantissa->decode(input);
      // decimal fields HighLimitPrice
      HighLimitPrice_exponent->decode(input, pmap1);
      if (HighLimitPrice_exponent->isMandatory || HighLimitPrice_exponent->isValueSet)
        HighLimitPrice_mantissa->decode(input);
      // decimal fields TradingReferencePrice
      TradingReferencePrice_exponent->decode(input, pmap1);
      if (TradingReferencePrice_exponent->isMandatory || TradingReferencePrice_exponent->isValueSet)
        TradingReferencePrice_mantissa->decode(input);
      PriceBandMidpointPriceType->decode(input);
      AvgDailyTradedQty->decode(input);
      ExpireDate->decode(input);
      EarlyTermination->decode(input);
      MaxTradeVol->decode(input);
      PriceBandType->decode(input, pmap1);
      // decode for sequence Underlyings
      NoUnderlyings->decode(input);

      int NoUnderlyings_len = NoUnderlyings->previousValue.getValue();
      if (NoUnderlyings->previousValue.isAssigned()) {
        for (int i = 0; i < NoUnderlyings_len; ++i) {
          UnderlyingSecurityID->decode(input);
          UnderlyingSecurityIDSource->decode();
          UnderlyingSecurityExchange->decode();
          // decimal fields UnderlyingPx
          UnderlyingPx_exponent->decode(input, pmap1);
          if (UnderlyingPx_exponent->isMandatory || UnderlyingPx_exponent->isValueSet)
            UnderlyingPx_mantissa->decode(input);
          UnderlyingPxType->decode(input);
        }
      }
      // decode for sequence Underlyings ends
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
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
    Currency->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    // decimal fields MDEntryInterestRate
    MDEntryInterestRate_exponent->reset();
    MDEntryInterestRate_mantissa->reset();
    IndexSeq->reset();
    MDEntrySize->reset();
    TradeVolume->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    MDInsertDate->reset();
    MDInsertTime->reset();
    TickDirection->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    MDStreamID->reset();
    PriceType->reset();
    TradingSessionSubID->reset();
    SecurityTradingStatus->reset();
    TradSesOpenTime->reset();
    TradingSessionID->reset();
    SecurityTradingEvent->reset();
    TradeCondition->reset();
    OpenCloseSettlFlag->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    QuoteCondition->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    SellerDays->reset();
    SettPriceType->reset();
    LastTradeDate->reset();
    PriceAdjustmentMethod->reset();
    PriceLimitType->reset();
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent->reset();
    LowLimitPrice_mantissa->reset();
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent->reset();
    HighLimitPrice_mantissa->reset();
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent->reset();
    TradingReferencePrice_mantissa->reset();
    PriceBandMidpointPriceType->reset();
    AvgDailyTradedQty->reset();
    ExpireDate->reset();
    EarlyTermination->reset();
    MaxTradeVol->reset();
    PriceBandType->reset();
    // reset for sequence Underlyings
    NoUnderlyings->reset();
    UnderlyingSecurityID->reset();
    UnderlyingSecurityIDSource->reset();
    UnderlyingSecurityExchange->reset();
    // decimal fields UnderlyingPx
    UnderlyingPx_exponent->reset();
    UnderlyingPx_mantissa->reset();
    UnderlyingPxType->reset();
    // reset for sequence Underlyings ends
    // reset for sequence MDEntries ends
  }

  // Destructor
  ~MDSnapshotFullRefresh_146() {
    delete MsgType;
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
    delete Currency;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    // decimal fields MDEntryInterestRate
    delete MDEntryInterestRate_exponent;
    delete MDEntryInterestRate_mantissa;
    delete IndexSeq;
    delete MDEntrySize;
    delete TradeVolume;
    delete MDEntryDate;
    delete MDEntryTime;
    delete MDInsertDate;
    delete MDInsertTime;
    delete TickDirection;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete MDStreamID;
    delete PriceType;
    delete TradingSessionSubID;
    delete SecurityTradingStatus;
    delete TradSesOpenTime;
    delete TradingSessionID;
    delete SecurityTradingEvent;
    delete TradeCondition;
    delete OpenCloseSettlFlag;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete QuoteCondition;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete SellerDays;
    delete SettPriceType;
    delete LastTradeDate;
    delete PriceAdjustmentMethod;
    delete PriceLimitType;
    // decimal fields LowLimitPrice
    delete LowLimitPrice_exponent;
    delete LowLimitPrice_mantissa;
    // decimal fields HighLimitPrice
    delete HighLimitPrice_exponent;
    delete HighLimitPrice_mantissa;
    // decimal fields TradingReferencePrice
    delete TradingReferencePrice_exponent;
    delete TradingReferencePrice_mantissa;
    delete PriceBandMidpointPriceType;
    delete AvgDailyTradedQty;
    delete ExpireDate;
    delete EarlyTermination;
    delete MaxTradeVol;
    delete PriceBandType;
    // destructor for sequence Underlyings
    delete NoUnderlyings;
    delete UnderlyingSecurityID;
    delete UnderlyingSecurityIDSource;
    delete UnderlyingSecurityExchange;
    // decimal fields UnderlyingPx
    delete UnderlyingPx_exponent;
    delete UnderlyingPx_mantissa;
    delete UnderlyingPxType;
    // destructor for sequence Underlyings ends
    // destructor for sequence MDEntries ends
  }
};
class MDSnapshotFullRefresh_139 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
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
  DefaultField<FFUtils::ByteArr> *Currency;
  // decimal fields MDEntryPx
  DefaultField<int32_t> *MDEntryPx_exponent;
  DeltaField<int64_t> *MDEntryPx_mantissa;
  // decimal fields MDEntryInterestRate
  DefaultField<int32_t> *MDEntryInterestRate_exponent;
  DeltaField<int64_t> *MDEntryInterestRate_mantissa;
  NoOpField<uint32_t> *IndexSeq;
  DeltaField<int32_t> *MDEntrySize;
  DeltaField<uint64_t> *TradeVolume;
  CopyField<uint32_t> *MDEntryDate;
  CopyField<FFUtils::ByteArr> *MDEntryTime;
  CopyField<uint32_t> *MDInsertDate;
  CopyField<uint32_t> *MDInsertTime;
  DefaultField<FFUtils::ByteArr> *TickDirection;
  // decimal fields NetChgPrevDay
  DefaultField<int32_t> *NetChgPrevDay_exponent;
  DeltaField<int64_t> *NetChgPrevDay_mantissa;
  DefaultField<FFUtils::ByteArr> *MDStreamID;
  DefaultField<FFUtils::ByteArr> *PriceType;
  NoOpField<FFUtils::ByteArr> *TradingSessionSubID;
  DefaultField<uint32_t> *SecurityTradingStatus;
  DefaultField<uint64_t> *TradSesOpenTime;
  NoOpField<uint32_t> *TradingSessionID;
  NoOpField<uint32_t> *SecurityTradingEvent;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  DefaultField<FFUtils::ByteArr> *OpenCloseSettlFlag;
  DefaultField<FFUtils::ByteArr> *OrderID;
  DefaultField<FFUtils::ByteArr> *TradeID;
  DefaultField<FFUtils::ByteArr> *MDEntryBuyer;
  DefaultField<FFUtils::ByteArr> *MDEntrySeller;
  DefaultField<FFUtils::ByteArr> *QuoteCondition;
  CopyField<uint32_t> *NumberOfOrders;
  DefaultField<uint32_t> *MDEntryPositionNo;
  DefaultField<uint32_t> *SellerDays;
  DefaultField<uint32_t> *SettPriceType;
  NoOpField<uint32_t> *LastTradeDate;
  NoOpField<uint32_t> *PriceAdjustmentMethod;
  DefaultField<uint32_t> *PriceLimitType;
  // decimal fields LowLimitPrice
  DefaultField<int32_t> *LowLimitPrice_exponent;
  DeltaField<int64_t> *LowLimitPrice_mantissa;
  // decimal fields HighLimitPrice
  DefaultField<int32_t> *HighLimitPrice_exponent;
  DeltaField<int64_t> *HighLimitPrice_mantissa;
  // decimal fields TradingReferencePrice
  DefaultField<int32_t> *TradingReferencePrice_exponent;
  DeltaField<int64_t> *TradingReferencePrice_mantissa;
  NoOpField<uint32_t> *PriceBandMidpointPriceType;
  NoOpField<uint64_t> *AvgDailyTradedQty;
  NoOpField<uint64_t> *ExpireDate;
  NoOpField<uint64_t> *EarlyTermination;
  NoOpField<uint64_t> *MaxTradeVol;
  DefaultField<FFUtils::ByteArr> *PriceBandType;
  // fields inside sequence Underlyings
  NoOpField<uint32_t> *NoUnderlyings;
  DeltaField<uint64_t> *UnderlyingSecurityID;
  ConstantFieldMandatory<uint32_t> *UnderlyingSecurityIDSource;
  ConstantFieldMandatory<FFUtils::ByteArr> *UnderlyingSecurityExchange;
  // decimal fields UnderlyingPx
  DefaultField<int32_t> *UnderlyingPx_exponent;
  DeltaField<int64_t> *UnderlyingPx_mantissa;
  NoOpField<uint32_t> *UnderlyingPxType;
  // fields inside sequence Underlyings ends
  // fields inside sequence MDEntries ends

  // Constructor
  MDSnapshotFullRefresh_139() {
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("W"));
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
    Currency = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields MDEntryPx
    MDEntryPx_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields MDEntryInterestRate
    MDEntryInterestRate_exponent = new DefaultField<int32_t>(false, true, -2);
    MDEntryInterestRate_mantissa = new DeltaField<int64_t>(true, false, 0);
    IndexSeq = new NoOpField<uint32_t>(false, false, 0);
    MDEntrySize = new DeltaField<int32_t>(false, false, 0);
    TradeVolume = new DeltaField<uint64_t>(false, false, 0);
    MDEntryDate = new CopyField<uint32_t>(false, false, 0);
    MDEntryTime = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDInsertDate = new CopyField<uint32_t>(false, false, 0);
    MDInsertTime = new CopyField<uint32_t>(false, false, 0);
    TickDirection = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent = new DefaultField<int32_t>(false, false, 0);
    NetChgPrevDay_mantissa = new DeltaField<int64_t>(true, false, 0);
    MDStreamID = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("E"));
    PriceType = new DefaultField<FFUtils::ByteArr>(false, true, FFUtils::ByteArr("2"));
    TradingSessionSubID = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    SecurityTradingStatus = new DefaultField<uint32_t>(false, false, 0);
    TradSesOpenTime = new DefaultField<uint64_t>(false, false, 0);
    TradingSessionID = new NoOpField<uint32_t>(false, false, 0);
    SecurityTradingEvent = new NoOpField<uint32_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettlFlag = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OrderID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    TradeID = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntryBuyer = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    MDEntrySeller = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    QuoteCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NumberOfOrders = new CopyField<uint32_t>(false, false, 0);
    MDEntryPositionNo = new DefaultField<uint32_t>(false, false, 0);
    SellerDays = new DefaultField<uint32_t>(false, false, 0);
    SettPriceType = new DefaultField<uint32_t>(false, false, 0);
    LastTradeDate = new NoOpField<uint32_t>(false, false, 0);
    PriceAdjustmentMethod = new NoOpField<uint32_t>(false, false, 0);
    PriceLimitType = new DefaultField<uint32_t>(false, false, 0);
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    LowLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent = new DefaultField<int32_t>(false, false, 0);
    HighLimitPrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent = new DefaultField<int32_t>(false, false, 0);
    TradingReferencePrice_mantissa = new DeltaField<int64_t>(true, false, 0);
    PriceBandMidpointPriceType = new NoOpField<uint32_t>(false, false, 0);
    AvgDailyTradedQty = new NoOpField<uint64_t>(false, false, 0);
    ExpireDate = new NoOpField<uint64_t>(false, false, 0);
    EarlyTermination = new NoOpField<uint64_t>(false, false, 0);
    MaxTradeVol = new NoOpField<uint64_t>(false, false, 0);
    PriceBandType = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence Underlyings
    NoUnderlyings = new NoOpField<uint32_t>(false, false, 0);
    UnderlyingSecurityID = new DeltaField<uint64_t>(true, false, 0);
    UnderlyingSecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    UnderlyingSecurityExchange = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("BVMF"));
    // decimal fields UnderlyingPx
    UnderlyingPx_exponent = new DefaultField<int32_t>(true, true, -2);
    UnderlyingPx_mantissa = new DeltaField<int64_t>(true, false, 0);
    UnderlyingPxType = new NoOpField<uint32_t>(false, false, 0);
    // constructor for sequence Underlyings ends
    // constructor for sequence MDEntries ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    MsgType->decode();
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
      Currency->decode(input, pmap1);
      // decimal fields MDEntryPx
      MDEntryPx_exponent->decode(input, pmap1);
      if (MDEntryPx_exponent->isMandatory || MDEntryPx_exponent->isValueSet) MDEntryPx_mantissa->decode(input);
      // decimal fields MDEntryInterestRate
      MDEntryInterestRate_exponent->decode(input, pmap1);
      if (MDEntryInterestRate_exponent->isMandatory || MDEntryInterestRate_exponent->isValueSet)
        MDEntryInterestRate_mantissa->decode(input);
      IndexSeq->decode(input);
      MDEntrySize->decode(input);
      TradeVolume->decode(input);
      MDEntryDate->decode(input, pmap1);
      MDEntryTime->decode(input, pmap1);
      MDInsertDate->decode(input, pmap1);
      MDInsertTime->decode(input, pmap1);
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
      TradingSessionID->decode(input);
      SecurityTradingEvent->decode(input);
      TradeCondition->decode(input, pmap1);
      OpenCloseSettlFlag->decode(input, pmap1);
      OrderID->decode(input, pmap1);
      TradeID->decode(input, pmap1);
      MDEntryBuyer->decode(input, pmap1);
      MDEntrySeller->decode(input, pmap1);
      QuoteCondition->decode(input, pmap1);
      NumberOfOrders->decode(input, pmap1);
      MDEntryPositionNo->decode(input, pmap1);
      SellerDays->decode(input, pmap1);
      SettPriceType->decode(input, pmap1);
      LastTradeDate->decode(input);
      PriceAdjustmentMethod->decode(input);
      PriceLimitType->decode(input, pmap1);
      // decimal fields LowLimitPrice
      LowLimitPrice_exponent->decode(input, pmap1);
      if (LowLimitPrice_exponent->isMandatory || LowLimitPrice_exponent->isValueSet)
        LowLimitPrice_mantissa->decode(input);
      // decimal fields HighLimitPrice
      HighLimitPrice_exponent->decode(input, pmap1);
      if (HighLimitPrice_exponent->isMandatory || HighLimitPrice_exponent->isValueSet)
        HighLimitPrice_mantissa->decode(input);
      // decimal fields TradingReferencePrice
      TradingReferencePrice_exponent->decode(input, pmap1);
      if (TradingReferencePrice_exponent->isMandatory || TradingReferencePrice_exponent->isValueSet)
        TradingReferencePrice_mantissa->decode(input);
      PriceBandMidpointPriceType->decode(input);
      AvgDailyTradedQty->decode(input);
      ExpireDate->decode(input);
      EarlyTermination->decode(input);
      MaxTradeVol->decode(input);
      PriceBandType->decode(input, pmap1);
      // decode for sequence Underlyings
      NoUnderlyings->decode(input);

      int NoUnderlyings_len = NoUnderlyings->previousValue.getValue();
      if (NoUnderlyings->previousValue.isAssigned()) {
        for (int i = 0; i < NoUnderlyings_len; ++i) {
          UnderlyingSecurityID->decode(input);
          UnderlyingSecurityIDSource->decode();
          UnderlyingSecurityExchange->decode();
          // decimal fields UnderlyingPx
          UnderlyingPx_exponent->decode(input, pmap1);
          if (UnderlyingPx_exponent->isMandatory || UnderlyingPx_exponent->isValueSet)
            UnderlyingPx_mantissa->decode(input);
          UnderlyingPxType->decode(input);
        }
      }
      // decode for sequence Underlyings ends
    }
    // decode for sequence MDEntries ends
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    MsgType->reset();
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
    Currency->reset();
    // decimal fields MDEntryPx
    MDEntryPx_exponent->reset();
    MDEntryPx_mantissa->reset();
    // decimal fields MDEntryInterestRate
    MDEntryInterestRate_exponent->reset();
    MDEntryInterestRate_mantissa->reset();
    IndexSeq->reset();
    MDEntrySize->reset();
    TradeVolume->reset();
    MDEntryDate->reset();
    MDEntryTime->reset();
    MDInsertDate->reset();
    MDInsertTime->reset();
    TickDirection->reset();
    // decimal fields NetChgPrevDay
    NetChgPrevDay_exponent->reset();
    NetChgPrevDay_mantissa->reset();
    MDStreamID->reset();
    PriceType->reset();
    TradingSessionSubID->reset();
    SecurityTradingStatus->reset();
    TradSesOpenTime->reset();
    TradingSessionID->reset();
    SecurityTradingEvent->reset();
    TradeCondition->reset();
    OpenCloseSettlFlag->reset();
    OrderID->reset();
    TradeID->reset();
    MDEntryBuyer->reset();
    MDEntrySeller->reset();
    QuoteCondition->reset();
    NumberOfOrders->reset();
    MDEntryPositionNo->reset();
    SellerDays->reset();
    SettPriceType->reset();
    LastTradeDate->reset();
    PriceAdjustmentMethod->reset();
    PriceLimitType->reset();
    // decimal fields LowLimitPrice
    LowLimitPrice_exponent->reset();
    LowLimitPrice_mantissa->reset();
    // decimal fields HighLimitPrice
    HighLimitPrice_exponent->reset();
    HighLimitPrice_mantissa->reset();
    // decimal fields TradingReferencePrice
    TradingReferencePrice_exponent->reset();
    TradingReferencePrice_mantissa->reset();
    PriceBandMidpointPriceType->reset();
    AvgDailyTradedQty->reset();
    ExpireDate->reset();
    EarlyTermination->reset();
    MaxTradeVol->reset();
    PriceBandType->reset();
    // reset for sequence Underlyings
    NoUnderlyings->reset();
    UnderlyingSecurityID->reset();
    UnderlyingSecurityIDSource->reset();
    UnderlyingSecurityExchange->reset();
    // decimal fields UnderlyingPx
    UnderlyingPx_exponent->reset();
    UnderlyingPx_mantissa->reset();
    UnderlyingPxType->reset();
    // reset for sequence Underlyings ends
    // reset for sequence MDEntries ends
  }

  // Destructor
  ~MDSnapshotFullRefresh_139() {
    delete MsgType;
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
    delete Currency;
    // decimal fields MDEntryPx
    delete MDEntryPx_exponent;
    delete MDEntryPx_mantissa;
    // decimal fields MDEntryInterestRate
    delete MDEntryInterestRate_exponent;
    delete MDEntryInterestRate_mantissa;
    delete IndexSeq;
    delete MDEntrySize;
    delete TradeVolume;
    delete MDEntryDate;
    delete MDEntryTime;
    delete MDInsertDate;
    delete MDInsertTime;
    delete TickDirection;
    // decimal fields NetChgPrevDay
    delete NetChgPrevDay_exponent;
    delete NetChgPrevDay_mantissa;
    delete MDStreamID;
    delete PriceType;
    delete TradingSessionSubID;
    delete SecurityTradingStatus;
    delete TradSesOpenTime;
    delete TradingSessionID;
    delete SecurityTradingEvent;
    delete TradeCondition;
    delete OpenCloseSettlFlag;
    delete OrderID;
    delete TradeID;
    delete MDEntryBuyer;
    delete MDEntrySeller;
    delete QuoteCondition;
    delete NumberOfOrders;
    delete MDEntryPositionNo;
    delete SellerDays;
    delete SettPriceType;
    delete LastTradeDate;
    delete PriceAdjustmentMethod;
    delete PriceLimitType;
    // decimal fields LowLimitPrice
    delete LowLimitPrice_exponent;
    delete LowLimitPrice_mantissa;
    // decimal fields HighLimitPrice
    delete HighLimitPrice_exponent;
    delete HighLimitPrice_mantissa;
    // decimal fields TradingReferencePrice
    delete TradingReferencePrice_exponent;
    delete TradingReferencePrice_mantissa;
    delete PriceBandMidpointPriceType;
    delete AvgDailyTradedQty;
    delete ExpireDate;
    delete EarlyTermination;
    delete MaxTradeVol;
    delete PriceBandType;
    // destructor for sequence Underlyings
    delete NoUnderlyings;
    delete UnderlyingSecurityID;
    delete UnderlyingSecurityIDSource;
    delete UnderlyingSecurityExchange;
    // decimal fields UnderlyingPx
    delete UnderlyingPx_exponent;
    delete UnderlyingPx_mantissa;
    delete UnderlyingPxType;
    // destructor for sequence Underlyings ends
    // destructor for sequence MDEntries ends
  }
};
class MDSnapshotFullRefresh_128 : public FastDecoder {
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
  DeltaField<uint64_t> *TradeVolume;
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
  DefaultField<uint64_t> *TradSesOpenTime;
  DefaultField<FFUtils::ByteArr> *TradeCondition;
  NoOpField<uint32_t> *OpenCloseSettleFlag;
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
  MDSnapshotFullRefresh_128() {
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
    TradeVolume = new DeltaField<uint64_t>(false, false, 0);
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
    TradSesOpenTime = new DefaultField<uint64_t>(false, false, 0);
    TradeCondition = new DefaultField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    OpenCloseSettleFlag = new NoOpField<uint32_t>(false, false, 0);
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
      OpenCloseSettleFlag->decode(input);
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
  ~MDSnapshotFullRefresh_128() {
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
class MDNewsMessage_143 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  CopyField<uint64_t> *OrigTime;
  CopyField<FFUtils::ByteArr> *NewsSource;
  CopyField<FFUtils::ByteArr> *NewsID;
  CopyField<FFUtils::ByteArr> *LanguageCode;
  CopyField<FFUtils::ByteArr> *Headline;
  CopyField<FFUtils::ByteArr> *URLLink;
  NoOpField<uint32_t> *EncodedHeadlineLen;
  NoOpField<FFUtils::ByteVec> *EncodedHeadline;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t> *NoRelatedSym;
  CopyField<uint64_t> *SecurityID;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  NoOpField<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence RelatedSym ends
  // fields inside sequence LinesOfText
  NoOpField<uint32_t> *NoLinesOfText;
  CopyField<FFUtils::ByteArr> *Text;
  NoOpField<uint32_t> *EncodedTextLen;
  NoOpField<FFUtils::ByteVec> *EncodedText;
  // fields inside sequence LinesOfText ends

  // Constructor
  MDNewsMessage_143() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("B"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    OrigTime = new CopyField<uint64_t>(false, false, 0);
    NewsSource = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NewsID = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LanguageCode = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Headline = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    URLLink = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    EncodedHeadlineLen = new NoOpField<uint32_t>(false, false, 0);
    EncodedHeadline = new NoOpField<FFUtils::ByteVec>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t>(false, false, 0);
    SecurityID = new CopyField<uint64_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityExchange = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence RelatedSym ends
    // constructor for sequence LinesOfText
    NoLinesOfText = new NoOpField<uint32_t>(false, false, 0);
    Text = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    EncodedTextLen = new NoOpField<uint32_t>(false, false, 0);
    EncodedText = new NoOpField<FFUtils::ByteVec>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence LinesOfText ends
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
    OrigTime->decode(input, pmap0);
    NewsSource->decode(input, pmap0);
    NewsID->decode(input, pmap0);
    LanguageCode->decode(input, pmap0);
    Headline->decode(input, pmap0);
    URLLink->decode(input, pmap0);
    EncodedHeadlineLen->decode(input);
    EncodedHeadline->decode(input);
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
    // decode for sequence LinesOfText
    NoLinesOfText->decode(input);

    int NoLinesOfText_len = NoLinesOfText->previousValue.getValue();
    if (NoLinesOfText->previousValue.isAssigned()) {
      for (int i = 0; i < NoLinesOfText_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap1 = input.extractPmap();

        Text->decode(input, pmap1);
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
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
    OrigTime->reset();
    NewsSource->reset();
    NewsID->reset();
    LanguageCode->reset();
    Headline->reset();
    URLLink->reset();
    EncodedHeadlineLen->reset();
    EncodedHeadline->reset();
    // reset for sequence RelatedSym
    NoRelatedSym->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence RelatedSym ends
    // reset for sequence LinesOfText
    NoLinesOfText->reset();
    Text->reset();
    EncodedTextLen->reset();
    EncodedText->reset();
    // reset for sequence LinesOfText ends
  }

  // Destructor
  ~MDNewsMessage_143() {
    delete ApplVerID;
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
    delete OrigTime;
    delete NewsSource;
    delete NewsID;
    delete LanguageCode;
    delete Headline;
    delete URLLink;
    delete EncodedHeadlineLen;
    delete EncodedHeadline;
    // destructor for sequence RelatedSym
    delete NoRelatedSym;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence RelatedSym ends
    // destructor for sequence LinesOfText
    delete NoLinesOfText;
    delete Text;
    delete EncodedTextLen;
    delete EncodedText;
    // destructor for sequence LinesOfText ends
  }
};
class MDNewsMessage_137 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;
  CopyField<uint64_t> *OrigTime;
  CopyField<FFUtils::ByteArr> *NewsSource;
  CopyField<FFUtils::ByteArr> *NewsID;
  CopyField<FFUtils::ByteArr> *LanguageCode;
  CopyField<FFUtils::ByteArr> *Headline;
  CopyField<FFUtils::ByteArr> *URLLink;
  NoOpField<uint32_t> *EncodedHeadlineLen;
  NoOpField<FFUtils::ByteArr> *EncodedHeadline;
  // fields inside sequence RelatedSym
  NoOpField<uint32_t> *NoRelatedSym;
  CopyField<uint64_t> *SecurityID;
  ConstantFieldMandatory<uint32_t> *SecurityIDSource;
  NoOpField<FFUtils::ByteArr> *SecurityExchange;
  // fields inside sequence RelatedSym ends
  // fields inside sequence LinesOfText
  NoOpField<uint32_t> *NoLinesOfText;
  CopyField<FFUtils::ByteArr> *Text;
  NoOpField<uint32_t> *EncodedTextLen;
  NoOpField<FFUtils::ByteArr> *EncodedText;
  // fields inside sequence LinesOfText ends

  // Constructor
  MDNewsMessage_137() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("B"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
    OrigTime = new CopyField<uint64_t>(false, false, 0);
    NewsSource = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    NewsID = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    LanguageCode = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    Headline = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    URLLink = new CopyField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    EncodedHeadlineLen = new NoOpField<uint32_t>(false, false, 0);
    EncodedHeadline = new NoOpField<FFUtils::ByteArr>(false, false, FFUtils::ByteArr(""));
    // constructor for sequence RelatedSym
    NoRelatedSym = new NoOpField<uint32_t>(false, false, 0);
    SecurityID = new CopyField<uint64_t>(true, false, 0);
    SecurityIDSource = new ConstantFieldMandatory<uint32_t>(true, 8);
    SecurityExchange = new NoOpField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
    // constructor for sequence RelatedSym ends
    // constructor for sequence LinesOfText
    NoLinesOfText = new NoOpField<uint32_t>(false, false, 0);
    Text = new CopyField<FFUtils::ByteArr>(true, false, FFUtils::ByteArr(""));
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
    OrigTime->decode(input, pmap0);
    NewsSource->decode(input, pmap0);
    NewsID->decode(input, pmap0);
    LanguageCode->decode(input, pmap0);
    Headline->decode(input, pmap0);
    URLLink->decode(input, pmap0);
    EncodedHeadlineLen->decode(input);
    EncodedHeadline->decode(input);
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
    // decode for sequence LinesOfText
    NoLinesOfText->decode(input);

    int NoLinesOfText_len = NoLinesOfText->previousValue.getValue();
    if (NoLinesOfText->previousValue.isAssigned()) {
      for (int i = 0; i < NoLinesOfText_len; ++i) {
        // extract pmap
        FFUtils::PMap pmap1 = input.extractPmap();

        Text->decode(input, pmap1);
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
    NewsID->reset();
    LanguageCode->reset();
    Headline->reset();
    URLLink->reset();
    EncodedHeadlineLen->reset();
    EncodedHeadline->reset();
    // reset for sequence RelatedSym
    NoRelatedSym->reset();
    SecurityID->reset();
    SecurityIDSource->reset();
    SecurityExchange->reset();
    // reset for sequence RelatedSym ends
    // reset for sequence LinesOfText
    NoLinesOfText->reset();
    Text->reset();
    EncodedTextLen->reset();
    EncodedText->reset();
    // reset for sequence LinesOfText ends
  }

  // Destructor
  ~MDNewsMessage_137() {
    delete ApplVerID;
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
    delete OrigTime;
    delete NewsSource;
    delete NewsID;
    delete LanguageCode;
    delete Headline;
    delete URLLink;
    delete EncodedHeadlineLen;
    delete EncodedHeadline;
    // destructor for sequence RelatedSym
    delete NoRelatedSym;
    delete SecurityID;
    delete SecurityIDSource;
    delete SecurityExchange;
    // destructor for sequence RelatedSym ends
    // destructor for sequence LinesOfText
    delete NoLinesOfText;
    delete Text;
    delete EncodedTextLen;
    delete EncodedText;
    // destructor for sequence LinesOfText ends
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
  ~MDNewsMessage_120() {
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
class MDHeartbeat_144 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MsgType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;

  // Constructor
  MDHeartbeat_144() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MsgType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("0"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MsgType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MsgType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
  }

  // Destructor
  ~MDHeartbeat_144() {
    delete ApplVerID;
    delete MsgType;
    delete MsgSeqNum;
    delete SendingTime;
  }
};
class MDHeartbeat_129 : public FastDecoder {
 public:
  // All member functions
  ConstantFieldMandatory<FFUtils::ByteArr> *ApplVerID;
  ConstantFieldMandatory<FFUtils::ByteArr> *MessageType;
  NoOpField<uint32_t> *MsgSeqNum;
  NoOpField<uint64_t> *SendingTime;

  // Constructor
  MDHeartbeat_129() {
    ApplVerID = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("9"));
    MessageType = new ConstantFieldMandatory<FFUtils::ByteArr>(true, FFUtils::ByteArr("0"));
    MsgSeqNum = new NoOpField<uint32_t>(true, false, 0);
    SendingTime = new NoOpField<uint64_t>(true, false, 0);
  }

  // Decode
  void decode(FFUtils::ByteStreamReader &input, FFUtils::PMap &pmap0) {
    ApplVerID->decode();
    MessageType->decode();
    MsgSeqNum->decode(input);
    SendingTime->decode(input);
  }

  // process : TO be done in corresponding cpp file manually
  void process();

  // Reset
  void reset() {
    ApplVerID->reset();
    MessageType->reset();
    MsgSeqNum->reset();
    SendingTime->reset();
  }

  // Destructor
  ~MDHeartbeat_129() {
    delete ApplVerID;
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
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
  ~MDHeartbeat_101() {
    delete ApplVerID;
    delete MessageType;
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
  ~MDLogon_118() {
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
  ~MDLogout_119() {
    delete ApplVerID;
    delete MessageType;
    delete MsgSeqNum;
    delete SendingTime;
    delete ApplID;
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
  ~MDSequenceReset() {
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
    t_map[140] = new MDNonFix();
    t_map[117] = new MDTcpRequestReject_117();
    t_map[149] = new MDSecurityList_149();
    t_map[148] = new MDSecurityList_148();
    t_map[141] = new MDSecurityList_141();
    t_map[111] = new MDSecurityList_111();
    t_map[81] = new MDIncRefresh_81();
    t_map[150] = new MDIncRefresh_150();
    t_map[145] = new MDIncRefresh_145();
    t_map[138] = new MDIncRefresh_138();
    t_map[123] = new MDIncRefresh_123();
    t_map[126] = new MDIncRefresh_126();
    t_map[142] = new MDSecurityStatus_142();
    t_map[134] = new MDSecurityStatus_134();
    t_map[125] = new MDSecurityStatus_125();
    t_map[127] = new MDSnapshotFullRefresh_127();
    t_map[151] = new MDSnapshotFullRefresh_151();
    t_map[147] = new MDSnapshotFullRefresh_147();
    t_map[146] = new MDSnapshotFullRefresh_146();
    t_map[139] = new MDSnapshotFullRefresh_139();
    t_map[128] = new MDSnapshotFullRefresh_128();
    t_map[143] = new MDNewsMessage_143();
    t_map[137] = new MDNewsMessage_137();
    t_map[120] = new MDNewsMessage_120();
    t_map[144] = new MDHeartbeat_144();
    t_map[129] = new MDHeartbeat_129();
    t_map[101] = new MDHeartbeat_101();
    t_map[118] = new MDLogon_118();
    t_map[119] = new MDLogout_119();
    t_map[122] = new MDSequenceReset();
  }
  static void cleanUpMem(std::map<int, FastDecoder *> &t_map) {
    delete t_map[140];
    delete t_map[117];
    delete t_map[149];
    delete t_map[148];
    delete t_map[141];
    delete t_map[111];
    delete t_map[81];
    delete t_map[150];
    delete t_map[145];
    delete t_map[138];
    delete t_map[123];
    delete t_map[126];
    delete t_map[142];
    delete t_map[134];
    delete t_map[125];
    delete t_map[127];
    delete t_map[151];
    delete t_map[147];
    delete t_map[146];
    delete t_map[139];
    delete t_map[128];
    delete t_map[143];
    delete t_map[137];
    delete t_map[120];
    delete t_map[144];
    delete t_map[129];
    delete t_map[101];
    delete t_map[118];
    delete t_map[119];
    delete t_map[122];
  }
};
};
