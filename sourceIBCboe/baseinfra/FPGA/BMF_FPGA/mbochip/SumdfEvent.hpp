/*--------------------------------------------------------------------------------
--
-- This file is owned and controlled by MBOChip and must be used solely
-- for design, simulation, implementation and creation of design files
-- limited to MBOChip products. Do not distribute to third parties.
--
-- *************************************************
-- ** Copyright (C) 2013, MBOChip Private Limited **
-- ** All Rights Reserved. **
-- *************************************************
--
--------------------------------------------------------------------------------
-- Filename: SumdfEvent.hpp
--
-- Description:
-- Declares waitEvents return data types used on the SiliconUmdf Event API
--
*/
#ifndef SUMDFEVENT_HPP
#define SUMDFEVENT_HPP

#include "SumdfTypes.hpp"
#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <tr1/unordered_map>
#include "MDEntryInit.hpp"
#include "MDEntryX.hpp"
#include <stdint.h>

/*! \file SumdfEvent.hpp
    \brief SiliconUMDF EventAPI Events
*/

namespace SiliconUmdf {
namespace EventAPI {

#pragma pack(push) /* push current alignment to stack */
#pragma pack(1)

struct BookEntry_t {
  bool MDEntryPx_p;             ///< \brief MDEntryPx (FIX tag 270) is present.
  int MDEntryPxExp;             ///< \brief FIX tag 270 exponent. Optional.
  long long MDEntryPxMant;      ///< \brief FIX tag 270 mantissa. Optional.
  int MDEntrySize;              ///< \brief FIX tag 271. Mandatory.
  int MDEntryDate;              ///< \brief FIX tag 272. Mandatory.
  int MDEntryTime;              ///< \brief FIX tag 273. Mandatory.
  char MDEntryBuyer[51];        ///< \brief FIX tag 288. Optional.
  char MDEntrySeller[51];       ///< \brief FIX tag 289. Optional.
  char QuoteCondition[5];       ///< \brief FIX tag 276. Optional.
  unsigned int NumberOfOrders;  ///< \brief FIX tag 346. Optional.
  char OrderID[51];             ///< \brief FIX tag 37. Optional.
};

struct Book_t {
  std::vector<BookEntry_t*> bid;    ///< \brief Book bid side.
  std::vector<BookEntry_t*> offer;  ///< \brief Book offer side.
  unsigned last_RptSeq;             ///< \brief Last RptSeq (FIX tag 83) processed
};

struct Group_t {
  unsigned int MDEntryDate;          ///< \brief FIX tag 272
  unsigned int MDEntryTime;          ///< \brief FIX tag 273
  unsigned int TradingSessionSubID;  ///< \brief FIX tag 625
  unsigned int TradingSessionID;     ///< \brief FIX tag 336

  bool TradSesOpenTime_p;  ///< \brief TradSesOpenTime (FIX tag 342) is present
  Uint64 TradSesOpenTime;  ///< \brief FIX tag 342. Optional
};

struct PrivateStruct_t;

struct FeedTypes_t {
  char MDFeedType[4];        ///< \brief FIX tag 1022. Optional
  unsigned int MarketDepth;  ///< \brief FIX tag 264. Mandatory
};

struct ApplIDs_t {
  char ApplID[7];          ///< \brief FIX tag 1180. Mandatory
  unsigned NoMDFeedTypes;  ///< \brief FIX tag 1141. Optional
  std::vector<FeedTypes_t> FeedTypes;
};

struct SecurityAltIDs_t  // SecurityAltIDs from "Security List" Message (y)
{
  char SecurityAltID[51];    ///< \brief FIX tag 455. Optional
  char SecurityAltIDSource;  ///< \brief FIX tag 456. Mandatory
};

struct Underlyings_t  // Underlyings from "Security List" Message (y)
{
  char UnderlyingSymbol[33];        ///< \brief FIX tag 311. Mandatory
  Uint64 UnderlyingSecurityID;      ///< \brief FIX tag 309. Mandatory
  char UnderlyingSecurityIDSource;  ///< \brief FIX tag 305. Mandatory
};

struct Legs_t  // Legs from "Security List" Message (y)
{
  char LegSymbol[33];        ///< \brief FIX tag 600. Mandatory
  Uint64 LegSecurityID;      ///< \brief FIX tag 602. Mandatory
  char LegSecurityIDSource;  ///< \brief FIX tag 603. Mandatory
};

struct LotTypeRules_t  // LotTypeRules from "Security List" Message (y)
{
  int LotType;              ///< \brief FIX tag 1093. Mandatory
  unsigned int MinLotSize;  ///< \brief FIX tag 1231. Mandatory
};

struct InstrumentInfo_t {
  char Symbol[33];           ///< \brief FIX tag 55. Mandatory
  Uint64 SecurityID;         ///< \brief FIX tag 48. Mandatory
  char SecurityIDSource;     ///< \brief FIX tag 22. Mandatory
  char SecurityExchange[5];  ///< \brief FIX tag 207. Optional

