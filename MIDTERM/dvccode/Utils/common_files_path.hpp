#ifndef _COMMON_FILES_PATH_
#define _COMMON_FILES_PATH_

namespace HFSAT {
namespace FILEPATH {
const char* const kExchangeHolidaysFile = "/home/pengine/prod/live_configs/exchange_holidays.txt";
const char* const kExchangeStartDateFile = "/home/pengine/prod/live_configs/exchange_start_dates.txt";
const char* const kProductHolidaysFile = "/home/pengine/prod/live_configs/product_holidays.txt";
const char* const kProductStartDateFile = "/home/pengine/prod/live_configs/product_start_dates.txt";
const char* const kGlobalSimDataFile = "/spare/local/files/global_sim_data.txt";
const char* const kSACIPositionFolder = "/spare/local/ORSlogs/";

const char* const kJenkinsFolderPrefix = "/var/lib/jenkins/workspace/";

const char* const kGatewayShcMapFile = "/home/pengine/prod/live_configs/gateway_shc_map.txt";
const char* const kConsoleTraderCommonTagsFile = "/home/pengine/prod/live_configs/common_tags.txt";

const char* const kRealPacketsOrderDirectory = "/NAS1/data/SHMPacketsLoggedData/";
const char* const kRealSHMPacketsOrderDirectory = "/spare/local/MDSlogs/SHMPackets/";
const char* const kRealPacketsOrderFileName = "real_packets_order";

const char* const kCommonInitialMarginFileGitDir = "/home/pengine/codebase/margin_server/dvccode";
const char* const kCommonInitialMarginFilePath =
    "/home/pengine/codebase/margin_server/dvccode/configs/common_initial_margin_file.txt";
const char* const kModifiedCommonInitialMarginFilePath = "/tmp/common_initial_margin_file_modified.txt";
const char* const kExceptionalNormalTradingDateFile = "/home/pengine/prod/live_configs/exceptional_normal_trading_Date.txt";

extern char EUREX_CONTRACT_CODE_FILE[];
extern char CME_CONTRACT_CODE_FILE[];
}
}

#endif
