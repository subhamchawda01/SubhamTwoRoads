/*--------------------------------------------------------------------------------
--
-- This file is owned and controlled by MBOChip and must be used solely
-- for design, simulation, implementation and creation of design files
-- limited to MBOChip products. Do not distribute to third parties. 
--
--            *************************************************
--            ** Copyright (C) 2013, MBOChip Private Limited **
--            ** All Rights Reserved.                        **
--            *************************************************
--
--------------------------------------------------------------------------------
-- Filename: MDEntryInit.hpp
--
-- Description: 
-- 
--
*/
#ifndef MDENTRYINIT_HPP
#define MDENTRYINIT_HPP

///@cond
#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <stdint.h>
///@endcond

/*! \file MDEntryInit.hpp
    \brief MDEntries structs for initialization
*/

namespace SiliconUmdf {
namespace EventAPI {

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)

namespace BVMF {

struct BidInit_t {

    Uint64 SecurityID;          ///< \brief FIX tag 48. Mandatory

    bool RptSeq_p;              ///< \brief RptSeq (FIX tag 83) is present
    unsigned RptSeq;            ///< \brief FIX tag 83. Mandatory

    char QuoteCondition[5];     ///< \brief FIX tag 276. Optional

    bool MDEntryPx_p;           ///< \brief MDEntryPx (FIX tag 270) is present.
    int MDEntryPxExp;           ///< \brief FIX tag 270 exponent. Optional.
    long long MDEntryPxMant;    ///< \brief FIX tag 270 mantissa. Optional.

    bool NumberOfOrders_p;      ///< \brief NumberOfOrders (FIX tag 346) is present.
    unsigned NumberOfOrders;    ///< \brief FIX tag 346. Optional.

    unsigned MDEntryTime;       ///< \brief FIX tag 273. Mandatory.

    bool MDEntrySize_p;         ///< \brief MDEntrySize (FIX tag 271) is present.
    int MDEntrySize;            ///< \brief FIX tag 271. Optional.

    unsigned MDEntryDate;       ///< \brief FIX tag 272. Mandatory.
    char MDEntryBuyer[51];      ///< \brief FIX tag 288. Optional.

    unsigned MDEntryPositionNo; ///< \brief FIX tag 290. Mandatory.

    char OrderID[51];           ///< \brief FIX tag 37. Optional.
};

struct OfferInit_t	{

    Uint64 	SecurityID;              ///< \brief FIX tag 48. Mandatory.

    bool RptSeq_p;                   ///< \brief RptSeq (FIX tag 83) is present
    unsigned RptSeq;                 ///< \brief FIX tag 83. Mandatory.

    char QuoteCondition[5];          ///< \brief FIX tag 276. Optional.

    bool MDEntryPx_p;                ///< \brief MDEntryPx (FIX tag 270) is present.
    int MDEntryPxExp;                ///< \brief FIX tag 270 exponent. Optional.
    long long MDEntryPxMant;         ///< \brief FIX tag 270 mantissa. Optional.

    bool NumberOfOrders_p;           ///< \brief NumberOfOrders (FIX tag 346) is present.
    unsigned NumberOfOrders;         ///< \brief FIX tag 346. Optional.

    unsigned MDEntryTime;            ///< \brief FIX tag 273. Mandatory.

    bool MDEntrySize_p;              ///< \brief MDEntrySize (FIX tag 271) is present.
    int MDEntrySize;                 ///< \brief FIX tag 271. Optional.

    unsigned MDEntryDate;            ///< \brief FIX tag 272. Mandatory.
    char MDEntrySeller[51];          ///< \brief FIX tag 288. Optional.

    unsigned MDEntryPositionNo;      ///< \brief FIX tag 290. Mandatory.
    char OrderID[51];                ///< \brief FIX tag 37. Optional.
};

struct TradeInit_t 	{

    Uint64 SecurityID;              ///< \brief FIX tag 48. Mandatory.

    bool RptSeq_p;                  ///< \brief RptSeq (FIX tag 83) is present.
    unsigned RptSeq;                ///< \brief FIX tag 83. Optional.

    bool MDEntryPx_p;               ///< \brief MDEntryPx (FIX tag 270) is present.
    int MDEntryPxExp;               ///< \brief FIX tag 270 exponent. Optional.
    long long MDEntryPxMant;        ///< \brief FIX tag 270 mantissa. Optional.


    unsigned MDEntryTime;           ///< \brief FIX tag 273. Mandatory.

    bool MDEntrySize_p;             ///< \brief MDEntrySize (FIX tag 271) is present.
    int MDEntrySize;                ///< \brief FIX tag 271. Optional.

