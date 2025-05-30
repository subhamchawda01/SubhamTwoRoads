// =====================================================================================
//
//       Filename:  risk_manager.hpp
//
//    Description:  main header file for prod risk manager
//
//        Version:  1.0
//        Created:  03/18/2016 06:08:45 PM
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
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "dvccode/Utils/tcp_server_socket.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/lock.hpp"
#include "dvctrade/RiskManager/risk_events_listener.hpp"
#include <vector>
#include <map>

#define kLineBufferLen 1024
#define RISK_CONFIG_FILE "/home/pengine/prod/live_configs/risk_rules.cfg"

using namespace std;

namespace HFSAT {
// Map from "symbol-tag" string to tcp server socket (connected to prod servers)
static map<int, bool> client_fd_map_;

// Monitors risk values for all the rules using a config file (singleton object)
class ProductRiskManager {
  // Maps from tag->shortcode->value
  map<std::string, map<std::string, int> > positions_;
  map<std::string, map<std::string, double> > realized_pnl_;
  map<std::string, map<std::string, double> > unrealized_pnl_;
  // Maps from shortcode to mkt_price
  // Index is tag
  map<std::string, double> max_pnl_;  // For computing drawdown
  // Map from tag to rule details
  map<std::string, pair<double, double> > tag_rule_map_;
  map<std::string, bool> tag_gotflat_;
  HFSAT::SlackManager* slack_manager_;
  std::string hostname_;

