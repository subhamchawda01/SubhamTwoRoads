/**
   file Tools/cpu_affinity_monitor.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551

*/
#include <iostream>
#include <sys/syscall.h>
#include <vector>
#include <map>
#include <signal.h>
#include <fstream>
#include <string>
#include <sstream>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/CPUAffinity.hpp"
#include "dvccode/Utils/PIDTracker.hpp"
#include "dvccode/Utils/send_alert.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

HFSAT::DebugLogger dbglogger_(10240, 1);

class CPUAffinityMonitor : public HFSAT::Thread {
  int affinity_monitor_assigned_core_;
  int sleep_interval_;
  std::vector<std::string> affinity_process_list_;
  process_type_map process_and_type_;

 public:
  CPUAffinityMonitor(int _amac_, int sleepTime = 15)
      : affinity_monitor_assigned_core_(_amac_), sleep_interval_(sleepTime) {
    int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/alllogs/CPU_AFFIN.log." << tradingdate_;
    dbglogger_.OpenLogFile(t_temp_oss_.str().c_str(), std::ofstream::app);
    /*
     std::ifstream affinity_process_list_file_ ;
     affinity_process_list_file_.open( AFFINITY_PROC_LIST_FILENAME );

     if( !affinity_process_list_file_.is_open() ){

       std::cerr << " File : " << AFFINITY_PROC_LIST_FILENAME << " does not exist " << std::endl;
       exit( -1 );

     }

     char line_buffer_[ 1024 ] ;
     std::string line_read_ = " ";

     while( affinity_process_list_file_.good() ){

       memset( line_buffer_, 0, sizeof( line_buffer_ ) );

       affinity_process_list_file_.getline( line_buffer_, sizeof( line_buffer_ ) );
       line_read_ = line_buffer_ ;

       if( line_read_.find( "#" ) != std::string::npos )  continue ;

       HFSAT::PerishableStringTokenizer st_ ( line_buffer_, 1024 );
       const std::vector < const char * > & tokens_ = st_.GetTokens ( );

       if( tokens_.size() > 0 )
         affinity_process_list_.push_back( tokens_[0] );

     }

     affinity_process_list_file_.close( );
 */
    // process_type_map process_and_type;
    process_and_type_ = AffinityAllocator::parseProcessListFile(affinity_process_list_);
  }

  void EmailOnConflictResolution(std::string msg_body_) {
    // also send an alert
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    std::string alert_message =
        "ALERT: Core Conflict at " + std::string(hostname);  // need to differentiate alert and recovery
    HFSAT::SendAlert::sendAlert(alert_message);

    HFSAT::Email email_;
    email_.setSubject("Subject: Core Conflict/Resolution");
    email_.addRecepient("nseall@tworoads.co.in");
    email_.addSender("nseall@tworoads.co.in");
    email_.content_stream << msg_body_ << "<br/>";
    email_.content_stream << "host_machine: " << hostname << "<br/>";
    email_.sendMail();
  }

