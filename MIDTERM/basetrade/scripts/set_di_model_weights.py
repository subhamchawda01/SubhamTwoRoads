#!/usr/bin/python

from datetime import date
import sys
import os
import subprocess
import time

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.definitions import execs

# To do Not done yet to be done in Diwakar's way of updating indicators in ~/basetrade/files/IndicatorInfo/ind_class and reading from there

''' 
def IdentifyIndicatorType(line, dep):
    BookIndicators = [" MultMktComplexOrderPrice", "MultMidOrderPrice", "MultMidPrice", "MultMktComplexPriceShortAvg", "MultMktComplexPriceTopOff", "MultMktComplexPrice", "MultMktOrderPriceTopOff", "MultMktOrderPrice", "MultMktPerOrderComplexPrice", "MultMktPrice", "BidAskToPayCutoffL1",
                      "BidAskToPayCutoff", "BidAskToPayGeneralizedAllLvls", "BidAskToPayGeneralized", "BidAskToPayL1", "BidAskToPayNotionalDynamicSD", "BidAskToPayNotional", "BidAskToPay", "BookOrderCDiff", "BookOrderDiff", "BookSizeDiff", "L1Bookbias", "L1PortPrice", " L1Price", "SimpleBook"]

    TradeIndicators = ["BigTradePairImpact", "CombTDSumTDiffFSizeLvl", "CombTDSumTDiffSqrtFSqrtSize", "ConvexPositioning", "DiffEDAvgTPxBasepx", "DiffEDSizeAvgTPxBasepx", "DiffTDAvgTPxBasepxLvl", "DiffTDAvgTPxBasepxOneLvl", "DiffTDAvgTPxBasepxOneLvl", "DiffTDSizeL1SizeRatio", "DiffTDSizeAvgTPxBasepxLvl", "DiffTDSizeAvgTPxBasepxOneLvl", "DiffTDSizeAvgTPxBasepx", "DiffTRAvgTPxBasepx", "DiffTRSizeAvgTPxBasepx", "EDSumTDiffFSizeLvl", "EDSumTDiffFSize", "EDSumTDiffFSqrtSize", "EDSumTDiffNSizeLvl", "EDSumTDiffNSize", "EDSumTDiffSqrtTSize", "EDSumTDiffSqrtFSqrtSize", "EDSumTDiffTSizeLvl", "EDSumTDiffTSize", "SizeWeightedTradePriceCrossover", "SqrtTSizeTDAvgTDiffLvl", "SqrtTSizeTDAvgTDiff", "SqrtTSizeTRAvgTDiff", "TDLvlSumTTypeSqrtTSize", "TDLvlSumTType", "TDSizeAvgTTypeStTrend", "TDSumHVLBTDiffTSize",
                       "TDSumLBTDiffTSize", "TDSumTDiffFSizeLvl", "TDSumTDiffFSize", "TDSumTDiffFSqrtSize", "TDSumTDiffLvlFSqrtSize", "TDSumTDiffNSizeLvl", "TDSumTDiffNSizeOneLvl", "TDSumTDiffNSize", "TDSumTDiffSqrtTSize", "TDSumTDiffSqrtFSqrtSize", "TDSumTDiffTSizeLvl", "TDSumTDiffTSizeOneLvl", "TDSumTDiffTSize", "TDSumTradeFlow", "TDSumTTypeSqrtTSize", "TDSumTTypeTSize", "TDSumTType", "TRAvgTDiffSqrtTSizeVolfactor", "TRSumTDiffFSizeLvl", "TRSumTDiffFSize", "TRSumTDiffFSqrtSize", "TRSumTDiffNSizeLvl", "TRSumTDiffNSize", "TRSumTDiffSqrtTSize", "TRSumTDiffSqrtFSqrtSize", "TRSumTDiffTSizeLvl", "TRSumTDiffTSize", "TRSumTPriceTSize", "TRSumTTypeTSize", "TRSumTType", "TSizeEDSqrtAvgTDiff", "TSizeTDAvgTDiffLvl", "TSizeTDAvgTDiff", "TSizeTDAvgTDiff", "TSizeTDSqrtAvgTDiff", "TSizeTRAvgTDiff", "TSizeTRSqrtAvgTDiff"]

    TrendIndicators = ["ConvexTrendPort", "ConvexTrend", "CorrBasedSimpleTrend", "L1OrderTrend", "LeveredSimpleTrend", "ReturnsSimpleTrend", "ScaledTrendMktEventsPort", "ScaledTrendMktEvents", "ScaledTrendPort", "ScaledTrend", "SelfPositionSimpleTrend", "SimplePriceTrend", "SimpleReturnsMktEventsPort", "SimpleReturnsMktEvents", "SimpleReturnsPort", "SimpleReturns", "SimpleTrendIndepMktEvents", "SimpleTrendMktEventsPort", "SimpleTrendMktEvents", "SimpleTrendPort", "SimpleTrend", "StableScaledTrend2",
                       "StableScaledTrendPort", "StableScaledTrendVersion2MktEventsPort", "StableScaledTrendVersion2MktEvents", "StableScaledTrendVersion2Port", "StableScaledTrendVersion2", "StableScaledTrend", "TradeAdjustedSimpleTrend", "TradeBookAdjustedPrice", "TrendReversalMktEvents", "TrendReversal", "VolumeWeightedScaledTrendMktEvents", "VolumeWeightedScaledTrendPort", "VolumeWeightedScaledTrend", "VolumeWeightedSimpleTrendMktEvents", "VolumeWeightedSimpleTrendPort", "VolumeWeightedSimpleTrend", "VolumeWeightedStudPriceReturnsDiff"]

    CurveIndicators = ["CurveAdjustedPrice", "CurveAdjustedSimpleTrendBond", "CurveAdjustedSimpleTrendMktEventsMomentum", "CurveAdjustedSimpleTrendMktEvents", "CurveAdjustedSimpleTrendMomentumBond", "CurveAdjustedSimpleTrendMomentum", "CurveAdjustedSimpleTrend", "DI1CurveAdjustedLinearSimpleTrendMktEventsMomentum", "DI1CurveAdjustedLinearSimpleTrendMktEvents",
                       "DI1CurveAdjustedLinearSimpleTrendMomentum", "DI1CurveAdjustedLinearSimpleTrend", "DI1CurveAdjustedPrice", "DI1CurveAdjustedSimpleTrendMktEventsMomentum", "DI1CurveAdjustedSimpleTrendMktEvents", "DI1CurveAdjustedSimpleTrendMktEvents", "DI1CurveAdjustedSimpleTrend", "DI1CurveVolAdjustedTrend", "DI1LeveredSimpleTrendMktEvents", "DI1LeveredSimpleTrend"]

    RelativeIndicators = ["OfflineBreakoutAdjustedPairs2", "OfflineBreakoutAdjustedPairsPort2", "OfflineBreakoutAdjustedPairsPort", "OfflineBreakoutAdjustedPairsTrend", "OfflineBreakoutAdjustedPairs", "OfflineComputedCutoffPairsMktEventsPort", "OfflineComputedCutoffPairsMktEvents", "OfflineComputedCutoffPairsPort", "OfflineComputedCutoffPairs", "OfflineComputedPairsDynamicWeightPort", "OfflineComputedPairsMktEventsPort", "OfflineComputedPairsMktEvents", "OfflineComputedPairsMktEvents2", "OfflineComputedPairsPort", "OfflineComputedPairs", "OfflineComputedReturnsCutoffPairsMktEventsPort", "OfflineComputedReturnsCutoffPairsMktEvents", "OfflineComputedReturnsCutoffPairsPort", "OfflineComputedReturnsCutoffPairs", "OfflineComputedReturnsMult", "OfflineComputedReturnsPairsMktEventsPort", "OfflineComputedReturnsPairsMktEvents", "OfflineComputedReturnsPairsPort", "OfflineComputedReturnsPairs", "OfflineComputedSelfCutoffPairsPort", "OfflineComputedSelfCutoffPairs", "OfflineComputedSerbanPort", "OfflineComputedSerban", "OfflineComputedSmartCutoffPairs", "OfflineComputedSourceCutoffPairsPort", "OfflineComputedSourceCutoffPairs", "OfflineComputedVolumeAdjustedPairsMktEvents", "OfflineComputedVolumeAdjustedPairs", "OfflineCorradjustedCutoffPairsPort", "OfflineCorradjustedCutoffPairs", "OfflineCorradjustedPairsMktEventsPort", "OfflineCorradjustedPairsMktEvents", "OfflineCorradjustedPairsNormalizedCombo", "OfflineCorradjustedPairsPort", "OfflineCorradjustedPairs", "OfflineCorradjustedSerbanPort", "OfflineCorradjustedSerban", "OfflineLowCorrelationPairsPort", "OfflineLowCorrelationPairs", "OfflinePricePairDiff", "OfflineReturnsLRDB", "OfflineReturnsPairsDB", "OfflineReturnsRetLRDB", "OfflineVolumeCorradjustedPairsMktEvents",
                          "OfflineVolumeCorradjustedPairs", "OnlineBetaComputedPairs", "OnlineBetaKalman", "OnlineBetaTrend", "OnlineBeta", "OnlineComputedCutoffPairMktEventsPort", "OnlineComputedCutoffPairMktEvents", "OnlineComputedCutoffPairPort", "OnlineComputedCutoffPair", "OnlineComputedNegativelyCorrelatedCutoffPairMktEvents", "OnlineComputedNegativelyCorrelatedCutoffPair", "OnlineComputedNegativelyCorrelatedPairMktEventsPort", "OnlineComputedNegativelyCorrelatedPairMktEvents", "OnlineComputedNegativelyCorrelatedPairPort", "OnlineComputedNegativelyCorrelatedPair", "OnlineComputedNegativelyCorrelatedPairsMktEventsPort", "OnlineComputedNegativelyCorrelatedPairsMktEvents", "OnlineComputedNegativelyCorrelatedPairsPort", "OnlineComputedNegativelyCorrelatedPairs", "OnlineComputedPairMktEventsPort", "OnlineComputedPairMktEvents", "OnlineComputedPairPort", "OnlineComputedPairPort2", "OnlineComputedPair", "OnlineComputedPairsMktEventsPort", "OnlineComputedPairsMktEvents", "OnlineComputedPairsPort", "OnlineComputedPairs", "OnlineCorradjustedDiffPairs", "OnlineDiffPairsNK", "OnlineDiffPairs", "OnlineImpliedYen", "OnlineOfflineCorrDiffBaseRegime", "OnlineRatioPairsMktEventsPort", "OnlineRatioPairsMktEvents", "OnlineRatioPairs", "OnlineRatioProjectedPriceDiff", "OnlineRatioProjectedPrice", "ReturnsBasedProjectedPrice", "ReturnsBetaComputedPairsMktEvents", "ReturnsBetaComputedPairs", "ReturnsBetaComputedSerban", "SpotFutureSpread", "StdevAdjustedReturnsDiffNorm", "StdevAdjustedReturnsDiffPort", "StdevAdjustedReturnsDiff", "StudPriceDiffMktEvents", "StudPriceDiffPort", "StudPriceDiffPort", "StudPriceReturnsDiffPort", "StudPriceReturnsDiff", "StudPriceTrendDiffDynamicWeightPort", "StudPriceTrendDiffPort", "StudPriceTrendDiff"]

    IndicatorStringArray = line.split()
    if IndicatorStringArray[2] in BookIndicators:
        return "Book"
    elif IndictorStringArray[2] in TrendIndicators:
        if IndictorStringArray[3] == dep:
            return "SelfTrend"
        elif IndicatorStringArray[3] =="BR_DOL_0:
            return "DOLTrend"
        else:
            return "SourceTrend"
    if IndicatorStringArray[2] in RelativeIndicators:
        if IndicatorStringArray[4] == "BR_DOL_0":
            return "RelativeDOL"
        else:
            return "RelativeSource" ''''
