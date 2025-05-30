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
   -- Filename: MDEntryX.hpp
   --
   -- Description:
   --
   --
 */
#ifndef MDENTRYX_HPP
#define MDENTRYX_HPP

///@cond
#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <stdint.h>
///@endcond

/*! \file MDEntryX.hpp
    \brief MDEntries structs from incremental messages
*/

namespace SiliconUmdf {
namespace EventAPI {
namespace BVMF {
namespace PUMA_2_0 {
#pragma pack(push) /* push current alignment to stack */
#pragma pack(1)

struct BidX_t  // Bid MDEntry from "Market Data Incremental
// Refresh" Message (X)
{
  Uint64 SecurityID;           ///< \brief FIX tag 48. Mandatory
  bool RptSeq_p;               ///< \brief RptSeq (FIX tag 83) is present
  unsigned RptSeq;             ///< \brief FIX tag 83. Optional.
  char QuoteCondition[5];      ///< \brief FIX tag 276. Optional
  bool MDEntryPx_p;            ///< \brief MDEntryPx (FIX tag 270) is present.
  int MDEntryPxExp;            ///< \brief FIX tag 270 exponent. Optional.
  long long MDEntryPxMant;     ///< \brief FIX tag 270 mantissa. Optional.
  bool NumberOfOrders_p;       ///< \brief NumberOfOrders (FIX tag 346) is present.
  unsigned NumberOfOrders;     ///< \brief FIX tag 346. Optional.
  unsigned MDEntryTime;        ///< \brief FIX tag 273. Mandatory.
  bool MDEntrySize_p;          ///< \brief MDEntrySize (FIX tag 271) is present.
  long long MDEntrySize;       ///< \brief FIX tag 271. Optional.
  unsigned MDEntryDate;        ///< \brief FIX tag 272. Mandatory.
  char OrderID[27];            ///< \brief FIX tag 37. Optional.
  char MDEntryBuyer[51];       ///< \brief FIX tag 288. Optional.
  unsigned MDEntryPositionNo;  ///< \brief FIX tag 290. Mandatory.
};

struct OfferX_t  // Offer MDEntry from "Market Data Incremental
// Refresh" Message (X)
{
  Uint64 SecurityID;        ///< \brief FIX tag 48. Mandatory
  bool RptSeq_p;            ///< \brief RptSeq (FIX tag 83) is present
  unsigned RptSeq;          ///< \brief FIX tag 83. Optional
  char QuoteCondition[5];   ///< \brief FIX tag 276. Optional
  bool MDEntryPx_p;         ///< \brief MDEntryPx (FIX tag 270) is present.
  int MDEntryPxExp;         ///< \brief FIX tag 270 exponent. Optional.
  long long MDEntryPxMant;  ///< \brief FIX tag 270 mantissa. Optional.
  bool NumberOfOrders_p;    ///< \brief NumberOfOrders (FIX tag 346) is present.
  unsigned NumberOfOrders;  ///< \brief FIX tag 346. Optional.
  unsigned MDEntryTime;     ///< \brief FIX tag 273. Mandatory.
  bool MDEntrySize_p;       ///< \brief MDEntrySize (FIX tag 271) is present.
  long long MDEntrySize;    ///< \brief FIX tag 271. Optional.

  unsigned MDEntryDate;        ///< \brief FIX tag 272. Mandatory.
  char OrderID[27];            ///< \brief FIX tag 37. Optional.
  char MDEntrySeller[51];      ///< \brief FIX tag 289. Optional.
  unsigned MDEntryPositionNo;  ///< \brief FIX tag 290. Mandatory.
};

struct TradeX_t  // Trade MDEntry from "Market Data Incremental
// Refresh" Message (X)
{
  Uint64 SecurityID;            ///< \brief FIX tag 48. Mandatory
  bool RptSeq_p;                ///< \brief RptSeq (FIX tag 83) is present
  unsigned RptSeq;              ///< \brief FIX tag 83. Optional
  bool MDEntryPx_p;             ///< \brief MDEntryPx (FIX tag 270) is present.
  int MDEntryPxExp;             ///< \brief FIX tag 270 exponent. Optional.
  long long MDEntryPxMant;      ///< \brief FIX tag 270 mantissa. Optional.
  unsigned MDEntryTime;         ///< \brief FIX tag 273. Mandatory.
  bool MDEntrySize_p;           ///< \brief MDEntrySize (FIX tag 271) is present.
  long long MDEntrySize;        ///< \brief FIX tag 271. Optional.
  unsigned MDEntryDate;         ///< \brief FIX tag 272. Mandatory.
  char MDStreamID[3];           ///< \brief FIX tag 1500. Optional.
  bool NetChgPrevDay_p;         ///< \brief NetChgPrevDay (FIX tag 451) is present.
  int NetChgPrevDayExp;         ///< \brief FIX tag 451 exponent. Optional.
  long long NetChgPrevDayMant;  ///< \brief FIX tag 451 mantissa. Optional.
  char TickDirection;           ///< \brief FIX tag 274. Optional.
  char TradeCondition[15];      ///< \brief FIX tag 277. Optional.
  bool TradingSessionID_p;      ///< \brief TradingSessionID (FIX tag 336) is present.
  unsigned TradingSessionID;    ///< \brief FIX tag 336. Optional.
  char TradeID[33];             ///< \brief FIX tag 1003. Optional.
  char MDEntryBuyer[51];        ///< \brief FIX tag 288. Optional.
  char MDEntrySeller[51];       ///< \brief FIX tag 289. Optional.
};

struct OpeningPriceX_t  // OpeningPrice MDEntry from "Market Data
// Incremental Refresh" Message (X)
{
  Uint64 SecurityID;                  ///< \brief FIX tag 48. Mandatory
  bool RptSeq_p;                      ///< \brief RptSeq (FIX tag 83) is present
  unsigned RptSeq;                    ///< \brief FIX tag 83. Optional
  bool MDEntryPx_p;                   ///< \brief MDEntryPx (FIX tag 270) is present.
  int MDEntryPxExp;                   ///< \brief FIX tag 270 exponent. Optional.
  long long MDEntryPxMant;            ///< \brief FIX tag 270 mantissa. Optional.
  bool MDEntryInterestRate_p;         ///< \brief MDEntryInterestRate (FIX tag 37014) is present.
  int MDEntryInterestRateExp;         ///< \brief FIX tag 37014 exponent. Optional.
  long long MDEntryInterestRateMant;  ///< \brief FIX tag 37014 mantissa. Optional.
  unsigned MDEntryTime;               ///< \brief FIX tag 273. Mandatory.
  bool MDEntrySize_p;                 ///< \brief MDEntrySize (FIX tag 271) is present.
  long long MDEntrySize;              ///< \brief FIX tag 271. Optional.
  unsigned MDEntryDate;               ///< \brief FIX tag 272. Mandatory.
  char MDStreamID[3];                 ///< \brief FIX tag 1500. Optional.
  bool NetChgPrevDay_p;               ///< \brief NetChgPrevDay (FIX tag 451) is present.
  int NetChgPrevDayExp;               ///< \brief FIX tag 451 exponent. Optional.
  long long NetChgPrevDayMant;        ///< \brief FIX tag 451 mantissa. Optional.
  char TickDirection;                 ///< \brief FIX tag 274. Optional.
  bool OpenCloseSettleFlag_p;         ///< \brief OpenCloseSettleFlag (FIX tag 286) is present.
  unsigned OpenCloseSettleFlag;       ///< \brief FIX tag 286. Optional.
};

struct ImbalanceX_t {
  Uint64 SecurityID;        ///< \brief FIX tag 48. Mandatory
  bool RptSeq_p;            ///< \brief RptSeq (FIX tag 83) is present
  unsigned RptSeq;          ///< \brief FIX tag 83. Optional
  unsigned MDEntryTime;     ///< \brief FIX tag 273. Mandatory.
  bool MDEntrySize_p;       ///< \brief MDEntrySize (FIX tag 271) is present.
  long long MDEntrySize;    ///< \brief FIX tag 271. Optional.
  unsigned MDEntryDate;     ///< \brief FIX tag 272. Mandatory.
  char TradeCondition[15];  ///< \brief FIX tag 277. Optional.
};

struct EmptyBook_t      // EmptyBook MDEntry from "Market Data
{                       // Incremental Refresh" Message (X)
  Uint64 SecurityID;    ///< \brief FIX tag 48. Mandatory
  bool RptSeq_p;        ///< \brief RptSeq (FIX tag 83) is present
  unsigned int RptSeq;  ///< \brief FIX tag 83. Optional.
};

struct MDEntryX_t               // MDEntry from "Market Data Incremental Refresh"
{                               // Message (X)
  unsigned int MDUpdateAction;  ///< \brief FIX tag 279. Mandatory
  char MDEntryType;             ///< \brief FIX tag 269. Mandatory
  union {
    BidX_t bid;                    ///< \brief Valid when \ref MDEntryType = '0' (Bid).
    OfferX_t offer;                ///< \brief Valid when \ref MDEntryType = '1' (Offer).
    TradeX_t trade;                ///< \brief Valid when \ref MDEntryType = '2' (Trade).
    OpeningPriceX_t openingPrice;  ///< \brief Valid when \ref MDEntryType = '4' (OpeningPrice). Check if your API
                                   ///version supports this MDEntryType.
    ImbalanceX_t imbalance;        ///< \brief Valid when \ref MDEntryType = 'A' (Imbalance). Check if your API version
                                   ///supports this MDEntryType.
    EmptyBook_t emptyBook;         ///< \brief Valid when \ref MDEntryType = 'J' (EmptyBook).
  };
};

typedef MDEntryX_t MDEntryX_array[512];

struct MDEntriesX_t {
  Uint64 SendingTime;         //!< SendingTime of incremental message
  unsigned NoMDEntries;       //!< Number of MDEntries in the array pointed by \ref MDEntries
  MDEntryX_array *MDEntries;  //!< Pointer to an array of \ref MDEntryX_t
  unsigned msgArrivedFPGAticks;
  unsigned msgDecodedFPGAticks;
};
}  // namespace PUMA_2_0

namespace PUMA_1_6 {
struct BidX_t  // Bid MDEntry from "Market Data Incremental
// Refresh" Message (X)
{
  Uint64 SecurityID;           ///< \brief FIX tag 48. Mandatory
  bool RptSeq_p;               ///< \brief RptSeq (FIX tag 83) is present
  unsigned RptSeq;             ///< \brief FIX tag 83. Optional
  char QuoteCondition[5];      ///< \brief FIX tag 276. Optional.
  bool MDEntryPx_p;            ///< \brief MDEntryPx (FIX tag 270) is present.
  int MDEntryPxExp;            ///< \brief FIX tag 270 exponent. Optional.
  long long MDEntryPxMant;     ///< \brief FIX tag 270 mantissa. Optional.
  bool NumberOfOrders_p;       ///< \brief NumberOfOrders (FIX tag 346) is present.
  unsigned NumberOfOrders;     ///< \brief FIX tag 346. Optional. C
  unsigned MDEntryTime;        ///< \brief FIX tag 273. Mandatory.
  bool MDEntrySize_p;          ///< \brief MDEntrySize (FIX tag 271) is present.
  long long MDEntrySize;       ///< \brief FIX tag 271. Optional. C
  unsigned MDEntryDate;        ///< \brief FIX tag 272. Mandatory.
  char OrderID[27];            ///< \brief FIX tag 37. Optional.
  char MDEntryBuyer[51];       ///< \brief FIX tag 288. Optional.
  unsigned MDEntryPositionNo;  ///< \brief FIX tag 290. Mandatory.
};

struct OfferX_t  // Offer MDEntry from "Market Data Incremental
// Refresh" Message (X)
{
  Uint64 SecurityID;           ///< \brief FIX tag 48. Mandatory
  bool RptSeq_p;               ///< \brief RptSeq (FIX tag 83) is present
  unsigned RptSeq;             ///< \brief FIX tag 83. Optional
  char QuoteCondition[5];      ///< \brief FIX tag 276. Optional.
  bool MDEntryPx_p;            ///< \brief MDEntryPx (FIX tag 270) is present.
  int MDEntryPxExp;            ///< \brief FIX tag 270 exponent. Optional.
  long long MDEntryPxMant;     ///< \brief FIX tag 270 mantissa. Optional.
  bool NumberOfOrders_p;       ///< \brief NumberOfOrders (FIX tag 346) is present.
  unsigned NumberOfOrders;     ///< \brief FIX tag 346. Optional.
  unsigned MDEntryTime;        ///< \brief FIX tag 273. Mandatory.
  bool MDEntrySize_p;          ///< \brief MDEntrySize (FIX tag 271) is present.
  long long MDEntrySize;       ///< \brief FIX tag 271. Optional. N
  unsigned MDEntryDate;        ///< \brief FIX tag 272. Mandatory.
  char OrderID[27];            ///< \brief FIX tag 37. Optional.
  char MDEntrySeller[51];      ///< \brief FIX tag 289. Optional.
  unsigned MDEntryPositionNo;  ///< \brief FIX tag 290. Mandatory.
};

struct TradeX_t  // Trade MDEntry from "Market Data Incremental
// Refresh" Message (X)
{
  Uint64 SecurityID;            ///< \brief FIX tag 48. Mandatory
  bool RptSeq_p;                ///< \brief RptSeq (FIX tag 83) is present
  unsigned RptSeq;              ///< \brief FIX tag 83. Optional
  bool MDEntryPx_p;             ///< \brief MDEntryPx (FIX tag 270) is present.
  int MDEntryPxExp;             ///< \brief FIX tag 270 exponent. Optional.
  long long MDEntryPxMant;      ///< \brief FIX tag 270 mantissa. Optional.
  unsigned MDEntryTime;         ///< \brief FIX tag 273. Mandatory.
  bool MDEntrySize_p;           ///< \brief MDEntrySize (FIX tag 271) is present.
  long long MDEntrySize;        ///< \brief FIX tag 271. Optional.
  unsigned MDEntryDate;         ///< \brief FIX tag 272. Mandatory.
  char MDStreamID[3];           ///< \brief FIX tag 1500. Optional.
  bool NetChgPrevDay_p;         ///< \brief NetChgPrevDay (FIX tag 451) is present.
  int NetChgPrevDayExp;         ///< \brief FIX tag 451 exponent. Optional.
  long long NetChgPrevDayMant;  ///< \brief FIX tag 451 mantissa. Optional.
  char TickDirection;           ///< \brief FIX tag 274. Optional.
  char TradeCondition[15];      ///< \brief FIX tag 277. Optional.
  char TradeID[33];             ///< \brief FIX tag 1003. Optional.
  char MDEntryBuyer[51];        ///< \brief FIX tag 288. Optional.
  char MDEntrySeller[56];       ///< \brief FIX tag 289. Optional.
};

struct OpeningPriceX_t  // OpeningPrice MDEntry from "Market Data
// Incremental Refresh" Message (X)
{
  Uint64 SecurityID;             ///< \brief FIX tag 48. Mandatory
  bool RptSeq_p;                 ///< \brief RptSeq (FIX tag 83) is present
  unsigned RptSeq;               ///< \brief FIX tag 83. Optional
  bool MDEntryPx_p;              ///< \brief MDEntryPx (FIX tag 270) is present.
  int MDEntryPxExp;              ///< \brief FIX tag 270 exponent. Optional.
  long long MDEntryPxMant;       ///< \brief FIX tag 270 mantissa. Optional.
  unsigned MDEntryTime;          ///< \brief FIX tag 273. Mandatory.
  bool MDEntrySize_p;            ///< \brief MDEntrySize (FIX tag 271) is present.
  long long MDEntrySize;         ///< \brief FIX tag 271. Optional.
  unsigned MDEntryDate;          ///< \brief FIX tag 272. Mandatory.
  char MDStreamID[3];            ///< \brief FIX tag 1500. Optional.
  bool NetChgPrevDay_p;          ///< \brief NetChgPrevDay (FIX tag 451) is present.
  int NetChgPrevDayExp;          ///< \brief FIX tag 451 exponent. Optional.
  long long NetChgPrevDayMant;   ///< \brief FIX tag 451 mantissa. Optional.
  char TickDirection;            ///< \brief FIX tag 274. Optional.
  bool OpenCloseSettleFlag_p;    ///< \brief OpenCloseSettleFlag (FIX tag 286) is present.
  unsigned OpenCloseSettleFlag;  ///< \brief FIX tag 286. Optional.
};

struct ImbalanceX_t         // Imbalance MDEntry from "Market Data
{                           // Incremental Refresh" Message (X)
  Uint64 SecurityID;        ///< \brief FIX tag 48. Mandatory
  bool RptSeq_p;            ///< \brief RptSeq (FIX tag 83) is present
  unsigned RptSeq;          ///< \brief FIX tag 83. Optional
  unsigned MDEntryTime;     ///< \brief FIX tag 273. Mandatory.
  bool MDEntrySize_p;       ///< \brief MDEntrySize (FIX tag 271) is present.
  long long MDEntrySize;    ///< \brief FIX tag 271. Optional.
  unsigned MDEntryDate;     ///< \brief FIX tag 272. Mandatory.
  char TradeCondition[15];  ///< \brief FIX tag 277. Optional.
};

struct EmptyBook_t      // EmptyBook MDEntry from "Market Data
{                       // Incremental Refresh" Message (X)
  Uint64 SecurityID;    ///< \brief FIX tag 48. Mandatory
  bool RptSeq_p;        ///< \brief RptSeq (FIX tag 83) is present
  unsigned int RptSeq;  ///< \brief FIX tag 83. Optional.
};

struct MDEntryX_t               // MDEntry from "Market Data Incremental Refresh"
{                               // Message (X)
  unsigned int MDUpdateAction;  ///< \brief FIX tag 279. Mandatory
  char MDEntryType;             ///< \brief FIX tag 269. Mandatory
  union {
    BidX_t bid;                    ///< \brief Valid when \ref MDEntryType = '0' (Bid).
    OfferX_t offer;                ///< \brief Valid when \ref MDEntryType = '1' (Offer).
    TradeX_t trade;                ///< \brief Valid when \ref MDEntryType = '2' (Trade).
    OpeningPriceX_t openingPrice;  ///< \brief Valid when \ref MDEntryType = '4' (OpeningPrice). Check if your API
                                   ///version supports this MDEntryType.
    ImbalanceX_t imbalance;        ///< \brief Valid when \ref MDEntryType = 'A' (Imbalance). Check if your API version
                                   ///supports this MDEntryType.
    EmptyBook_t emptyBook;         ///< \brief Valid when \ref MDEntryType = 'J' (EmptyBook).
  };
};

typedef MDEntryX_t MDEntryX_array[512];

struct MDEntriesX_t {
  Uint64 SendingTime;         //!< SendingTime of incremental message. FIX tag 52. Mandatory.
  unsigned NoMDEntries;       //!< Number of MDEntries in the array pointed by \ref MDEntries.  FIX tag 268. Mandatory.
  MDEntryX_array *MDEntries;  //!< Pointer to an array of \ref MDEntryX_t
  unsigned msgArrivedFPGAticks;
  unsigned msgDecodedFPGAticks;
};

#pragma pack(pop) /* restore original alignment from stack */
}  // namespace PUMA_1_6
}
}  // namespace EventAPI
}  // namespace SiliconUmdf
#endif  // ifndef MDENTRYX_HPP
