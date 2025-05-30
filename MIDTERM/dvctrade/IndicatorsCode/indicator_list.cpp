#include "dvctrade/Indicators/indicator_list.hpp"

namespace HFSAT {

std::map<std::string, IndicatorStaticFuncStruct_t> IndicatorMap_Global;

void SetIndicatorListMap() {          // std::map<std::string, IndicatorStaticFuncStruct_t> & IndicatorMap_Global_) {
  if (IndicatorMap_Global.empty()) {  // initialize only once
    IndicatorMap_Global[ComboIndicator::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ComboIndicator>, ComboIndicator::CollectShortCodes);
    IndicatorMap_Global[SpreadPricing::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SpreadPricing>, SpreadPricing::CollectShortCodes);
    IndicatorMap_Global[AggressivePartyPrice::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<AggressivePartyPrice>, AggressivePartyPrice::CollectShortCodes);
    // IndicatorMap_Global [ BidAskToPayCombo::VarName() ]     = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<BidAskToPayCombo>, BidAskToPayCombo::CollectShortCodes );
    IndicatorMap_Global[ORSSelfExec::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ORSSelfExec>, ORSSelfExec::CollectShortCodes);
    IndicatorMap_Global[ORSSelfExecRecentOrders::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ORSSelfExecRecentOrders>, ORSSelfExecRecentOrders::CollectShortCodes);
    IndicatorMap_Global[ORSSelfExecTD::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ORSSelfExecTD>, ORSSelfExecTD::CollectShortCodes);
    IndicatorMap_Global[ORSSelfExecRecentOrdersTD::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ORSSelfExecRecentOrdersTD>, ORSSelfExecRecentOrdersTD::CollectShortCodes);
    IndicatorMap_Global[BidAskToPay::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<BidAskToPay>, BidAskToPay::CollectShortCodes);
    IndicatorMap_Global[BidAskToPayL1::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<BidAskToPayL1>, BidAskToPayL1::CollectShortCodes);
    IndicatorMap_Global[BidAskToPayCutoff::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<BidAskToPayCutoff>, BidAskToPayCutoff::CollectShortCodes);

    IndicatorMap_Global[BidAskToPayCutoffL1::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<BidAskToPayCutoffL1>, BidAskToPayCutoffL1::CollectShortCodes);
    IndicatorMap_Global[BidAskToPayNotional::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<BidAskToPayNotional>, BidAskToPayNotional::CollectShortCodes);
    IndicatorMap_Global[BidAskToPayNotionalDynamicSD::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<BidAskToPayNotionalDynamicSD>, BidAskToPayNotionalDynamicSD::CollectShortCodes);
    // IndicatorMap_Global [ BookOrderCDiffCombo::VarName() ]  = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<BookOrderCDiffCombo>, BookOrderCDiffCombo::CollectShortCodes );
    IndicatorMap_Global[BookOrderCDiff::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<BookOrderCDiff>, BookOrderCDiff::CollectShortCodes);
    // IndicatorMap_Global [ BookOrderDiffCombo::VarName() ]   = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<BookOrderDiffCombo>, BookOrderDiffCombo::CollectShortCodes );
    IndicatorMap_Global[BookOrderDiff::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<BookOrderDiff>, BookOrderDiff::CollectShortCodes);

    IndicatorMap_Global[CorrRatioCalculator::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<CorrRatioCalculator>, CorrRatioCalculator::CollectShortCodes);

    // IndicatorMap_Global [ BookSizeDiffCombo::VarName() ]    = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<BookSizeDiffCombo>, BookSizeDiffCombo::CollectShortCodes );
    IndicatorMap_Global[BookSizeDiff::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<BookSizeDiff>, BookSizeDiff::CollectShortCodes);
    IndicatorMap_Global[StdevWeightedSourceSelfTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StdevWeightedSourceSelfTrend>, StdevWeightedSourceSelfTrend::CollectShortCodes);
    IndicatorMap_Global[CombTDSumTDiffFSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<CombTDSumTDiffFSize>, CombTDSumTDiffFSize::CollectShortCodes);
    IndicatorMap_Global[CombTDSumTDiffFSizeLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<CombTDSumTDiffFSizeLvl>, CombTDSumTDiffFSizeLvl::CollectShortCodes);
    IndicatorMap_Global[CombTDSumTDiffFSqrtSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<CombTDSumTDiffFSqrtSize>, CombTDSumTDiffFSqrtSize::CollectShortCodes);
    IndicatorMap_Global[CombTDSumTDiffSqrtFSqrtSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<CombTDSumTDiffSqrtFSqrtSize>, CombTDSumTDiffSqrtFSqrtSize::CollectShortCodes);
    IndicatorMap_Global[DiffEDAvgTPxBasepx::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DiffEDAvgTPxBasepx>, DiffEDAvgTPxBasepx::CollectShortCodes);
    IndicatorMap_Global[DiffEDSizeAvgTPxBasepx::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DiffEDSizeAvgTPxBasepx>, DiffEDSizeAvgTPxBasepx::CollectShortCodes);
    // IndicatorMap_Global [ DiffPairPriceTypeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<DiffPairPriceTypeCombo>, DiffPairPriceTypeCombo::CollectShortCodes );
    IndicatorMap_Global[DiffPairPriceType::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<DiffPairPriceType>, DiffPairPriceType::CollectShortCodes);
    IndicatorMap_Global[DiffPriceL1::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<DiffPriceL1>, DiffPriceL1::CollectShortCodes);
    IndicatorMap_Global[DiffPriceEBS::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<DiffPriceEBS>, DiffPriceEBS::CollectShortCodes);
    IndicatorMap_Global[OwpDiffPriceL1::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OwpDiffPriceL1>, OwpDiffPriceL1::CollectShortCodes);
    IndicatorMap_Global[TimeOwpDiffPriceL1::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TimeOwpDiffPriceL1>, TimeOwpDiffPriceL1::CollectShortCodes);
    IndicatorMap_Global[EventOwpDiffPriceL1::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<EventOwpDiffPriceL1>, EventOwpDiffPriceL1::CollectShortCodes);
    IndicatorMap_Global[DynamicPriceL1::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<DynamicPriceL1>, DynamicPriceL1::CollectShortCodes);

    // IndicatorMap_Global [ DiffPriceTypeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<DiffPriceTypeCombo>, DiffPriceTypeCombo::CollectShortCodes );
    IndicatorMap_Global[DiffPriceType::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<DiffPriceType>, DiffPriceType::CollectShortCodes);
    IndicatorMap_Global[DiffPriceTypeSpread::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DiffPriceTypeSpread>, DiffPriceTypeSpread::CollectShortCodes);
    // IndicatorMap_Global [ DiffTDAvgTPxBasepxCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<DiffTDAvgTPxBasepxCombo>, DiffTDAvgTPxBasepxCombo::CollectShortCodes );
    IndicatorMap_Global[DiffTDAvgTPxBasepx::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DiffTDAvgTPxBasepx>, DiffTDAvgTPxBasepx::CollectShortCodes);
    // IndicatorMap_Global [ DiffTDAvgTPxBasepxLvlCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<DiffTDAvgTPxBasepxLvlCombo>, DiffTDAvgTPxBasepxLvlCombo::CollectShortCodes );
    IndicatorMap_Global[DiffTDAvgTPxBasepxLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DiffTDAvgTPxBasepxLvl>, DiffTDAvgTPxBasepxLvl::CollectShortCodes);
    // IndicatorMap_Global [ DiffTDAvgTPxBasepxOneLvlCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<DiffTDAvgTPxBasepxOneLvlCombo>, DiffTDAvgTPxBasepxOneLvlCombo::CollectShortCodes );
    IndicatorMap_Global[DiffTDAvgTPxBasepxOneLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DiffTDAvgTPxBasepxOneLvl>, DiffTDAvgTPxBasepxOneLvl::CollectShortCodes);
    // IndicatorMap_Global [ DiffTDSizeAvgTPxBasepxCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<DiffTDSizeAvgTPxBasepxCombo>, DiffTDSizeAvgTPxBasepxCombo::CollectShortCodes );
    IndicatorMap_Global[DiffTDSizeAvgTPxBasepx::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DiffTDSizeAvgTPxBasepx>, DiffTDSizeAvgTPxBasepx::CollectShortCodes);
    // IndicatorMap_Global [ DiffTDSizeAvgTPxBasepxLvlCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<DiffTDSizeAvgTPxBasepxLvlCombo>, DiffTDSizeAvgTPxBasepxLvlCombo::CollectShortCodes );
    IndicatorMap_Global[DiffTDSizeAvgTPxBasepxLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DiffTDSizeAvgTPxBasepxLvl>, DiffTDSizeAvgTPxBasepxLvl::CollectShortCodes);
    IndicatorMap_Global[DiffTDSizeAvgTPxBasepxOneLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DiffTDSizeAvgTPxBasepxOneLvl>, DiffTDSizeAvgTPxBasepxOneLvl::CollectShortCodes);
    // IndicatorMap_Global [ DiffTRAvgTPxBasepxCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<DiffTRAvgTPxBasepxCombo>, DiffTRAvgTPxBasepxCombo::CollectShortCodes );
    IndicatorMap_Global[DiffTRAvgTPxBasepx::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DiffTRAvgTPxBasepx>, DiffTRAvgTPxBasepx::CollectShortCodes);
    // IndicatorMap_Global [ DiffTRSizeAvgTPxBasepxCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<DiffTRSizeAvgTPxBasepxCombo>, DiffTRSizeAvgTPxBasepxCombo::CollectShortCodes );
    IndicatorMap_Global[DiffTRSizeAvgTPxBasepx::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DiffTRSizeAvgTPxBasepx>, DiffTRSizeAvgTPxBasepx::CollectShortCodes);
    IndicatorMap_Global[EDSumTDiffFSize::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<EDSumTDiffFSize>, EDSumTDiffFSize::CollectShortCodes);
    IndicatorMap_Global[EDSumTDiffFSizeLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<EDSumTDiffFSizeLvl>, EDSumTDiffFSizeLvl::CollectShortCodes);
    IndicatorMap_Global[EDSumTDiffFSqrtSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<EDSumTDiffFSqrtSize>, EDSumTDiffFSqrtSize::CollectShortCodes);
    IndicatorMap_Global[EDSumTDiffNSize::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<EDSumTDiffNSize>, EDSumTDiffNSize::CollectShortCodes);
    IndicatorMap_Global[EDSumTDiffNSizeLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<EDSumTDiffNSizeLvl>, EDSumTDiffNSizeLvl::CollectShortCodes);
    IndicatorMap_Global[EDSumTDiffSqrtFSqrtSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<EDSumTDiffSqrtFSqrtSize>, EDSumTDiffSqrtFSqrtSize::CollectShortCodes);
    IndicatorMap_Global[EDSumTDiffSqrtTSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<EDSumTDiffSqrtTSize>, EDSumTDiffSqrtTSize::CollectShortCodes);
    IndicatorMap_Global[EDSumTDiffTSize::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<EDSumTDiffTSize>, EDSumTDiffTSize::CollectShortCodes);
    IndicatorMap_Global[EDSumTDiffTSizeLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<EDSumTDiffTSizeLvl>, EDSumTDiffTSizeLvl::CollectShortCodes);
    IndicatorMap_Global[ImpliedPrice::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ImpliedPrice>, ImpliedPrice::CollectShortCodes);
    IndicatorMap_Global[ImpliedPriceMPS::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ImpliedPriceMPS>, ImpliedPriceMPS::CollectShortCodes);
    IndicatorMap_Global[ImpliedPriceSpread::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ImpliedPriceSpread>, ImpliedPriceSpread::CollectShortCodes);
    // IndicatorMap_Global [ MultMidOrderPriceCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<MultMidOrderPriceCombo>, MultMidOrderPriceCombo::CollectShortCodes );
    IndicatorMap_Global[MultMidOrderPrice::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<MultMidOrderPrice>, MultMidOrderPrice::CollectShortCodes);
    // IndicatorMap_Global [ MultMidPriceCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<MultMidPriceCombo>, MultMidPriceCombo::CollectShortCodes );
    IndicatorMap_Global[MultMidPrice::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<MultMidPrice>, MultMidPrice::CollectShortCodes);
    // IndicatorMap_Global [ MultMktComplexOrderPriceCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<MultMktComplexOrderPriceCombo>, MultMktComplexOrderPriceCombo::CollectShortCodes );
    IndicatorMap_Global[MultMktComplexOrderPrice::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<MultMktComplexOrderPrice>, MultMktComplexOrderPrice::CollectShortCodes);
    // IndicatorMap_Global [ MultMktComplexPriceCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<MultMktComplexPriceCombo>, MultMktComplexPriceCombo::CollectShortCodes );
    IndicatorMap_Global[MultMktComplexPrice::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<MultMktComplexPrice>, MultMktComplexPrice::CollectShortCodes);
    IndicatorMap_Global[MultMktPerOrderComplexPrice::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<MultMktPerOrderComplexPrice>, MultMktPerOrderComplexPrice::CollectShortCodes);
    // IndicatorMap_Global [ MultMktComplexPriceShortAvgCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<MultMktComplexPriceShortAvgCombo>, MultMktComplexPriceShortAvgCombo::CollectShortCodes
    // );
    IndicatorMap_Global[MultMktComplexPriceShortAvg::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<MultMktComplexPriceShortAvg>, MultMktComplexPriceShortAvg::CollectShortCodes);
    // IndicatorMap_Global [ MultMktComplexPriceTopOffCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<MultMktComplexPriceTopOffCombo>, MultMktComplexPriceTopOffCombo::CollectShortCodes );
    IndicatorMap_Global[MultMktComplexPriceTopOff::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<MultMktComplexPriceTopOff>, MultMktComplexPriceTopOff::CollectShortCodes);
    // IndicatorMap_Global [ MultMktOrderPriceCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<MultMktOrderPriceCombo>, MultMktOrderPriceCombo::CollectShortCodes );
    IndicatorMap_Global[MultMktOrderPrice::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<MultMktOrderPrice>, MultMktOrderPrice::CollectShortCodes);
    // IndicatorMap_Global [ MultMktOrderPriceTopOffCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<MultMktOrderPriceTopOffCombo>, MultMktOrderPriceTopOffCombo::CollectShortCodes );
    IndicatorMap_Global[MultMktOrderPriceTopOff::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<MultMktOrderPriceTopOff>, MultMktOrderPriceTopOff::CollectShortCodes);
    // IndicatorMap_Global [ MultMktPriceCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<MultMktPriceCombo>, MultMktPriceCombo::CollectShortCodes );
    IndicatorMap_Global[MultMktPrice::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<MultMktPrice>, MultMktPrice::CollectShortCodes);
    // IndicatorMap_Global [ OfflineComputedCutoffPairsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OfflineComputedCutoffPairsCombo>, OfflineComputedCutoffPairsCombo::CollectShortCodes );
    IndicatorMap_Global[OfflineComputedCutoffPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineComputedCutoffPairs>, OfflineComputedCutoffPairs::CollectShortCodes);
    IndicatorMap_Global[PCRPortReturns::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<PCRPortReturns>, PCRPortReturns::CollectShortCodes);
    IndicatorMap_Global[OffPortReturns::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OffPortReturns>, OffPortReturns::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedSmartCutoffPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineComputedSmartCutoffPairs>, OfflineComputedSmartCutoffPairs::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedCutoffPairsMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineComputedCutoffPairsMktEvents>,
                                    OfflineComputedCutoffPairsMktEvents::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedCutoffPairsMktEventsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineComputedCutoffPairsMktEventsPort>,
                                    OfflineComputedCutoffPairsMktEventsPort::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedCutoffPairsPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineComputedCutoffPairsPort>, OfflineComputedCutoffPairsPort::CollectShortCodes);

    IndicatorMap_Global[OfflineComputedSourceCutoffPairs::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineComputedSourceCutoffPairs>,
                                    OfflineComputedSourceCutoffPairs::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedSelfCutoffPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineComputedSelfCutoffPairs>, OfflineComputedSelfCutoffPairs::CollectShortCodes);

    IndicatorMap_Global[OfflineComputedSourceCutoffPairsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineComputedSourceCutoffPairsPort>,
                                    OfflineComputedSourceCutoffPairsPort::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedSelfCutoffPairsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineComputedSelfCutoffPairsPort>,
                                    OfflineComputedSelfCutoffPairsPort::CollectShortCodes);

    // IndicatorMap_Global [ OfflineComputedPairsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OfflineComputedPairsCombo>, OfflineComputedPairsCombo::CollectShortCodes );
    IndicatorMap_Global[OfflineComputedPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineComputedPairs>, OfflineComputedPairs::CollectShortCodes);
    IndicatorMap_Global[NotionalBasedLeadLag::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<NotionalBasedLeadLag>, NotionalBasedLeadLag::CollectShortCodes);
    IndicatorMap_Global[NotionalTraded::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<NotionalTraded>, NotionalTraded::CollectShortCodes);
    // IndicatorMap_Global [ OfflineComputedPairsMktEventsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OfflineComputedPairsMktEventsCombo>,
    // OfflineComputedPairsMktEventsCombo::CollectShortCodes );
    IndicatorMap_Global[OfflineComputedPairsMktEvents::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineComputedPairsMktEvents>, OfflineComputedPairsMktEvents::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedPairsMktEvents2::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineComputedPairsMktEvents2>, OfflineComputedPairsMktEvents2::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedPairsMktEventsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineComputedPairsMktEventsPort>,
                                    OfflineComputedPairsMktEventsPort::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedPairsPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineComputedPairsPort>, OfflineComputedPairsPort::CollectShortCodes);
    // IndicatorMap_Global [ OfflineComputedVolumeAdjustedPairsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OfflineComputedVolumeAdjustedPairsCombo>,
    // OfflineComputedVolumeAdjustedPairsCombo::CollectShortCodes );
    IndicatorMap_Global[OfflineComputedVolumeAdjustedPairs::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineComputedVolumeAdjustedPairs>,
                                    OfflineComputedVolumeAdjustedPairs::CollectShortCodes);
    // IndicatorMap_Global [ OfflineComputedVolumeAdjustedPairsMktEventsCombo::VarName() ] = IndicatorStaticFuncStruct_t
    // ( GetUniqueInstanceWrapper<OfflineComputedVolumeAdjustedPairsMktEventsCombo>,
    // OfflineComputedVolumeAdjustedPairsMktEventsCombo::CollectShortCodes );
    IndicatorMap_Global[OfflineComputedVolumeAdjustedPairsMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineComputedVolumeAdjustedPairsMktEvents>,
                                    OfflineComputedVolumeAdjustedPairsMktEvents::CollectShortCodes);
    IndicatorMap_Global[OfflineBreakoutAdjustedPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineBreakoutAdjustedPairs>, OfflineBreakoutAdjustedPairs::CollectShortCodes);
    IndicatorMap_Global[OfflineBreakoutAdjustedPairsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineBreakoutAdjustedPairsPort>,
                                    OfflineBreakoutAdjustedPairsPort::CollectShortCodes);
    IndicatorMap_Global[OfflineBreakoutAdjustedPairs2::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineBreakoutAdjustedPairs2>, OfflineBreakoutAdjustedPairs2::CollectShortCodes);
    IndicatorMap_Global[OfflineBreakoutAdjustedPairsPort2::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineBreakoutAdjustedPairsPort2>,
                                    OfflineBreakoutAdjustedPairsPort2::CollectShortCodes);
    IndicatorMap_Global[OfflineBreakoutAdjustedPairsTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineBreakoutAdjustedPairsTrend>,
                                    OfflineBreakoutAdjustedPairsTrend::CollectShortCodes);
    IndicatorMap_Global[TrendVolBasedRegime::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TrendVolBasedRegime>, TrendVolBasedRegime::CollectShortCodes);
    IndicatorMap_Global[TrendBasedRegime::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TrendBasedRegime>, TrendBasedRegime::CollectShortCodes);

    IndicatorMap_Global[TrendVolBasedRegimePort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TrendVolBasedRegimePort>, TrendVolBasedRegimePort::CollectShortCodes);

    IndicatorMap_Global[ExponentialMovingAverage::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ExponentialMovingAverage>, ExponentialMovingAverage::CollectShortCodes);
    IndicatorMap_Global[AvgAbsTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<AvgAbsTrend>, AvgAbsTrend::CollectShortCodes);
    IndicatorMap_Global[AdaptiveMovingAverage::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<AdaptiveMovingAverage>, AdaptiveMovingAverage::CollectShortCodes);
    IndicatorMap_Global[EMACrossover::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<EMACrossover>, EMACrossover::CollectShortCodes);
    IndicatorMap_Global[SizeWeightedTradePriceCrossover::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SizeWeightedTradePriceCrossover>, SizeWeightedTradePriceCrossover::CollectShortCodes);

    IndicatorMap_Global[StdevSimpleTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<StdevSimpleTrend>, StdevSimpleTrend::CollectShortCodes);
    IndicatorMap_Global[StdevL1Bias::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<StdevL1Bias>, StdevL1Bias::CollectShortCodes);
    IndicatorMap_Global[StdevSimpleReturns::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StdevSimpleReturns>, StdevSimpleReturns::CollectShortCodes);
    IndicatorMap_Global[VXStdevAdjustedSimpleTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<VXStdevAdjustedSimpleTrend>, VXStdevAdjustedSimpleTrend::CollectShortCodes);
    IndicatorMap_Global[VolumeWeightedSimpleTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<VolumeWeightedSimpleTrend>, VolumeWeightedSimpleTrend::CollectShortCodes);
    IndicatorMap_Global[VolumeWeightedSimpleTrendMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<VolumeWeightedSimpleTrendMktEvents>,
                                    VolumeWeightedSimpleTrendMktEvents::CollectShortCodes);
    IndicatorMap_Global[VolumeWeightedScaledTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<VolumeWeightedScaledTrend>, VolumeWeightedScaledTrend::CollectShortCodes);
    IndicatorMap_Global[VolumeWeightedScaledTrendMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<VolumeWeightedScaledTrendMktEvents>,
                                    VolumeWeightedScaledTrendMktEvents::CollectShortCodes);
    IndicatorMap_Global[VolumeWeightedSimpleTrendPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<VolumeWeightedSimpleTrendPort>, VolumeWeightedSimpleTrendPort::CollectShortCodes);
    IndicatorMap_Global[VolumeWeightedScaledTrendPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<VolumeWeightedScaledTrendPort>, VolumeWeightedScaledTrendPort::CollectShortCodes);
    IndicatorMap_Global[SimpleTrendRegime::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SimpleTrendRegime>, SimpleTrendRegime::CollectShortCodes);
    IndicatorMap_Global[TrendingMeanReverting::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TrendingMeanReverting>, TrendingMeanReverting::CollectShortCodes);
    IndicatorMap_Global[OfflineCorradjustedCutoffPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineCorradjustedCutoffPairs>, OfflineCorradjustedCutoffPairs::CollectShortCodes);
    IndicatorMap_Global[OfflineCorradjustedCutoffPairsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineCorradjustedCutoffPairsPort>,
                                    OfflineCorradjustedCutoffPairsPort::CollectShortCodes);
    // IndicatorMap_Global [ OfflineCorradjustedPairsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OfflineCorradjustedPairsCombo>, OfflineCorradjustedPairsCombo::CollectShortCodes );
    IndicatorMap_Global[OfflineCorradjustedPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineCorradjustedPairs>, OfflineCorradjustedPairs::CollectShortCodes);
    // IndicatorMap_Global [ OfflineCorradjustedPairsMktEventsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OfflineCorradjustedPairsMktEventsCombo>,
    // OfflineCorradjustedPairsMktEventsCombo::CollectShortCodes );
    IndicatorMap_Global[OfflineCorradjustedPairsMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineCorradjustedPairsMktEvents>,
                                    OfflineCorradjustedPairsMktEvents::CollectShortCodes);
    IndicatorMap_Global[OfflineCorradjustedPairsMktEventsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineCorradjustedPairsMktEventsPort>,
                                    OfflineCorradjustedPairsMktEventsPort::CollectShortCodes);
    IndicatorMap_Global[OfflineCorradjustedPairsNormalizedCombo::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineCorradjustedPairsNormalizedCombo>,
        OfflineCorradjustedPairsNormalizedCombo::CollectShortCodes);  // No clue how to handle this....
    IndicatorMap_Global[OfflineCorradjustedPairsPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineCorradjustedPairsPort>, OfflineCorradjustedPairsPort::CollectShortCodes);
    // IndicatorMap_Global [ OfflineLowCorrelationPairsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OfflineLowCorrelationPairsCombo>, OfflineLowCorrelationPairsCombo::CollectShortCodes );
    IndicatorMap_Global[OfflineLowCorrelationPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineLowCorrelationPairs>, OfflineLowCorrelationPairs::CollectShortCodes);
    IndicatorMap_Global[OfflineLowCorrelationPairsPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineLowCorrelationPairsPort>, OfflineLowCorrelationPairsPort::CollectShortCodes);
    // IndicatorMap_Global [ OfflineVolumeCorradjustedPairsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OfflineVolumeCorradjustedPairsCombo>,
    // OfflineVolumeCorradjustedPairsCombo::CollectShortCodes );
    IndicatorMap_Global[OfflineVolumeCorradjustedPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineVolumeCorradjustedPairs>, OfflineVolumeCorradjustedPairs::CollectShortCodes);
    // IndicatorMap_Global [ OfflineVolumeCorradjustedPairsMktEventsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OfflineVolumeCorradjustedPairsMktEventsCombo>,
    // OfflineVolumeCorradjustedPairsMktEventsCombo::CollectShortCodes );
    IndicatorMap_Global[OfflineVolumeCorradjustedPairsMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineVolumeCorradjustedPairsMktEvents>,
                                    OfflineVolumeCorradjustedPairsMktEvents::CollectShortCodes);
    // IndicatorMap_Global [ OnlineComputedCutoffPairCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OnlineComputedCutoffPairCombo>, OnlineComputedCutoffPairCombo::CollectShortCodes );
    IndicatorMap_Global[OnlineComputedCutoffPair::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineComputedCutoffPair>, OnlineComputedCutoffPair::CollectShortCodes);
    // IndicatorMap_Global [ OnlineComputedCutoffPairMktEventsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OnlineComputedCutoffPairMktEventsCombo>,
    // OnlineComputedCutoffPairMktEventsCombo::CollectShortCodes );
    IndicatorMap_Global[OnlineComputedCutoffPairMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineComputedCutoffPairMktEvents>,
                                    OnlineComputedCutoffPairMktEvents::CollectShortCodes);
    IndicatorMap_Global[OnlineComputedCutoffPairMktEventsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineComputedCutoffPairMktEventsPort>,
                                    OnlineComputedCutoffPairMktEventsPort::CollectShortCodes);
    IndicatorMap_Global[OnlineComputedNegativelyCorrelatedCutoffPair::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineComputedNegativelyCorrelatedCutoffPair>,
                                    OnlineComputedNegativelyCorrelatedCutoffPair::CollectShortCodes);
    IndicatorMap_Global[OnlineComputedNegativelyCorrelatedCutoffPairMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineComputedNegativelyCorrelatedCutoffPairMktEvents>,
                                    OnlineComputedNegativelyCorrelatedCutoffPairMktEvents::CollectShortCodes);
    // IndicatorMap_Global [ OnlineComputedNegativelyCorrelatedPairCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OnlineComputedNegativelyCorrelatedPairCombo>,
    // OnlineComputedNegativelyCorrelatedPairCombo::CollectShortCodes );
    // IndicatorMap_Global [ OnlineComputedNegativelyCorrelatedPairMktEventsCombo::VarName() ] =
    // IndicatorStaticFuncStruct_t ( GetUniqueInstanceWrapper<OnlineComputedNegativelyCorrelatedPairMktEventsCombo>,
    // OnlineComputedNegativelyCorrelatedPairMktEventsCombo::CollectShortCodes );
    IndicatorMap_Global[OnlineComputedNegativelyCorrelatedPair::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineComputedNegativelyCorrelatedPair>,
                                    OnlineComputedNegativelyCorrelatedPair::CollectShortCodes);
    IndicatorMap_Global[OnlineComputedNegativelyCorrelatedPairMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineComputedNegativelyCorrelatedPairMktEvents>,
                                    OnlineComputedNegativelyCorrelatedPairMktEvents::CollectShortCodes);
    IndicatorMap_Global[OnlineComputedNegativelyCorrelatedPairMktEventsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineComputedNegativelyCorrelatedPairMktEventsPort>,
                                    OnlineComputedNegativelyCorrelatedPairMktEventsPort::CollectShortCodes);
    IndicatorMap_Global[OnlineComputedNegativelyCorrelatedPairPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineComputedNegativelyCorrelatedPairPort>,
                                    OnlineComputedNegativelyCorrelatedPairPort::CollectShortCodes);
    // IndicatorMap_Global [ OnlineComputedNegativelyCorrelatedPairsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OnlineComputedNegativelyCorrelatedPairsCombo>,
    // OnlineComputedNegativelyCorrelatedPairsCombo::CollectShortCodes );
    // IndicatorMap_Global [ OnlineComputedNegativelyCorrelatedPairsMktEventsCombo::VarName() ] =
    // IndicatorStaticFuncStruct_t ( GetUniqueInstanceWrapper<OnlineComputedNegativelyCorrelatedPairsMktEventsCombo>,
    // OnlineComputedNegativelyCorrelatedPairsMktEventsCombo::CollectShortCodes );
    IndicatorMap_Global[OnlineComputedNegativelyCorrelatedPairs::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineComputedNegativelyCorrelatedPairs>,
                                    OnlineComputedNegativelyCorrelatedPairs::CollectShortCodes);
    IndicatorMap_Global[OnlineComputedNegativelyCorrelatedPairsMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineComputedNegativelyCorrelatedPairsMktEvents>,
                                    OnlineComputedNegativelyCorrelatedPairsMktEvents::CollectShortCodes);
    IndicatorMap_Global[OnlineComputedNegativelyCorrelatedPairsMktEventsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineComputedNegativelyCorrelatedPairsMktEventsPort>,
                                    OnlineComputedNegativelyCorrelatedPairsMktEventsPort::CollectShortCodes);
    IndicatorMap_Global[OnlineComputedNegativelyCorrelatedPairsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineComputedNegativelyCorrelatedPairsPort>,
                                    OnlineComputedNegativelyCorrelatedPairsPort::CollectShortCodes);
    // IndicatorMap_Global [ OnlineComputedPairCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OnlineComputedPairCombo>, OnlineComputedPairCombo::CollectShortCodes );
    IndicatorMap_Global[OnlineComputedPair::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineComputedPair>, OnlineComputedPair::CollectShortCodes);
    // IndicatorMap_Global [ OnlineComputedPairMktEventsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OnlineComputedPairMktEventsCombo>, OnlineComputedPairMktEventsCombo::CollectShortCodes
    // );
    IndicatorMap_Global[OnlineComputedPairMktEvents::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineComputedPairMktEvents>, OnlineComputedPairMktEvents::CollectShortCodes);
    IndicatorMap_Global[OnlineComputedPairMktEventsPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineComputedPairMktEventsPort>, OnlineComputedPairMktEventsPort::CollectShortCodes);
    IndicatorMap_Global[OnlineComputedPairPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineComputedPairPort>, OnlineComputedPairPort::CollectShortCodes);
    IndicatorMap_Global[OnlineComputedPairPort2::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineComputedPairPort2>, OnlineComputedPairPort2::CollectShortCodes);
    // IndicatorMap_Global [ OnlineComputedPairsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OnlineComputedPairsCombo>, OnlineComputedPairsCombo::CollectShortCodes );
    // IndicatorMap_Global [ OnlineComputedPairsMktEventsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OnlineComputedPairsMktEventsCombo>, OnlineComputedPairsMktEventsCombo::CollectShortCodes
    // );
    IndicatorMap_Global[OnlineComputedPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineComputedPairs>, OnlineComputedPairs::CollectShortCodes);
    IndicatorMap_Global[OnlineComputedPairsMktEvents::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineComputedPairsMktEvents>, OnlineComputedPairsMktEvents::CollectShortCodes);
    IndicatorMap_Global[OnlineComputedPairsMktEventsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineComputedPairsMktEventsPort>,
                                    OnlineComputedPairsMktEventsPort::CollectShortCodes);
    IndicatorMap_Global[OnlineComputedPairsPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineComputedPairsPort>, OnlineComputedPairsPort::CollectShortCodes);
    // IndicatorMap_Global [ OnlineRatioPairsComboFilter::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OnlineRatioPairsComboFilter>, OnlineRatioPairsComboFilter::CollectShortCodes );
    // IndicatorMap_Global [ OnlineRatioPairsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OnlineRatioPairsCombo>, OnlineRatioPairsCombo::CollectShortCodes );
    IndicatorMap_Global[OnlineRatioPairsFilter::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineRatioPairsFilter>, OnlineRatioPairsFilter::CollectShortCodes);
    IndicatorMap_Global[OnlineRatioPairs::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineRatioPairs>, OnlineRatioPairs::CollectShortCodes);
    IndicatorMap_Global[OnlineRatioProjectedPrice::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineRatioProjectedPrice>, OnlineRatioProjectedPrice::CollectShortCodes);
    IndicatorMap_Global[OnlineRatioProjectedPriceDiff::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineRatioProjectedPriceDiff>, OnlineRatioProjectedPriceDiff::CollectShortCodes);

    // IndicatorMap_Global [ OnlineRatioPairsMktEventsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OnlineRatioPairsMktEventsCombo>, OnlineRatioPairsMktEventsCombo::CollectShortCodes );
    IndicatorMap_Global[OnlineRatioPairsMktEvents::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineRatioPairsMktEvents>, OnlineRatioPairsMktEvents::CollectShortCodes);
    IndicatorMap_Global[OnlineRatioPairsMktEventsPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineRatioPairsMktEventsPort>, OnlineRatioPairsMktEventsPort::CollectShortCodes);
    IndicatorMap_Global[OnlineRatioPairsPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineRatioPairsPort>, OnlineRatioPairsPort::CollectShortCodes);
    IndicatorMap_Global[OnlineDiffPairs::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineDiffPairs>, OnlineDiffPairs::CollectShortCodes);
    IndicatorMap_Global[OnlineDiffPairsNK::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineDiffPairsNK>, OnlineDiffPairsNK::CollectShortCodes);
    IndicatorMap_Global[OnlineCorradjustedDiffPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineCorradjustedDiffPairs>, OnlineCorradjustedDiffPairs::CollectShortCodes);
    IndicatorMap_Global[OnlineBeta::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineBeta>, OnlineBeta::CollectShortCodes);
    IndicatorMap_Global[OnlineBetaTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineBetaTrend>, OnlineBetaTrend::CollectShortCodes);
    IndicatorMap_Global[OnlineBetaKalman::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineBetaKalman>, OnlineBetaKalman::CollectShortCodes);
    IndicatorMap_Global[OnlineBetaComputedPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineBetaComputedPairs>, OnlineBetaComputedPairs::CollectShortCodes);
    // IndicatorMap_Global [ OrderWBPMomentumCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<OrderWBPMomentumCombo>, OrderWBPMomentumCombo::CollectShortCodes );
    IndicatorMap_Global[OrderWBPMomentum::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OrderWBPMomentum>, OrderWBPMomentum::CollectShortCodes);
    IndicatorMap_Global[OrderWBPMomentum2::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OrderWBPMomentum2>, OrderWBPMomentum2::CollectShortCodes);

    IndicatorMap_Global[PricePortfolio::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<PricePortfolio>, PricePortfolio::CollectShortCodes);
    IndicatorMap_Global[PCADeviationPairsPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<PCADeviationPairsPort>, PCADeviationPairsPort::CollectShortCodes);
    IndicatorMap_Global[VolumePCADeviationPairsPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<VolumePCADeviationPairsPort>, VolumePCADeviationPairsPort::CollectShortCodes);
    IndicatorMap_Global[VolumePCADeviationPairsPortMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<VolumePCADeviationPairsPortMktEvents>,
                                    VolumePCADeviationPairsPortMktEvents::CollectShortCodes);
    IndicatorMap_Global[VolumePCADeviationPairsPort2::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<VolumePCADeviationPairsPort2>, VolumePCADeviationPairsPort2::CollectShortCodes);
    // IndicatorMap_Global [ ScaledTrendCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<ScaledTrendCombo>, ScaledTrendCombo::CollectShortCodes );
    IndicatorMap_Global[ScaledTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ScaledTrend>, ScaledTrend::CollectShortCodes);
    // IndicatorMap_Global [ ScaledTrendMktEventsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<ScaledTrendMktEventsCombo>, ScaledTrendMktEventsCombo::CollectShortCodes );
    IndicatorMap_Global[ScaledTrendMktEvents::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ScaledTrendMktEvents>, ScaledTrendMktEvents::CollectShortCodes);
    IndicatorMap_Global[ScaledTrendMktEventsPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ScaledTrendMktEventsPort>, ScaledTrendMktEventsPort::CollectShortCodes);
    IndicatorMap_Global[ScaledTrendPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ScaledTrendPort>, ScaledTrendPort::CollectShortCodes);
    // IndicatorMap_Global [ SecondDerivativeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<SecondDerivativeCombo>, SecondDerivativeCombo::CollectShortCodes );
    IndicatorMap_Global[SecondDerivative::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SecondDerivative>, SecondDerivative::CollectShortCodes);
    // IndicatorMap_Global [ SelfPositionCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<SelfPositionCombo>, SelfPositionCombo::CollectShortCodes );
    IndicatorMap_Global[SelfPosition::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SelfPosition>, SelfPosition::CollectShortCodes);
    IndicatorMap_Global[SelfPositionSimpleTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SelfPositionSimpleTrend>, SelfPositionSimpleTrend::CollectShortCodes);
    IndicatorMap_Global[SimpleBook::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SimpleBook>, SimpleBook::CollectShortCodes);
    IndicatorMap_Global[SimplePriceType::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SimplePriceType>, SimplePriceType::CollectShortCodes);

    // IndicatorMap_Global [ SimpleTrendCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<SimpleTrendCombo>, SimpleTrendCombo::CollectShortCodes );
    IndicatorMap_Global[L1BidAskSizeFlow::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<L1BidAskSizeFlow>, L1BidAskSizeFlow::CollectShortCodes);
    IndicatorMap_Global[L1BidAskSizeFlowMini::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<L1BidAskSizeFlowMini>, L1BidAskSizeFlowMini::CollectShortCodes);
    IndicatorMap_Global[L1SizeTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<L1SizeTrend>, L1SizeTrend::CollectShortCodes);
    IndicatorMap_Global[L1OrderTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<L1OrderTrend>, L1OrderTrend::CollectShortCodes);

    IndicatorMap_Global[SimpleTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SimpleTrend>, SimpleTrend::CollectShortCodes);
    IndicatorMap_Global[SimpleSpread::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SimpleSpread>, SimpleSpread::CollectShortCodes);
    IndicatorMap_Global[SimpleSpreadTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SimpleSpreadTrend>, SimpleSpreadTrend::CollectShortCodes);
    IndicatorMap_Global[SimpleReturns::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SimpleReturns>, SimpleReturns::CollectShortCodes);
    IndicatorMap_Global[SimpleReturnsMktEvents::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SimpleReturnsMktEvents>, SimpleReturnsMktEvents::CollectShortCodes);
    IndicatorMap_Global[SimpleMean::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SimpleMean>, SimpleMean::CollectShortCodes);
    IndicatorMap_Global[SimplePriceTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SimplePriceTrend>, SimplePriceTrend::CollectShortCodes);
    IndicatorMap_Global[SimpleHawkesPriceProcessMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SimpleHawkesPriceProcessMktEvents>,
                                    SimpleHawkesPriceProcessMktEvents::CollectShortCodes);
    IndicatorMap_Global[Positioning::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<Positioning>, Positioning::CollectShortCodes);
    IndicatorMap_Global[SimpleHawkesPriceProcessTREvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SimpleHawkesPriceProcessTREvents>,
                                    SimpleHawkesPriceProcessTREvents::CollectShortCodes);
    IndicatorMap_Global[CorrBasedSimpleTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<CorrBasedSimpleTrend>, CorrBasedSimpleTrend::CollectShortCodes);
    // IndicatorMap_Global [ SimpleTrendMktEventsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<SimpleTrendMktEventsCombo>, SimpleTrendMktEventsCombo::CollectShortCodes );
    IndicatorMap_Global[SimpleTrendMktEvents::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SimpleTrendMktEvents>, SimpleTrendMktEvents::CollectShortCodes);
    IndicatorMap_Global[SimpleTrendMktEventsPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SimpleTrendMktEventsPort>, SimpleTrendMktEventsPort::CollectShortCodes);
    IndicatorMap_Global[DynamicWeightPortTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DynamicWeightPortTrend>, DynamicWeightPortTrend::CollectShortCodes);
    IndicatorMap_Global[StudPriceTrendDiffDynamicWeightPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<StudPriceTrendDiffDynamicWeightPort>,
                                    StudPriceTrendDiffDynamicWeightPort::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedPairsDynamicWeightPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineComputedPairsDynamicWeightPort>,
                                    OfflineComputedPairsDynamicWeightPort::CollectShortCodes);
    IndicatorMap_Global[SimpleTrendPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SimpleTrendPort>, SimpleTrendPort::CollectShortCodes);
    // IndicatorMap_Global [ SizeWBPMomentumCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<SizeWBPMomentumCombo>, SizeWBPMomentumCombo::CollectShortCodes );
    IndicatorMap_Global[SizeWBPMomentum::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SizeWBPMomentum>, SizeWBPMomentum::CollectShortCodes);
    IndicatorMap_Global[SizeWDiffTDSizeAvgTPxBasepx::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SizeWDiffTDSizeAvgTPxBasepx>, SizeWDiffTDSizeAvgTPxBasepx::CollectShortCodes);
    IndicatorMap_Global[TDSizeAvgTTypeStTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TDSizeAvgTTypeStTrend>, TDSizeAvgTTypeStTrend::CollectShortCodes);
    // IndicatorMap_Global [ SqrtTSizeTDAvgTDiffCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<SqrtTSizeTDAvgTDiffCombo>, SqrtTSizeTDAvgTDiffCombo::CollectShortCodes );
    IndicatorMap_Global[SqrtTSizeTDAvgTDiff::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SqrtTSizeTDAvgTDiff>, SqrtTSizeTDAvgTDiff::CollectShortCodes);
    // IndicatorMap_Global [ SqrtTSizeTDAvgTDiffLvlCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<SqrtTSizeTDAvgTDiffLvlCombo>, SqrtTSizeTDAvgTDiffLvlCombo::CollectShortCodes );
    IndicatorMap_Global[SqrtTSizeTDAvgTDiffLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SqrtTSizeTDAvgTDiffLvl>, SqrtTSizeTDAvgTDiffLvl::CollectShortCodes);
    // IndicatorMap_Global [ SqrtTSizeTRAvgTDiffCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<SqrtTSizeTRAvgTDiffCombo>, SqrtTSizeTRAvgTDiffCombo::CollectShortCodes );
    IndicatorMap_Global[SqrtTSizeTRAvgTDiff::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SqrtTSizeTRAvgTDiff>, SqrtTSizeTRAvgTDiff::CollectShortCodes);
    // IndicatorMap_Global [ StableScaledTrendCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<StableScaledTrendCombo>, StableScaledTrendCombo::CollectShortCodes );
    IndicatorMap_Global[StableScaledTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<StableScaledTrend>, StableScaledTrend::CollectShortCodes);
    IndicatorMap_Global[NormalizedStableScaledTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<NormalizedStableScaledTrend>, NormalizedStableScaledTrend::CollectShortCodes);
    IndicatorMap_Global[StableScaledTrendPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StableScaledTrendPort>, StableScaledTrendPort::CollectShortCodes);
    IndicatorMap_Global[StableScaledTrend2::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StableScaledTrend2>, StableScaledTrend2::CollectShortCodes);
    IndicatorMap_Global[StableScaledTrendVersion2::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StableScaledTrendVersion2>, StableScaledTrendVersion2::CollectShortCodes);
    IndicatorMap_Global[StableScaledTrendVersion2Port::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StableScaledTrendVersion2Port>, StableScaledTrendVersion2Port::CollectShortCodes);
    IndicatorMap_Global[StableScaledTrendVersion2MktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<StableScaledTrendVersion2MktEvents>,
                                    StableScaledTrendVersion2MktEvents::CollectShortCodes);
    IndicatorMap_Global[StableScaledTrendVersion2MktEventsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<StableScaledTrendVersion2MktEventsPort>,
                                    StableScaledTrendVersion2MktEventsPort::CollectShortCodes);
    // IndicatorMap_Global [ TAOfflineCorradjustedPairsCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TAOfflineCorradjustedPairsCombo>, TAOfflineCorradjustedPairsCombo::CollectShortCodes );
    IndicatorMap_Global[TAOfflineCorradjustedPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TAOfflineCorradjustedPairs>, TAOfflineCorradjustedPairs::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedSerban::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineComputedSerban>, OfflineComputedSerban::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedSerbanPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineComputedSerbanPort>, OfflineComputedSerbanPort::CollectShortCodes);
    IndicatorMap_Global[ReturnsBetaComputedSerban::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ReturnsBetaComputedSerban>, ReturnsBetaComputedSerban::CollectShortCodes);
    IndicatorMap_Global[OfflineCorradjustedSerban::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineCorradjustedSerban>, OfflineCorradjustedSerban::CollectShortCodes);
    IndicatorMap_Global[OfflineCorradjustedSerbanPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineCorradjustedSerbanPort>, OfflineCorradjustedSerbanPort::CollectShortCodes);
    IndicatorMap_Global[ReturnsBetaComputedPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ReturnsBetaComputedPairs>, ReturnsBetaComputedPairs::CollectShortCodes);
    IndicatorMap_Global[ReturnsBetaComputedPairsMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ReturnsBetaComputedPairsMktEvents>,
                                    ReturnsBetaComputedPairsMktEvents::CollectShortCodes);
    IndicatorMap_Global[MultBasedDelta::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<MultBasedDelta>, MultBasedDelta::CollectShortCodes);
    // IndicatorMap_Global [ TDLvlSumTTypeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TDLvlSumTTypeCombo>, TDLvlSumTTypeCombo::CollectShortCodes );
    IndicatorMap_Global[TDLvlSumTType::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TDLvlSumTType>, TDLvlSumTType::CollectShortCodes);
    IndicatorMap_Global[TDLvlSumTTypeSqrtTSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TDLvlSumTTypeSqrtTSize>, TDLvlSumTTypeSqrtTSize::CollectShortCodes);
    // IndicatorMap_Global [ TDSumHVLBTDiffTSizeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TDSumHVLBTDiffTSizeCombo>, TDSumHVLBTDiffTSizeCombo::CollectShortCodes );
    IndicatorMap_Global[TDSumHVLBTDiffTSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TDSumHVLBTDiffTSize>, TDSumHVLBTDiffTSize::CollectShortCodes);
    // IndicatorMap_Global [ TDSumLBTDiffTSizeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TDSumLBTDiffTSizeCombo>, TDSumLBTDiffTSizeCombo::CollectShortCodes );
    IndicatorMap_Global[TDSumLBTDiffTSize::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TDSumLBTDiffTSize>, TDSumLBTDiffTSize::CollectShortCodes);
    // IndicatorMap_Global [ TDSumTDiffFSizeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TDSumTDiffFSizeCombo>, TDSumTDiffFSizeCombo::CollectShortCodes );
    IndicatorMap_Global[TDSumTDiffFSize::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TDSumTDiffFSize>, TDSumTDiffFSize::CollectShortCodes);
    // IndicatorMap_Global [ TDSumTDiffFSizeLvlCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TDSumTDiffFSizeLvlCombo>, TDSumTDiffFSizeLvlCombo::CollectShortCodes );
    IndicatorMap_Global[TDSumTDiffFSizeLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TDSumTDiffFSizeLvl>, TDSumTDiffFSizeLvl::CollectShortCodes);
    // IndicatorMap_Global [ TDSumTDiffFSizeOneLvlCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TDSumTDiffFSizeOneLvlCombo>, TDSumTDiffFSizeOneLvlCombo::CollectShortCodes );
    IndicatorMap_Global[TDSumTDiffFSizeOneLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TDSumTDiffFSizeOneLvl>, TDSumTDiffFSizeOneLvl::CollectShortCodes);
    // IndicatorMap_Global [ TDSumTDiffFSqrtSizeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TDSumTDiffFSqrtSizeCombo>, TDSumTDiffFSqrtSizeCombo::CollectShortCodes );
    IndicatorMap_Global[TDSumTDiffFSqrtSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TDSumTDiffFSqrtSize>, TDSumTDiffFSqrtSize::CollectShortCodes);
    // IndicatorMap_Global [ TDSumTDiffLvlFSqrtSizeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TDSumTDiffLvlFSqrtSizeCombo>, TDSumTDiffLvlFSqrtSizeCombo::CollectShortCodes );
    IndicatorMap_Global[TDSumTDiffLvlFSqrtSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TDSumTDiffLvlFSqrtSize>, TDSumTDiffLvlFSqrtSize::CollectShortCodes);
    // IndicatorMap_Global [ TDSumTDiffNSizeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TDSumTDiffNSizeCombo>, TDSumTDiffNSizeCombo::CollectShortCodes );
    IndicatorMap_Global[TDSumTDiffNSize::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TDSumTDiffNSize>, TDSumTDiffNSize::CollectShortCodes);
    // IndicatorMap_Global [ TDSumTDiffNSizeLvlCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TDSumTDiffNSizeLvlCombo>, TDSumTDiffNSizeLvlCombo::CollectShortCodes );
    IndicatorMap_Global[TDSumTDiffNSizeLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TDSumTDiffNSizeLvl>, TDSumTDiffNSizeLvl::CollectShortCodes);
    // IndicatorMap_Global [ TDSumTDiffNSizeOneLvlCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TDSumTDiffNSizeOneLvlCombo>, TDSumTDiffNSizeOneLvlCombo::CollectShortCodes );
    IndicatorMap_Global[TDSumTDiffNSizeOneLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TDSumTDiffNSizeOneLvl>, TDSumTDiffNSizeOneLvl::CollectShortCodes);
    // IndicatorMap_Global [ TDSumTDiffSqrtFSqrtSizeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TDSumTDiffSqrtFSqrtSizeCombo>, TDSumTDiffSqrtFSqrtSizeCombo::CollectShortCodes );
    IndicatorMap_Global[TDSumTDiffSqrtFSqrtSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TDSumTDiffSqrtFSqrtSize>, TDSumTDiffSqrtFSqrtSize::CollectShortCodes);
    // IndicatorMap_Global [ TDSumTDiffSqrtTSizeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TDSumTDiffSqrtTSizeCombo>, TDSumTDiffSqrtTSizeCombo::CollectShortCodes );
    IndicatorMap_Global[TDSumTDiffSqrtTSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TDSumTDiffSqrtTSize>, TDSumTDiffSqrtTSize::CollectShortCodes);
    // IndicatorMap_Global [ TDSumTDiffTSizeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TDSumTDiffTSizeCombo>, TDSumTDiffTSizeCombo::CollectShortCodes );
    IndicatorMap_Global[TDSumTDiffTSize::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TDSumTDiffTSize>, TDSumTDiffTSize::CollectShortCodes);
    IndicatorMap_Global[TDSumTDiffTSizeLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TDSumTDiffTSizeLvl>, TDSumTDiffTSizeLvl::CollectShortCodes);
    IndicatorMap_Global[TDSumTDiffTSizeOneLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TDSumTDiffTSizeOneLvl>, TDSumTDiffTSizeOneLvl::CollectShortCodes);
    // IndicatorMap_Global [ TDSumTTypeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TDSumTTypeCombo>, TDSumTTypeCombo::CollectShortCodes );
    IndicatorMap_Global[TDSumTType::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TDSumTType>, TDSumTType::CollectShortCodes);
    IndicatorMap_Global[TDSumTTypeSqrtTSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TDSumTTypeSqrtTSize>, TDSumTTypeSqrtTSize::CollectShortCodes);
    IndicatorMap_Global[TDSumTTypeTSize::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TDSumTTypeTSize>, TDSumTTypeTSize::CollectShortCodes);
    // IndicatorMap_Global [ TradeAdjustedSimpleTrendCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TradeAdjustedSimpleTrendCombo>, TradeAdjustedSimpleTrendCombo::CollectShortCodes );
    IndicatorMap_Global[TradeAdjustedSimpleTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TradeAdjustedSimpleTrend>, TradeAdjustedSimpleTrend::CollectShortCodes);
    IndicatorMap_Global[TradeBookAdjustedPrice::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TradeBookAdjustedPrice>, TradeBookAdjustedPrice::CollectShortCodes);

    IndicatorMap_Global[RecentSimpleVolumeMeasure::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<RecentSimpleVolumeMeasure>, RecentSimpleVolumeMeasure::CollectShortCodes);
    IndicatorMap_Global[RecentVolumeMeasure::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<RecentVolumeMeasure>, RecentVolumeMeasure::CollectShortCodes);
    IndicatorMap_Global[RecentSimpleTradesMeasure::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<RecentSimpleTradesMeasure>, RecentSimpleTradesMeasure::CollectShortCodes);
    IndicatorMap_Global[RecentSimpleEventsMeasure::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<RecentSimpleEventsMeasure>, RecentSimpleEventsMeasure::CollectShortCodes);
    IndicatorMap_Global[MovingAvgTradeSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<MovingAvgTradeSize>, MovingAvgTradeSize::CollectShortCodes);
    IndicatorMap_Global[VolumeTraded::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<VolumeTraded>, VolumeTraded::CollectShortCodes);
    IndicatorMap_Global[VolumeRatioCalculator::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<VolumeRatioCalculator>, VolumeRatioCalculator::CollectShortCodes);
    IndicatorMap_Global[VolumeRatioCalculatorPort2::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<VolumeRatioCalculatorPort2>, VolumeRatioCalculatorPort2::CollectShortCodes);

    IndicatorMap_Global[StdevRatioCalculator::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StdevRatioCalculator>, StdevRatioCalculator::CollectShortCodes);

    IndicatorMap_Global[RecentScaledVolumeCalculator::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<RecentScaledVolumeCalculator>, RecentScaledVolumeCalculator::CollectShortCodes);

    IndicatorMap_Global[DIPricingIndicator::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DIPricingIndicator>, DIPricingIndicator::CollectShortCodes);
    IndicatorMap_Global[DIPricingIndicatorMktEvents::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DIPricingIndicatorMktEvents>, DIPricingIndicatorMktEvents::CollectShortCodes);

    IndicatorMap_Global[DI1CurveVolAdjustedTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DI1CurveVolAdjustedTrend>, DI1CurveVolAdjustedTrend::CollectShortCodes);

    IndicatorMap_Global[DI1CurveAdjustedPrice::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DI1CurveAdjustedPrice>, DI1CurveAdjustedPrice::CollectShortCodes);

    IndicatorMap_Global[FRAPrice::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<FRAPrice>, FRAPrice::CollectShortCodes);

    IndicatorMap_Global[DI1CurveAdjustedSimpleTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DI1CurveAdjustedSimpleTrend>, DI1CurveAdjustedSimpleTrend::CollectShortCodes);
    IndicatorMap_Global[DI1CurveAdjustedSimpleTrendMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<DI1CurveAdjustedSimpleTrendMktEvents>,
                                    DI1CurveAdjustedSimpleTrendMktEvents::CollectShortCodes);

    IndicatorMap_Global[DI1CurveAdjustedLinearSimpleTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<DI1CurveAdjustedLinearSimpleTrend>,
                                    DI1CurveAdjustedLinearSimpleTrend::CollectShortCodes);
    IndicatorMap_Global[DI1CurveAdjustedLinearSimpleTrendMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<DI1CurveAdjustedLinearSimpleTrendMktEvents>,
                                    DI1CurveAdjustedLinearSimpleTrendMktEvents::CollectShortCodes);

    IndicatorMap_Global[DI1CurveAdjustedSimpleTrendMomentum::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<DI1CurveAdjustedSimpleTrendMomentum>,
                                    DI1CurveAdjustedSimpleTrendMomentum::CollectShortCodes);
    IndicatorMap_Global[DI1CurveAdjustedSimpleTrendMktEventsMomentum::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<DI1CurveAdjustedSimpleTrendMktEventsMomentum>,
                                    DI1CurveAdjustedSimpleTrendMktEventsMomentum::CollectShortCodes);

    IndicatorMap_Global[DI1CurveAdjustedLinearSimpleTrendMomentum::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<DI1CurveAdjustedLinearSimpleTrend>,
                                    DI1CurveAdjustedLinearSimpleTrend::CollectShortCodes);
    IndicatorMap_Global[DI1CurveAdjustedLinearSimpleTrendMktEventsMomentum::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<DI1CurveAdjustedLinearSimpleTrendMktEvents>,
                                    DI1CurveAdjustedLinearSimpleTrendMktEvents::CollectShortCodes);

    IndicatorMap_Global[DI1LeveredSimpleTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DI1LeveredSimpleTrend>, DI1LeveredSimpleTrend::CollectShortCodes);
    IndicatorMap_Global[DI1LeveredSimpleTrendMktEvents::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DI1LeveredSimpleTrendMktEvents>, DI1LeveredSimpleTrendMktEvents::CollectShortCodes);

    IndicatorMap_Global[CurveAdjustedPrice::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<CurveAdjustedPrice>, CurveAdjustedPrice::CollectShortCodes);

    IndicatorMap_Global[CurveAdjustedSimpleTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<CurveAdjustedSimpleTrend>, CurveAdjustedSimpleTrend::CollectShortCodes);

    IndicatorMap_Global[CurveAdjustedVolumeWeightedSimpleTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<CurveAdjustedVolumeWeightedSimpleTrend>,
                                    CurveAdjustedVolumeWeightedSimpleTrend::CollectShortCodes);

    IndicatorMap_Global[CurveAdjustedPorts::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<CurveAdjustedPorts>, CurveAdjustedPorts::CollectShortCodes);

    IndicatorMap_Global[CurveAdjustedSimpleTrendMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<CurveAdjustedSimpleTrendMktEvents>,
                                    CurveAdjustedSimpleTrendMktEvents::CollectShortCodes);

    IndicatorMap_Global[CurveAdjustedSimpleTrendBond::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<CurveAdjustedSimpleTrendBond>, CurveAdjustedSimpleTrendBond::CollectShortCodes);
    IndicatorMap_Global[CurveAdjustedSimpleTrendMomentumBond::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<CurveAdjustedSimpleTrendMomentumBond>,
                                    CurveAdjustedSimpleTrendMomentumBond::CollectShortCodes);

    IndicatorMap_Global[CurveAdjustedSimpleTrendMomentum::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<CurveAdjustedSimpleTrendMomentum>,
                                    CurveAdjustedSimpleTrendMomentum::CollectShortCodes);
    IndicatorMap_Global[CurveAdjustedSimpleTrendMktEventsMomentum::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<CurveAdjustedSimpleTrendMktEventsMomentum>,
                                    CurveAdjustedSimpleTrendMktEventsMomentum::CollectShortCodes);

    IndicatorMap_Global[LeveredSimpleTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<LeveredSimpleTrend>, LeveredSimpleTrend::CollectShortCodes);
    IndicatorMap_Global[LeveredSimpleTrendMktEvents::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<LeveredSimpleTrendMktEvents>, LeveredSimpleTrendMktEvents::CollectShortCodes);

    IndicatorMap_Global[ConvexPositioning::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ConvexPositioning>, ConvexPositioning::CollectShortCodes);
    IndicatorMap_Global[ConvexTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ConvexTrend>, ConvexTrend::CollectShortCodes);
    IndicatorMap_Global[ConvexTrendPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ConvexTrendPort>, ConvexTrendPort::CollectShortCodes);
    IndicatorMap_Global[AmplifyLevelChange::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<AmplifyLevelChange>, AmplifyLevelChange::CollectShortCodes);
    IndicatorMap_Global[AmplifyLevelChangeL1::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<AmplifyLevelChangeL1>, AmplifyLevelChangeL1::CollectShortCodes);
    IndicatorMap_Global[MoorePenrose::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<MoorePenrose>, MoorePenrose::CollectShortCodes);
    IndicatorMap_Global[MoorePenroseFxSf::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<MoorePenroseFxSf>, MoorePenroseFxSf::CollectShortCodes);
    // IndicatorMap_Global [ TrendAdjustedSelfPositionCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TrendAdjustedSelfPositionCombo>, TrendAdjustedSelfPositionCombo::CollectShortCodes );
    IndicatorMap_Global[TrendAdjustedSelfPosition::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TrendAdjustedSelfPosition>, TrendAdjustedSelfPosition::CollectShortCodes);
    // IndicatorMap_Global [ TrendReversalCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TrendReversalCombo>, TrendReversalCombo::CollectShortCodes );
    IndicatorMap_Global[TrendReversal::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TrendReversal>, TrendReversal::CollectShortCodes);
    IndicatorMap_Global[TrendReversalMktEvents::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TrendReversalMktEvents>, TrendReversalMktEvents::CollectShortCodes);
    // IndicatorMap_Global [ TrendStartingCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TrendStartingCombo>, TrendStartingCombo::CollectShortCodes );
    IndicatorMap_Global[TrendStarting::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TrendStarting>, TrendStarting::CollectShortCodes);
    IndicatorMap_Global[SimpleTrendIndepMktEvents::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SimpleTrendIndepMktEvents>, SimpleTrendIndepMktEvents::CollectShortCodes);
    // IndicatorMap_Global [ TRSumTDiffFSizeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TRSumTDiffFSizeCombo>, TRSumTDiffFSizeCombo::CollectShortCodes );
    IndicatorMap_Global[TRAvgTDiffSqrtTSizeVolfactor::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TRAvgTDiffSqrtTSizeVolfactor>, TRAvgTDiffSqrtTSizeVolfactor::CollectShortCodes);
    IndicatorMap_Global[TRSumTDiffFSize::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TRSumTDiffFSize>, TRSumTDiffFSize::CollectShortCodes);
    IndicatorMap_Global[TRSumTDiffFSizeLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TRSumTDiffFSizeLvl>, TRSumTDiffFSizeLvl::CollectShortCodes);
    // IndicatorMap_Global [ TRSumTDiffFSqrtSizeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TRSumTDiffFSqrtSizeCombo>, TRSumTDiffFSqrtSizeCombo::CollectShortCodes );
    IndicatorMap_Global[TRSumTDiffFSqrtSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TRSumTDiffFSqrtSize>, TRSumTDiffFSqrtSize::CollectShortCodes);
    // IndicatorMap_Global [ TRSumTDiffNSizeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TRSumTDiffNSizeCombo>, TRSumTDiffNSizeCombo::CollectShortCodes );
    IndicatorMap_Global[TRSumTDiffNSize::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TRSumTDiffNSize>, TRSumTDiffNSize::CollectShortCodes);
    IndicatorMap_Global[TRSumTDiffNSizeLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TRSumTDiffNSizeLvl>, TRSumTDiffNSizeLvl::CollectShortCodes);
    // IndicatorMap_Global [ TRSumTDiffSqrtFSqrtSizeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TRSumTDiffSqrtFSqrtSizeCombo>, TRSumTDiffSqrtFSqrtSizeCombo::CollectShortCodes );
    IndicatorMap_Global[TRSumTDiffSqrtFSqrtSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TRSumTDiffSqrtFSqrtSize>, TRSumTDiffSqrtFSqrtSize::CollectShortCodes);
    // IndicatorMap_Global [ TRSumTDiffSqrtTSizeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TRSumTDiffSqrtTSizeCombo>, TRSumTDiffSqrtTSizeCombo::CollectShortCodes );
    IndicatorMap_Global[TRSumTDiffSqrtTSize::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TRSumTDiffSqrtTSize>, TRSumTDiffSqrtTSize::CollectShortCodes);
    // IndicatorMap_Global [ TRSumTDiffTSizeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TRSumTDiffTSizeCombo>, TRSumTDiffTSizeCombo::CollectShortCodes );
    IndicatorMap_Global[TRSumTDiffTSize::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TRSumTDiffTSize>, TRSumTDiffTSize::CollectShortCodes);
    IndicatorMap_Global[TRSumTDiffTSizeLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TRSumTDiffTSizeLvl>, TRSumTDiffTSizeLvl::CollectShortCodes);
    // IndicatorMap_Global [ TRSumTTypeCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TRSumTTypeCombo>, TRSumTTypeCombo::CollectShortCodes );
    IndicatorMap_Global[TRSumTType::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TRSumTType>, TRSumTType::CollectShortCodes);
    IndicatorMap_Global[TRSumTTypeTSize::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TRSumTTypeTSize>, TRSumTType::CollectShortCodes);
    IndicatorMap_Global[TRSumTPriceTSize::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TRSumTPriceTSize>, TRSumTPriceTSize::CollectShortCodes);
    // IndicatorMap_Global [ TSizeEDSqrtAvgTDiffCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TSizeEDSqrtAvgTDiffCombo>, TSizeEDSqrtAvgTDiffCombo::CollectShortCodes );
    IndicatorMap_Global[TSizeEDSqrtAvgTDiff::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TSizeEDSqrtAvgTDiff>, TSizeEDSqrtAvgTDiff::CollectShortCodes);
    // IndicatorMap_Global [ TSizeTDAvgTDiffCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TSizeTDAvgTDiffCombo>, TSizeTDAvgTDiffCombo::CollectShortCodes );
    IndicatorMap_Global[TSizeTDAvgTDiff::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TSizeTDAvgTDiff>, TSizeTDAvgTDiff::CollectShortCodes);
    // IndicatorMap_Global [ TSizeTDAvgTDiffLvlCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TSizeTDAvgTDiffLvlCombo>, TSizeTDAvgTDiffLvlCombo::CollectShortCodes );
    IndicatorMap_Global[TSizeTDAvgTDiffLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TSizeTDAvgTDiffLvl>, TSizeTDAvgTDiffLvl::CollectShortCodes);
    // IndicatorMap_Global [ TSizeTDSqrtAvgTDiffCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TSizeTDSqrtAvgTDiffCombo>, TSizeTDSqrtAvgTDiffCombo::CollectShortCodes );
    IndicatorMap_Global[TSizeTDSqrtAvgTDiff::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TSizeTDSqrtAvgTDiff>, TSizeTDSqrtAvgTDiff::CollectShortCodes);
    // IndicatorMap_Global [ TSizeTDSqrtAvgTDiffLvlCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TSizeTDSqrtAvgTDiffLvlCombo>, TSizeTDSqrtAvgTDiffLvlCombo::CollectShortCodes );
    IndicatorMap_Global[TSizeTDSqrtAvgTDiffLvl::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TSizeTDSqrtAvgTDiffLvl>, TSizeTDSqrtAvgTDiffLvl::CollectShortCodes);
    // IndicatorMap_Global [ TSizeTRAvgTDiffCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TSizeTRAvgTDiffCombo>, TSizeTRAvgTDiffCombo::CollectShortCodes );
    IndicatorMap_Global[TSizeTRAvgTDiff::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TSizeTRAvgTDiff>, TSizeTRAvgTDiff::CollectShortCodes);
    // IndicatorMap_Global [ TSizeTRSqrtAvgTDiffCombo::VarName() ] = IndicatorStaticFuncStruct_t (
    // GetUniqueInstanceWrapper<TSizeTRSqrtAvgTDiffCombo>, TSizeTRSqrtAvgTDiffCombo::CollectShortCodes );
    IndicatorMap_Global[TSizeTRSqrtAvgTDiff::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TSizeTRSqrtAvgTDiff>, TSizeTRSqrtAvgTDiff::CollectShortCodes);
    IndicatorMap_Global[TimeBasedRegime::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TimeBasedRegime>, TimeBasedRegime::CollectShortCodes);

    IndicatorMap_Global[CurrentTime::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<CurrentTime>, CurrentTime::CollectShortCodes);

    IndicatorMap_Global[TimeBasedRegimeBinary::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TimeBasedRegimeBinary>, TimeBasedRegimeBinary::CollectShortCodes);
    IndicatorMap_Global[OnlineRatioCalculator::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineRatioCalculator>, OnlineRatioCalculator::CollectShortCodes);
    IndicatorMap_Global[FutureToSpotPricing::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<FutureToSpotPricing>, FutureToSpotPricing::CollectShortCodes);
    IndicatorMap_Global[StudPriceDiff::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<StudPriceDiff>, StudPriceDiff::CollectShortCodes);
    IndicatorMap_Global[StudPriceDiffMktEvents::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StudPriceDiffMktEvents>, StudPriceDiffMktEvents::CollectShortCodes);
    IndicatorMap_Global[StudPriceTrendDiff::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StudPriceTrendDiff>, StudPriceTrendDiff::CollectShortCodes);
    IndicatorMap_Global[StudPriceDiffPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<StudPriceDiffPort>, StudPriceDiffPort::CollectShortCodes);
    IndicatorMap_Global[StudPriceTrendDiffPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StudPriceTrendDiffPort>, StudPriceTrendDiffPort::CollectShortCodes);
    IndicatorMap_Global[StdevAdjustedReturnsDiff::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StdevAdjustedReturnsDiff>, StdevAdjustedReturnsDiff::CollectShortCodes);
    IndicatorMap_Global[StdevAdjustedReturnsDiffNorm::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StdevAdjustedReturnsDiffNorm>, StdevAdjustedReturnsDiffNorm::CollectShortCodes);
    IndicatorMap_Global[StudPriceReturnsDiff::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StudPriceReturnsDiff>, StudPriceReturnsDiff::CollectShortCodes);
    IndicatorMap_Global[VolumeWeightedStudPriceReturnsDiff::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<VolumeWeightedStudPriceReturnsDiff>,
                                    VolumeWeightedStudPriceReturnsDiff::CollectShortCodes);
    IndicatorMap_Global[StdevAdjustedReturnsDiffPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StdevAdjustedReturnsDiffPort>, StdevAdjustedReturnsDiffPort::CollectShortCodes);
    IndicatorMap_Global[StudPriceReturnsDiffPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StudPriceReturnsDiffPort>, StudPriceReturnsDiffPort::CollectShortCodes);
    IndicatorMap_Global[RSVMRatioAdjSPDiff::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<RSVMRatioAdjSPDiff>, RSVMRatioAdjSPDiff::CollectShortCodes);
    IndicatorMap_Global[ProjectedPricePairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ProjectedPricePairs>, ProjectedPricePairs::CollectShortCodes);
    IndicatorMap_Global[ProjectedPriceConstPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ProjectedPriceConstPairs>, ProjectedPriceConstPairs::CollectShortCodes);
    IndicatorMap_Global[ProjectedPriceTypePairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ProjectedPriceTypePairs>, ProjectedPriceTypePairs::CollectShortCodes);
    IndicatorMap_Global[ProjectedPriceTypeConstPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ProjectedPriceTypeConstPairs>, ProjectedPriceTypeConstPairs::CollectShortCodes);
    IndicatorMap_Global[ReturnsBasedProjectedPrice::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ReturnsBasedProjectedPrice>, ReturnsBasedProjectedPrice::CollectShortCodes);
    IndicatorMap_Global[SlowStdevCalculator::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SlowStdevCalculator>, SlowStdevCalculator::CollectShortCodes);
    IndicatorMap_Global[SlowStdevTrendCalculator::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SlowStdevTrendCalculator>, SlowStdevTrendCalculator::CollectShortCodes);
    IndicatorMap_Global[SlowStdevReturnsCalculator::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SlowStdevReturnsCalculator>, SlowStdevReturnsCalculator::CollectShortCodes);
    IndicatorMap_Global[StdevRatioNormalised::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StdevRatioNormalised>, StdevRatioNormalised::CollectShortCodes);
    IndicatorMap_Global[SlowStdevCalculatorPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SlowStdevCalculatorPort>, SlowStdevCalculatorPort::CollectShortCodes);
    IndicatorMap_Global[SlowBetaCalculator::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SlowBetaCalculator>, SlowBetaCalculator::CollectShortCodes);
    IndicatorMap_Global[SlowCorrCalculator::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SlowCorrCalculator>, SlowCorrCalculator::CollectShortCodes);
    IndicatorMap_Global[OfflinePricePairDiff::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflinePricePairDiff>, OfflinePricePairDiff::CollectShortCodes);
    IndicatorMap_Global[ContractTerm::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ContractTerm>, ContractTerm::CollectShortCodes);
    IndicatorMap_Global[L1Price::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<L1Price>, L1Price::CollectShortCodes);
    IndicatorMap_Global[VolumeWeightedPrice::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<VolumeWeightedPrice>, VolumeWeightedPrice::CollectShortCodes);
    IndicatorMap_Global[L1BookChange::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<L1BookChange>, L1BookChange::CollectShortCodes);

    IndicatorMap_Global[L1PortPrice::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<L1PortPrice>, L1PortPrice::CollectShortCodes);
    IndicatorMap_Global[TodTomNormSpread::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TodTomNormSpread>, TodTomNormSpread::CollectShortCodes);
    IndicatorMap_Global[SpotFutureSpread::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SpotFutureSpread>, SpotFutureSpread::CollectShortCodes);
    IndicatorMap_Global[StableBidSpotFutureSpread::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StableBidSpotFutureSpread>, StableBidSpotFutureSpread::CollectShortCodes);
    IndicatorMap_Global[StableAskSpotFutureSpread::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<StableAskSpotFutureSpread>, StableAskSpotFutureSpread::CollectShortCodes);
    IndicatorMap_Global[CMVFL1Price::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<CMVFL1Price>, CMVFL1Price::CollectShortCodes);
    IndicatorMap_Global[USDXPrice::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<USDXPrice>, USDXPrice::CollectShortCodes);
    IndicatorMap_Global[BollingerBand::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<BollingerBand>, BollingerBand::CollectShortCodes);
    IndicatorMap_Global[YBSimpleTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<YBSimpleTrend>, YBSimpleTrend::CollectShortCodes);
    IndicatorMap_Global[BollingerBand::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<BollingerBand>, BollingerBand::CollectShortCodes);
    IndicatorMap_Global[MeanNormalizedDiff::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<MeanNormalizedDiff>, MeanNormalizedDiff::CollectShortCodes);
    IndicatorMap_Global[LevelSize::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<LevelSize>, LevelSize::CollectShortCodes);
    IndicatorMap_Global[LevelSizePerOrder::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<LevelSizePerOrder>, LevelSizePerOrder::CollectShortCodes);
    IndicatorMap_Global[LevelOrderSize::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<LevelOrderSize>, LevelOrderSize::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedReturnsMult::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineComputedReturnsMult>, OfflineComputedReturnsMult::CollectShortCodes);
    IndicatorMap_Global[MovingCorrelationCutOff::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<MovingCorrelationCutOff>, MovingCorrelationCutOff::CollectShortCodes);
    IndicatorMap_Global[VolatilityRegime::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<VolatilityRegime>, VolatilityRegime::CollectShortCodes);
    IndicatorMap_Global[VolumeRatioRegime::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<VolumeRatioRegime>, VolumeRatioRegime::CollectShortCodes);
    IndicatorMap_Global[ScaledVolumeRatioRegime::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ScaledVolumeRatioRegime>, ScaledVolumeRatioRegime::CollectShortCodes);
    IndicatorMap_Global[StdevRatioRegime::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<StdevRatioRegime>, StdevRatioRegime::CollectShortCodes);

    IndicatorMap_Global[SizeBasedRegime::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SizeBasedRegime>, SizeBasedRegime::CollectShortCodes);
    IndicatorMap_Global[RollingL1SzRegime::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<RollingL1SzRegime>, RollingL1SzRegime::CollectShortCodes);
    IndicatorMap_Global[L1BookbiasRegime::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<L1BookbiasRegime>, L1BookbiasRegime::CollectShortCodes);
    IndicatorMap_Global[L1Bookbias::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<L1Bookbias>, L1Bookbias::CollectShortCodes);
    IndicatorMap_Global[L1Targetbias::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<L1Targetbias>, L1Targetbias::CollectShortCodes);
    IndicatorMap_Global[RegimeMarketMaking::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<RegimeMarketMaking>, RegimeMarketMaking::CollectShortCodes);
    IndicatorMap_Global[RegimeVolStdev::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<RegimeVolStdev>, RegimeVolStdev::CollectShortCodes);
    IndicatorMap_Global[RegimeSlowStdev::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<RegimeSlowStdev>, RegimeSlowStdev::CollectShortCodes);
    IndicatorMap_Global[RegimeStdev::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<RegimeStdev>, RegimeStdev::CollectShortCodes);
    IndicatorMap_Global[RegimeSlowStdev2::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<RegimeSlowStdev2>, RegimeSlowStdev2::CollectShortCodes);
    IndicatorMap_Global[RegimeSlowStdevTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<RegimeSlowStdevTrend>, RegimeSlowStdevTrend::CollectShortCodes);
    IndicatorMap_Global[CurveRegime::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<CurveRegime>, CurveRegime::CollectShortCodes);
    IndicatorMap_Global[RegimeDiffPxModDiffPx::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<RegimeDiffPxModDiffPx>, RegimeDiffPxModDiffPx::CollectShortCodes);
    IndicatorMap_Global[DiffTDSizeL1SizeRatio::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DiffTDSizeL1SizeRatio>, DiffTDSizeL1SizeRatio::CollectShortCodes);
    IndicatorMap_Global[MaxMovingCorrelation::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<MaxMovingCorrelation>, MaxMovingCorrelation::CollectShortCodes);
    IndicatorMap_Global[OnlineOfflineCorrDiffBaseRegime::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineOfflineCorrDiffBaseRegime>, OnlineOfflineCorrDiffBaseRegime::CollectShortCodes);
    IndicatorMap_Global[RegimeOnlineOfflineBeta::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<RegimeOnlineOfflineBeta>, RegimeOnlineOfflineBeta::CollectShortCodes);
    IndicatorMap_Global[RegimeOnlineOfflineStdevRatio::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<RegimeOnlineOfflineStdevRatio>, RegimeOnlineOfflineStdevRatio::CollectShortCodes);
    IndicatorMap_Global[RegimeSlope::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<RegimeSlope>, RegimeSlope::CollectShortCodes);
    IndicatorMap_Global[PriceRegime::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<PriceRegime>, PriceRegime::CollectShortCodes);
    IndicatorMap_Global[RegimeStatsDiff::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<RegimeStatsDiff>, RegimeStatsDiff::CollectShortCodes);
    IndicatorMap_Global[RegimeOnlineOfflineCorr::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<RegimeOnlineOfflineCorr>, RegimeOnlineOfflineCorr::CollectShortCodes);
    IndicatorMap_Global[TradedEzoneRegimeIndicator::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<TradedEzoneRegimeIndicator>, TradedEzoneRegimeIndicator::CollectShortCodes);
    IndicatorMap_Global[EventBasedRegime::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<EventBasedRegime>, EventBasedRegime::CollectShortCodes);
    IndicatorMap_Global[MovingAvgTradeImpact::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<MovingAvgTradeImpact>, MovingAvgTradeImpact::CollectShortCodes);

    IndicatorMap_Global[ImpliedVolCalculator::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ImpliedVolCalculator>, ImpliedVolCalculator::CollectShortCodes);
    IndicatorMap_Global[MovingAverageImpliedVol::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<MovingAverageImpliedVol>, MovingAverageImpliedVol::CollectShortCodes);
    IndicatorMap_Global[MovingAvgPriceImpliedVol::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<MovingAvgPriceImpliedVol>, MovingAvgPriceImpliedVol::CollectShortCodes);
    IndicatorMap_Global[DynamicImpliedVolATM::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DynamicImpliedVolATM>, DynamicImpliedVolATM::CollectShortCodes);
    IndicatorMap_Global[ImpliedVolSpread::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ImpliedVolSpread>, ImpliedVolSpread::CollectShortCodes);
    IndicatorMap_Global[ImpliedVolTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ImpliedVolTrend>, ImpliedVolTrend::CollectShortCodes);
    IndicatorMap_Global[ImpliedVolCalculatorSpread::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ImpliedVolCalculatorSpread>, ImpliedVolCalculatorSpread::CollectShortCodes);
    IndicatorMap_Global[VWAPImpliedVol::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<VWAPImpliedVol>, VWAPImpliedVol::CollectShortCodes);
    IndicatorMap_Global[MovingAvgBidAskIVSpread::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<MovingAvgBidAskIVSpread>, MovingAvgBidAskIVSpread::CollectShortCodes);
    IndicatorMap_Global[RealizedVolCalculator::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<RealizedVolCalculator>, RealizedVolCalculator::CollectShortCodes);
    IndicatorMap_Global[OptionsGreek::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OptionsGreek>, OptionsGreek::CollectShortCodes);
    IndicatorMap_Global[OptionsInfo::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OptionsInfo>, OptionsInfo::CollectShortCodes);
    IndicatorMap_Global[OptionsPriceBias::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OptionsPriceBias>, OptionsPriceBias::CollectShortCodes);
    IndicatorMap_Global[ValueWBPMomentum::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ValueWBPMomentum>, ValueWBPMomentum::CollectShortCodes);
    IndicatorMap_Global[ValueWBPMomentum2::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ValueWBPMomentum2>, ValueWBPMomentum2::CollectShortCodes);
    IndicatorMap_Global[ReturnsDiffSyntheticIndex::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ReturnsDiffSyntheticIndex>, ReturnsDiffSyntheticIndex::CollectShortCodes);
    IndicatorMap_Global[SyntheticIndex::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SyntheticIndex>, SyntheticIndex::CollectShortCodes);
    IndicatorMap_Global[ETFSyntheticPriceDiff::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ETFSyntheticPriceDiff>, ETFSyntheticPriceDiff::CollectShortCodes);
    IndicatorMap_Global[IndexFutureToSpotPricing::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<IndexFutureToSpotPricing>, IndexFutureToSpotPricing::CollectShortCodes);
    IndicatorMap_Global[MovingAvgBidAskSpread::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<MovingAvgBidAskSpread>, MovingAvgBidAskSpread::CollectShortCodes);
    IndicatorMap_Global[TrendSwitchRegime::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TrendSwitchRegime>, TrendSwitchRegime::CollectShortCodes);
    IndicatorMap_Global[SrcSwitchRegPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SrcSwitchRegPort>, SrcSwitchRegPort::CollectShortCodes);
    IndicatorMap_Global[TDSumTradeFlow::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TDSumTradeFlow>, TDSumTradeFlow::CollectShortCodes);
    IndicatorMap_Global[BidAskToPayGeneralized::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<BidAskToPayGeneralized>, BidAskToPayGeneralized::CollectShortCodes);
    IndicatorMap_Global[BidAskToPayGeneralizedAllLvls::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<BidAskToPayGeneralizedAllLvls>, BidAskToPayGeneralizedAllLvls::CollectShortCodes);
    IndicatorMap_Global[DecayedComplexBookGeneralized::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<DecayedComplexBookGeneralized>, DecayedComplexBookGeneralized::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedReturnsPairs::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineComputedReturnsPairs>, OfflineComputedReturnsPairs::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedReturnsPairsMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineComputedReturnsPairsMktEvents>,
                                    OfflineComputedReturnsPairsMktEvents::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedReturnsCutoffPairs::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineComputedReturnsCutoffPairs>,
                                    OfflineComputedReturnsCutoffPairs::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedReturnsPairsPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OfflineComputedReturnsPairsPort>, OfflineComputedReturnsPairsPort::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedReturnsCutoffPairsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineComputedReturnsCutoffPairsPort>,
                                    OfflineComputedReturnsCutoffPairsPort::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedReturnsCutoffPairsMktEvents::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineComputedReturnsCutoffPairsMktEvents>,
                                    OfflineComputedReturnsCutoffPairsMktEvents::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedReturnsPairsMktEventsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineComputedReturnsPairsMktEventsPort>,
                                    OfflineComputedReturnsPairsMktEventsPort::CollectShortCodes);
    IndicatorMap_Global[OfflineComputedReturnsCutoffPairsMktEventsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OfflineComputedReturnsCutoffPairsMktEventsPort>,
                                    OfflineComputedReturnsCutoffPairsMktEventsPort::CollectShortCodes);
    IndicatorMap_Global[SimpleReturnsPort::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SimpleReturnsPort>, SimpleReturnsPort::CollectShortCodes);
    IndicatorMap_Global[SimpleReturnsMktEventsPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SimpleReturnsMktEventsPort>, SimpleReturnsMktEventsPort::CollectShortCodes);
    IndicatorMap_Global[EventBiasOfflineIndicator::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<EventBiasOfflineIndicator>, EventBiasOfflineIndicator::CollectShortCodes);
    IndicatorMap_Global[ExpressionIndicator::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ExpressionIndicator>, ExpressionIndicator::CollectShortCodes);
    IndicatorMap_Global[ConstantIndicator::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ConstantIndicator>, ConstantIndicator::CollectShortCodes);
    IndicatorMap_Global[TurnOverRate::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<TurnOverRate>, TurnOverRate::CollectShortCodes);
    IndicatorMap_Global[RegimeTurnOverRate::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<RegimeTurnOverRate>, RegimeTurnOverRate::CollectShortCodes);
    IndicatorMap_Global[Kalman::VXSpotPrice::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<Kalman::VXSpotPrice>, Kalman::VXSpotPrice::CollectShortCodes);
    IndicatorMap_Global[ImpliedVolPrice::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ImpliedVolPrice>, ImpliedVolPrice::CollectShortCodes);
    IndicatorMap_Global[SpreadImpliedVolPrice::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<SpreadImpliedVolPrice>, SpreadImpliedVolPrice::CollectShortCodes);
    IndicatorMap_Global[Kalman::KalmanTradePrice::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<Kalman::KalmanTradePrice>, Kalman::KalmanTradePrice::CollectShortCodes);
    IndicatorMap_Global[SmoothTrend::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SmoothTrend>, SmoothTrend::CollectShortCodes);
    IndicatorMap_Global[SmoothZscore::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SmoothZscore>, SmoothZscore::CollectShortCodes);
    IndicatorMap_Global[SgMgTrRegime::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<SgMgTrRegime>, SgMgTrRegime::CollectShortCodes);
    IndicatorMap_Global[OnlineImpliedYen::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<OnlineImpliedYen>, OnlineImpliedYen::CollectShortCodes);
    IndicatorMap_Global[ReturnsStdev::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<ReturnsStdev>, ReturnsStdev::CollectShortCodes);
    IndicatorMap_Global[FastOrdersDiffPx::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<FastOrdersDiffPx>, FastOrdersDiffPx::CollectShortCodes);
    IndicatorMap_Global[BookSize::VarName()] =
        IndicatorStaticFuncStruct_t(GetUniqueInstanceWrapper<BookSize>, BookSize::CollectShortCodes);
    IndicatorMap_Global[ReturnsSimpleTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<ReturnsSimpleTrend>, ReturnsSimpleTrend::CollectShortCodes);
    IndicatorMap_Global[PriceNormalizedReturnsTrend::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<PriceNormalizedReturnsTrend>, PriceNormalizedReturnsTrend::CollectShortCodes);
    IndicatorMap_Global[PriceNormalizedReturnsStdev::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<PriceNormalizedReturnsStdev>, PriceNormalizedReturnsStdev::CollectShortCodes);
    IndicatorMap_Global[PortfolioDeviation::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<PortfolioDeviation>, PortfolioDeviation::CollectShortCodes);
    IndicatorMap_Global[OnlineComputedCutoffPairPort::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<OnlineComputedCutoffPairPort>, OnlineComputedCutoffPairPort::CollectShortCodes);
    IndicatorMap_Global[BigTradeVolumeVersion::VarName()] = IndicatorStaticFuncStruct_t(
        GetUniqueInstanceWrapper<BigTradeVolumeVersion>, BigTradeVolumeVersion::CollectShortCodes);
  }
}

CommonIndicator* DummyUniqueInstance(DebugLogger& a, const Watch& b, const std::vector<const char*>& c, PriceType_t p) {
  return NULL;
}
void DummmyCollectShortcode(std::vector<std::string>& a, std::vector<std::string>& b,
                            const std::vector<const char*>& c) {}

bool isComboIndicator(std::string varname_) {
  if ((varname_.find("Combo") != varname_.npos) &&
      (varname_ != "OfflineCorradjustedPairsNormalizedCombo")  // what the hell is this.. just skip this..
      )
    //|| ( varname_.find("Comb" ) != ind_var_name_.npos ) ) // CombXyZAbc are not combo indicator - Careful
    return true;
  return false;
}

CommonIndicatorUniqueInstancePtr GetUniqueInstanceFunc(std::string varname_) {
  if (isComboIndicator(varname_)) {
    varname_ = ComboIndicator::VarName();
  }
  if (IndicatorMap_Global.find(varname_) != IndicatorMap_Global.end()) {
    return IndicatorMap_Global[varname_].unique_instance_ptr_;
  } else {
    std::cerr << __FUNCTION__ << " Could not find Indicator : " << varname_ << std::endl;
    return &DummyUniqueInstance;
  }
}

CommonIndicatorCollectShortCodePtr CollectShortCodeFunc(std::string varname_) {
  if (isComboIndicator(varname_)) {
    varname_ = ComboIndicator::VarName();
  }
  if (IndicatorMap_Global.find(varname_) != IndicatorMap_Global.end()) {
    return IndicatorMap_Global[varname_].collect_shortcode_ptr_;
  } else {
    std::cerr << __FUNCTION__ << " Could not find Indicator : " << varname_ << std::endl;
    return &DummmyCollectShortcode;
  }
}

std::vector<std::string> GetIndicatorOrder() {
  std::vector<std::string> indicator_order_vec_;
  indicator_order_vec_.push_back(SlowStdevCalculator::VarName());
  indicator_order_vec_.push_back(SlowStdevCalculatorPort::VarName());
  indicator_order_vec_.push_back(SlowStdevCalculator::VarName());
  indicator_order_vec_.push_back(RegimeStdev::VarName());
  indicator_order_vec_.push_back(VolumeRatioCalculator::VarName());
  indicator_order_vec_.push_back(VolumeWeightedScaledTrend::VarName());
  indicator_order_vec_.push_back(VolumeWeightedScaledTrendMktEvents::VarName());
  indicator_order_vec_.push_back(VolumeWeightedSimpleTrend::VarName());
  indicator_order_vec_.push_back(VolumeWeightedSimpleTrendMktEvents::VarName());
  indicator_order_vec_.push_back(SimpleTrend::VarName());
  indicator_order_vec_.push_back(ExponentialMovingAverage::VarName());
  indicator_order_vec_.push_back(SimpleTrend::VarName() + "Combo");
  indicator_order_vec_.push_back(SimpleTrendMktEvents::VarName());
  indicator_order_vec_.push_back(SimpleTrendMktEvents::VarName() + "Combo");
  indicator_order_vec_.push_back(SimpleReturns::VarName());
  indicator_order_vec_.push_back(SimpleReturns::VarName() + "Combo");
  indicator_order_vec_.push_back(SimpleReturnsMktEvents::VarName());
  indicator_order_vec_.push_back(SimpleReturnsMktEvents::VarName() + "Combo");
  indicator_order_vec_.push_back(SimpleReturnsPort::VarName());
  indicator_order_vec_.push_back(SimpleReturnsMktEventsPort::VarName());
  indicator_order_vec_.push_back(SimpleTrendMktEventsPort::VarName());
  indicator_order_vec_.push_back(SimpleTrendPort::VarName());
  indicator_order_vec_.push_back(ScaledTrend::VarName());
  indicator_order_vec_.push_back(ScaledTrendMktEvents::VarName());
  indicator_order_vec_.push_back(ScaledTrendMktEvents::VarName());
  indicator_order_vec_.push_back(ScaledTrend::VarName() + "Combo");
  indicator_order_vec_.push_back(ScaledTrendPort::VarName());
  indicator_order_vec_.push_back(ScaledTrendMktEvents::VarName() + "Combo");
  indicator_order_vec_.push_back(ScaledTrendMktEventsPort::VarName());
  indicator_order_vec_.push_back(SecondDerivative::VarName());
  indicator_order_vec_.push_back(SecondDerivative::VarName() + "Combo");
  indicator_order_vec_.push_back(StableScaledTrend::VarName());
  indicator_order_vec_.push_back(StableScaledTrend::VarName() + "Combo");
  indicator_order_vec_.push_back(TradeAdjustedSimpleTrend::VarName());
  indicator_order_vec_.push_back(TradeAdjustedSimpleTrend::VarName() + "Combo");
  indicator_order_vec_.push_back(TrendReversal::VarName());
  indicator_order_vec_.push_back(TrendReversal::VarName() + "Combo");
  indicator_order_vec_.push_back(TrendStarting::VarName());
  indicator_order_vec_.push_back(SimpleTrendIndepMktEvents::VarName());
  indicator_order_vec_.push_back(TrendStarting::VarName() + "Combo");
  indicator_order_vec_.push_back(MultMidOrderPrice::VarName() + "Combo");
  indicator_order_vec_.push_back(MultMidPrice::VarName() + "Combo");
  indicator_order_vec_.push_back(MultMktComplexOrderPrice::VarName() + "Combo");
  indicator_order_vec_.push_back(MultMktComplexPrice::VarName() + "Combo");
  indicator_order_vec_.push_back(MultMktComplexPriceShortAvg::VarName() + "Combo");
  indicator_order_vec_.push_back(MultMktOrderPrice::VarName() + "Combo");
  indicator_order_vec_.push_back(MultMktOrderPriceTopOff::VarName() + "Combo");
  indicator_order_vec_.push_back(MultMktPrice::VarName() + "Combo");
  indicator_order_vec_.push_back(BidAskToPay::VarName());
  indicator_order_vec_.push_back(BookOrderCDiff::VarName());
  indicator_order_vec_.push_back(BookSizeDiff::VarName());
  indicator_order_vec_.push_back(DiffPriceType::VarName());
  indicator_order_vec_.push_back(MultMidOrderPrice::VarName());
  indicator_order_vec_.push_back(MultMidPrice::VarName());
  indicator_order_vec_.push_back(MultMktComplexOrderPrice::VarName());
  indicator_order_vec_.push_back(MultMktComplexPrice::VarName());
  indicator_order_vec_.push_back(MultMktComplexPriceShortAvg::VarName());
  indicator_order_vec_.push_back(MultMktComplexPriceTopOff::VarName());
  indicator_order_vec_.push_back(MultMktComplexPriceTopOff::VarName() + "Combo");
  indicator_order_vec_.push_back(MultMktOrderPrice::VarName());
  indicator_order_vec_.push_back(MultMktOrderPriceTopOff::VarName());
  indicator_order_vec_.push_back(MultMktPrice::VarName());
  indicator_order_vec_.push_back(SimpleBook::VarName());
  indicator_order_vec_.push_back(OfflineCorradjustedCutoffPairs::VarName() + "Combo");
  indicator_order_vec_.push_back(OnlineComputedCutoffPairMktEvents::VarName() + "Combo");
  indicator_order_vec_.push_back(OfflineComputedPairs::VarName());
  indicator_order_vec_.push_back(PCRPortReturns::VarName());
  indicator_order_vec_.push_back(OffPortReturns::VarName());
  indicator_order_vec_.push_back(OfflineComputedPairs::VarName() + "Combo");
  indicator_order_vec_.push_back(OfflineComputedPairsMktEvents::VarName());
  indicator_order_vec_.push_back(OfflineComputedPairsMktEvents::VarName() + "Combo");
  indicator_order_vec_.push_back(OfflineComputedPairsMktEvents2::VarName());
  indicator_order_vec_.push_back(OfflineComputedPairsMktEvents2::VarName() + "Combo");
  indicator_order_vec_.push_back(OfflineComputedReturnsPairs::VarName());
  indicator_order_vec_.push_back(OfflineComputedReturnsPairs::VarName() + "Combo");
  indicator_order_vec_.push_back(OfflineComputedReturnsPairsMktEvents::VarName());
  indicator_order_vec_.push_back(OfflineComputedReturnsPairsMktEvents::VarName() + "Combo");
  indicator_order_vec_.push_back(OfflineComputedReturnsCutoffPairs::VarName());
  indicator_order_vec_.push_back(OfflineComputedReturnsCutoffPairs::VarName() + "Combo");
  indicator_order_vec_.push_back(OfflineComputedReturnsPairsPort::VarName());
  indicator_order_vec_.push_back(OfflineComputedReturnsCutoffPairsPort::VarName());
  indicator_order_vec_.push_back(OfflineComputedReturnsCutoffPairsMktEvents::VarName());
  indicator_order_vec_.push_back(OfflineComputedReturnsCutoffPairsMktEvents::VarName() + "Combo");
  indicator_order_vec_.push_back(OfflineComputedReturnsPairsMktEventsPort::VarName());
  indicator_order_vec_.push_back(OfflineComputedReturnsCutoffPairsMktEventsPort::VarName());
  indicator_order_vec_.push_back(OfflineComputedPairsPort::VarName());
  indicator_order_vec_.push_back(OfflineComputedVolumeAdjustedPairs::VarName());
  indicator_order_vec_.push_back(OfflineComputedVolumeAdjustedPairs::VarName() + "Combo");
  indicator_order_vec_.push_back(OfflineComputedVolumeAdjustedPairsMktEvents::VarName());
  indicator_order_vec_.push_back(OfflineComputedVolumeAdjustedPairsMktEvents::VarName() + "Combo");
  indicator_order_vec_.push_back(OfflineCorradjustedPairs::VarName());
  indicator_order_vec_.push_back(OfflineCorradjustedPairs::VarName() + "Combo");
  indicator_order_vec_.push_back(OfflineCorradjustedPairsMktEvents::VarName());
  indicator_order_vec_.push_back(OfflineCorradjustedPairsMktEvents::VarName() + "Combo");
  indicator_order_vec_.push_back(OfflineLowCorrelationPairs::VarName());
  indicator_order_vec_.push_back(OfflineLowCorrelationPairsPort::VarName());
  indicator_order_vec_.push_back(OfflineVolumeCorradjustedPairs::VarName());
  indicator_order_vec_.push_back(OfflineVolumeCorradjustedPairs::VarName() + "Combo");
  indicator_order_vec_.push_back(OfflineVolumeCorradjustedPairsMktEvents::VarName());
  indicator_order_vec_.push_back(OfflineVolumeCorradjustedPairsMktEvents::VarName() + "Combo");
  indicator_order_vec_.push_back(TAOfflineCorradjustedPairs::VarName());
  indicator_order_vec_.push_back(TAOfflineCorradjustedPairs::VarName() + "Combo");
  indicator_order_vec_.push_back(SqrtTSizeTDAvgTDiffLvl::VarName());
  indicator_order_vec_.push_back(DiffTDAvgTPxBasepxLvl::VarName());
  indicator_order_vec_.push_back(DiffTDSizeAvgTPxBasepx::VarName());
  indicator_order_vec_.push_back(DiffTDSizeAvgTPxBasepxLvl::VarName());
  indicator_order_vec_.push_back(DiffTRAvgTPxBasepx::VarName());
  indicator_order_vec_.push_back(DiffTRSizeAvgTPxBasepx::VarName());
  indicator_order_vec_.push_back(TDSumHVLBTDiffTSize::VarName());
  indicator_order_vec_.push_back(TDSumLBTDiffTSize::VarName());
  indicator_order_vec_.push_back(TDSumTDiffFSize::VarName());
  indicator_order_vec_.push_back(TDSumTDiffFSizeOneLvl::VarName());
  indicator_order_vec_.push_back(TDSumTDiffFSqrtSize::VarName());
  indicator_order_vec_.push_back(TDSumTDiffLvlFSqrtSize::VarName());
  indicator_order_vec_.push_back(TDSumTDiffNSize::VarName());
  indicator_order_vec_.push_back(TDSumTDiffNSizeLvl::VarName());
  indicator_order_vec_.push_back(TDSumTDiffNSizeOneLvl::VarName());
  indicator_order_vec_.push_back(TDSumTDiffSqrtFSqrtSize::VarName());
  indicator_order_vec_.push_back(TDSumTDiffSqrtTSize::VarName());
  indicator_order_vec_.push_back(TDSumTDiffTSize::VarName());
  indicator_order_vec_.push_back(TDSumTDiffTSizeLvl::VarName());
  indicator_order_vec_.push_back(TDSumTDiffTSizeOneLvl::VarName());
  indicator_order_vec_.push_back(TDSumTType::VarName());
  indicator_order_vec_.push_back(TRAvgTDiffSqrtTSizeVolfactor::VarName());
  indicator_order_vec_.push_back(TRSumTDiffFSize::VarName());
  indicator_order_vec_.push_back(TRSumTDiffNSize::VarName());
  indicator_order_vec_.push_back(TRSumTDiffSqrtFSqrtSize::VarName());
  indicator_order_vec_.push_back(TRSumTDiffSqrtTSize::VarName());
  indicator_order_vec_.push_back(TRSumTDiffTSize::VarName());
  indicator_order_vec_.push_back(TRSumTType::VarName());
  indicator_order_vec_.push_back(TRSumTPriceTSize::VarName());
  indicator_order_vec_.push_back(ValueWBPMomentum::VarName());
  indicator_order_vec_.push_back(ValueWBPMomentum2::VarName());
  indicator_order_vec_.push_back(BidAskToPayGeneralized::VarName());
  indicator_order_vec_.push_back(BidAskToPayGeneralizedAllLvls::VarName());
  indicator_order_vec_.push_back(StudPriceDiffPort::VarName());
  indicator_order_vec_.push_back(EventBiasOfflineIndicator::VarName());
  indicator_order_vec_.push_back(ExpressionIndicator::VarName());
  indicator_order_vec_.push_back(TurnOverRate::VarName());
  indicator_order_vec_.push_back(RegimeTurnOverRate::VarName());
  indicator_order_vec_.push_back(L1PortPrice::VarName());
  indicator_order_vec_.push_back(SmoothTrend::VarName());
  indicator_order_vec_.push_back(SmoothZscore::VarName());
  indicator_order_vec_.push_back(SgMgTrRegime::VarName());
  indicator_order_vec_.push_back(OnlineImpliedYen::VarName());
  indicator_order_vec_.push_back(ReturnsStdev::VarName());
  indicator_order_vec_.push_back(ReturnsSimpleTrend::VarName());
  indicator_order_vec_.push_back(PriceNormalizedReturnsTrend::VarName());
  indicator_order_vec_.push_back(PriceNormalizedReturnsStdev::VarName());
  indicator_order_vec_.push_back(PortfolioDeviation::VarName());
  indicator_order_vec_.push_back(DynamicPriceL1::VarName());
  indicator_order_vec_.push_back(ImpliedVolCalculator::VarName());
  indicator_order_vec_.push_back(MovingAverageImpliedVol::VarName());
  indicator_order_vec_.push_back(OptionsGreek::VarName());
  indicator_order_vec_.push_back(L1Targetbias::VarName());
  indicator_order_vec_.push_back(RSVMRatioAdjSPDiff::VarName());
  indicator_order_vec_.push_back(OnlineComputedCutoffPairPort::VarName());
  indicator_order_vec_.push_back(OptionsInfo::VarName());
  indicator_order_vec_.push_back(DynamicWeightPortTrend::VarName());
  indicator_order_vec_.push_back(StudPriceTrendDiffDynamicWeightPort::VarName());
  indicator_order_vec_.push_back(OfflineComputedPairsDynamicWeightPort::VarName());
  indicator_order_vec_.push_back(MovingAvgBidAskIVSpread::VarName());
  indicator_order_vec_.push_back(RealizedVolCalculator::VarName());
  indicator_order_vec_.push_back(VWAPImpliedVol::VarName());
  // indicator_order_vec_.push_back(OptionsInfo::VarName());
  return indicator_order_vec_;
}

bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) {
  std::vector<std::string> core_shortcodes_;
  GetCoreShortcodes(r_dep_shortcode_, core_shortcodes_);

  std::vector<std::string> this_shortcodes_affecting_;
  std::vector<std::string> this_ors_source_needed_vec_;
  (CollectShortCodeFunc(tokens_[2]))(this_shortcodes_affecting_, this_ors_source_needed_vec_, tokens_);
  for (unsigned int idx = 0; idx < this_shortcodes_affecting_.size(); idx++) {
    if (VectorUtils::LinearSearchValue(
            core_shortcodes_,
            this_shortcodes_affecting_[idx]))  // making an assumption that this string refers to the shortcode
    {
      return true;
    }
  }
  return false;
}
}
