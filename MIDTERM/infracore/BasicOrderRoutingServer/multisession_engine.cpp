#include <algorithm>
#include <ctime>
#include <ctype.h>
#include <fstream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"
#include "dvccode/Utils/async_writer.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/Utils/misc.hpp"
#include "dvccode/Utils/settings.hpp"
#include "dvccode/Utils/thread.hpp"
#include "infracore/BMFEP/BMFEPEngine.hpp"
#include "infracore/BasicOrderRoutingServer/base_engine.hpp"
#include "infracore/BasicOrderRoutingServer/engine_listener.hpp"
#include "infracore/BasicOrderRoutingServer/multisession_engine.hpp"
#include "infracore/BasicOrderRoutingServer/multisession_settings.hpp"
#include "infracore/NSET/NSEEngine.hpp"
#include "infracore/SimEngine/SimEngine.hpp"

using std::vector;

static string GetMailingList() {
  auto v = getenv("ERRMAIL");
  return v ? string(v) : "nseall@tworoads.co.in";
}

namespace HFSAT {
namespace ORS {

template <typename TEngine>
class MultiSessionEngine : public BaseEngine, public SimpleSecuritySymbolIndexerListener {
  typedef vector<TEngine*> TEngines;
  TEngines engines_;
  std::vector<int> engine_to_sgx_partition_;
  time_t last_time_accessed_;
  int last_engine;
  bool use_affinity_;
  string exchange_name;

  AsyncWriter async_sender, async_reciever;

  std::vector<std::vector<int> > account_to_engines_;  // Account to engines map (contains vector of indices
                                                       // corresponding to engines with an account)
  std::vector<int> account_to_current_engine_;  // vector of counters (which engine was last used for an account), to
                                                // use engines in round-robin fashion
  std::vector<int> security_to_account_;  // Mapping from security id to assigned engine index in account_to_engines_
                                          // map
  bool security_based_segregation;  // Is security message count based segregation mode active (presently expected to be
                                    // used only for BMF equities)
  bool is_sgx_;
  std::map<std::string, int> sgx_product_to_partition_;
  std::vector<int> sgx_sec_id_to_partition_;
  std::vector<int> sgx_partition_last_engine_index_;

  SimpleSecuritySymbolIndexer& simple_security_symbol_indexer_;  // For avoiding string comparisons

  OrderManager& order_manager_;
  BroadcastManager* bcast_manager_;
  MultisessionSettings multisession_settings_;