    unsigned MDEntryDate;           ///< \brief FIX tag 272. Mandatory.
    char MDStreamID[3];             ///< \brief FIX tag 1500. Optional.

    bool NetChgPrevDay_p;           ///< \brief NetChgPrevDay (FIX tag 451) is present.
    int NetChgPrevDayExp; 	        ///< \brief FIX tag 451 exponent. Optional.
    long long NetChgPrevDayMant;    ///< \brief FIX tag 451 mantissa. Optional.

    char TickDirection;             ///< \brief FIX tag 274. Optional.
    char TradeCondition[15];        ///< \brief FIX tag 277. Optional.
    char TradeID[33];               ///< \brief FIX tag 1003. Optional.
    char MDEntryBuyer[51];          ///< \brief FIX tag 288. Optional.
    char MDEntrySeller[51];         ///< \brief FIX tag 289. Optional.
};


struct OpeningPriceInit_t
{
    Uint64 SecurityID;                  ///< \brief FIX tag 48. Mandatory.

    bool RptSeq_p;                      ///< \brief RptSeq (FIX tag 83) is present.
    unsigned RptSeq;                    ///< \brief FIX tag 83. Optional.

    bool MDEntryPx_p;                   ///< \brief MDEntryPx (FIX tag 270) is present.
    int MDEntryPxExp;                   ///< \brief FIX tag 270 exponent. Optional.
    long long MDEntryPxMant;            ///< \brief FIX tag 270 mantissa. Optional.

    bool MDEntrySize_p;                 ///< \brief MDEntrySize (FIX tag 271) is present.
    int MDEntrySize;                    ///< \brief FIX tag 271. Optional.

    bool MDEntryInterestRate_p;         ///< \brief MDEntryInterestRate (FIX tag 37014) is present.
    int MDEntryInterestRateExp;         ///< \brief FIX tag 37014 exponent. Optional.
    long long MDEntryInterestRateMant;  ///< \brief FIX tag 37014 mantissa. Optional.

    unsigned MDEntryTime;               ///< \brief FIX tag 273. Mandatory.
    unsigned MDEntryDate;               ///< \brief FIX tag 272. Mandatory.
    char MDStreamID[3];                 ///< \brief FIX tag 1500. Optional.
    bool NetChgPrevDay_p;	            ///< \brief NetChgPrevDay (FIX tag 451) is present.
    int NetChgPrevDayExp;               ///< \brief FIX tag 451 exponent. Optional.
    long long NetChgPrevDayMant;        ///< \brief FIX tag 451 mantissa. Optional.
    char TickDirection;                 ///< \brief FIX tag 274. Optional.

    bool OpenCloseSettleFlag_p;         ///< \brief OpenCloseSettleFlag (FIX tag 286) is present.
    unsigned OpenCloseSettleFlag;       ///< \brief FIX tag 286. Optional.

};

struct ImbalanceInit_t{
    Uint64 SecurityID;              ///< \brief FIX tag 48. Mandatory.
    bool RptSeq_p;                  ///< \brief RptSeq (FIX tag 83) is present.
    unsigned RptSeq;                ///< \brief FIX tag 83. Optional.
    bool MDEntrySize_p;             ///< \brief MDEntrySize (FIX tag 271) is present.
    int MDEntrySize; 		        ///< \brief FIX tag 271. Optional.
    unsigned MDEntryTime;           ///< \brief FIX tag 273. Mandatory.
    unsigned MDEntryDate;           ///< \brief FIX tag 272. Mandatory.
    char TradeCondition[15];        ///< \brief FIX tag 277. Optional.
};



struct MDEntryInit_t {

    char     MDEntryType;                                           ///< \brief FIX tag 269. Mandatory.
    union
    {
        BidInit_t		bid;                                        ///< \brief Valid when \ref MDEntryType = '0' (Bid).
        OfferInit_t		offer;                                      ///< \brief Valid when \ref MDEntryType = '1' (Offer).
        TradeInit_t		trade;                                      ///< \brief Valid when \ref MDEntryType = '2' (Trade).
        OpeningPriceInit_t           openingPrice;                  //< \brief Valid when \ref MDEntryType = '4' (OpeningPrice). Check if your API version supports this MDEntryType.
        ImbalanceInit_t                imbalance;                   //< \brief Valid when \ref MDEntryType = '7' (TradingSessionHighPrice). Check if your API version supports this MDEntryType.
    };
};

} // namespace BVMF

#pragma pack(pop)  /* pop current alignment from stack */

} // namespace EventAPI
} // namespace SiliconUmdf
#endif
