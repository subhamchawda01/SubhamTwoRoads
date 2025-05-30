/* Exec to send getflat/start trading/reload config file messages to the central risk server
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <list>
#include "dvctrade/RiskManager/risk_events_listener.hpp"
#include "dvctrade/RiskManager/risk_manager.hpp"
#include "dvccode/Utils/slack_utils.hpp"

int main(int argc, char** argv) {
  if (argc > 1 && argc != 6) {
    std::cerr << " USAGE : <exec> (for getflat/start/update_limits resp.)" << std::endl
              << " OR " << std::endl
              << " <exec> queryid saci tags start_time end_time(for notifying saci--queryid-tag triplet)" << std::endl;
    exit(0);
  }

  // Notification of queryid, saci, tag, end_time triplet to the risk_monitor_client and ny11 server
  if (argc == 6) {
    HFSAT::QueryTag query_tag(atoi(argv[2]), atoi(argv[1]), argv[3], atoi(argv[4]), atoi(argv[5]));
    std::cerr << "Notifying risk client of : " << query_tag.ToString();

    if (!HFSAT::RiskNotifier::InformMappingViaTcp(query_tag)) {
      // TCP failed => risk monitor slave not running => Write to file (it can load later)
      std::cerr << "Couldn't notifying risk client of : " << query_tag.ToString() << " Dumping to file.\n";

      HFSAT::RiskNotifier::DumpSACITagMapping(query_tag);
    }

    if (!HFSAT::RiskNotifier::InformMappingViaTcpToNy11(query_tag)) {
      char hostname[128];
      hostname[127] = '\0';
      gethostname(hostname, 127);

      std::stringstream ss;
      ss << hostname << " couldn't notify NY11 of : " << query_tag.ToString();
      std::cerr << ss.str();
      HFSAT::SlackManager slack_pnl(TAG_PNL);
      slack_pnl.sendNotification(ss.str());
    } else {
      std::cout << "Successfully notified NY11 of " << query_tag.ToString();
    }
    exit(0);
  }

  int interrupt_type;
  std::cout << "Enter 0 for issuing GetFlat for a tag, 1 for StartTrading, 2 for updating limits, 3 for viewing "
               "current limits: " << std::endl;
  std::cin >> interrupt_type;

  if (interrupt_type > 3) {
    std::cerr << "Invalid input value" << std::endl;
    exit(1);
  }
  if (interrupt_type == 3) {
    std::ifstream fin(RISK_CONFIG_FILE);
    std::string line;
    while (fin.good()) {
      getline(fin, line);
      if (line.length() <= 0) break;
      std::cout << line << std::endl;
    }
    fin.close();
    exit(0);
  }
  std::string tag;
  std::cout << "Enter tag name " << std::endl;
  std::cin >> tag;

  // Create struct and write to TCP socket
  HFSAT::RiskUpdateStruct manual_risk_update;
  manual_risk_update.update_type_ = (HFSAT::RiskUpdateType)interrupt_type;
  strcpy(manual_risk_update.tag_, tag.c_str());
  // Get updated values in case of manual limit update
  if (manual_risk_update.update_type_ == HFSAT::UPDATE_CONFIG_FILE) {
    std::cout << "You have chosen to update/add/remove limits. Press CTRL+C to abort." << std::endl;
    std::cout << "Enter maxloss value for tag " << manual_risk_update.tag_ << " (0 to remove limits): ";
    std::cin >> manual_risk_update.realized_pnl_;
    // No need to ask for drawdown if user wants to remove the limit
    if (fabs(manual_risk_update.realized_pnl_ - 0) > 0.5) {
      std::cout << "Enter drawdown value for tag " << manual_risk_update.tag_ << " : ";
      std::cin >> manual_risk_update.unrealized_pnl_;
    }
  }

  // setup DebugLogger
  HFSAT::DebugLogger dbglogger_(1024000, 1);
  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/risk_logs/risk_control_log_" << HFSAT::DateTime::GetCurrentIsoDateLocal();
    std::string logfilename_ = t_temp_oss_.str();
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);
  }

  // notifier object
  HFSAT::RiskNotifier& risk_notifier = HFSAT::RiskNotifier::setInstance(dbglogger_);
  risk_notifier.NotifyServer(manual_risk_update);
  return 0;
}
