// =====================================================================================
//
//       Filename:  ors_margin_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/06/2017 07:55:32 AM
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
#include "infracore/ORSUtils/ors_security_db.hpp"

#define SECURITYMARGINFILEPATH "/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/"
#define BSESECURITYMARGINFILEPATH "/spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/"

namespace HFSAT {
namespace ORSUtils {

class ORSMarginManager {
 private:
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::ORS::PositionManager& position_manager_;
  SecurityDBArr& security_db_arr_;
  std::map<std::string, double> shortcode_to_margin_factor_;
  std::string exch_;
  double gross_margin_check_;
  double net_margin_check_;
  double margin_factor_;
  double gross_margin_used_;
  double net_margin_used_;
  bool is_margin_check_enforced_;
  bool alert_sent_;

  ORSMarginManager(HFSAT::DebugLogger& logger, std::string exch = "NSE")
      : dbglogger_(logger),
        position_manager_(HFSAT::ORS::PositionManager::GetUniqueInstance()),
        security_db_arr_(SecurityDBManager::GetUniqueInstance().GetSecurityDB()),
        exch_(exch),
        gross_margin_check_(0),
        net_margin_check_(0),
        margin_factor_(1),
        gross_margin_used_(0),
        net_margin_used_(0),
        is_margin_check_enforced_(false),
        alert_sent_(false)

  {}

  ORSMarginManager(ORSMarginManager const& disabled_copy_constructor) = delete;

 public:
  static ORSMarginManager& GetUniqueInstance(HFSAT::DebugLogger& logger, std::string exch = "NSE") {
    static ORSMarginManager unique_instance(logger,exch);
    return unique_instance;
  }

  void EnableMarginChecks() {
    if (false == is_margin_check_enforced_) {
      if ( exch_ == "NSE" )
        LoadMarginValues();
      else if ( exch_ == "BSE" )
        LoadBseMarginValues();
    }

    is_margin_check_enforced_ = true;
  }

  void DisableMarginChecks() { is_margin_check_enforced_ = false; }

  void UpdateMarginFactor(double fac) { margin_factor_ = fac; }

  void LoadBseMarginValues() {
    std::ostringstream t_temp_oss;
    t_temp_oss << BSESECURITYMARGINFILEPATH << "security_margin_" << HFSAT::DateTime::GetCurrentIsoDateLocal() << ".txt";

    std::string fname = t_temp_oss.str();

    std::ifstream mstream;
    mstream.open(fname.c_str(), std::ifstream::in);

    if (false == mstream.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "UNABLE TO OPEN THE FILE -> " << fname << " SYSERROR : " << strerror(errno)
                                   << DBGLOG_ENDL_DUMP_AND_EXIT;
    }

    char buffer[1024];
    while (mstream.good()) {
      mstream.getline(buffer, 1024);
      std::string linebuffer = buffer;

      if (std::string::npos != linebuffer.find("#")) continue;  // comments

      HFSAT::PerishableStringTokenizer st(buffer, 1024);
      std::vector<char const*> const& tokens = st.GetTokens();

      if (2 != tokens.size()) continue;

      shortcode_to_margin_factor_[tokens[0]] = atof(tokens[1]);
      // will only work if the shortcodes are already setup
      UpdateSecurityMarginForShortCode(tokens[0], atof(tokens[1]));
    }

    mstream.close();

    if (0 == shortcode_to_margin_factor_.size()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "UNABLE TO LOAD SECURITY MARGIN VALUES FROM -> " << fname
                                   << DBGLOG_ENDL_DUMP_AND_EXIT;
    }
  }

  void LoadMarginValues() {
    std::ostringstream t_temp_oss;
    t_temp_oss << SECURITYMARGINFILEPATH << "security_margin_" << HFSAT::DateTime::GetCurrentIsoDateLocal() << ".txt";

    std::string fname = t_temp_oss.str();

    std::ifstream mstream;
    mstream.open(fname.c_str(), std::ifstream::in);

    if (false == mstream.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "UNABLE TO OPEN THE FILE -> " << fname << " SYSERROR : " << strerror(errno)
                                   << DBGLOG_ENDL_DUMP_AND_EXIT;
    }

    char buffer[1024];
    while (mstream.good()) {
      mstream.getline(buffer, 1024);
      std::string linebuffer = buffer;

      if (std::string::npos != linebuffer.find("#")) continue;  // comments

      HFSAT::PerishableStringTokenizer st(buffer, 1024);
      std::vector<char const*> const& tokens = st.GetTokens();

      if (2 != tokens.size()) continue;

      shortcode_to_margin_factor_[tokens[0]] = atof(tokens[1]);
      // will only work if the shortcodes are already setup
      UpdateSecurityMarginForShortCode(tokens[0], atof(tokens[1]));
    }

    mstream.close();

    if (0 == shortcode_to_margin_factor_.size()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "UNABLE TO LOAD SECURITY MARGIN VALUES FROM -> " << fname
                                   << DBGLOG_ENDL_DUMP_AND_EXIT;
    }
    //LoadBseMarginValues();
  }