  void thread_main() {
    int threadId = ((int)(syscall(SYS_gettid)));

    setName("AffinityMonitor");

    if (CPUManager::setAffinity(affinity_monitor_assigned_core_))
      dbglogger_ << "Thread : AffinityMonitor"
                 << " Thread ID : " << threadId << " CORE # " << affinity_monitor_assigned_core_ << "\n\n";

    std::map<int, int> affinity_tracker_map_;        // core to pid map
    std::map<int, std::string> pid_to_process_map_;  // pid to processname map
    std::map<int, std::string>::iterator pid_to_process_map_iterator_;
    std::vector<int> pid_list_;  // pid list -- redundant, not a critical thread abt opt
    std::map<int, bool> pid_to_conflict_notified_;
    std::map<int, bool> pid_to_conflict_persists_notified_;
    std::ifstream affinity_pid_process_list_file_;
    char line_buffer_[1024];
    std::string line_read_ = "";

    std::ostringstream email_;

    while (true) {
      pid_to_process_map_.clear();
      pid_list_.clear();

      affinity_pid_process_list_file_.open("/spare/local/files/affinity_pid_process.txt");

      if (!affinity_pid_process_list_file_.is_open()) {
        std::cerr << " File : /spare/local/files/affinity_pid_process.txt doesn't exist " << std::endl;
        exit(-1);
      }

      while (affinity_pid_process_list_file_.good()) {
        memset(line_buffer_, 0, sizeof(line_buffer_));
        line_read_ = "";

        affinity_pid_process_list_file_.getline(line_buffer_, sizeof(line_buffer_));
        line_read_ = line_buffer_;

        if (line_read_.find("#") != std::string::npos) continue;  // comments etc.

        HFSAT::PerishableStringTokenizer st_(line_buffer_, sizeof(line_buffer_));
        const std::vector<const char *> &tokens_ = st_.GetTokens();

        if (tokens_.size() == 2) {
          pid_to_process_map_[atoi(tokens_[0])] = tokens_[1];
          pid_list_.push_back(atoi(tokens_[0]));
          pid_to_conflict_notified_[atoi(tokens_[0])] = false;
          pid_to_conflict_persists_notified_[atoi(tokens_[0])] = false;
        }
      }

      affinity_pid_process_list_file_.close();

      affinity_tracker_map_.clear();

      email_.clear();
      email_.str("");

      for (unsigned int pid_counter_ = 0; pid_counter_ < pid_list_.size(); pid_counter_++) {
        unsigned int mask = CPUManager::getAffinity(pid_list_[pid_counter_]);

        if (!mask) continue;  // ignoring dead processes

        if (pid_to_process_map_[pid_list_[pid_counter_]].compare("tradeinit") == 0) {
          if (mask != CPUManager::getFreeMask())

          {
            affinity_tracker_map_[mask] = pid_list_[pid_counter_];
          }

          continue;
        }

        if (mask == CPUManager::getFreeMask()) {
          dbglogger_ << "Freemask       --- ProcessName : " << pid_to_process_map_[pid_list_[pid_counter_]]
                     << " | PID : " << pid_list_[pid_counter_] << "\n\n";
          dbglogger_.DumpCurrentBuffer();

          if (pid_to_conflict_notified_.find(pid_list_[pid_counter_]) != pid_to_conflict_notified_.end()) {
            if (!pid_to_conflict_notified_[pid_list_[pid_counter_]]) {
              email_ << "Freemask       --- ProcessName : " << pid_to_process_map_[pid_list_[pid_counter_]]
                     << " | PID : " << pid_list_[pid_counter_] << "<br/>";
              pid_to_conflict_notified_[pid_list_[pid_counter_]] = true;  // mark this pid caused an alert
              pid_to_conflict_persists_notified_[pid_list_[pid_counter_]] = false;
            }
          }

          int core = CPUManager::allocateFirstBestAvailableCore(process_and_type_, affinity_process_list_,
                                                                pid_list_[pid_counter_], false);

          if (core != -1) {
            dbglogger_ << "Allocation   --- ProcessName : " << pid_to_process_map_[pid_list_[pid_counter_]]
                       << " | PID : " << pid_list_[pid_counter_] << " | NewCore # " << core << "\n\n";

            if (pid_to_conflict_notified_.find(pid_list_[pid_counter_]) != pid_to_conflict_notified_.end()) {
              if (pid_to_conflict_notified_[pid_list_[pid_counter_]]) {
                email_ << "Allocation   --- ProcessName : " << pid_to_process_map_[pid_list_[pid_counter_]]
                       << " | PID : " << pid_list_[pid_counter_] << " | NewCore # " << core << "<br/>";
                pid_to_conflict_notified_[pid_list_[pid_counter_]] = false;
                pid_to_conflict_persists_notified_[pid_list_[pid_counter_]] = false;
              }
            }

          }

          else {
            dbglogger_ << "Allocation Failed --- ProcessName : " << pid_to_process_map_[pid_list_[pid_counter_]]
                       << " | PID : " << pid_list_[pid_counter_] << " | FREEMASK "
                       << "\n\n";

            if (pid_to_conflict_notified_.find(pid_list_[pid_counter_]) != pid_to_conflict_notified_.end()) {
              if (!pid_to_conflict_notified_[pid_list_[pid_counter_]]) {
                // send an email for resolution
                email_ << "Allocation Failed --- ProcessName : " << pid_to_process_map_[pid_list_[pid_counter_]]
                       << " | PID : " << pid_list_[pid_counter_] << " | FREEMASK "
                       << "<br/>";
                pid_to_conflict_persists_notified_[pid_list_[pid_counter_]] = true;
              }
            }
          }

          dbglogger_.DumpCurrentBuffer();

          continue;

        } else if (affinity_tracker_map_.find(mask) != affinity_tracker_map_.end()) {
          dbglogger_ << "Conflict       --- ProcessName : " << pid_to_process_map_[pid_list_[pid_counter_]]
                     << " | PID : " << pid_list_[pid_counter_]
                     << " Vs ProcessName : " << pid_to_process_map_[affinity_tracker_map_[mask]]
                     << " | PID : " << affinity_tracker_map_[mask] << " --- CoreMask # " << mask << "\n\n";

          if (pid_to_conflict_notified_.find(pid_list_[pid_counter_]) != pid_to_conflict_notified_.end()) {
            if (!pid_to_conflict_notified_[pid_list_[pid_counter_]]) {
              email_ << "Conflict       --- ProcessName : " << pid_to_process_map_[pid_list_[pid_counter_]]
                     << " | PID : " << pid_list_[pid_counter_]
                     << " Vs ProcessName : " << pid_to_process_map_[affinity_tracker_map_[mask]]
                     << " | PID : " << affinity_tracker_map_[mask] << " --- CoreMask # " << mask << "<br/>";
              pid_to_conflict_notified_[pid_list_[pid_counter_]] = true;  // mark this pid caused an alert
              pid_to_conflict_persists_notified_[pid_list_[pid_counter_]] = false;
            }
          }

          int pid_reallocated_ = -1;

          int core = CPUManager::allocateFirstBestAvailableCore(process_and_type_, affinity_process_list_,
                                                                pid_list_[pid_counter_], false);
          pid_reallocated_ = pid_list_[pid_counter_];

          if (core == -1) {
            core = CPUManager::allocateFirstBestAvailableCore(
                process_and_type_, affinity_process_list_, affinity_tracker_map_[mask],
                false);  // try to resolve conflict by allocation of former process even later caused it and failed to
                         // be affined
            pid_reallocated_ = (core == -1) ? pid_list_[pid_counter_] : affinity_tracker_map_[mask];
          }

          if (core != -1) {
            dbglogger_ << "Reallocation   --- ProcessName : " << pid_to_process_map_[pid_reallocated_]
                       << " | PID : " << pid_reallocated_ << " | NewCore # " << core << "\n\n";

            if (pid_to_conflict_notified_.find(pid_list_[pid_counter_]) != pid_to_conflict_notified_.end()) {
              if (pid_to_conflict_notified_[pid_list_[pid_counter_]]) {
                email_ << "Reallocation   --- ProcessName : " << pid_to_process_map_[pid_list_[pid_counter_]]
                       << " | PID : " << pid_reallocated_ << " | NewCore # " << core << "<br/>";
                pid_to_conflict_notified_[pid_list_[pid_counter_]] = false;
                pid_to_conflict_persists_notified_[pid_list_[pid_counter_]] = false;
              }
            }

          } else {
            dbglogger_ << "Reallocation Failed --- ProcessName : " << pid_to_process_map_[pid_list_[pid_counter_]]
                       << " | PID : " << pid_list_[pid_counter_] << " | CoreMask # " << mask << "\n\n";

            if (pid_to_conflict_notified_.find(pid_list_[pid_counter_]) != pid_to_conflict_notified_.end()) {
              if (!pid_to_conflict_persists_notified_[pid_list_[pid_counter_]]) {
                email_ << "Reallocation Failed --- ProcessName : " << pid_to_process_map_[pid_list_[pid_counter_]]
                       << " | PID : " << pid_list_[pid_counter_] << " | CoreMask # " << mask << "<br/>";
                pid_to_conflict_notified_[pid_list_[pid_counter_]] = true;
                pid_to_conflict_persists_notified_[pid_list_[pid_counter_]] = true;
              }
            }
          }

          continue;

        } else {
          affinity_tracker_map_[mask] = pid_list_[pid_counter_];
        }
      }
      if ((email_.str()).compare("") != 0) {
        EmailOnConflictResolution(email_.str());
        email_.clear();
        email_.str("");
      }

      // TODO sleeping for now - need to change the code to make it run in user space only
      sleep(sleep_interval_);

      affinity_pid_process_list_file_.close();
    }
  }
};

CPUAffinityMonitor *p_cpu_affinity_monitor_ = NULL;

void termination_handler(int signal_no_) {
  if (p_cpu_affinity_monitor_) {
    dbglogger_.DumpCurrentBuffer();
    dbglogger_.Close();
    p_cpu_affinity_monitor_->stop();
  }

  exit(0);
}

int main(int argc, char **argv) {
  signal(SIGINT, termination_handler);

  if (argc < 2) {
    std::cerr << " USAGE : <exec> <core> <opt : sleep_time=15>\n";
    exit(-1);
  }

  int core_ = atoi(argv[1]);
  int sleeptime_ = 15;

  if (argc >= 3) {
    sleeptime_ = atoi(argv[2]);
  }

  CPUAffinityMonitor cpmon(core_, sleeptime_);

  p_cpu_affinity_monitor_ = &cpmon;

  cpmon.run();
  cpmon.stop();

  return 0;
}