 public:
  TEngines& GetEngines() { return engines_; }
  MultiSessionEngine(Settings& settings, HFSAT::DebugLogger& logger, std::string output_log_dir_, string exchange_name_)
      : BaseEngine(settings, logger),
        last_engine(-1),
        exchange_name(exchange_name_),
        async_sender(1 * 1024 * 1024),
        async_reciever(1 * 1024 * 1024),
        is_sgx_(exchange_name_ == "SGX"),
        sgx_partition_last_engine_index_(3),
        simple_security_symbol_indexer_(HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance()),
        order_manager_(OrderManager::GetUniqueInstance()),
        bcast_manager_(BroadcastManager::GetUniqueInstance(dbglogger_, "", 0, 0)),
        multisession_settings_(settings.GetFileName()) {
    if (settings.has("SECURITY_SET-0")) {
      // It is implied that securities are mapped to individual accounts
      security_based_segregation = true;
    } else {
      security_based_segregation = false;
    }
    use_affinity_ = settings.has("UseAffinity") && (settings.getValue("UseAffinity") == "Y");
    auto sessions = settings.getValue("Multisessions", "0");
    std::replace(sessions.begin(), sessions.end(), ',', ' ');
    stringstream ss(sessions);
    int i = 0;
    dbglogger_ << "MSES: sessions: '" << sessions << "'\n";
    std::map<std::string, int> accounts_available;
    while (ss >> i) {
      dbglogger_ << "MSES: sessions id: " << i << "\n";
      Settings* session_setting = multisession_settings_.GetSessionSettings(i);
      engines_.push_back(new TEngine(*session_setting, logger, output_log_dir_, i, &async_sender, &async_reciever));

      engines_.back()->SetEngineID(i);

      if (is_sgx_) {
        int partition_id = session_setting->getIntValue("Partition", -1);
        if (partition_id < 1 || partition_id > 2) {
          dbglogger_ << "Invalid Partition Id " << partition_id << " for User " << session_setting->getValue("UserName")
                     << ". Exiting.\n";
          dbglogger_.DumpCurrentBuffer();
          exit(1);
        }
        engine_to_sgx_partition_.push_back(partition_id);
      }

      if (security_based_segregation) {
        // If we need to ensure that a single security is being traded via a single account
        std::string current_account_name = multisession_settings_.GetSessionSettings(i)->getValue("AccountName");
        if (accounts_available.find(current_account_name) == accounts_available.end()) {
          // No engine-index vector for this account yet => add new vector
          vector<int> temp_vec;
          account_to_engines_.push_back(temp_vec);
          account_to_current_engine_.push_back(0);
          accounts_available[current_account_name] = account_to_engines_.size() - 1;
        }
        // Add engine index to existing vector
        account_to_engines_[accounts_available[current_account_name]].push_back(engines_.size() - 1);
      }
    }

    // Map each present security to an account (Note that securities are not mapped to a specific account name)
    // Each security is mapped to any one account name
    int cur_account = 0;
    std::ostringstream temp_oss;
    while (security_based_segregation) {
      temp_oss.str("");  // Clear ostringstream
      temp_oss << "SECURITY_SET-" << cur_account;
      if (settings.has(temp_oss.str())) {
        // One more security set (assuming a space-separated list of securities
        std::string security_list = settings.getValue(temp_oss.str()), security_name;
        std::replace(security_list.begin(), security_list.end(), ',', ' ');
        int message_count, max_message_count;
        std::stringstream security_names(security_list);
        HFSAT::SecurityDefinitions& security_defines =
            HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());
        while (security_names >> security_name >> message_count >> max_message_count) {
          // security_name can be either exchange symbol or shortcode => appropriate handling for both
          char* this_exch_symbol_ = NULL;
          if (security_defines.IsValidContract(security_name)) {
            // Provided input is a shortcode
            const char* temp_exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(security_name);
            if (temp_exch_symbol_ != NULL && strlen(temp_exch_symbol_) > 0) {
              this_exch_symbol_ = new char[strlen(temp_exch_symbol_) + 1];
              strcpy(this_exch_symbol_, temp_exch_symbol_);
            }
          }
          if (this_exch_symbol_ == NULL) {
            // Provided input is a security symbol
            this_exch_symbol_ = new char[security_name.length() + 1];
            strcpy(this_exch_symbol_, security_name.c_str());
          }

          // We are using the security name itself in case we are not provided shortcode
          simple_security_symbol_indexer_.AddString(this_exch_symbol_);
          int security_id = simple_security_symbol_indexer_.GetIdFromSecname(this_exch_symbol_);

          while (security_id >= (int)security_to_account_.size()) {
            // ADDTRADINGSYMBOL was called between the process (new securities added in between => insert dummy values)
            // These values will get updated when the corresponding security is seen
            security_to_account_.push_back(0);
          }

          if (message_count >= max_message_count) {
            // Message count crossed threshold breached for this account
            // We expect another script/exec to reset msg counts at EOD (and rotate accounts) in case of breaches (but
            // additional handling here, just in case...)
            security_to_account_[security_id] =
                (cur_account + 1) % account_to_engines_.size();  // rotate account for this security
          } else {
            // Message count is fine for now
            security_to_account_[security_id] =
                cur_account % account_to_engines_.size();  // take modulo just to avoid boundary cases
          }
          std::cout << "Assigned Account " << security_to_account_[security_id] << " to Security: " << security_name
                    << ", secid: " << security_id << ", exchange_symbol: " << this_exch_symbol_ << "\n";
        }
        cur_account++;  // Allocating accounts in round-robin fashion
      } else {
        break;
      }
    }
  }
  ~MultiSessionEngine() { Misc::deleteAll(engines_); }
  virtual void setListener(EngineListener* l) {
    for (auto e : engines_) e->setListener(l);
    p_engine_listener_ = l;
  }
  void AdjustSymbol(const char* s) {
    std::cout << "calling adjust symbol from multisession "
              << "\n";
    for (auto e : engines_) {
      e->AdjustSymbol(s);
    }
  }