  unsigned int NoApplIDs;  ///< \brief FIX tag 1351. Mandatory
  std::vector<ApplIDs_t> ApplIDs;

  unsigned int NoSecurityAltID;  ///< \brief FIX tag 454. Optional
  std::vector<SecurityAltIDs_t> SecurityAltIDs;

  unsigned int NoUnderlyings;  ///< \brief FIX tag 711. Optional
  std::vector<Underlyings_t> Underlyings;

  unsigned int NoLegs;  ///< \brief FIX tag 555. Optional
  std::vector<Legs_t> Legs;

  unsigned int NoLotTypeRules;  ///< \brief FIX tag 1234. Optional
  std::vector<LotTypeRules_t> LotTypeRules;

  char SecurityGroup[16];  ///< \brief FIX tag 555. Optional

  unsigned RoundLot;                             ///< \brief FIX tag 561. Mandatory
  bool MinTradeVol_p;                            ///< \brief MinTradeVol (FIX tag 562) is present
  Uint64 MinTradeVol;                            ///< \brief FIX tag 562. Optional
  bool MinPriceIncrement_p;                      ///< \brief MinPriceIncrement (FIX tag 969) is present
  int MinPriceIncrementExp;                      ///< \brief FIX tag 969 exponent. Optional
  long long MinPriceIncrementMant;               ///< \brief FIX tag 969 mantissa. Optional
  bool TickSizeDenominator_p;                    ///< \brief TickSizeDenominator (FIX tag 5151) is present
  unsigned TickSizeDenominator;                  ///< \brief FIX tag 5151. Optional
  bool PriceDivisor_p;                           ///< \brief PriceDivisor (FIX tag 37012) is present
  int PriceDivisorExp;                           ///< \brief FIX tag 37012 exponent. Optional
  long long PriceDivisorMant;                    ///< \brief FIX tag 37012 mantissa. Optional
  bool MinOrderQty_p;                            ///< \brief MinOrderQty (FIX tag 9749) is present
  unsigned MinOrderQty;                          ///< \brief FIX tag 9749. Optional
  bool MaxOrderQty_p;                            ///< \brief MaxOrderQty (FIX tag 9748) is present
  unsigned MaxOrderQty;                          ///< \brief FIX tag 9748. Optional
  char InstrumentId[32];                         ///< \brief FIX tag 9219. Optional
  char Currency[4];                              ///< \brief FIX tag 15. Optional
  bool Product_p;                                ///< \brief Product (FIX tag 460) is present
  int Product;                                   ///< \brief FIX tag 460. Optional
  char SecurityType[33];                         ///< \brief FIX tag 167. Optional
  char SecuritySubType[33];                      ///< \brief FIX tag 762. Optional
  char Asset[11];                                ///< \brief FIX tag 6937. Optional
  char SecurityDesc[1001];                       ///< \brief FIX tag 107. Optional
  bool MaturityDate_p;                           ///< \brief MaturityDate (FIX tag 541) is present
  unsigned MaturityDate;                         ///< \brief FIX tag 541. Optional
  bool MaturityMonthYear_p;                      ///< \brief MaturityMonthYear (FIX tag 200) is present
  unsigned MaturityMonthYear;                    ///< \brief FIX tag 200. Optional
  bool StrikePrice_p;                            ///< \brief StrikePrice (FIX tag 202) is present
  int StrikePriceExp;                            ///< \brief FIX tag 202 exponent. Optional
  long long StrikePriceMant;                     ///< \brief FIX tag 202 mantissa. Optional
  char StrikeCurrency[4];                        ///< \brief FIX tag 947. Optional
  bool ContractMultiplier_p;                     ///< \brief ContractMultiplier (FIX tag 231) is present
  int ContractMultiplierExp;                     ///< \brief FIX tag 231 exponent. Optional
  long long ContractMultiplierMant;              ///< \brief FIX tag 231 mantissa. Optional
  bool ContractSettlMonth_p;                     ///< \brief ContractSettlMonth (FIX tag 667) is present
  unsigned ContractSettlMonth;                   ///< \brief FIX tag 667. Optional
  char CFICode[7];                               ///< \brief FIX tag 461. Optional
  char CountryOfIssue[3];                        ///< \brief FIX tag 470. Optional
  unsigned IssueDate;                            ///< \brief FIX tag 225. Mandatory
  bool DatedDate_p;                              ///< \brief DatedDate (FIX tag 873) is present
  unsigned DatedDate;                            ///< \brief FIX tag 873. Optional
  bool StartDate_p;                              ///< \brief StartDate (FIX tag 916) is present
  unsigned StartDate;                            ///< \brief FIX tag 916. Optional
  bool EndDate_p;                                ///< \brief EndDate (FIX tag 917) is present
  unsigned EndDate;                              ///< \brief FIX tag 917. Optional
  char SettlType[5];                             ///< \brief FIX tag 63. Optional
  bool SettlDate_p;                              ///< \brief SettlDate (FIX tag 64) is present
  unsigned SettlDate;                            ///< \brief FIX tag 64. Optional
  bool SecurityValidityTimestamp_p;              ///< \brief SecurityValidityTimestamp (FIX tag 6938) is present
  unsigned long long SecurityValidityTimestamp;  ///< \brief FIX tag 6938. Optional

