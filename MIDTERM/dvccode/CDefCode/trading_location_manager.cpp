#include <fstream>
#include <cstdio>
#include <sys/types.h>
#include <pwd.h>
#include <sys/unistd.h>
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include <sstream>
#include <string>

namespace HFSAT {
namespace TradingLocationUtils {

#define TRADING_LOC_FILE "infracore_install/files/trading_locations.txt"

TradingLocation_t GetTradingLocationFromLOC_NAME(const std::string location) {
  // might be optimized, perhaps we dont care now
  if (location == "CHI") return kTLocCHI;

  if (location == "FR2") return kTLocFR2;

  if (location == "NY4") return kTLocNY4;

  if (location == "BRZ")  // changed from BMF to BRZ for correct logged data path
    return kTLocBMF;

  if (location == "MADRID") return kTLocMAD;

  if (location == "HK") return kTLocHK;

  if (location == "IX4") return kTLocIX4;

  if (location == "TOR") return kTLocTMX;

  if (location == "TOK") return kTLocJPY;

  if (location == "BSL") return kTLocBSL;

  if (location == "CRT") return kTLocCRT;

  if (location == "OTK") return kTLocOTK;

  if (location == "M1") return kTLocM1Old;

  if (location == "MOS") return kTLocM1;

  if (location == "CFE") return kTLocCFE;

  if (location == "NSE") return kTLocNSE;

  if (location == "SPR") return kTLocSPR;

  if (location == "BSE") return kTLocBSE;

  if (location == "ASX") return kTLocSYD;

  if (location == "SPR") return kTLocSPR;

  if (location == "KRX") return kTLocKRX;

  return kTLocMAX;
}

TradingLocation_t GetTradingLocationFromHostname() {
  char hostname[1024];
  hostname[1023] = '\0';
  gethostname(hostname, 1023);
  /**
   * sdv-ny4
   * sdv-chi
   * sdv-fr2
   * sdv-tor
   * sdv-bmf
   * DVC-BSL-01
   */

  if (strncmp("sdv-ny4", hostname, 7) == 0) return kTLocNY4;
  if (strncmp("sdv-chi", hostname, 7) == 0) return kTLocCHI;
  if (strncmp("sdv-fr2", hostname, 7) == 0) return kTLocFR2;
  if (strncmp("sdv-tor", hostname, 7) == 0) return kTLocTMX;
  if (strncmp("sdv-bmf", hostname, 7) == 0) return kTLocBMF;
  if (!strncmp("sdv-bsl", hostname, 7)) return kTLocBSL;
  if (strncmp("sdv-crt", hostname, 7) == 0) return kTLocCRT;
  if (!strncmp("dcm-osl", hostname, 7)) return kTLocOTK;
  if (!strncmp("sdv-mos", hostname, 7)) return kTLocM1;
  if (!strncmp("sdv-m1", hostname, 6)) return kTLocM1Old;
  if (strncmp("SDV-TOK", hostname, 7) == 0) return kTLocJPY;
  if (strncmp("sdv-ose", hostname, 7) == 0) return kTLocJPY;
  if (strncmp("SDV-HK", hostname, 6) == 0) return kTLocHK;
  if (strncmp("sdv-cfe", hostname, 7) == 0) return kTLocCFE;
  if (strncmp("SDV-ASX", hostname, 7) == 0) return kTLocSYD;
  if (strncmp("sdv-ind", hostname, 7) == 0) return kTLocNSE;
  if (strncmp("sdv-sgx", hostname, 7) == 0) return kTLocSPR;
  return kTLocMAX;
}

TradingLocation_t GetTradingLocationFromIP(std::string ip_address) {
  TradingLocation_t toRet = kTLocMAX;
  std::fstream file(HFSAT::FileUtils::AppendHome(TRADING_LOC_FILE).c_str(), std::ofstream::in);
  if (!file || !file.is_open()) {
    fprintf(stderr, "Could not open file %s in trading location manager", TRADING_LOC_FILE);
    // return default value
    // exit(-1);
    return toRet;
  }

  char line[1024];
  char* loc;

  memset(line, 0, sizeof(line));

  while (file.getline(line, sizeof(line))) {
    loc = NULL;
    loc = strtok(line, "\n\t ");
    if (loc && strstr(loc, "#") == NULL) {
      std::string interface = std::string(strtok(NULL, "\n\t "));
      std::string ip = std::string(strtok(NULL, "\n\t "));
      if (ip == ip_address) {
        toRet = GetTradingLocationFromLOC_NAME(loc);
        break;
      }
    }
  }
  file.close();
  return toRet;
}

const char* GetTradingLocationName(const TradingLocation_t r_trading_location_) {
  switch (r_trading_location_) {
    case kTLocCHI:
      return "CHI";
      break;
    case kTLocFR2:
      return "FR2";
      break;
    case kTLocNY4:
      return "NY4";
      break;
    case kTLocTMX:
      return "TOR";
      break;
    case kTLocBMF:
      return "BRZ";
      break;
    case kTLocMAD:
      return "MADRID";
      break;
    case kTLocMIL:
      return "MILAN";
      break;
    case kTLocHK:
      return "HK";
      break;
    case kTLocIX4:
      return "IX4";
      break;
    case kTLocBSL:
      return "BSL";
      break;
    case kTLocCRT:
      return "CRT";
      break;
    case kTLocOTK:
      return "OTK";
      break;
    case kTLocM1:
      return "MOS";
      break;
    case kTLocM1Old:
      return "M1";
      break;
    case kTLocJPY:
      return "TOK";
      break;
    case kTLocCFE:
      return "CFE";
      break;
    case kTLocSYD:
      return "SYD";
      break;
    case kTLocNSE:
      return "NSE";
      break;
    case kTLocSPR:
      return "SPR";
      break;
    case kTLocBSE:
      return "BSE";
      break;
    case kTLocKRX:
      return "KRX";
      break;
    case kTLocMAX:
    default:
      return "-X-";
      break;
  }
}

bool IsCurrentLocationPrimary(const ExchSource_t r_exch_source_) {
  if (HFSAT::TradingLocationUtils::GetTradingLocationExch(r_exch_source_) ==
      HFSAT::TradingLocationUtils::GetTradingLocationFromHostname())
    return true;
  return false;
}

int GetAddedDelay(const TradingLocation_t trading_location_file_read_, const TradingLocation_t r_trading_location_) {
  std::string dst_exchange_name = std::string(GetTradingLocationName(trading_location_file_read_));
  std::string src_exchange_name = std::string(GetTradingLocationName(r_trading_location_));
  int added_delay = 0;
  int added_delay_temp;

  // Getting current username
  std::string current_user = getenv("USER");
  std::string line;
  std::string added_delay_file_location = "/spare/local/" + current_user + "/line_latency_.txt";
  std::ifstream infile(added_delay_file_location);

  while (std::getline(infile, line)) {
    std::istringstream iss(line);
    std::string loc_1, loc_2;  // location 1 and 2

    if (!(iss >> loc_1 >> loc_2 >> added_delay_temp)) {
      std::cout << "ERROR ADDED DELAY FILE NOT IN CORRECT FORMAT AT " << added_delay_file_location << std::endl;
      break;
    }  // error

    if ((loc_1.compare(dst_exchange_name) == 0 || loc_2.compare(src_exchange_name) == 0) ||
        (loc_2.compare(dst_exchange_name) == 0 || loc_1.compare(src_exchange_name) == 0)) {
      std::cout << "USING ADDED LINE DELAY BETWEEN " << loc_1 << "\t" << loc_2 << "\tDelay Value: " << added_delay_temp
                << std::endl;
      added_delay = (added_delay_temp / 2);
    }
  }

  return added_delay;
}

unsigned int GetUSecsBetweenTradingLocations(const TradingLocation_t dest_trading_location_,
                                             const TradingLocation_t src_trading_location_) {
#define CRT_CHI_USECS 7300
#define SYD_TOK_USECS 50000
#define HK_TOK_USECS 26000
#define CHI_TOK_USECS 64000
#define CHI_SYD_USECS 90000
#define SYD_FR2_USECS 140000
#define SYD_BSL_USECS 132000
#define SGX_NSE_USECS 27810
#define TOK_SGX_USECS 33550
#define HK_SGX_USECS 15600

  switch (dest_trading_location_) {
    case kTLocCHI: {
      switch (src_trading_location_) {
        case kTLocCRT:
          return CRT_CHI_USECS;
          break;
        case kTLocSYD:
          return (CHI_SYD_USECS);
          break;
        // case kTLocSYD: return ( CHI_TOK_USECS + SYD_TOK_USECS ); break;
        case kTLocSPR:
          return (TOK_SGX_USECS + CHI_TOK_USECS);
          break;
        default:
          return GetMSecsBetweenTradingLocations(dest_trading_location_, src_trading_location_) * 1000;
          break;
      }
    } break;

    case kTLocCRT: {
      switch (src_trading_location_) {
        case kTLocCHI:
          return CRT_CHI_USECS;
          break;
        default:
          return GetMSecsBetweenTradingLocations(dest_trading_location_, src_trading_location_) * 1000;
          break;
      }
    } break;

    case kTLocJPY: {
      switch (src_trading_location_) {
        case kTLocSYD:
          return (CHI_SYD_USECS + CHI_TOK_USECS);
          break;
        case kTLocSPR:
          return TOK_SGX_USECS;
          break;
        default:
          return GetMSecsBetweenTradingLocations(dest_trading_location_, src_trading_location_) * 1000;
          break;
      }
    } break;

    case kTLocSYD: {
      switch (src_trading_location_) {
        case kTLocJPY:
          return (CHI_SYD_USECS + CHI_TOK_USECS);
          break;
        case kTLocHK:
          return (CHI_SYD_USECS + CHI_TOK_USECS + HK_TOK_USECS);
          break;
        case kTLocCHI:
          return (CHI_SYD_USECS);
          break;
        default:
          return GetMSecsBetweenTradingLocations(dest_trading_location_, src_trading_location_) * 1000;
          break;
      }
    } break;

    case kTLocHK: {
      switch (src_trading_location_) {
        case kTLocSYD:
          return (CHI_SYD_USECS + CHI_TOK_USECS + HK_TOK_USECS);
          break;
        case kTLocSPR:
          return HK_SGX_USECS;
          break;
        default:
          return GetMSecsBetweenTradingLocations(dest_trading_location_, src_trading_location_) * 1000;
          break;
      }
    } break;
    case kTLocNSE: {
      switch (src_trading_location_) {
        case kTLocHK: {
          return (HK_SGX_USECS + SGX_NSE_USECS);
        }
        case kTLocJPY: {
          return (TOK_SGX_USECS + SGX_NSE_USECS);
        }
        case kTLocSPR: {
          return (SGX_NSE_USECS);
        }
        default:
          return GetMSecsBetweenTradingLocations(dest_trading_location_, src_trading_location_) * 1000;
      }
    } break;
    case kTLocFR2: {
      switch (src_trading_location_) {
        case kTLocSYD:
          return (SYD_FR2_USECS);
          break;
        default:
          return GetMSecsBetweenTradingLocations(dest_trading_location_, src_trading_location_) * 1000;
          break;
      }
    }
    case kTLocBSL: {
      switch (src_trading_location_) {
        case kTLocSYD:
          return (SYD_BSL_USECS);
          break;
        default:
          return GetMSecsBetweenTradingLocations(dest_trading_location_, src_trading_location_) * 1000;
          break;
      }
    }
    case kTLocSPR: {
      switch (src_trading_location_) {
        case kTLocNSE:
          return SGX_NSE_USECS;
        case kTLocHK:
          return HK_SGX_USECS;
        case kTLocJPY:
          return TOK_SGX_USECS;
        case kTLocCHI:
          return (CHI_TOK_USECS + TOK_SGX_USECS);
        default:
          return GetMSecsBetweenTradingLocations(dest_trading_location_, src_trading_location_) * 1000;
          break;
      }
    }
    default: { return GetMSecsBetweenTradingLocations(dest_trading_location_, src_trading_location_) * 1000; } break;
  }

  return 0;
}

unsigned int GetMSecsBetweenTradingLocations(const TradingLocation_t dest_trading_location_,
                                             const TradingLocation_t src_trading_location_) {
#define SELF_DELAY 0
#define CHI_FR2_DELAY 45
#define CHI_NY4_DELAY 7  // rounded up
#define CHI_BMF_DELAY 63
#define CHI_TMX_DELAY 11
#define CHI_TOK_DELAY 64
#define CHI_HK_DELAY 83
#define FR2_NY4_DELAY 38
#define FR2_BMF_DELAY 93
#define FR2_TMX_DELAY 51
#define FR2_TOK_DELAY 110
#define FR2_CRT_DELAY 35
#define CRT_FR2_DELAY 35
#define BMF_NY4_DELAY 55
#define BMF_CRT_DELAY 54
#define CRT_BMF_DELAY 54
#define BMF_TMX_DELAY 78
#define TMX_NY4_DELAY 13
#define BSL_FR2_DELAY 8
#define BSL_NY4_DELAY 39  // rounded up
#define BSL_CHI_DELAY 42  // rounded up
#define BSL_BMF_DELAY 94  // verify
#define BSL_CRT_DELAY 31
#define CRT_BSL_DELAY 31
#define HK_TOK_DELAY 26
#define BSL_MOS_DELAY 23
#define MOS_BSL_DELAY 23
#define MOS_FR2_DELAY 28
#define FR2_MOS_DELAY 28
#define MOS_CHI_DELAY 62
#define CHI_MOS_DELAY 62
#define MOS_CRT_DELAY 55
#define CRT_MOS_DELAY 55
#define M1OlD_MOS_DELAY 0
#define OTK_MOS_DELAY 0
#define M1_MOS_DELAY 0
#define CRT_CHI_DELAY 7  // rounded up
#define NY4_CFE_DELAY 0
#define CFE_NY4_DELAY 0
#define MOS_BMF_DELAY 109
#define BMF_MOS_DELAY 109
#define SYD_TOK_DELAY 50
#define CHI_SYD_DELAY 90
#define SGX_NSE_DELAY 28
#define TOK_SGX_DELAY 33   // rounded
#define HK_SGX_DELAY 15    // rounded
#define CHI_NSE_DELAY 131  // CHI-TOK-SGX-NSE
#define FR2_NSE_DELAY 190  // please verify
#define TOK_KRX_DELAY 9

  switch (dest_trading_location_) {
    case kTLocCHI:
      switch (src_trading_location_) {
        case kTLocCHI:
          return SELF_DELAY;
          break;
        case kTLocFR2:
          return CHI_FR2_DELAY;
          break;
        case kTLocNY4:
          return CHI_NY4_DELAY;
          break;
        case kTLocTMX:
          return CHI_TMX_DELAY;
          break;
        case kTLocBMF:
          return CHI_BMF_DELAY;
          break;
        case kTLocBSL:
          return BSL_CHI_DELAY;
          break;
        case kTLocJPY:
          return CHI_TOK_DELAY;
          break;
        case kTLocHK:
          return CHI_HK_DELAY;
          break;
        case kTLocOTK:
          return CHI_MOS_DELAY;
          break;
        case kTLocM1:
          return CHI_MOS_DELAY;
          break;
        case kTLocCRT:
          return CRT_CHI_DELAY;
          break;
        case kTLocSYD:
          return CHI_SYD_DELAY;
          break;
        case kTLocNSE:
          return CHI_NSE_DELAY;
          break;
        default:
          return SELF_DELAY;
          break;
      }
      break;

    case kTLocFR2:
      switch (src_trading_location_) {
        case kTLocCHI:
          return CHI_FR2_DELAY;
          break;
        case kTLocFR2:
          return SELF_DELAY;
          break;
        case kTLocNY4:
          return FR2_NY4_DELAY;
          break;
        case kTLocTMX:
          return FR2_TMX_DELAY;
          break;
        case kTLocBMF:
          return FR2_BMF_DELAY;
          break;
        case kTLocBSL:
          return BSL_FR2_DELAY;
          break;
        case kTLocOTK:
          return FR2_MOS_DELAY;
          break;
        case kTLocM1:
          return FR2_MOS_DELAY;
          break;
        case kTLocJPY:
          return FR2_TOK_DELAY;
          break;
        case kTLocCRT:
          return CRT_FR2_DELAY;
          break;
        case kTLocNSE:
          return FR2_NSE_DELAY;
          break;
        default:
          return SELF_DELAY;
          break;
      }
      break;

    case kTLocBSL:
      switch (src_trading_location_) {
        case kTLocCHI:
          return BSL_CHI_DELAY;
          break;
        case kTLocBSL:
          return SELF_DELAY;
          break;
        case kTLocFR2:
          return BSL_FR2_DELAY;
          break;
        case kTLocNY4:
          return BSL_NY4_DELAY;
          break;
        case kTLocBMF:
          return BSL_BMF_DELAY;
          break;
        case kTLocOTK:
          return BSL_MOS_DELAY;
          break;
        case kTLocM1:
          return BSL_MOS_DELAY;
          break;
        case kTLocCRT:
          return CRT_BSL_DELAY;
          break;
        default:
          return SELF_DELAY;
          break;
      }
      break;

    case kTLocBMF:
      switch (src_trading_location_) {
        case kTLocCHI:
          return CHI_BMF_DELAY;
          break;
        case kTLocFR2:
          return FR2_BMF_DELAY;
          break;
        case kTLocNY4:
          return BMF_NY4_DELAY;
          break;
        case kTLocTMX:
          return BMF_TMX_DELAY;
          break;
        case kTLocBSL:
          return BSL_BMF_DELAY;
          break;
        case kTLocCRT:
          return CRT_BMF_DELAY;
          break;
        case kTLocM1:
          return BMF_MOS_DELAY;
          break;
        case kTLocBMF:
          return SELF_DELAY;
          break;
        default:
          return SELF_DELAY;
          break;
      }
      break;

    case kTLocTMX:
      switch (src_trading_location_) {
        case kTLocCHI:
          return CHI_TMX_DELAY;
          break;
        case kTLocFR2:
          return FR2_TMX_DELAY;
          break;
        case kTLocNY4:
          return TMX_NY4_DELAY;
          break;
        case kTLocTMX:
          return SELF_DELAY;
          break;
        case kTLocBMF:
          return BMF_TMX_DELAY;
          break;
        default:
          return SELF_DELAY;
          break;
      }
      break;

    case kTLocJPY:
      switch (src_trading_location_) {
        case kTLocHK:
          return HK_TOK_DELAY;
          break;
        case kTLocCHI:
          return CHI_TOK_DELAY;
          break;
        case kTLocFR2:
          return FR2_TOK_DELAY;
          break;
        case kTLocSYD:
          return (CHI_SYD_DELAY + CHI_TOK_DELAY);
          break;
        default:
          return SELF_DELAY;
          break;
      }
      break;

    case kTLocHK:
      switch (src_trading_location_) {
        case kTLocJPY:
          return HK_TOK_DELAY;
          break;
        case kTLocCHI:
          return CHI_HK_DELAY;
          break;
        case kTLocSYD:
          return (CHI_HK_DELAY + CHI_SYD_DELAY);
          break;
        default:
          return SELF_DELAY;
          break;
      }
      break;
    case kTLocNSE: {
      switch (src_trading_location_) {
        case kTLocJPY:
          return (TOK_SGX_DELAY + SGX_NSE_DELAY);
          break;
        case kTLocHK:
          return (HK_SGX_DELAY + SGX_NSE_DELAY);
        case kTLocCHI:
          return (CHI_NSE_DELAY);
          break;
        case kTLocFR2:
          return (FR2_NSE_DELAY);
        case kTLocSPR:
          return (SGX_NSE_DELAY);
          break;
        default:
          return SELF_DELAY;
          break;
      }
    } break;
    case kTLocCRT:
      switch (src_trading_location_) {
        case kTLocBMF:
          return CRT_BMF_DELAY;
          break;
        case kTLocFR2:
          return FR2_CRT_DELAY;
          break;
        case kTLocBSL:
          return BSL_CRT_DELAY;
          break;
        case kTLocCHI:
          return CRT_CHI_DELAY;
          break;
        default:
          return SELF_DELAY;
          break;
      }
      break;

    case kTLocOTK:
    case kTLocM1:
    case kTLocM1Old: {
      switch (src_trading_location_) {
        case kTLocFR2:
          return FR2_MOS_DELAY;
          break;
        case kTLocCHI:
          return CHI_MOS_DELAY;
          break;
        case kTLocBSL:
          return BSL_MOS_DELAY;
          break;
        case kTLocCRT:
          return CRT_MOS_DELAY;
          break;
        default:
          return SELF_DELAY;
          break;
      }
      break;
    }

    case kTLocCFE:
      switch (src_trading_location_) {
        case kTLocNY4:
          return CFE_NY4_DELAY;
          break;
        default:
          return SELF_DELAY;
          break;
      }
      break;

    case kTLocSYD: {
      switch (src_trading_location_) {
        case kTLocJPY:
          return (CHI_SYD_DELAY + CHI_TOK_DELAY);
          break;
        case kTLocHK:
          return (CHI_SYD_DELAY + CHI_HK_DELAY);
          break;
        case kTLocCHI:
          return CHI_SYD_DELAY;
          break;
        default:
          return SELF_DELAY;
          break;
      }
    } break;

    case kTLocNY4: {
      switch (src_trading_location_) {
        case kTLocFR2:
          return FR2_NY4_DELAY;
          break;
        default:
          return SELF_DELAY;
          break;
      }
    } break;

    case kTLocSPR: {
      switch (src_trading_location_) {
        case kTLocHK:
          return HK_SGX_DELAY;
          break;
        case kTLocJPY:
          return TOK_SGX_DELAY;
        case kTLocCHI:
          return (CHI_TOK_DELAY + TOK_SGX_DELAY);
        case kTLocFR2:
          return (FR2_TOK_DELAY + TOK_SGX_DELAY);
        default:
          return SELF_DELAY;
          break;
      }
    } break;
    default:
      return SELF_DELAY;
      break;

    case kTLocKRX: {
      switch (src_trading_location_) {
        case kTLocJPY:
          return TOK_KRX_DELAY;
        case kTLocCHI:
          return (CHI_TOK_DELAY + TOK_KRX_DELAY);
        case kTLocFR2:
          return (FR2_TOK_DELAY + TOK_KRX_DELAY);
        case kTLocHK:
          return (HK_TOK_DELAY + TOK_KRX_DELAY);
        default:
          return SELF_DELAY;
          break;
      }
    }
  }
}

bool UseFasterDataForShortcodeAtLocation(std::string t_shortcode_, const TradingLocation_t t_trading_location_,
                                         const int& _trading_date_, bool t_use_fake_faster_data_) {
  // GetUsecsForFasterDataForShortcodeAtLocation ( t_shortcode_ , t_trading_location_ ) != 0
  // only if this returns true for ( t_shortcode_ , t_trading_location_ )
  /*
   if (t_trading_location_ == kTLocBMF || t_trading_location_ == kTLocBSL || t_trading_location_ == kTLocFR2 ||
      t_trading_location_ == kTLocM1 || t_trading_location_ == kTLocCRT || t_trading_location_ == kTLocCFE) {
    if (t_shortcode_.substr(0, 1) == "6" || t_shortcode_.substr(0, 2) == "ES" || t_shortcode_.substr(0, 2) == "YM" ||
        t_shortcode_.substr(0, 2) == "NQ") {
      return true;
    }
    }*/

  if ((t_trading_location_ == kTLocBSL || t_trading_location_ == kTLocFR2) &&
      (_trading_date_ < 20141029))  // WE moved to using arbitration of CFN microwave on 29th
  {
    if (t_shortcode_.substr(0, 2) == "ZN" || t_shortcode_.substr(0, 2) == "ZB" || t_shortcode_.substr(0, 2) == "ZF" ||
        t_shortcode_.substr(0, 2) == "ZT" || t_shortcode_.substr(0, 2) == "UB") {
      return true;  // we don't get fast data for these in real, so return true only if we want to use fake acceleration
    }
  }
  return false;
}

unsigned int GetUsecsForFasterDataForShortcodeAtLocation(std::string t_shortcode_,
                                                         const TradingLocation_t t_trading_location_,
                                                         const int& _trading_date_, bool t_use_fake_faster_data_) {
  if (!UseFasterDataForShortcodeAtLocation(t_shortcode_, t_trading_location_, _trading_date_,
                                           t_use_fake_faster_data_)) {
    return 0;
  }
  unsigned int speedup_usecs_ = 0;

  switch (t_trading_location_) {
    case kTLocFR2:
    case kTLocBSL: {
      if (t_shortcode_.substr(0, 2) == "ZN" || t_shortcode_.substr(0, 2) == "ZB" || t_shortcode_.substr(0, 2) == "ZF" ||
          t_shortcode_.substr(0, 2) == "ZT" || t_shortcode_.substr(0, 2) == "UB") {
        speedup_usecs_ = 2900;
      }

      if (t_shortcode_.substr(0, 1) == "6" || t_shortcode_.substr(0, 2) == "ES" || t_shortcode_.substr(0, 2) == "YM" ||
          t_shortcode_.substr(0, 2) == "NQ") {
        speedup_usecs_ = 3100;
      }
    } break;

    case kTLocCFE: {
      if (t_shortcode_.substr(0, 2) == "ES" || t_shortcode_.substr(0, 2) == "YM" || t_shortcode_.substr(0, 2) == "NQ") {
        speedup_usecs_ = 3150;
      }

      if (t_shortcode_.substr(0, 1) == "6") {
        speedup_usecs_ = 2900;
      }
    } break;

    case kTLocBMF:
    case kTLocM1:
    case kTLocCRT: {
      if (t_shortcode_.substr(0, 1) == "6" || t_shortcode_.substr(0, 2) == "ES" || t_shortcode_.substr(0, 2) == "YM" ||
          t_shortcode_.substr(0, 2) == "NQ") {
        speedup_usecs_ = 3100;
      }
    } break;

    default: { speedup_usecs_ = 0; } break;
  }

  return speedup_usecs_;
}
}
}
