/**
    \file CDefCode/error_codes.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/error_codes.hpp"

namespace HFSAT {

std::string ErrorCodeToString(const ExitErrorCode_t& _exit_error_code_) {
  std::cerr << _exit_error_code_ << std::endl;
  static std::string ExitErrorCodeStr[] = {"kExitErrorCodeZeroValue",
                                           "kExitErrorCodeGeneral",
                                           "kCallIterativeRegressInFileOpenError",
                                           "kCallIterativeRegressFewTokens",
                                           "kCallIterativeRegressDiffNumTokensError",
                                           "kCallIterativeRegressVeryFewLines",
                                           "kCallIterativeRegressHighSharpeDependant",
                                           "kCallIterativeRegressHighSharpeIndep",
                                           "kEZStrUnknown",
                                           "kModelCreationLinearModelMathNeedsNonNullDependant",
                                           "kModelCreationNeedNonNullBaseModelMath",
                                           "kModelCreationCouldNotOpenModelFile",
                                           "kModelCreationModelInitLineLessArgs",
                                           "kModelCreationModelInfoLessArgs",
                                           "kModelCreationDepBaseLineLessArgs",
                                           "kModelCreationIndicatorLineLessArgs",
                                           "kModelCreationIndicatorLineNull",
                                           "kModelCreationIndicatorIncorrectArgs",
                                           "kModelCreationModelMathLineLessArgs",
                                           "kModelCreationLinearModelMathLineLessArgs",
                                           "kModelCreationLogisticModelMathLineLessArgs",
                                           "kModelCreationSelectiveModelMathLineLessArgs",
                                           "kModelCreationSelectiveModelMathNoIndicatorForRegime",
                                           "kModelCreationNoModelWeightIndicator",
                                           "kModelCreationIndicatorWeightNAN",
                                           "kTradeInitCommandLineLessArgs",
                                           "kHistoricalStdevManagerMissingShortcodeFromMap",
                                           "kPortfolioConstituentManagerMissingArgs",
                                           "kPortfolioConstituentManagerMissingPortFromMap",
                                           "kPCAWeightManagerMissingPortFromMap",
                                           "kPCAWeightManagerMissingshortcodeFromStdevMap",
                                           "kRiskPortfolioConstituentManagerMissingArgs",
                                           "kRiskPortfolioConstituentManagerMissingPortFromMap",
                                           "kGetContractSpecificationMissingCode",
                                           "kDataGenCommandLineLessArgs",
                                           "kShortcodeSecurityMarketViewMapNoSMVInMap",
                                           "kShortcodeORSMessageLivesourceMapNoSMVInMap",
                                           "kShortcodeORSMessageFilesourceMapNoSMVInMap",
                                           "kStrategyDescModelFileMissing",
                                           "kStrategyDescParamFileMissing",
                                           "kStrategyDescParamFileIncomplete",
                                           "kStrategyDescTimeZoneError",
                                           "kStrategyDescNoEntry",
                                           "kStrategyDescRunTimeIdNotUnique",
                                           "kDatToRegCommandLineLessArgs",
                                           "kExchangeSymbolManagerUnset",
                                           "kExchangeSymbolManagerUnhandledCase",
                                           "kNormalSpreadManagerMissingInfoShortCode",
                                           "kDepShortCodeVecEmptyTradeInit",
                                           "kDepShortCodeVecTooManyTradeInit",
                                           "kExitErrorCodeFIXFASTGeneral",
                                           "kExitErrorCodeNetworkAccountInfoManager",
                                           "kRiskManagerInitializationError",
                                           "kCHIXOLMVMErrorGeneral",
                                           "kNASDOLMVMErrorGeneral",
                                           "kModelCreationNonLinearComponentLineLessArgs",
                                           "kModelCreationNonLinearModelMathNeedsNonNullDependant",
                                           "kModelCreationNonLinearComponentLineNull",
                                           "kStrategyDescTradedEzoneMissing",
                                           "kStirExitError",
                                           "kShortcodeSpreadMarketViewMapNoSMVInMap",
                                           "kCombinedRetailStrategyNotSpecifiedProperly",
                                           "kDifferentOFFLINEMIXMMS_FILE",
                                           "kOmixWtsNotFound",
                                           "kSampleDataError",
                                           "kSimRealPacketOrderMismatchDetectorArgsLess",
                                           "kBoostFileSystemError",
                                           "kBoostFileSystemAllocationStorageFailure",
                                           "kNotOption",
                                           "kExitErrorCodeIntegratedServerConfigManager"};

  return ExitErrorCodeStr[(_exit_error_code_ - kExitErrorCodeZeroValue)];
}
}