 public:
  ProductRiskManager() {
    // Load values and symbols from config file
    LoadConfigFile();

    // fetcgh and store hostname
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    hostname_ = hostname;

    slack_manager_ = new HFSAT::SlackManager(RISKMANGER);
  }
  ~ProductRiskManager() { DumpConfigFile(); }
  // Load/Reload risk cfg file
  inline void LoadConfigFile() {
    std::ifstream fin(RISK_CONFIG_FILE);
    std::string line;
    while (fin.good()) {
      getline(fin, line);
      if (line.length() <= 0) break;
      std::string tag, shc;
      double pnl, dd;
      PerishableStringTokenizer st(strdup(line.c_str()), kLineBufferLen);
      const std::vector<const char*>& tokens = st.GetTokens();
      if (tokens.size() <= 2) {
        std::cerr << "Config file format incorrect. Exiting\n";
        exit(1);
      }
      tag = tokens[0];
      pnl = atof(tokens[1]);
      dd = atof(tokens[2]);
      tag_rule_map_[tag] = make_pair(pnl, dd);
    }
    fin.close();
  }
  // Dump config file (generally after values are updated)
  inline void DumpConfigFile() {
    std::cout << "Dumping config file\n";
    (std::cout).flush();
    std::ofstream fout(RISK_CONFIG_FILE);
    for (auto itr : tag_rule_map_) {
      fout << itr.first << " " << itr.second.first << " " << itr.second.second << std::endl;
    }
    fout.close();
  }
  // Called by ClientManager class on each risk update notification
  void OnRiskUpdate(RiskUpdateStruct& risk_update) {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    std::cout << current_time.tv_sec << "." << current_time.tv_usec << ": "
              << "Got Risk Update: " << risk_update.ToString();
    (std::cout).flush();

    switch (risk_update.update_type_) {
      case START_TRADING: {
        StartTradingTag(risk_update.tag_);
        break;
      }
      case GET_FLAT: {
        GetFlatOnTag(risk_update.tag_);
        break;
      }
      case UPDATE_CONFIG_FILE: {
        // realized_pnl contains maxloss and unrealized_pnl contains drawdown
        if (fabs(risk_update.realized_pnl_ - 0) < 0.5) {
          // Remove limits for this tag
          std::cout << "Removing tag: " << risk_update.tag_ << "\n";
          (std::cout).flush();
          tag_rule_map_.erase(risk_update.tag_);
          positions_.erase(risk_update.tag_);
          realized_pnl_.erase(risk_update.tag_);
          unrealized_pnl_.erase(risk_update.tag_);
        } else {
          std::cerr << "Updating tag value: " << risk_update.tag_ << " " << risk_update.realized_pnl_ << " "
                    << risk_update.unrealized_pnl_ << std::endl;
          (std::cerr).flush();
          tag_rule_map_[risk_update.tag_] = make_pair(risk_update.realized_pnl_, risk_update.unrealized_pnl_);
        }
        DumpConfigFile();
        break;
      }
      case UPDATE_VALUES: {
        if (tag_rule_map_.find(risk_update.tag_) != tag_rule_map_.end()) {
          // This tag is relevant
          positions_[risk_update.tag_][risk_update.shortcode_] = risk_update.position_;
          realized_pnl_[risk_update.tag_][risk_update.shortcode_] = risk_update.realized_pnl_;
          unrealized_pnl_[risk_update.tag_][risk_update.shortcode_] = risk_update.unrealized_pnl_;
          TakeAction(risk_update.tag_);
        }
        break;
      }
      default:
        std::cerr << "Unknown Risk Update type: " << risk_update.update_type_ << "\n";
        (std::cerr).flush();
    }
  }
  // Do some action depending on the type of update
  void TakeAction(std::string tag) {
    // Also, log the action for zabbix monitoring
    auto this_rule = tag_rule_map_.find(tag)->second;
    std::cout << "Testing for rule: " << tag << " : " << this_rule.first << ", " << this_rule.second << "\n";
    (std::cout).flush();
    // Calculate pnl and drawdown for this particular rule
    double open_pnl = 0, drawdown = 0;
    std::cout << "checking for tag: " << tag << "\n";
    (std::cout).flush();
    for (auto shc_entry : realized_pnl_[tag]) {
      open_pnl += realized_pnl_[tag][shc_entry.first] + unrealized_pnl_[tag][shc_entry.first];
    }
    std::stringstream ss;
    // Check if this rule's PNL has been breached
    if (open_pnl <= -this_rule.first && !tag_gotflat_[tag]) {
      // Send msg to getflat for this tag (as risk limit has been breached
      ss << "Maxloss limit breached for " << tag << "; Value: " << open_pnl << "; Limit: " << -this_rule.first << "\n";
      std::cout << ss.str();
      (std::cout).flush();
      GetFlatOnTag(tag, ss.str());
      return;  // No point in processing further
    }
    max_pnl_[tag] = max(max_pnl_[tag], open_pnl);
    drawdown = max_pnl_[tag] - open_pnl;
    // Check if this rule's drawdown has been breached
    /*if (drawdown >= this_rule.second && !tag_gotflat_[tag]) {
      ss << "Drawdown limit breached for " << tag << "; Value: " << drawdown << "; Limit: " << this_rule.second << "\n";
      std::cout << ss.str();
      (std::cout).flush();
      GetFlatOnTag(tag, ss.str());
    }
    */
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    std::cout << current_time.tv_sec << "." << current_time.tv_usec << ": Rule # " << tag << ", PnL: " << open_pnl
              << ", DD: " << drawdown << "\n";
    (std::cout).flush();
  }
  // Manual or automated getflat
  void GetFlatOnTag(const std::string& tag, std::string error_msg = "") {
    if (tag_gotflat_[tag]) return;
    tag_gotflat_[tag] = true;  // This should be called only once
    // Send TCP messages for corresponding tags
    GetFlatStruct getflat_message;
    getflat_message.getflat_ = true;
    strcpy(getflat_message.tag_, tag.c_str());
    SendMessageToClients(getflat_message, error_msg);
  }
  // Manual start trading for tags
  void StartTradingTag(const std::string& tag) {
    tag_gotflat_[tag] = false;  // This should be called only once
    // Send TCP messages for corresponding tags
    GetFlatStruct getflat_message;
    getflat_message.getflat_ = false;
    strcpy(getflat_message.tag_, tag.c_str());
    SendMessageToClients(getflat_message);
  }
  // Dispatches distress message to all connected clients
  void SendMessageToClients(GetFlatStruct& getflat_message, std::string error_msg = "") {
    slack_manager_->sendNotification(error_msg + " " + getflat_message.ToString());
    std::cout << "Sending to clients: " << error_msg << "; " << getflat_message.ToString() << "\n";
    for (auto fd : client_fd_map_) {
      int write_len = write(fd.first, &getflat_message, sizeof(getflat_message));
      if (write_len < (int)sizeof(getflat_message)) {
        close(fd.first);
        std::cerr << "Removing client fd! Network Error while writing on fd: " << fd.first
                  << ", getflat msg: " << getflat_message.ToString() << "\n";
        (std::cerr).flush();
        client_fd_map_.erase(fd.first);  // Remove this stale socket
      }
    }
  }
};
// Risk Manager class
static ProductRiskManager product_risk_manager_;

// This class acts as a server for each remote client running on each prod server, reads the updates and notifies them
class ClientManager : public HFSAT::Thread {
  int client_fd_;
  int offset_;
  int packet_num_;

