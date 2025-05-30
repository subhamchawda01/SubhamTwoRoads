#include "infracore/BasicOrderRoutingServer/control_receiver.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {
namespace ORS {

ControlReceiver::ControlReceiver(DebugLogger& _dbglogger_, Settings& _settings_,
                                 HFSAT::Utils::ClientLoggingSegmentInitializer* _client_logging_segment_initializer_,
                                 std::string t_output_log_dir_, bool &_cancel_live_order_flag_)
    : dbglogger_(_dbglogger_),
      tcp_server_socket_(atoi(_settings_.getValue("Control_Port").c_str())),
      settings_(_settings_),
      cancel_live_order_flag_(_cancel_live_order_flag_),
      use_affinity_((_settings_.has("UseAffinity") && _settings_.getValue("UseAffinity") == "Y")),
      is_addts_thread_running_(new bool(false)),
      client_logging_segment_initializer_(_client_logging_segment_initializer_),
      output_log_dir_(t_output_log_dir_) {
  LoadMarginFile();
}

ControlReceiver::~ControlReceiver() {
  tcp_server_socket_.Close();

  // Cleanup
  // Also we want to free the remaining control thread since recv. is dead
  delete is_addts_thread_running_;
  for (unsigned int i = 0; i < active_control_threads_.size(); i++) {
    if (active_control_threads_[i] != NULL) {
      delete active_control_threads_[i];
      active_control_threads_[i] = NULL;
    }
  }

  active_control_threads_.empty();
  active_control_threads_marking_.empty();
}

void ControlReceiver::LoadMarginFile() {
  std::vector<std::string> malformatted_lines;
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  std::string initial_margin_filename_;

  if (!strncmp(hostname, "SDV-HK-SRV", 10)) {
    if (!settings_.has("UseOverrideMarginFile")) {
      initial_margin_filename_ = "/home/newedge/limit.csv";
      dbglogger_ << "Using_default_limits_file  " << initial_margin_filename_ << "\n";
      dbglogger_.DumpCurrentBuffer();
    } else {
      initial_margin_filename_ = settings_.getValue("UseOverrideMarginFile");
    }
  } else {
    std::ostringstream t_temp_oss;
    t_temp_oss << PROD_CONFIGS_DIR << "common_initial_margin_file.txt";

    initial_margin_filename_ = t_temp_oss.str();
  }

  std::ifstream initial_margin_file_;
  initial_margin_file_.open(initial_margin_filename_.c_str(), std::ifstream::in);
  if (!(initial_margin_file_.is_open())) {
    dbglogger_ << "Cannot open file " << initial_margin_filename_ << "\n";
    dbglogger_.DumpCurrentBuffer();
    exit(-1);
  }

  char line[1024];
  while (!initial_margin_file_.eof()) {
    bzero(line, 1024);
    initial_margin_file_.getline(line, 1024);
    if (strlen(line) == 0 || strstr(line, "#") != NULL) continue;
    std::string line_str = line;
    HFSAT::PerishableStringTokenizer st_(line, 1024);
    const std::vector<const char*>& tokens_ = st_.GetTokens();
    if (tokens_.size() != 4) {
      malformatted_lines.push_back(line_str);
      dbglogger_ << "Ignoring Malformatted line in file " << initial_margin_filename_.c_str() << "\n";
      dbglogger_.DumpCurrentBuffer();
    }

    // This sets up the limits on the positions for different instruments.
    // Uses the limit information provided by MFGlobal.
    if (tokens_.size() == 4)  {
      sec_to_max_pos_map_[tokens_[0]] = atoi(tokens_[2]);
      sec_to_max_ord_sz_map_[tokens_[0]] = atoi(tokens_[3]);
    }
  }

  if (malformatted_lines.size() > 0) {
    HFSAT::Email e;
    std::string subject = "Malformatted line in " + initial_margin_filename_ + " " + hostname;
    e.setSubject(subject);
    e.addRecepient("nseall@tworoads.co.in");
    e.addSender("ravi.parikh@tworoads.co.in");
    for (auto i : malformatted_lines) {
      e.content_stream << i << "<br/>";
    }
    e.sendMail();
  }

  initial_margin_file_.close();
}

// Should be thread-safe
void ControlReceiver::RemoveThread(int32_t const& thread_id) { active_control_threads_marking_[thread_id] = false; }

void ControlReceiver::thread_main() {
  int32_t thread_id = 0;

  if (use_affinity_) {
    setName("ControlReceiver");
    //        AllocateCPUOrExit ();
  }
  // std::cout<<"Control Re ceiver thread started\n";

  while (int connected_socket_file_descriptor_ = tcp_server_socket_.Accept()) {
    if (connected_socket_file_descriptor_ < 0) {
      break;
    }

    for (auto& itr : active_control_threads_marking_) {
      if (false == itr.second) {
        if (NULL != active_control_threads_[itr.first]) {
          std::cout << " Deleting : " << itr.first << " " << std::endl;
          active_control_threads_[itr.first]->StopControlThread();
          std::cout << " Waiting to stop control thread..." << std::endl;
          active_control_threads_[itr.first]->stop();
          std::cout << " Stopped control thread for..." << itr.first << std::endl;
          // Doesn't make any difference anymore here, Will prevent further stop calls
          // TODO : Check Why should a stopped thread wait call matter
          itr.second = true;
          //          delete active_control_threads_[ itr.first ] ;
          //          active_control_threads_[ itr.first ] = NULL ;
        }
      }
    }

    // std::cout<<"Connection Accepted\n";
    ControlThread* _new_control_thread_ =
        new ControlThread(dbglogger_, connected_socket_file_descriptor_, settings_, client_logging_segment_initializer_,
                          output_log_dir_,cancel_live_order_flag_, sec_to_max_pos_map_, 
                          sec_to_max_ord_sz_map_, this, thread_id, is_addts_thread_running_);

    active_control_threads_[thread_id] = _new_control_thread_;
    active_control_threads_marking_[thread_id] = true;

    _new_control_thread_->run();  // starts the thread

    thread_id++;
  }
}

void ControlReceiver::StopControlThreads() {
  for (unsigned int i = 0; i < active_control_threads_.size(); i++) {
    active_control_threads_[i]->StopControlThread();
  }
}
}
}