  void ChangeSpreadRatio(const char* symbol, const char* ratio) {
    std::cout << "calling " << __func__ << " from multisession "
              << "\n";
    for (auto e : engines_) {
      e->ChangeSpreadRatio(symbol, ratio);
    }
  }

  //! As of now the mirror factor is equal to the number of multi-session engines.
  uint32_t GetEngineSize() { return engines_.size(); }

  bool CheckForThrottle(TEngine* eng, int ord_saos, bool is_ioc) {
    bool is_reject_for_time_throttle = eng->reject_for_time_throttle(ord_saos);
    if (is_reject_for_time_throttle && is_ioc) {
      is_reject_for_time_throttle = eng->reject_for_time_throttle_ioc(ord_saos);
    }
    return is_reject_for_time_throttle;
  }

  //! the mirror_saos is actually the max mirror_saos
  void MirrorOrder(Order* ord, int mirror_factor) override {
    //================================================
    // first loop and send all the mirror orders
    // then send the original base order
    int min_saos = ord->server_assigned_order_sequence_ - mirror_factor + 1;
    int max_saos = ord->server_assigned_order_sequence_;
    bool is_ioc = ord->is_ioc;
    bool is_rejected[mirror_factor] = {false};

    for (int ord_saos = min_saos; ord_saos <= max_saos; ord_saos++) {
      TEngine* eng = FindSendEngine(ord_saos, ord->symbol_);
      if (!eng) {
        continue;
      }

      bool is_reject_for_time_throttle = CheckForThrottle(eng, ord_saos, is_ioc);
      order_manager_.SetMirrorMetaData(ord_saos, max_saos, (uint64_t)eng, ord->size_remaining_, ord->size_executed_);
      ord->server_assigned_order_sequence_ = ord_saos;

      if (!is_reject_for_time_throttle) {
        eng->SendOrder(ord);
      } else {
        is_rejected[ord_saos - min_saos] = true;
      }
    }

    ord->server_assigned_order_sequence_ = max_saos;

    //==============================================================
    for (int i = 0; i < mirror_factor; i++) {
      if (is_rejected[i]) {
        // call onReject
        p_engine_listener_->OnReject(min_saos + i, kORSRejectThrottleLimitReached);
      } else {
        bcast_manager_->BroadcastSequenced(*ord, min_saos + i);
      }
    }
    //==============================================================
  }

  void SendOrder(Order* ord) override {
    TEngine* eng = FindSendEngine(ord->server_assigned_order_sequence_, ord->symbol_);

    if (!eng) {
      return;
    }

    ord->engine_ptr_ = (uint64_t)eng;

    bool is_reject_for_time_throttle = eng->reject_for_time_throttle(ord->server_assigned_order_sequence_);
    if (is_reject_for_time_throttle && ord->is_ioc) {
      is_reject_for_time_throttle = eng->reject_for_time_throttle_ioc(ord->server_assigned_order_sequence_);
    }

    if (is_reject_for_time_throttle) {
      p_engine_listener_->OnReject(ord->server_assigned_order_sequence_, kORSRejectThrottleLimitReached);
      SequenceGenerator::GetUniqueInstance(exchange_name).RevertToPreviousSequence();
      return;
    }

    eng->SendOrder(ord);
    bcast_manager_->BroadcastSequenced(*ord);
  }

  void CancelOrder(Order* ord) override {
    TEngine* eng = FindCxlModifyEngine(ord, ord->symbol_);

    if (!eng) {
      return;
    }

    if (eng->reject_for_time_throttle(ord->server_assigned_order_sequence_)) {
      bcast_manager_->BroadcastCancelRejection(*ord, kCxlRejectReasonThrottle);
      return;
    }

    eng->CancelOrder(ord);

    bcast_manager_->BroadcastCxlSequenced(*ord, ord->ors_timestamp_);
  }