 public:
  HFSAT::Lock& shared_maps_lock_;

 public:
  ClientManager(int client_fd, HFSAT::Lock& shared_maps_lock)
      : client_fd_(client_fd), offset_(0), packet_num_(0), shared_maps_lock_(shared_maps_lock) {}
  ~ClientManager() {}
  inline void thread_main() {
    // Continuously reads and processes updates from comrades on prod servers
    RiskUpdateStruct risk_update;

    while (true) {
      int read_len = read(client_fd_, ((char*)&risk_update) + offset_, sizeof(RiskUpdateStruct) - offset_);
      if (read_len + offset_ < (int)sizeof(RiskUpdateStruct)) {
        if (read_len < 0) {
          std::cerr << "Error in reading. Socket closed, fd: " << client_fd_ << ", Read_Len:  " << read_len
                    << ", Expected_Len: " << sizeof(RiskUpdateStruct) << ", Offset: " << offset_
                    << ", Error_Code: " << strerror(errno) << "\n";
          (std::cerr).flush();
          close(client_fd_);
          return;
        }
        offset_ += read_len;
        continue;
      }
      offset_ = 0;
      packet_num_++;

      // Lock here (to avoid updation of map by multiple threads)
      shared_maps_lock_.LockMutex();

      std::cout << "Read update: " << packet_num_ << "; " << risk_update.ToString();
      (std::cout).flush();
      // Add for all tags

      if (client_fd_map_.find(client_fd_) == client_fd_map_.end()) {
        client_fd_map_[client_fd_] = true;
      }

      product_risk_manager_.OnRiskUpdate(risk_update);
      // Unlock here
      shared_maps_lock_.UnlockMutex();
    }
  }
};

// This class listens on the given port for clients (from prod servers)
class ClientListener : public Thread {
  HFSAT::TCPServerSocket* tcp_server_;

  HFSAT::Lock shared_maps_lock_;

 public:
  ClientListener(int port_num) : tcp_server_(new HFSAT::TCPServerSocket(port_num)), shared_maps_lock_() {}

  inline void thread_main() {
    while (true) {
      int client_fd = tcp_server_->Accept();
      if (client_fd == -1) {
        std::cerr << "ClientListener Accept Failed : " << strerror(errno) << "\n";
        (std::cerr).flush();
        break;
      }
      union {
        struct sockaddr sa;
        struct sockaddr_in sa4;
        struct sockaddr_storage sas;
      } address;

      socklen_t size = sizeof(address);

      if (getpeername(client_fd, &address.sa, &size) < 0) {
        std::cout << " Failed To Retrieve Client IP Address \n";
        (std::cout).flush();
      }

      if (address.sa4.sin_family == AF_INET) {
        std::cout << " Initiating New Client Connection For Risk Updates .... Client IP : "
                  << inet_ntoa(address.sa4.sin_addr) << " Port : " << address.sa4.sin_port << " Fd: " << client_fd
                  << "\n";
        (std::cout).flush();
      }

      // Add handler for new risk client
      ClientManager* this_client_mgr = new ClientManager(client_fd, shared_maps_lock_);
      this_client_mgr->run();
    }
  }
};

//reads risk tag mapping from queries and logs to a file(required for tagwise pnl computation)
class TagMappingLogger : public HFSAT::Thread {
	int client_fd_;
	int offset_;
	int packet_num_;
public:
	std::ofstream& qid_saci_tag_file_;
	HFSAT::Lock& qid_saci_tags_file_lock_;
	TagMappingLogger(int client_fd, std::ofstream& qid_saci_tag_file, HFSAT::Lock& qid_saci_tags_file_lock) :
		client_fd_(client_fd),
		offset_(0),
		packet_num_(0),
		qid_saci_tag_file_(qid_saci_tag_file),
		qid_saci_tags_file_lock_(qid_saci_tags_file_lock){}