  int MarketDepth;  ///< \brief FIX tag 264. Mandatory. Valid when \ref initializedBySnapshot is true

  unsigned int StatusMDEntryDate;      ///< \brief Date of last instrument status update. Valid when \ref
                                       ///initializedBySnapshot is true
  unsigned int StatusMDEntryTime;      ///< \brief Time of last instrument status update. Valid when \ref
                                       ///initializedBySnapshot is true
  unsigned int SecurityTradingStatus;  ///< \brief FIX tag 326. Mandatory. Valid when \ref initializedBySnapshot is true
  unsigned int TradingSessionID;       ///< \brief FIX tag 336. Mandatory. Valid when \ref initializedBySnapshot is true
  bool TradSesOpenTime_p;              ///< \brief TradSesOpenTime (FIX tag 342) is present
  Uint64 TradSesOpenTime;              ///< \brief FIX tag 342. Optional. Valid when \ref initializedBySnapshot is true

  bool followingGroup;  ///< \brief Indicates if the status of the instrument is following the group status

  PrivateStruct_t* privateData;  ///< \brief API private data. Not accessible from user's application.

  bool initializedBySnapshot;  ///< \brief Indicates if instrument was initialized by snapshot.

  InstrumentInfo_t() {
    privateData = NULL;
    initializedBySnapshot = false;
  }
};

struct LastTrade_t  // Trade MDEntry from "Market Data Incremental
// Refresh" Message (X)
{
  bool valid;                   ///< \brief Indicates that LastTrade_t information is valid.
  bool MDEntryPx_p;             ///< \brief MDEntryPx (FIX tag 270) is present.
  int MDEntryPxExp;             ///< \brief FIX tag 270 exponent. Optional.
  long long MDEntryPxMant;      ///< \brief FIX tag 270 mantissa. Optional.
  unsigned MDEntryTime;         ///< \brief FIX tag 273. Mandatory.
  bool MDEntrySize_p;           ///< \brief MDEntrySize (FIX tag 271) is present.
  long long MDEntrySize;        ///< \brief FIX tag 271. Optional.
  unsigned MDEntryDate;         ///< \brief FIX tag 272. Optional.
  char MDStreamID[3];           ///< \brief FIX tag 1500. Optional.
  bool NetChgPrevDay_p;         ///< \brief NetChgPrevDay (FIX tag 451) is present.
  int NetChgPrevDayExp;         ///< \brief FIX tag 451 exponent. Optional.
  long long NetChgPrevDayMant;  ///< \brief FIX tag 451 mantissa. Optional.
  char TickDirection;           ///< \brief FIX tag 274. Optional.
  char TradeCondition[15];      ///< \brief FIX tag 277. Optional.
  char TradeID[33];             ///< \brief FIX tag 1003. Optional.
  char MDEntryBuyer[51];        ///< \brief FIX tag 288. Optional.
  char MDEntrySeller[56];       ///< \brief FIX tag 289. Optional.
  LastTrade_t() {
    valid = false;
    MDEntryPx_p = false;
    MDEntrySize_p = false;
    MDStreamID[0] = '\0';
    NetChgPrevDay_p = false;
    TradeCondition[0] = '\0';
    TradeID[0] = '\0';
    MDEntryBuyer[0] = '\0';
    MDEntrySeller[0] = '\0';
  }
};

struct OpeningPrice_t {
  bool valid;                         ///< \brief Indicates that OpeningPrice_t information is valid.
  bool MDEntryPx_p;                   ///< \brief MDEntryPx (FIX tag 270) is present.
  int MDEntryPxExp;                   ///< \brief FIX tag 270 exponent. Optional.
  long long MDEntryPxMant;            ///< \brief FIX tag 270 mantissa. Optional.
  bool MDEntryInterestRate_p;         ///< \brief MDEntryInterestRate (FIX tag 37014) is present.
  int MDEntryInterestRateExp;         ///< \brief FIX tag 37014 exponent. Optional.
  long long MDEntryInterestRateMant;  ///< \brief FIX tag 37014 mantissa. Optional.
  unsigned MDEntryTime;               ///< \brief FIX tag 273. Mandatory.
  bool MDEntrySize_p;                 ///< \brief MDEntrySize (FIX tag 271) is present.
  long long MDEntrySize;              ///< \brief FIX tag 271. Optional.
  unsigned MDEntryDate;               ///< \brief FIX tag 272. Optional.
  char MDStreamID[3];                 ///< \brief FIX tag 1500. Optional.
  bool NetChgPrevDay_p;               ///< \brief MDEntrySize (FIX tag 451) is present.
  int NetChgPrevDayExp;               ///< \brief FIX tag 451. Optional.
  long long NetChgPrevDayMant;        ///< \brief FIX tag 451. Optional.
  char TickDirection;                 ///< \brief FIX tag 274. Optional.
  OpeningPrice_t() {
    valid = false;
    MDEntryPx_p = false;
    MDEntryInterestRate_p = false;
    MDEntrySize_p = false;
    MDStreamID[0] = '\0';
    NetChgPrevDay_p = false;
    TickDirection = '\0';
  }
};

struct TheoreticalOpeningPrice_t {
  bool valid;                         ///< \brief Indicates that TheoreticalOpeningPrice_t information is valid.
  bool MDEntryPx_p;                   ///< \brief MDEntryPx (FIX tag 270) is present.
  int MDEntryPxExp;                   ///< \brief FIX tag 270 exponent. Optional.
  long long MDEntryPxMant;            ///< \brief FIX tag 270 mantissa. Optional.
  bool MDEntryInterestRate_p;         ///< \brief MDEntryInterestRate (FIX tag 37014) is present.
  int MDEntryInterestRateExp;         ///< \brief FIX tag 37014 exponent. Optional.
  long long MDEntryInterestRateMant;  ///< \brief FIX tag 37014 mantissa. Optional.
  unsigned MDEntryTime;               ///< \brief FIX tag 273. Mandatory.
  bool MDEntrySize_p;                 ///< \brief MDEntrySize (FIX tag 271) is present.
  long long MDEntrySize;              ///< \brief FIX tag 271. Optional.
  unsigned MDEntryDate;               ///< \brief FIX tag 272. Optional.
  char MDStreamID[3];                 ///< \brief FIX tag 1500. Optional.
  bool NetChgPrevDay_p;               ///< \brief MDEntrySize (FIX tag 451) is present.
  int NetChgPrevDayExp;               ///< \brief FIX tag 451. Optional.
  long long NetChgPrevDayMant;        ///< \brief FIX tag 451. Optional.
  char TickDirection;                 ///< \brief FIX tag 274. Optional.
  TheoreticalOpeningPrice_t() {
    valid = false;
    MDEntryPx_p = false;
    MDEntryInterestRate_p = false;
    MDEntrySize_p = false;
    MDStreamID[0] = '\0';
    NetChgPrevDay_p = false;
    TickDirection = '\0';
  }
};

struct Imbalance_t {
  bool valid;               ///< \brief Indicates that Imbalance_t information is valid.
  bool MDEntrySize_p;       ///< \brief MDEntrySize (FIX tag 271) is present.
  long long MDEntrySize;    ///< \brief FIX tag 271. Optional.
  unsigned MDEntryTime;     ///< \brief FIX tag 273. Mandatory.
  unsigned MDEntryDate;     ///< \brief FIX tag 272. Mandatory.
  char TradeCondition[15];  ///< \brief FIX tag 277. Optional.
  Imbalance_t() {
    valid = false;
    MDEntrySize_p = false;
    TradeCondition[0] = '\0';
  }
};

struct Instrument_t {
  Book_t* book;           ///< \brief Pointer to book structures
  InstrumentInfo_t info;  ///< \brief Instrument information
  LastTrade_t lastTrade;  ///< \brief Last trade received