  void ReloadMarginValues() {
    if (false == is_margin_check_enforced_) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "MARGIN CHECK DISABLED, NO POINT LOADING THE MARGIN FILE" << DBGLOG_ENDL_DUMP;
    } else {
        if ( exch_ == "NSE" )
          LoadMarginValues();
        else if ( exch_ == "BSE" )
          LoadBseMarginValues();
    }
  }

  bool IsMarginChecksInPlace() { return is_margin_check_enforced_; }

  bool AllowAddTSForShortCode(std::string shc) {
    if (true == IsMarginChecksInPlace()) {
      if (shortcode_to_margin_factor_.end() == shortcode_to_margin_factor_.find(shc)) return false;
    }

    return true;
  }

  void SetGrossMarginCheck(double gross_margin_check) {
    if (0 == gross_margin_check_ && gross_margin_check > 0) {
      EnableMarginChecks();
    }
    gross_margin_check_ = (gross_margin_check < 0) ? 0 : gross_margin_check;
    if (0 == gross_margin_check_) {
      DisableMarginChecks();
    }

    alert_sent_ = false;
    DBGLOG_CLASS_FUNC_LINE_INFO << "GROSS MARGIN CHECK SETUP AT : " << gross_margin_check_
                                << " MARGIN_CHECK_ENFORCED ? " << IsMarginChecksInPlace() << " ALERT SENT ? "
                                << alert_sent_ << DBGLOG_ENDL_DUMP;
  }

  void SetNetMarginCheck(double net_margin_check) {
    if (0 == net_margin_check_ && net_margin_check > 0) {
      EnableMarginChecks();
    }
    net_margin_check_ = (net_margin_check < 0) ? 0 : net_margin_check;
    if (0 == net_margin_check_) {
      DisableMarginChecks();
    }

    alert_sent_ = false;
    DBGLOG_CLASS_FUNC_LINE_INFO << "GROSS MARGIN CHECK SETUP AT : " << net_margin_check_ << " MARGIN_CHECK_ENFORCED ? "
                                << IsMarginChecksInPlace() << " ALERT SENT ? " << alert_sent_ << DBGLOG_ENDL_DUMP;
  }

  inline bool AllowsGrossMarginCheck() {
    if (!is_margin_check_enforced_) return true;
    return gross_margin_check_ > gross_margin_used_;
  }

  inline bool AllowsNetMarginCheck() {
    if (!is_margin_check_enforced_) return true;
    return net_margin_check_ > std::fabs(net_margin_used_);
  }

  void UpdateSecurityMarginForShortCode(std::string shc, double new_value) {
    shortcode_to_margin_factor_[shc] = new_value;

    for (int32_t i = 0; i < DEF_MAX_SEC_ID; i++) {
      if (shc == security_db_arr_[i].shortcode) {
        security_db_arr_[i].margin_factor = shortcode_to_margin_factor_[shc];
        DBGLOG_CLASS_FUNC_LINE_INFO << "UPDATING SECURITY MARGIN : " << i << " " << shc << " " << new_value
                                    << DBGLOG_ENDL_DUMP;
      }
    }
  }

