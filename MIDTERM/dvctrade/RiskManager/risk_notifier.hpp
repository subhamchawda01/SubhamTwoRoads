#pragma once
#include <map>
#include <sys/file.h>
#include <vector>

#include "dvccode/Utils/slack_utils.hpp"
#include "dvccode/CombinedControlUtils/combined_control_messages.hpp"
#include "dvccode/CDef/control_messages.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Utils/slack_utils.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "dvccode/Utils/tcp_server_socket.hpp"
#include "dvccode/Utils/thread.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"
#include "dvctrade/RiskManager/risk_monitor_defines.hpp"

namespace HFSAT {

#define SERVER_PORT 4001
#define PNL_SERVER_PORT 4006
#define QUERY_LISTENER_PORT 4002
#define SACI_RECORD_FILE "/spare/local/files/.saci_record.txt"

// This class listens for risk updates and issues getflat notifications if required
class RiskNotifier {
  HFSAT::TCPClientSocket tcp_socket_;
  RiskUpdateStruct new_update_;
  HFSAT::MulticastSenderSocket* mcast_sock_;
  HFSAT::CombinedControlMessage* comb_control_request_;
  std::map<int, std::string> saci_tags_map_;
  std::map<int, int> saci_queryid_map_;
  std::map<std::string, std::vector<int> > tag_queryvec_map_;
  std::map<std::string, int> shortcode_pos_map_;
  int packet_num_;  // For debugging
  std::map<std::string, std::map<std::string, int> > positions_;
  std::map<std::string, std::map<std::string, int> > volume_;
  std::map<std::string, std::map<std::string, double> > realized_pnls_;
  std::map<std::string, std::map<std::string, double> > unrealized_pnls_;
  std::map<std::string, std::map<std::string, double> > last_seen_saos_;
  std::map<int, int> saci_to_last_sams_map;  // during recovery, remember all execs processed. used to ignore duplicates
                                             // execs after recovery
  // Maps from shortcode to mkt_price
  std::map<std::string, double> last_mkt_price_;
  std::map<std::string, int> old_global_pos_;
  std::map<std::string, int> di_reserves_map_;
  std::map<std::string, std::map<std::string, double> > old_vals;
  std::ofstream trades_file_;
  std::ifstream sec_file_;  // AddTradingSymbolConfig
  HFSAT::SlackManager* slack_manager_;
  std::string hostname_;
  int trading_date_;
  std::vector<ExecutionMsg*> recovery_buffer;
  DebugLogger& dbglogger_;
  const std::vector<std::string>& sec_list_vec_;  // list of shc for recovery

  // maintains query/tag start-end time
  std::map<int, int> saci_end_time_map_;
  std::map<std::string, int> tag_end_time_map_;
  std::map<int, int> saci_start_time_map_;
  std::map<std::string, int> tag_start_time_map_;

  RiskNotifier(DebugLogger& dbglogger, const std::vector<std::string>& sec_list_vec);  // private constructor
  RiskNotifier(const RiskNotifier&);                                                   // disable copy constructor
  ~RiskNotifier();
  static RiskNotifier* notifier_;
  void PushToRecoveryBuffer(ExecutionMsg* exec_msg);
  void ApplyRecoveryBuffer();

 public:
  bool in_recovery;  // true when risk monitor is in recovery(after receiving first execution update)
  bool recovered;    // true when recovery is complete, ensures we recover just once throughout the day
  static RiskNotifier& setInstance(DebugLogger& dbglogger, const std::vector<std::string>& sec_list_vec = {});
  static RiskNotifier& getInstance();
  static std::string GetServerIpToConnect();
  static std::string GetClientIpToNotifySaciMapping();
  inline HFSAT::TCPClientSocket& GetClientSocket() { return tcp_socket_; }

  // Notify the qid-saci-tag mapping to ny11 server to be used by pnl calculations
  static bool InformMappingViaTcpToNy11(QueryTag& query_tag);
  // Notify TCP server of the SACI-tag map (to be called by the query- TCP client)
  static bool InformMappingViaTcp(QueryTag& query_tag);

  // safely dump all mappings to saci_record file on exit.
  // It will be required during recovery
  void DumpAllRiskMappingOnSigInit();
  static int AcquireFileLock(int append = 0);   // Acquire lock on SACI-tag map file, and return fd
  static inline void ReleaseFileLock(int fd) {  // Release acquired lock
                                                // Unlock fd for others before leaving
    flock(fd, LOCK_UN);
    close(fd);
  }
  static void DumpSACITagMapping(QueryTag& query_tag, int given_fd = -1);  // To dump SACI-tag maps to a file
  void LoadSACITagMap();                                                   // To load SACI-tag maps from a file
  inline void AddSACIMapping(QueryTag& query_tag) {                        // Add a single SACI mapping to the maps

    dbglogger_ << "SACI mapping: " << query_tag.ToString();
    dbglogger_.DumpCurrentBuffer();
    saci_queryid_map_[query_tag.saci_] = query_tag.query_id_;
    saci_tags_map_[query_tag.saci_] = query_tag.tags_;
    saci_end_time_map_[query_tag.saci_] = query_tag.utc_end_time_;
    saci_start_time_map_[query_tag.saci_] = query_tag.utc_start_time_;

    char tmp_tags[128];  // default length of tags in risk_monitor_defines
    strcpy(tmp_tags, query_tag.tags_);
    // break the tags, and add this query id to all the concerned vectors
    char* tag = strtok(tmp_tags, ":");
    while (tag != NULL) {
      // Add a query id only once
      HFSAT::VectorUtils::UniqueVectorAdd(tag_queryvec_map_[tag], query_tag.query_id_);
      tag_end_time_map_[tag] = std::max(
          tag_end_time_map_[tag], query_tag.utc_end_time_);  // store the max of current query and existing tag end time
      // Important: store the max of current query and existing tag start time
      // For AS queries, start_time(eg 2200) is greater than end_time(530).
      // Storing max start time ensures we don't ignore updates between 2200-2359(lets say) for AS hours
      tag_start_time_map_[tag] = std::max(tag_start_time_map_[tag], query_tag.utc_start_time_);

      tag = strtok(NULL, ":");
    }
  }

  void LoadPastTradesFromOrsBinFile();  // Loads trades during recovery
  void ReadORSLoggedDataForTrades(std::string shc);
  std::string GetORSLoggedDataFilePath(std::string shc);
  void IssueGetFlat(GetFlatStruct& get_flat);  // Issue getflat messages to all queries associated with the given tag
  double GetN2D(std::string& shortcode, int yyyymmdd, double cur_price);
  void OnMktPriceChange(std::string shortcode, double mkt_price);  // Called on mktsizewprice changes

  void OnOrderExecuted(int saci, std::string shortcode, TradeType_t side, int order_size_executed, double price,
                       int global_pos, int saos, std::string timestamp, int sams);  // Called on our order executions

  static void GetTagsFromString(
      std::string tags,
      std::vector<std::string>& tag_vec);  // Prepare a vector of strings (tags) from the combo string

  void NotifyPNLUpdate(std::string& shc, std::string& tag,
                       std::string update_type = "mkt_update");  // Update server about the PNL change
  bool ReconnectToServer();
  void NotifyServer(RiskUpdateStruct& new_update);  // Write the RiskUpdateStruct to TCP socket
};
}