  OpeningPrice_t openingPrice;  ///< \brief Current \b OpeningPrice information. Check if your API version supports this
                                ///MDEntryType.
  TheoreticalOpeningPrice_t theoreticalOpeningPrice;  ///< \brief Current \b TheoreticalOpeningPrice information. Check
                                                      ///if your API version supports this MDEntryType.
  Imbalance_t imbalance;  ///< \brief Current \b TheoreticalOpeningPrice information. Check if your API version supports
                          ///this MDEntryType.

  Instrument_t() { book = NULL; }
};

struct InstrumentUpdate_t {
  char Symbol[33];           ///< \brief FIX tag 55. Mandatory
  Uint64 SecurityID;         ///< \brief FIX tag 48. Mandatory
  char SecurityIDSource;     ///< \brief FIX tag 22. Mandatory
  char SecurityExchange[5];  ///< \brief FIX tag 207. Optional

  unsigned int NoApplIDs;  ///< \brief FIX tag 1351. Mandatory
  std::vector<ApplIDs_t> ApplIDs;

  unsigned int NoSecurityAltID;  ///< \brief FIX tag 454. Optional
  std::vector<SecurityAltIDs_t> SecurityAltIDs;

  unsigned int NoUnderlyings;  ///< \brief FIX tag 711. Optional
  std::vector<Underlyings_t> Underlyings;