  void ModifyOrder(Order* ord, ORS::Order *orig_order) override {
    TEngine* eng = FindCxlModifyEngine(ord, ord->symbol_);

    if (!eng) {
      return;
    }

    if (eng->reject_for_time_throttle(ord->server_assigned_order_sequence_)) {
      Order* old_ord = order_manager_.GetOrderByOrderSequence(ord->server_assigned_order_sequence_);

      if (NULL != old_ord) {
        bcast_manager_->BroadcastORSCancelReplaceRejection(*ord, kORSCxReRejectThrottleLimitReached, old_ord->price_,
                                                           old_ord->int_price_);
      }
      return;
    }

    eng->ModifyOrder(ord, orig_order);

    bcast_manager_->BroadcastCxlReSequenced(*ord, ord->ors_timestamp_);
  }

  void CheckToSendHeartbeat() {
    for (auto e : engines_) {
      e->CheckToSendHeartbeat();
    }
  }

  void ProcessMessageQueue() {
    for (auto e : engines_) {
      e->ProcessMessageQueue();
    }
  }

  void ProcessFakeSend(Order* ord, ORQType_t type) {
    for (auto e : engines_) {
      e->ProcessFakeSend(ord, type);
    }
  }

  /// technical session control calls
  void Connect() {
    for (auto e : engines_) {
      dbglogger_ << "MSES:(" << e->id_ << ") trying to Connect\n";
      e->Connect();
      sleep(30);
    }
    if ("NSE_FO" == exchange_name || "NSE_CD" == exchange_name || "NSE_EQ" == exchange_name) {
      async_sender.start();
      async_reciever.start();
    }
  }

  void SetPlaybackMode(bool val) {
    for (auto e : engines_) {
      dbglogger_ << "MSES:(" << e->id_ << ") Set Playback Mode " << val << "\n";
      e->SetPlaybackMode(val);
    }
  }
  void DisConnect() {
    TEngines es = engines_;  // make a copy because engines_ can be modified
    for (auto e : es) {
      dbglogger_ << "MSES:(" << e->id_ << ") trying to DisConnect\n";
      e->DisConnect();
    }
    async_sender.stop();
    async_reciever.stop();
  }
  void Login() {
    for (auto e : engines_) {
      dbglogger_ << "MSES:(" << e->id_ << ") trying to Login\n";
      e->Login();
    }

    if ( "NSE_FO" == exchange_name || "NSE_CD" == exchange_name || "NSE_EQ" == exchange_name ||
        "BMFEP" == exchange_name ) {
      async_sender.start();
      async_reciever.start();
    }

    multisession_settings_.DumpSettingsToFile();
  }
  void Logout() {
    multisession_settings_.DumpSettingsToFile();

    TEngines es = engines_;  // make a copy because engines_ can be modified
    dbglogger_ << "MSES: Sessions alive : " << es.size() << "\n";
    for (auto e : es) {
      dbglogger_ << "MSES:(" << e->id_ << ") trying to Logout\n";
      e->Logout();
    }
  }

  void ProcessGeneralControlCommand(const char* input_stream, int stream_length) override {
    char temp_command_buffer_[1025];
    strcpy(temp_command_buffer_, input_stream);
    PerishableStringTokenizer st_(temp_command_buffer_, 1024);
    const std::vector<const char*>& tokens_ = st_.GetTokens();

    if ((tokens_.size() >= 2) && (strcmp(tokens_[0], "CONTROLCOMMAND") == 0)) {
      // start processing here
      // tokens[2] would be function keyword and arguments follow
    }
    for (auto e : engines_) {
      dbglogger_ << "MSES:(" << e->id_ << ") Process General Control Command\n";
      e->ProcessGeneralControlCommand(input_stream, stream_length);
    }
  }