	~TagMappingLogger(){}

	inline void thread_main() {
		// reads and processes updates from each query on prod servers
		QueryTag query_tag;
		while (true) {
			int read_len = read(client_fd_, ((char*)&query_tag) + offset_, sizeof(QueryTag) - offset_);
			if (read_len + offset_ < (int)sizeof(QueryTag)) {
				if (read_len < 0) { // Some network error
				  std::cerr << "Error in reading by TagMappingLogger. Socket closed, fd: " << client_fd_ << ", Read_Len:  " << read_len
							<< ", Expected_Len: " << sizeof(QueryTag) << ", Offset: " << offset_
							<< ", Error_Code: " << strerror(errno) << "\n";
				  (std::cerr).flush();
				  close(client_fd_);
				  return;
				}
				offset_ += read_len;
				continue;
			}
			offset_ = 0;
			packet_num_++;

			// Acquire Lock before updating the file ( to avoid updation of file by multiple threads)
			qid_saci_tags_file_lock_.LockMutex();
			std::cout << "Got QueryTag msg: " << read_len << ", " << query_tag.ToString() << "\n";
			(std::cout).flush();
			// Add for all tags
			qid_saci_tag_file_ << query_tag.query_id_ << " " << query_tag.saci_ << " " << query_tag.tags_ << "\n";
			qid_saci_tag_file_.flush();
			// Unlock here
			qid_saci_tags_file_lock_.UnlockMutex();
			close(client_fd_); //close the client socket, mapping read
			break;	//read the message completely, query notifies mapping only once(i.e. at start)
		}
	}
};


class TagMappingListener : public HFSAT::Thread {
  int port_num_;
  HFSAT::TCPServerSocket tcp_socket_;
  std::ofstream qid_saci_tag_file_;
  HFSAT::Lock qid_saci_tags_file_lock_;

 public:
  TagMappingListener(int port_num) : port_num_(port_num), tcp_socket_(port_num), qid_saci_tags_file_lock_() {
    std::stringstream ss;
    ss << "/spare/local/logs/risk_logs/qid_saci_tag";
    qid_saci_tag_file_.open(ss.str().c_str(), std::ofstream::out | std::ofstream::app);
  }

  void thread_main() {

	    while (true) {
	      int client_fd = tcp_socket_.Accept();
	      if (client_fd == -1) {
	        std::cerr << "TagMappingListener Accept Failed : " << strerror(errno) << "\n";
	        (std::cerr).flush();
	      }
	      union {
	        struct sockaddr sa;
	        struct sockaddr_in sa4;
	        struct sockaddr_storage sas;
	      } address;

	      socklen_t size = sizeof(address);

	      if (getpeername(client_fd, &address.sa, &size) < 0) {
	        std::cout << "TagMappingListener Failed To Retrieve Client IP Address \n";
	        (std::cout).flush();
	      }

	      if (address.sa4.sin_family == AF_INET) {
	        std::cout << " New query connected for querytag.... Client IP : "
	                  << inet_ntoa(address.sa4.sin_addr) << " Port : " << address.sa4.sin_port << " Fd: " << client_fd
	                  << "\n";
	        (std::cout).flush();
	      }

	      // Add handler for new query tag
	      TagMappingLogger* tag_mapping_lgr = new TagMappingLogger(client_fd, qid_saci_tag_file_, qid_saci_tags_file_lock_);
	      tag_mapping_lgr->run();
	    }
	    std::cerr << "TagMappingListener exited : " << strerror(errno) << "\n";
	    (std::cerr).flush();
  	  }
};
}
