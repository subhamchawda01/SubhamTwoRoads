#ifndef BASE_INDICATORS_README_H
#define BASE_INDICATORS_README_H
/**
    \file Indicators/README.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

/// basetrade/Indicators
///
/// About OfflineReturnsLRDB based variables
/// Market Data based Combo variables ignore the weights.
/// Perhaps it could be an improvement to take weights into account at some point ?
/// Portfolio weights are only used in port variables to get a PortfolioPrice
///
/// Make version of OfflineReturnsLRDB that incorporate lr_correlation_ as well.
///
/// Make version of trade variables that use EventDecayed or TradeDecayed averaging
///
/// Market Data based Indicators
///     Quote Based
///         SimpleTrend
///             SimpleTrendPort
///             SimpleTrendCombo
///         SimpleTrendMktEvents
///             SimpleTrendMktEventsPort
///             SimpleTrendMktEventsCombo
///         ScaledTrend
///             ScaledTrendPort
///             ScaledTrendCombo
///         ScaledTrendMktEvents
///             ScaledTrendMktEventsPort
///             ScaledTrendMktEventsCombo
///     Book Based
///         DiffPriceType
///             DiffPriceTypeCombo
///         BookSizeDiff
///             BookSizeDiffCombo
///         MultMidOrderPrice
///             MultMidOrderPriceCombo
///         MultMidPrice
///             MultMidPriceCombo
///         MultMktOrderPrice
///             MultMktOrderPriceCombo
///         MultMktPrice
///             MultMktPriceCombo
///         MultMktComplexOrderPrice
///             MultMktComplexOrderPriceCombo
///         MultMktComplexPrice
///             MultMktComplexPriceCombo
///         MultMktComplexPriceShortAvg
///             MultMktComplexPriceShortAvgCombo
///     Multiple Products Based
///         OnlineComputedPairs
///             OnlineComputedPairsPort
///             OnlineComputedPairsCombo
///         OnlineComputedPairsMktEvents
///             OnlineComputedPairsMktEventsPort
///             OnlineComputedPairsMktEventsCombo
///         OnlineComputedNegativelyCorrelatedPairs
///             OnlineComputedNegativelyCorrelatedPairsPort
///             OnlineComputedNegativelyCorrelatedPairsCombo
///         OnlineComputedNegativelyCorrelatedPairsMktEvents
///             OnlineComputedNegativelyCorrelatedPairsMktEventsPort
///             OnlineComputedNegativelyCorrelatedPairsMktEventsCombo
///         OfflineComputedPairs
///             OfflineComputedPairsPort
///             OfflineComputedPairsCombo
///         OfflineComputedPairsMktEvents
///             OfflineComputedPairsMktEventsPort
///             OfflineComputedPairsMktEventsCombo
///         OfflineCorradjustedPairs
///             OfflineCorradjustedPairsPort
///             OfflineCorradjustedPairsCombo
///         OfflineCorradjustedPairsMktEvents
///             OfflineCorradjustedPairsMktEventsPort
///             OfflineCorradjustedPairsMktEventsCombo
///     Trade Based ( also using Quote Data )
///         DiffTDAvgTPxBasepx
///             DiffTDAvgTPxBasepxCombo
///         DiffTDSizeAvgTPxBasepx
///             DiffTDSizeAvgTPxBasepxCombo
///         SqrtTSizeTDAvgTDiff
///             SqrtTSizeTDAvgTDiffCombo
///         TDSumTDiffFSize
///             TDSumTDiffFSizeCombo
///         TDSumTDiffFSqrtSize
///             TDSumTDiffFSqrtSizeCombo
///         TDSumTDiffNSize
///             TDSumTDiffNSizeCombo
///         TDSumTDiffSqrtFSqrtSize
///             TDSumTDiffSqrtFSqrtSizeCombo
///         TDSumTDiffSqrtTSize
///             TDSumTDiffSqrtTSizeCombo
///         TDSumTDiffTSize
///             TDSumTDiffTSizeCombo
///         TDSumTType
///             TDSumTTypeCombo
///         TSizeTDAvgTDiff
///             TSizeTDAvgTDiffCombo
///         TSizeTDSqrtAvgTDiff
///             TSizeTDSqrtAvgTDiffCombo
///     ORS Based
///         SelfPosition
///             SelfPositionCombo
///         SelfPositionSimpleTrend
///         TrendAdjustedSelfPosition
///             TrendAdjustedSelfPositionCombo

#endif  // BASE_INDICATORS_README_H