  void setSenderSubId(const char* sender_sub_id_) {
    TEngines es = engines_;  // make a copy because engines_ can be modified
    for (auto e : es) {
      dbglogger_ << "MSES:(" << e->id_ << ") trying to setSenderSubId to " << sender_sub_id_ << "\n";
      e->setSenderSubId(sender_sub_id_);
    }
  }

  void CommitFixSequences() {
    TEngines es = engines_;  // make a copy because engines_ can be modified
    dbglogger_ << "MSES: Sessions alive : " << es.size() << "\n";
    for (auto e : es) {
      dbglogger_ << "MSES:(" << e->id_ << ") trying to Save Sequences\n";
      e->CommitFixSequences();
    }
  }

  TEngine* FindSendEngine(ServerSeq _saos, const char* secname = NULL) {
    if (!is_sgx_) {
      last_engine = (last_engine + 1) % engines_.size();
      return engines_[last_engine];
    } else {
      int partition_id = sgx_sec_id_to_partition_[simple_security_symbol_indexer_.GetIdFromChar16(secname)];
      if (partition_id == 0) {
        // No Ref Data for this product
        return nullptr;
      }
      int num_trails = engines_.size();
      int size = engines_.size();

      for (unsigned int i = (sgx_partition_last_engine_index_[partition_id] + 1) % size; num_trails;
           i = (i + 1) % size, num_trails--) {
        if (engine_to_sgx_partition_[i] == partition_id) {
          sgx_partition_last_engine_index_[partition_id] = i;
          return engines_[i];
        }
      }
      return nullptr;
    }
    return nullptr;
  }

  TEngine* FindCxlModifyEngine(const Order* this_order, const char* secname = NULL) {
    if (this_order) {
      TEngine* eng = (TEngine*)this_order->engine_ptr_;

      if (eng != 0) {
        return eng;
      } else {
        Email mail;
        mail.toMyself(GetMailingList())
                .withSubject("Multisession: Multiplexing error. Cancel Order with unknown engine binding")
                .content_stream
            << "for SAOS: " << this_order->server_assigned_order_sequence_
            << " we could not find an appropriate ORS engine\n\n";
        mail.sendMail();
        return nullptr;
      }
    } else {
      // Most likely already cancelled or filled
      return nullptr;
    }
    return nullptr;
  }

  void thread_main() {
    if (use_affinity_) {
      setName("MultiSessionEngine");
      if (AllocateToSiblingCore() < 0) {
        std::cout << "Failed to allocated to Sibling Core. Will allocate to any other core." << std::endl;
        AllocateCPUOrExit();
      }
    }
    do {
      auto faulty_sessions = executeTillFailure(engines_);
      for (auto e : faulty_sessions) {
        auto pos = std::find(engines_.begin(), engines_.end(), e);
        assert(pos != engines_.end());
        if (is_sgx_) {
          for (unsigned int i = 0; i < engines_.size(); i++) {
            if (engines_[i] == e) {
              engine_to_sgx_partition_.erase(engine_to_sgx_partition_.begin() + i);
            }
          }
        }
        engines_.erase(pos);

        // donot delete the engine; let it leak; there are many direct references to the that
        // engine; incase a msg is sent by mistake, the msg will be rejected followed by an e-mail alert.
        // deleting will result in a crash
      }
      dbglogger_ << "MSESS: Now working with: " << engines_.size() << " sessions\n";

      //For nse only currently

      time_t t = time(0);

      //! Note the naming convention of the variables are as per NSE market timings,
      //! but the assigned time are in GMT.
      struct tm tm_curr = *localtime(&t);
      struct tm tm_7_15 = *localtime(&t);
      struct tm tm_15_30 = *localtime(&t);

      tm_15_30.tm_hour = 10;
      tm_15_30.tm_min = 0;
      tm_15_30.tm_sec = 0;

      tm_7_15.tm_hour = 1;
      tm_7_15.tm_min = 45;
      tm_7_15.tm_sec = 0;

      time_t time_curr = mktime(&tm_curr);
      time_t time_15_30 = mktime(&tm_15_30);
      time_t time_7_15 = mktime(&tm_7_15);

      if (difftime(time_curr, time_7_15) >= 0 && difftime(time_curr, time_15_30) <= 0) {
        dbglogger_ << "SENT MAIL FOR DISCONNECTION... " << "\n";
        dbglogger_.DumpCurrentBuffer();

        std::ostringstream e_oss ;
        e_oss << "NOW WORKING WITH -> " << engines_.size () << " SESSIONS" ;
        SendMail("EngineSessionDisconnected", e_oss.str());
      }

    } while (engines_.size());
    dbglogger_ << "MSESS: ALL-SESSIONS TERMINATED:\n";
  }

