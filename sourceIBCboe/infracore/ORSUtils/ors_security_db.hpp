// =====================================================================================
//
//       Filename:  ors_security_db.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/07/2017 08:02:45 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include <iostream>
#include <cstdlib>

namespace HFSAT {
namespace ORSUtils {

struct SecurityDB {
  int32_t pos;
  double last_trade_px;
  double unreal_pnl;
  double realized_pnl;
  double n2d;
  std::string shortcode;
  double commission;
  bool is_nse;
  bool is_bse;
  bool is_cboe;
  bool is_options;
  double margin_factor;
  double margin_used;

  SecurityDB() {
    pos = 0;
    last_trade_px = 0;
    unreal_pnl = 0;
    realized_pnl = 0;
    n2d = 0;
    shortcode = "";
    commission = 0;
    is_nse = false;
    is_bse = false;
    margin_factor = 0;
  }
};

typedef SecurityDB SecurityDBArr[DEF_MAX_SEC_ID];

class SecurityDBManager {
 private:
  SecurityDBArr security_db_arr_;

  SecurityDBManager() : security_db_arr_() {}

  SecurityDBManager(SecurityDBManager const& disabled_copy_constructor) = delete;

 public:
  static SecurityDBManager& GetUniqueInstance() {
    static SecurityDBManager unique_instance;
    return unique_instance;
  }

  void SetSecurityShortcode(int32_t security_id, std::string sc) {

    std::cout << "SEC ID : " << security_id <<  "  SHC : " << sc << std::endl;
    
    security_db_arr_[security_id].shortcode = sc;
    security_db_arr_[security_id].n2d = 0;

    // Currently Only NSE & BSE Has Options
    if (std::string::npos != sc.find("NSE_")) {
      if (HFSAT::NSESecurityDefinitions::IsOption(sc)) {
        security_db_arr_[security_id].is_options = true;
        security_db_arr_[security_id].is_nse = true;
      }
    }
    else if (std::string::npos != sc.find("BSE_")) {
      if (HFSAT::BSESecurityDefinitions::IsOption(sc)) {
        security_db_arr_[security_id].is_options = true;
        security_db_arr_[security_id].is_bse = true;
      }
    }
    else if (std::string::npos != sc.find("CBOE_")) {
      if (HFSAT::CBOESecurityDefinitions::IsOption(sc)) {
        security_db_arr_[security_id].is_options = true;
        security_db_arr_[security_id].is_cboe = true;
      }
    }

  }

  SecurityDBArr& GetSecurityDB() { return security_db_arr_; }
};
}
}
