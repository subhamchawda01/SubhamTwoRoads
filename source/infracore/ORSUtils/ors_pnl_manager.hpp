// =====================================================================================
//
//       Filename:  ors_pnl_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/04/2017 04:25:08 AM
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

#include <cstdlib>
#include <iostream>

#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/bse_security_definition.hpp"
#include "infracore/ORSUtils/ors_security_db.hpp"

//#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"

namespace HFSAT {
namespace ORSUtils {

class ORSPnlManager {
 private:
  HFSAT::DebugLogger& dbglogger_;
  double total_pnl_;
  double stop_pnl_;
  SecurityDBArr& security_db_arr_;
  bool pnl_checks_enabled_;
  int32_t today_date_;
  std::map<std::string, int> di_reserves_map_;
  bool alert_sent_;
  bool should_alert_;

  ORSPnlManager(HFSAT::DebugLogger& logger)
      : dbglogger_(logger),
        total_pnl_(0),
        stop_pnl_(0),
        security_db_arr_(SecurityDBManager::GetUniqueInstance().GetSecurityDB()),
        pnl_checks_enabled_(false),
        today_date_(HFSAT::DateTime::GetCurrentIsoDateLocal()),
        di_reserves_map_(),
        alert_sent_(false),
        should_alert_(false) {
    if (HFSAT::GetCurrentHostName().find("ind") != std::string::npos) {
      should_alert_ = true;
    }
    //        HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance().AddSSSIListener(this);
  }

  ORSPnlManager(ORSPnlManager const& disabled_copy_constructor) = delete;

 public:
  static ORSPnlManager& GetUniqueInstance(HFSAT::DebugLogger& logger) {
    static ORSPnlManager unique_instance(logger);
    return unique_instance;
  }

  double GetN2D(std::string& shortcode, int yyyymmdd, double cur_price) {
    // Simple for non-DI contracts
    if (shortcode.substr(0, 3) != "DI1") {
      if (HFSAT::SecurityDefinitions::CheckIfContractSpecExists(shortcode, yyyymmdd)) {
        return HFSAT::SecurityDefinitions::GetContractNumbersToDollars(shortcode, yyyymmdd);
      } else {
        return 1;
      }
    }

    // For DI contracts
    if (di_reserves_map_.find(shortcode) == di_reserves_map_.end()) {
      di_reserves_map_[shortcode] = SecurityDefinitions::GetDIReserves(yyyymmdd, shortcode);
    }
    if (cur_price == 0.0) {
      // just to avoid inf values when cur_price is 0.0, we return invalid value
      return 1;
    }

    double unit_price = 0;
    double term = double(di_reserves_map_[shortcode] / 252.0);
    if (term > 0.000) {
      unit_price = 100000 / std::pow((cur_price / 100 + 1), term);
    }

    return -(unit_price * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD) / cur_price);
  }

  void SetORSStopPnl(double stop_pnl) {
    stop_pnl_ = stop_pnl > 0 ? (stop_pnl * -1) : stop_pnl;
    pnl_checks_enabled_ = !(0 == stop_pnl_);
    alert_sent_ = false;
    DBGLOG_CLASS_FUNC_LINE_INFO << "SETORSPNL : " << stop_pnl_ << " CHECKS_ENABLED ? : " << pnl_checks_enabled_
                                << " ALERT SENT ? " << alert_sent_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }

  int32_t GetCurrentORSPnl() { return total_pnl_; }

  inline bool AllowsPnlCheck() {
    if (!pnl_checks_enabled_) return true;
    return (total_pnl_ > stop_pnl_);
  }

  void DumpORSPnlStatus() {
    DBGLOG_CLASS_FUNC_LINE_INFO
        << "===================================== ORS PNL STATUS =====================================\n";

    DBGLOG_CLASS_FUNC_LINE_INFO << "TOTAL : " << total_pnl_ << " "
                                << "STOP : " << stop_pnl_ << DBGLOG_ENDL_DUMP;

    //    for (int32_t sec_id = 0; sec_id < DEF_MAX_SEC_ID; sec_id++) {
    //      if (0 == security_db_arr_[sec_id].shortcode.length()) continue;
    //
    //      DBGLOG_CLASS_FUNC_LINE_INFO << "SECURITY : " << security_db_arr_[sec_id].shortcode << " "
    //                                  << "POS : " << security_db_arr_[sec_id].pos << " "
    //                                  << "LAST_TPX : " << security_db_arr_[sec_id].last_trade_px << " "
    //                                  << "PREV_UNREAL : " << security_db_arr_[sec_id].unreal_pnl << " "
    //                                  << "PREV_REAL : " << security_db_arr_[sec_id].realized_pnl << " "
    //                                  << "N2D : " << security_db_arr_[sec_id].n2d << " "
    //                                  << "COMM : " << security_db_arr_[sec_id].commission << " "
    //                                  << "TOTAL : " << total_pnl_ << " "
    //                                  << "STOP : " << stop_pnl_ << DBGLOG_ENDL_DUMP;
    //    }

    DBGLOG_CLASS_FUNC_LINE_INFO
        << "===========================================================================================\n";
  }

