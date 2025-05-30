/**
    \file dvccode/CDef/error_codes.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_CDEF_ERROR_CODES_H
#define BASE_CDEF_ERROR_CODES_H

#include <iostream>
#include <string>

namespace HFSAT {

typedef enum {
  kExitErrorCodeZeroValue = 100,
  kExitErrorCodeGeneral,
  kCallIterativeRegressInFileOpenError,
  kCallIterativeRegressFewTokens,
  kCallIterativeRegressDiffNumTokensError,
  kCallIterativeRegressVeryFewLines,
  kCallIterativeRegressHighSharpeDependant,
  kCallIterativeRegressHighSharpeIndep,
  kEZStrUnknown,
  kModelCreationLinearModelMathNeedsNonNullDependant,
  kModelCreationNeedNonNullBaseModelMath,
  kModelCreationCouldNotOpenModelFile,
  kModelCreationModelInitLineLessArgs,
  kModelCreationModelInfoLessArgs,
  kModelCreationDepBaseLineLessArgs,
  kModelCreationIndicatorLineLessArgs,
  kModelCreationIndicatorLineNull,
  kModelCreationIndicatorIncorrectArgs,
  kModelCreationModelMathLineLessArgs,
  kModelCreationLinearModelMathLineLessArgs,
  kModelCreationLogisticModelMathLineLessArgs,
  kModelCreationSelectiveModelMathLineLessArgs,
  kModelCreationSelectiveModelMathNoIndicatorForRegime,
  kModelCreationNoModelWeightIndicator,
  kModelCreationIndicatorWeightNAN,
  kTradeInitCommandLineLessArgs,
  kHistoricalStdevManagerMissingShortcodeFromMap,
  kPortfolioConstituentManagerMissingArgs,
  kPortfolioConstituentManagerMissingPortFromMap,
  kPCAWeightManagerMissingPortFromMap,
  kPCAWeightManagerMissingshortcodeFromStdevMap,
  kRiskPortfolioConstituentManagerMissingArgs,
  kRiskPortfolioConstituentManagerMissingPortFromMap,
  kGetContractSpecificationMissingCode,
  kDataGenCommandLineLessArgs,
  kShortcodeSecurityMarketViewMapNoSMVInMap,
  kShortcodeORSMessageLivesourceMapNoSMVInMap,
  kShortcodeORSMessageFilesourceMapNoSMVInMap,
  kStrategyDescModelFileMissing,
  kStrategyDescParamFileMissing,
  kStrategyDescParamFileIncomplete,
  kStrategyDescTimeZoneError,
  kStrategyDescNoEntry,
  kStrategyDescRunTimeIdNotUnique,
  kDatToRegCommandLineLessArgs,
  kExchangeSymbolManagerUnset,
  kExchangeSymbolManagerUnhandledCase,
  kNormalSpreadManagerMissingInfoShortCode,
  kDepShortCodeVecEmptyTradeInit,
  kDepShortCodeVecTooManyTradeInit,
  kExitErrorCodeFIXFASTGeneral,
  kExitErrorCodeNetworkAccountInfoManager,
  kRiskManagerInitializationError,
  kCHIXOLMVMErrorGeneral,
  kNASDOLMVMErrorGeneral,
  kModelCreationNonLinearComponentLineLessArgs,
  kModelCreationNonLinearModelMathNeedsNonNullDependant,
  kModelCreationNonLinearComponentLineNull,
  kStrategyDescTradedEzoneMissing,
  kStirExitError,
  kShortcodeSpreadMarketViewMapNoSMVInMap,
  kCombinedRetailStrategyNotSpecifiedProperly,
  kDifferentOFFLINEMIXMMS_FILE,
  kOmixWtsNotFound,
  kSampleDataError,
  kSimRealPacketOrderMismatchDetectorArgsLess,
  kBoostFileSystemError,
  kBoostFileSystemAllocationStorageFailure,
  kNotOption,
  kExitErrorCodeIntegratedServerConfigManager
} ExitErrorCode_t;

std::string ErrorCodeToString(const ExitErrorCode_t& _exit_error_code_);
}
#endif  // BASE_CDEF_ERROR_CODES_H