  void DumpMarginStatus() {
    DBGLOG_CLASS_FUNC_LINE_INFO
        << "===================================== ORS MARGIN STATUS =====================================\n";

    DBGLOG_CLASS_FUNC_LINE_INFO << "GLOBAL_MARGIN_FACTOR : " << margin_factor_ << " "
                                << "GROSS : " << gross_margin_used_ << " "
                                << "NET : " << net_margin_used_ << " "
                                << "GCHECK : " << gross_margin_check_ << " "
                                << "NCHECK : " << net_margin_check_ << DBGLOG_ENDL_DUMP;

    //    for (int32_t sec_id = 0; sec_id < DEF_MAX_SEC_ID; sec_id++) {
    //      if (0 == security_db_arr_[sec_id].shortcode.length()) continue;
    //
    //      DBGLOG_CLASS_FUNC_LINE_INFO << "SECURITY : " << security_db_arr_[sec_id].shortcode << " "
    //                                  << "MARGIN : " << security_db_arr_[sec_id].margin_factor << " "
    //                                  << "PREV : " << security_db_arr_[sec_id].margin_used << " "
    //                                  << "POS : " << position_manager_.GetGlobalPosition(sec_id) << " "
    //                                  << "GLOBAL_MARGIN_FACTOR : " << margin_factor_ << " "
    //                                  << "GROSS : " << gross_margin_used_ << " "
    //                                  << "NET : " << net_margin_used_ << " "
    //                                  << "GCHECK : " << gross_margin_check_ << " "
    //                                  << "NCHECK : " << net_margin_check_ << DBGLOG_ENDL_DUMP;
    //    }

    DBGLOG_CLASS_FUNC_LINE_INFO
        << "=============================================================================================\n";
  }

  void OnOrderExec(int32_t security_id) {
    if (false == IsMarginChecksInPlace()) return;

    if (0 == security_db_arr_[security_id].margin_factor) {
      security_db_arr_[security_id].margin_factor =
          shortcode_to_margin_factor_[security_db_arr_[security_id].shortcode];
      DBGLOG_CLASS_FUNC_LINE_INFO << "SECURITY ID : " << security_id
                                  << " SHORTCODE : " << security_db_arr_[security_id].shortcode
                                  << " MARGIN : " << security_db_arr_[security_id].margin_factor << DBGLOG_ENDL_DUMP;
    }

    double prev_margin_value = security_db_arr_[security_id].margin_used;
    gross_margin_used_ -= std::fabs(prev_margin_value);
    net_margin_used_ -= prev_margin_value;

    int32_t current_pos = position_manager_.GetGlobalPosition(security_id);
    double current_margin = current_pos * security_db_arr_[security_id].margin_factor * margin_factor_;

    // long options don't fetch any margin
    if (true == security_db_arr_[security_id].is_options && current_pos > 0) {
      current_margin = 0;
    }

    // Update Gross and Net values
    gross_margin_used_ += std::fabs(current_margin);
    net_margin_used_ += current_margin;

    //    DBGLOG_CLASS_FUNC_LINE_INFO << "SECURITY : " << security_db_arr_[security_id].shortcode << " "
    //                                << "MARGIN : " << security_db_arr_[security_id].margin_factor << " "
    //                                << "PREV : " << security_db_arr_[security_id].margin_used << " "
    //                                << "POS : " << current_pos << " "
    //                                << "GLOBAL_MARGIN_FACTOR : " << margin_factor_ << " "
    //                                << "VALUE : " << current_margin << " "
    //                                << "GROSS : " << gross_margin_used_ << " "
    //                                << "NET : " << net_margin_used_ << " "
    //                                << "GCHECK : " << gross_margin_check_ << " "
    //                                << "NCHECK : " << net_margin_check_ << DBGLOG_ENDL_DUMP;

    // Update last seen value ;
    security_db_arr_[security_id].margin_used = current_margin;

    // Alert Mechanism
    if (false == alert_sent_) {
      if ((gross_margin_used_ >= (gross_margin_check_ * 0.9)) ||
          (std::fabs(net_margin_used_) >= (net_margin_check_ * 0.9))) {
        HFSAT::Email e;
        std::string subject = "MARGIN ALERT " +  HFSAT::GetCurrentHostName();
        e.setSubject(subject);
        e.addRecepient("nseall@tworoads.co.in");
        e.addSender("ravi.parikh@tworoads.co.in");
        e.content_stream << "host_machine: " << HFSAT::GetCurrentHostName() << "<br/>";
        e.content_stream << "CURRENT GROSS MARGIN USAGE : " << gross_margin_used_
                         << " NET MARGIN USAGE : " << net_margin_used_
                         << " GROSS MARGIN CHECK : " << gross_margin_check_
                         << " NET MARGIN CHECK : " << net_margin_check_ << "<br/>";
        e.sendMail();

        alert_sent_ = true;
      }
    } else {
      if ((gross_margin_used_ <= (gross_margin_check_ * 0.6)) &&
          (std::fabs(net_margin_used_) <= (net_margin_check_ * 0.6))) {
        alert_sent_ = false;
      }
    }
  }

  double GetNetMarginUsed() {
    return net_margin_used_;
  }

  double GetGrossMarginUsed() {
    return gross_margin_used_;
  }
};
}
}