  void OnOrderExec(int32_t sec_id, int32_t size_executed, double price, HFSAT::TradeType_t trade_type) {
    if (sec_id < 0) {
      return;
    }

    double prev_pnl = 0;
    SecurityDB& secdb = security_db_arr_[sec_id];
    prev_pnl = secdb.realized_pnl;

    if (0 == secdb.n2d) {
      secdb.n2d = GetN2D(secdb.shortcode, today_date_, price);
      if (std::string::npos != secdb.shortcode.find("NSE_")) {
        secdb.commission = HFSAT::NSESecurityDefinitions::GetNSECommission(secdb.shortcode);
        secdb.n2d = 1;
        secdb.is_nse = true;
      }
      else if (std::string::npos != secdb.shortcode.find("BSE_")) {
        secdb.commission = HFSAT::BSESecurityDefinitions::GetBSECommission(secdb.shortcode);
        secdb.n2d = 1;
        secdb.is_bse = true;
      }

      DBGLOG_CLASS_FUNC_LINE_INFO << "N2D : " << secdb.n2d << " COMMISSION : " << secdb.commission << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }

    if (HFSAT::kTradeTypeBuy == trade_type) {
      secdb.pos += size_executed;
      secdb.last_trade_px = price;
      secdb.unreal_pnl -= (price * size_executed * secdb.n2d);

    } else if (HFSAT::kTradeTypeSell == trade_type) {
      secdb.pos -= size_executed;
      secdb.last_trade_px = price;
      secdb.unreal_pnl += (price * size_executed * secdb.n2d);

    } else {
      // TODO - Error Notification
    }

    if (true == secdb.is_nse) {
      secdb.unreal_pnl -= (size_executed * secdb.commission * price);
      secdb.realized_pnl =
          (secdb.unreal_pnl + secdb.last_trade_px * secdb.pos * secdb.n2d) - (secdb.pos * secdb.commission * price);
    } else {
      secdb.unreal_pnl -= (size_executed * secdb.commission);
      secdb.realized_pnl =
          (secdb.unreal_pnl + secdb.last_trade_px * secdb.pos * secdb.n2d) - (secdb.pos * secdb.commission);
    }

    //    DBGLOG_CLASS_FUNC_LINE_INFO << "SEC: " << secdb.shortcode << " " << secdb.unreal_pnl << " " << secdb.pos << "
    //    "
    //                                << secdb.realized_pnl << DBGLOG_ENDL_FLUSH;

    total_pnl_ -= prev_pnl;            // reset pnl for this security
    total_pnl_ += secdb.realized_pnl;  // add recomputed pnl for this security

    // Alert Mechanism
    if (false == alert_sent_ && should_alert_) {
      if (total_pnl_ <= (stop_pnl_ * 0.9)) {
        HFSAT::Email e;
        std::string subject = "PNL ALERT " +  HFSAT::GetCurrentHostName();
        e.setSubject(subject);
        e.addRecepient("nseall@tworoads.co.in");
        e.addSender("ravi.parikh@tworoads.co.in");
        e.content_stream << "host_machine: " << HFSAT::GetCurrentHostName() << "<br/>";
        e.content_stream << "CURRENT PNL VALUE : " << total_pnl_ << " STOP PNL VALUE : " << stop_pnl_ << "<br/>";
        e.sendMail();

        alert_sent_ = true;
      }
    } else {
      if (total_pnl_ >= (stop_pnl_ * 0.6)) {
        alert_sent_ = false;
      }
    }

    //    DBGLOG_CLASS_FUNC_LINE_INFO << "TOTAL PNL : " << total_pnl_ << DBGLOG_ENDL_FLUSH;
    //    std::cout << " TOTAL PNL : " << total_pnl_ << std::endl ;
  }
};
}
}