  unsigned int NoLegs;  ///< \brief FIX tag 555. Optional
  std::vector<Legs_t> Legs;

  unsigned int NoLotTypeRules;  ///< \brief FIX tag 1234. Optional
  std::vector<LotTypeRules_t> LotTypeRules;

  char SecurityUpdateAction;  ///< \brief FIX tag 980. Mandatory
  char SecurityGroup[16];     ///< \brief FIX tag 1151. Optional

  unsigned RoundLot;                             ///< \brief FIX tag 561. Mandatory
  bool MinTradeVol_p;                            ///< \brief MinTradeVol (FIX tag 562) is present
  Uint64 MinTradeVol;                            ///< \brief FIX tag 562. Optional
  bool MinPriceIncrement_p;                      ///< \brief MinPriceIncrement (FIX tag 969) is present
  int MinPriceIncrementExp;                      ///< \brief FIX tag 969 exponent. Optional
  long long MinPriceIncrementMant;               ///< \brief FIX tag 969 mantissa. Optional
  bool TickSizeDenominator_p;                    ///< \brief TickSizeDenominator (FIX tag 5151) is present
  unsigned TickSizeDenominator;                  ///< \brief FIX tag 5151. Optional
  bool PriceDivisor_p;                           ///< \brief PriceDivisor (FIX tag 37012) is present
  int PriceDivisorExp;                           ///< \brief FIX tag 37012 exponent. Optional
  long long PriceDivisorMant;                    ///< \brief FIX tag 37012 mantissa. Optional
  bool MinOrderQty_p;                            ///< \brief MinOrderQty (FIX tag 9749) is present
  unsigned MinOrderQty;                          ///< \brief FIX tag 9749. Optional
  bool MaxOrderQty_p;                            ///< \brief MaxOrderQty (FIX tag 9748) is present
  unsigned MaxOrderQty;                          ///< \brief FIX tag 9748. Optional
  char InstrumentId[32];                         ///< \brief FIX tag 9219. Optional
  char Currency[4];                              ///< \brief FIX tag 15. Optional
  bool Product_p;                                ///< \brief Product (FIX tag 460) is present
  int Product;                                   ///< \brief FIX tag 460. Optional
  char SecurityType[33];                         ///< \brief FIX tag 167. Optional
  char SecuritySubType[33];                      ///< \brief FIX tag 762. Optional
  char Asset[11];                                ///< \brief FIX tag 6937. Optional
  char SecurityDesc[1001];                       ///< \brief FIX tag 107. Optional
  bool MaturityDate_p;                           ///< \brief MaturityDate (FIX tag 541) is present
  unsigned MaturityDate;                         ///< \brief FIX tag 541. Optional
  bool MaturityMonthYear_p;                      ///< \brief MaturityMonthYear (FIX tag 200) is present
  unsigned MaturityMonthYear;                    ///< \brief FIX tag 200. Optional
  bool StrikePrice_p;                            ///< \brief StrikePrice (FIX tag 202) is present
  int StrikePriceExp;                            ///< \brief FIX tag 202 exponent. Optional
  long long StrikePriceMant;                     ///< \brief FIX tag 202 mantissa. Optional
  char StrikeCurrency[4];                        ///< \brief FIX tag 947. Optional
  bool ContractMultiplier_p;                     ///< \brief ContractMultiplier (FIX tag 231) is present
  int ContractMultiplierExp;                     ///< \brief FIX tag 231 exponent. Optional
  long long ContractMultiplierMant;              ///< \brief FIX tag 231 mantissa. Optional
  bool ContractSettlMonth_p;                     ///< \brief ContractSettlMonth (FIX tag 667) is present
  unsigned ContractSettlMonth;                   ///< \brief FIX tag 667. Optional
  char CFICode[7];                               ///< \brief FIX tag 461. Optional
  char CountryOfIssue[3];                        ///< \brief FIX tag 470. Optional
  unsigned IssueDate;                            ///< \brief FIX tag 225. Mandatory
  bool DatedDate_p;                              ///< \brief DatedDate (FIX tag 873) is present
  unsigned DatedDate;                            ///< \brief FIX tag 873. Optional
  bool StartDate_p;                              ///< \brief StartDate (FIX tag 916) is present
  unsigned StartDate;                            ///< \brief FIX tag 916. Optional
  bool EndDate_p;                                ///< \brief EndDate (FIX tag 917) is present
  unsigned EndDate;                              ///< \brief FIX tag 917. Optional
  char SettlType[5];                             ///< \brief FIX tag 63. Optional
  bool SettlDate_p;                              ///< \brief SettlDate (FIX tag 64) is present
  unsigned SettlDate;                            ///< \brief FIX tag 64. Optional
  bool SecurityValidityTimestamp_p;              ///< \brief SecurityValidityTimestamp (FIX tag 6938) is present
  unsigned long long SecurityValidityTimestamp;  ///< \brief FIX tag 6938. Optional
};

struct InstrumentStatus_t {
  Uint64 SendingTime;                  ///< \brief FIX tag 52. Mandatory
  unsigned TradeDate;                  ///< \brief FIX tag 75. Mandatory
  char Symbol[33];                     ///< \brief FIX tag 55. Optional
  Uint64 SecurityID;                   ///< \brief FIX tag 48. Mandatory
  unsigned SecurityIDSource;           ///< \brief FIX tag 22. Mandatory
  char SecurityExchange[5];            ///< \brief FIX tag 207. Mandatory
  unsigned int TradingSessionID;       ///< \brief FIX tag 336. Mandatory
  unsigned int SecurityTradingStatus;  ///< \brief FIX tag 326. Mandatory
  unsigned int SecurityTradingEvent;   ///< \brief FIX tag 1174. Mandatory
  unsigned int MDEntryDate;            ///< \brief FIX tag 272. Mandatory
  unsigned int MDEntryTime;            ///< \brief FIX tag 273. Mandatory