  void OnAddString(uint32_t num_sid) {
    if (is_sgx_) {
      if (sgx_sec_id_to_partition_.size() < num_sid) {
        if (sgx_product_to_partition_.find(simple_security_symbol_indexer_.GetSecuritySymbolFromId(num_sid - 1)) ==
            sgx_product_to_partition_.end()) {
          sgx_sec_id_to_partition_.push_back(0);
        } else {
          sgx_sec_id_to_partition_.push_back(
              sgx_product_to_partition_[simple_security_symbol_indexer_.GetSecuritySymbolFromId(num_sid - 1)]);
        }
      }
    }
  }

 private:
  static TEngines executeTillFailure(TEngines& _engines) {
    SocketSet sset;
    for (auto e : _engines) sset.add(e->init(), e);

    auto selected = sset.AllocateSelectedContainer();
    TEngines faulty_sessions;
    while (true) {
      int n = sset.waitForAny(selected);
      for (int i = 0; i < n; i++) {
        auto& sel = selected[i];
        auto eng = static_cast<TEngine*>(sel.ref);
        auto result = eng->onInputAvailable(sel.sock);
        if (result < 0) {
          if (kSessionClosed == result) std::cout << "SESSION CLOSED \n";
          if (kSessionTerminated == result) std::cout << "SESSION TERMINATED \n";
          if (kSessionNotRunning == result) std::cout << "Session Not running \n";
          if (kSessionClosed == result)
            faulty_sessions.push_back(eng);  // we should not send an email for this once things are stable
          if (kSessionTerminated == result)
            faulty_sessions.push_back(
                eng);  // we donot try to re-connect; just gracefully remove this from the session set;
          if (kSessionNotRunning == result) faulty_sessions.push_back(eng);
        }
      }
      if (faulty_sessions.size()) return faulty_sessions;
    }
  }

  void SendMail(const std::string subject, const std::string message) {
    HFSAT::Email e;
    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    e.setSubject(subject);
    e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
    e.addSender("ravi.parikh@tworoads.co.in");
    e.content_stream << message << "<br/>";
    e.content_stream << "Current Time: " << ctime(&(current_time.tv_sec)) << " <br/>";
    e.sendMail();
  }
};

BaseEngine* makeMultiSessionEngine(const std::string& ex, Settings& settings, HFSAT::DebugLogger& logger,
                                   std::string output_log_dir) {
  BaseEngine* en = nullptr;
  if (ex == "BMFEP" || "BMFEQ" == ex)
    en = new MultiSessionEngine<BMFEPEngine>(settings, logger, output_log_dir, ex);
  else if ("NSE_FO" == ex)
    en = new MultiSessionEngine<HFSAT::NSE::NSEEngine>(settings, logger, output_log_dir, ex);
  else if ("NSE_CD" == ex)
    en = new MultiSessionEngine<HFSAT::NSE::NSEEngine>(settings, logger, output_log_dir, ex);
  else if ("NSE_EQ" == ex)
    en = new MultiSessionEngine<HFSAT::NSE::NSEEngine>(settings, logger, output_log_dir, ex);
  else if ("MSSIM" == ex)
    en = new MultiSessionEngine<HFSAT::SimEngine::SIMEngine>(settings, logger, output_log_dir, ex);
  else
    throw std::runtime_error("unknown exchange in makeMultiSessionEngine : " + ex);
  return en;
}
}
}