  bool TradSesOpenTime_p;  ///< \brief TradSesOpenTime (FIX tag 342) is present
  Uint64 TradSesOpenTime;  ///< \brief FIX tag 342. Optional. Valid when \ref initializedBySnapshot is true
};

struct GroupStatus_t {
  Uint64 SendingTime;                 ///< \brief FIX tag 52. Mandatory
  unsigned TradeDate;                 ///< \brief FIX tag 75. Mandatory
  char SecurityGroup[16];             ///< \brief FIX tag 1151. Optional
  unsigned int TradingSessionSubID;   ///< \brief FIX tag 625. Mandatory
  unsigned int SecurityTradingEvent;  ///< \brief FIX tag 1174. Mandatory
  unsigned int MDEntryDate;           ///< \brief FIX tag 272. Mandatory
  unsigned int MDEntryTime;           ///< \brief FIX tag 273. Mandatory

  bool TradSesOpenTime_p;  ///< \brief TradSesOpenTime (FIX tag 342) is present
  Uint64 TradSesOpenTime;  ///< \brief FIX tag 342. Optional. Valid when \ref initializedBySnapshot is true
};

/*************************************
 * 				NEWS				 *
 ************************************/
struct RelatedSym_t {
  Uint64 SecurityID;              ///< \brief FIX tag 48. Mandatory
  unsigned int SecurityIDSource;  ///< \brief FIX tag 22. Mandatory
};

struct RoutingID_t {
  bool RoutingType_p;        ///< \brief RoutingType (FIX tag 216) is present
  unsigned int RoutingType;  ///< \brief FIX tag 216. Optional.
  char RoutingID;            ///< \brief FIX tag 217. Optional.
};

struct News_t {
  unsigned int MsgSeqNum;  ///< \brief FIX tag 34. Mandatory

  Uint64 SendingTime;  ///< \brief FIX tag 52. Mandatory

  bool OrigTime_p;       ///< \brief OrigTime (FIX tag 42) is present
  Uint64 OrigTime;       ///< \brief FIX tag 42. Optional
  char NewsSource[4];    ///< \brief FIX tag 6940. Mandatory
  char LanguageCode[3];  ///< \brief FIX tag 1474. Optional

  std::string URLLink;   ///< \brief FIX tag 149. Optional
  std::string Headline;  ///< \brief FIX tag 148. Mandatory

  unsigned int NoRelatedSym;  ///< \brief FIX tag 146. Optional
  std::vector<RelatedSym_t> RelatedSym;

  unsigned int NoLinesOfText;  ///< \brief FIX tag 33. Optional
  std::vector<std::string> LinesOfText;

  unsigned int NoRoutingIDs;  ///< \brief FIX tag 215. Optional
  std::vector<RoutingID_t> RoutingID;
};

struct Error_t {
  Error error;              ///< \brief Error type
  std::string description;  ///< \brief String describing the error
};

struct Events_t {
  unsigned sumdfChannelId;    ///< \brief Index of channel array to identify which channel the event refers to.
  EventsType eventsReturned;  ///< \brief Indicates the type of the event returned.
  std::vector<BVMF::MDEntryInit_t> MDEntriesInit;  ///< \brief Vector of MDEntries received on snapshot. Valid when \ref
                                                   ///eventsReturned is MDENTRIES_INIT_ONLY.
  BVMF::PUMA_1_6::MDEntriesX_t MDEntries_PUMA_1_6;  ///< \brief MDEntries struct from PUMA 1.6 incremental messages.
                                                    ///Valid when \ref eventsReturned is
                                                    ///BOOK_AND_MDENTRIES_BVMF_PUMA_1_6, MDENTRIES_ONLY_BVMF_PUMA_1_6.
  BVMF::PUMA_2_0::MDEntriesX_t MDEntries_PUMA_2_0;  ///< \brief MDEntries struct from PUMA 2.0 incremental messages.
                                                    ///Valid when \ref eventsReturned is
                                                    ///BOOK_AND_MDENTRIES_BVMF_PUMA_2_0, MDENTRIES_ONLY_BVMF_PUMA_2_0.
  Instrument_t* bookAndInstrumentInfo;  ///< \brief Pointer to book and instruments information. Valid when \ref
                                        ///eventsReturned is BOOK_AND_MDENTRIES_BVMF_PUMA_1_6,
                                        ///BOOK_AND_MDENTRIES_BVMF_PUMA_2_0, BOOK_ONLY_BVMF_PUMA_1_6,
                                        ///BOOK_ONLY_BVMF_PUMA_2_0 or BOOK_ONLY
  unsigned incremental_MsgSeqNum;  ///< \brief The MsgSeqNum of incremental message. Valid when \ref eventsReturned is
                                   ///BOOK_AND_MDENTRIES_BVMF_PUMA_1_6, BOOK_AND_MDENTRIES_BVMF_PUMA_2_0,
                                   ///BOOK_ONLY_BVMF_PUMA_1_6, BOOK_ONLY_BVMF_PUMA_2_0 or BOOK_ONLY
  std::tr1::unordered_map<Uint64, Instrument_t>* instrumentList;  ///< \brief Pointer to the instrument list of the
                                                                  ///channel. Valid when \ref eventsReturned is
                                                                  ///INSTRUMENT_LIST
  std::tr1::unordered_map<std::string, Group_t>* groupList;  ///< \brief Pointer to the group list of the channel. Valid
                                                             ///when \ref eventsReturned is INSTRUMENT_LIST
  std::vector<InstrumentUpdate_t>
      instrumentUpdates;  ///< \brief Vector of instrument updates. Valid when \ref eventsReturned is INSTRUMENT_UPDATES
  InstrumentStatus_t instrumentStatus;  ///< \brief Informs instrument status modifications. Valid when \ref
                                        ///eventsReturned is INSTRUMENT_STATUS
  GroupStatus_t
      groupStatus;  ///< \brief Informs group status modifications. Valid when \ref eventsReturned is GROUP_STATUS
  News_t news;      ///< \brief News messages. Valid when \ref eventsReturned is NEWS
  Error_t error;    ///< \brief Error information. Valid when \ref eventsReturned is ERROR
  bool messagesBufferIsEmpty;  ///< \brief Indicates that there is no other message on buffer
};

#pragma pack(pop) /* pop current alignment from stack */

}  // namespace EventAPI
}  // namespace SiliconUmdf
#endif
