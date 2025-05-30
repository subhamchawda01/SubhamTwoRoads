/**
   \file lwfixfast/mds_logger.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unordered_map>
#include <map>
#include <set>

#include "dvccode/Utils/mds_logger_settings.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/Utils/send_alert.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/lockfree_queue.hpp"
#include "dvccode/Utils/slack_utils.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CombinedControlUtils/combined_control_messages.hpp"
#include "dvccode/CombinedControlUtils/combined_control_message_listener.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#define MDS_LOGGER_CONFIG_FILE "/spare/local/files/mds_logger_config.txt"
#define LOGGED_DATA_PREFIX "/spare/local/MDSlogs/"

template <class T>
class MDSLogger : public HFSAT::Thread, public HFSAT::CombinedControlMessageListener {
 protected:
  std::string exch;
  std::map<std::string, HFSAT::BulkFileWriter*> inst_file_map_;
  std::map<HFSAT::ExchSource_t, struct timeval> exch_to_last_data_map_;
  std::map<HFSAT::ExchSource_t, bool> exch_to_silent_period_map_;
  std::map<HFSAT::ExchSource_t, HFSAT::MDSLoggerSettings> exch_to_mds_settings_map_;
  std::map<HFSAT::ExchSource_t, int> exch_to_timeout_map_;
  std::map<HFSAT::ExchSource_t, std::ofstream*> exch_to_notification_file_map_;
  HFSAT::LockFreeQ<T> queue;
  timeval tv_last_data;
  timeval current_time;
  bool stop_thread;
  std::string data_loggin_dir;
  std::string data_loggin_dir2;
  std::string data_loggin_dir_opt;
  std::string data_loggin_dir_opt_w;
  HFSAT::MDSLoggerSettings mds_logger_settings_;
  int timeout_sec_;
  bool is_liffe_;
  bool is_ice_;
  bool is_ose_;
  bool is_cme_;
  bool is_ebs_;
  bool is_generic_;
  bool is_bardata_;
  int yyyymmdd_;
  bool should_affine_;
  std::string name_;

 private:
  void AddToMap(std::string inst) {
    char date_str[9] = {'\0'};
    sprintf(date_str, "%d", yyyymmdd_);
    std::string fname = data_loggin_dir + "/" + inst + "_" + date_str;  // Save into corresponding exchange directory
    std::string symbol,file_key;
    if ( exch == "GENERIC" && (inst.find("_CE_") != std::string::npos)){
        fname = data_loggin_dir2 + "/" + inst + "_" + date_str;
    } else if ( exch == "BARDATA" ) {
      std::vector<char*> tokens_;
      char readline_buffer_copy_[1024];
      memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
      strcpy(readline_buffer_copy_, inst.c_str());
      HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, "_", tokens_);
      symbol = tokens_[1];
      uint16_t token_size = tokens_.size();
      if ( inst.find("_FUT") != std::string::npos ) {
        fname = data_loggin_dir + "/" + symbol; 
        file_key = symbol + "_FUT"; 
      } else if ( std::string(tokens_[token_size - 1] ) == "W" ) {
        fname = data_loggin_dir_opt_w + "/" + symbol;
        file_key = symbol + "_W";
      } else {
        fname = data_loggin_dir_opt + "/" + symbol;
        file_key = symbol;
      }
    }
    // 4 KB - most files are atleast this much.
    HFSAT::BulkFileWriter* new_file_ = new HFSAT::BulkFileWriter(
        fname.c_str(), 1 * 1024, std::ofstream::binary | std::ofstream::app | std::ofstream::ate);

    if ( exch == "BARDATA" ) 
      inst_file_map_[file_key] = new_file_;
    else
      inst_file_map_[inst] = new_file_;
  }

  void OnCombinedControlMessageReceived(HFSAT::CombinedControlMessage combined_control_request_) {
    std::cout << "CombinedControlMsgReceived: " << combined_control_request_.ToString() << "\n";
    if (combined_control_request_.message_code_ == HFSAT::kCmdControlMessageCodeChangeTolerance) {
      // user can optionally set a flag "on_location_only" which will change
      // tolerance of writers only on that server
      if (combined_control_request_.generic_combined_control_msg_.tolerance_msg_.on_location_only_) {
        char hostname[64];
        hostname[63] = '\0';
        gethostname(hostname, 63);

        if (strcmp(hostname, combined_control_request_.location_) != 0) {
          return;
        }
      }
      HFSAT::ExchSource_t exch_src_ = combined_control_request_.generic_combined_control_msg_.tolerance_msg_.exch_src_;
      int tolerance_ = combined_control_request_.generic_combined_control_msg_.tolerance_msg_.tolerance_;

      std::cout << HFSAT::ExchSourceStringForm(exch_src_) << " " << tolerance_ << "\n";

      if (exch_to_mds_settings_map_.find(exch_src_) != exch_to_mds_settings_map_.end()) {
        exch_to_mds_settings_map_[exch_src_].setExplicitTolerance(tolerance_);
      } else {
        std::cout << HFSAT::ExchSourceStringForm(exch_src_) << " not added as exchange \n";
      }

      cout.flush();
    } else if (combined_control_request_.message_code_ == HFSAT::kCmdControlMessageCodeDumpMdsFiles) {
      bool only_ors_files =
          combined_control_request_.generic_combined_control_msg_.dump_mds_files_.dump_only_ors_files_;
      DumpFiles(only_ors_files);
    } else {
      // TODO : use this code at dump live orders listener
      //        std::cout << combined_control_request_.ToString() << "\n";
      //
      //        char hostname[64];
      //        hostname[63] = '\0';
      //        gethostname(hostname, 63);
      //
      //        if ( strcmp( hostname, combined_control_request_.location_) == 0
      //        )
      //          {
      //            std::cout << "Same\n";
      //          }
    }
  }

  /*
   * Takes an optional parameter ( optional to allow backward compatibilty for
   * mktDD) _list_of_exch_sources_, which is only set
   * when using a combined source ( remains NULL for mktDD ). Its used to
   * initialize/use all the variables per exchange and not common.
   */
 public:
  MDSLogger(std::string exch_, std::set<HFSAT::ExchSource_t>* _list_of_exch_sources_ = NULL)
      : exch(exch_),
        exch_to_notification_file_map_(),
        stop_thread(false),
        data_loggin_dir(LOGGED_DATA_PREFIX + exch_),
        data_loggin_dir2(LOGGED_DATA_PREFIX + exch_ + "_NIFTY"),
        timeout_sec_(600),
        is_liffe_(exch_ == "LIFFE"),
        is_ice_(exch_ == "ICE" || exch_ == "ICE_PL" || exch_ == "ICE_FOD" || exch_ == "ICE_CF"),
        is_ose_(exch_ == "OSE" || exch_ == "OSE_L1" || exch_ == "OSEPriceFeed"),
        is_cme_(exch_ == "CME"),
        is_ebs_(exch_ == "EBS"),
        is_generic_(exch_ == "GENERIC"),
        is_bardata_(exch_ == "BARDATA"),
        yyyymmdd_(0),
        should_affine_(false) {
    if (exch_ == "OSE" || exch_ == "OSE_L1" || exch_ == "OSEPriceFeed") {
      // hack to avoid frequent alerts from OSE, mds config changes doesn't
      // apply here, starts on previous day
      timeout_sec_ = 1200;
    }

    if (is_bardata_) {
      data_loggin_dir_opt = LOGGED_DATA_PREFIX + exch_ + "_OPT";
      data_loggin_dir_opt_w = LOGGED_DATA_PREFIX + exch_ + "_OPT_W";
      std::cout << "MDSLogger: " << data_loggin_dir << "\n" 
                << data_loggin_dir_opt << "\n" 
                << data_loggin_dir_opt_w << std::endl;
    }
    //    if (!is_generic_) {
    exch_to_notification_file_map_[HFSAT::StringToExchSource(exch)] = new std::ofstream();
    std::string notif_filename = std::string("/spare/local/MDSlogs/zabbix/") + exch;
    HFSAT::FileUtils::MkdirEnclosing(notif_filename);
    exch_to_notification_file_map_[HFSAT::StringToExchSource(exch)]->open(notif_filename.c_str(), std::ofstream::out);

    if (!exch_to_notification_file_map_[HFSAT::StringToExchSource(exch)]->is_open()) {
      std::cerr << "Failed To Open Notification File : " << notif_filename << std::endl;
      exit(-1);
    }
    //    }

    gettimeofday(&tv_last_data, NULL);  // initialize time

    // Set the date
    time_t m_time_t;
    time(&m_time_t);
    struct tm m_tm;
    localtime_r(&m_time_t, &m_tm);
    yyyymmdd_ = (1900 + m_tm.tm_year) * 10000 + (1 + m_tm.tm_mon) * 100 + m_tm.tm_mday;

    if (is_ose_) {
      if (m_tm.tm_hour > 18) {
        yyyymmdd_ = HFSAT::DateTime::CalcNextDay(yyyymmdd_);
      }
    }
  }

  ~MDSLogger() { closeFiles(); }

  void EnableAffinity(std::string name) {
    should_affine_ = true;
    name_ = name;
  }

  void AddExchSources(std::set<HFSAT::ExchSource_t>* exch_sources) {
    if (is_generic_ && exch_sources != NULL) {
      // Initialize info for all the exchanges this combined source is looking
      // at

      // Setting up map values for invalid exchange (it is accessed for GENERIC)
      HFSAT::ExchSource_t invalid = HFSAT::kExchSourceInvalid;
      gettimeofday(&exch_to_last_data_map_[invalid], NULL);
      exch_to_silent_period_map_[invalid] = false;
      exch_to_mds_settings_map_[invalid] = HFSAT::MDSLoggerSettings();
      exch_to_mds_settings_map_[invalid].init(MDS_LOGGER_CONFIG_FILE, "GENERIC");
      exch_to_timeout_map_[invalid] = 180;
      exch_to_notification_file_map_[invalid] = new std::ofstream();
      std::string notif_filename = std::string("/spare/local/MDSlogs/zabbix/") + "GENERIC_GENERIC";
      HFSAT::FileUtils::MkdirEnclosing(notif_filename);
      exch_to_notification_file_map_[invalid]->open(notif_filename.c_str(), std::ofstream::out);

      for (auto exch_src_ : *(exch_sources)) {
        if (exch_src_ == HFSAT::kExchSourceMICEX_CR || exch_src_ == HFSAT::kExchSourceMICEX_EQ)
          exch_src_ = HFSAT::kExchSourceMICEX;

        if (exch_src_ == HFSAT::kExchSourceBMF) exch_src_ = HFSAT::kExchSourceNTP;

        if (exch_src_ == HFSAT::kExchSourceHONGKONG) exch_src_ = HFSAT::kExchSourceHKOMDPF;

        gettimeofday(&exch_to_last_data_map_[exch_src_], NULL);
        exch_to_silent_period_map_[exch_src_] = false;
        exch_to_mds_settings_map_[exch_src_] = HFSAT::MDSLoggerSettings();
        exch_to_mds_settings_map_[exch_src_].init(MDS_LOGGER_CONFIG_FILE, HFSAT::ExchSourceStringForm(exch_src_));
        exch_to_timeout_map_[exch_src_] = 180;
        exch_to_notification_file_map_[exch_src_] = new std::ofstream();

        std::string notif_filename =
            std::string("/spare/local/MDSlogs/zabbix/") + "GENERIC_" + HFSAT::ExchSourceStringForm(exch_src_);
        HFSAT::FileUtils::MkdirEnclosing(notif_filename);
        exch_to_notification_file_map_[exch_src_]->open(notif_filename.c_str(), std::ofstream::out);

        if (!exch_to_notification_file_map_[exch_src_]->is_open()) {
          std::cerr << "Failed To Open Notification File : " << notif_filename << std::endl;
          exit(-1);
        }
      }
    }
  }

  void set_log_dir(const std::string& new_dir) { data_loggin_dir = new_dir; data_loggin_dir2 = new_dir; }

  void DumpFiles(bool only_ors_files) {
    for (auto inst_file_pair : inst_file_map_) {
      if (strncmp(inst_file_pair.first.c_str(), "ORS_", 4) != 0 && only_ors_files) {
        continue;
      }
      if (inst_file_pair.second != NULL) {
        inst_file_pair.second->DumpCurrentBuffer();
      } else {
        std::cerr << "MDSLogger: DumpFiles: Writer null for product: " << inst_file_pair.first << std::endl;
      }
    }
  }

  void closeFiles() {
    stop_thread = true;
    // Close all instrument files, now using BulkFileWriter.
    for (std::map<std::string, HFSAT::BulkFileWriter*>::iterator itr_ = inst_file_map_.begin();
         itr_ != inst_file_map_.end(); ++itr_) {
      std::cout << " Closing File : " << (itr_->first) << "\n";
      if (itr_->second != NULL) {
        (itr_->second)->Close();
        delete itr_->second;
      }
    }
    inst_file_map_.clear();

    for (auto& notif_itr : exch_to_notification_file_map_) {
      if (NULL != notif_itr.second) {
        notif_itr.second->close();
        delete notif_itr.second;
        notif_itr.second = NULL;
      }
    }
  }

  // dont pass by reference. pass by value since the original struct is likely
  // to get corruptred.
  // note that logging is done in a separate thread. hence pass by value
  void log(T msg) {
    uint32_t cnt = 0;
    while (queue.isFull()) {
      ++cnt;
    }
    if (cnt > 0){
      std::cerr << "queue size inefficient, consider increasing size. (times "
                   "inside while loop: "
                << cnt << " Queue size : "<< queue.Size() << "\n";
    }
    queue.push(msg);
  }

  void logBardata(std::unordered_map<std::string,T*> shortcode_to_bardata_map_) {
    uint32_t cnt = 0;
    while (queue.isFull()) {
      ++cnt;
    }
    if (cnt > 0)
      std::cerr << "queue size inefficient, consider increasing size. (times "
                   "inside while loop: "
                << cnt << " Queue size : "<< queue.Size() << "\n";
    for (auto map_itr : shortcode_to_bardata_map_) {
      queue.push(*(map_itr.second));
    }

    gettimeofday(&tv_last_data, NULL);
  }

  void SetDate(int yyyymmdd) {
    if (yyyymmdd_ == yyyymmdd) {
      return;
    }

    // sleep for some time to give logger thread to finish the queue
    sleep(10);

    yyyymmdd_ = 0;

    if (!queue.isEmpty()) {
      std::cerr << "MDSLogger: queue still not empty."
                << " size: " << queue.Size() << " prev_date: " << yyyymmdd_ << " new_date: " << yyyymmdd << std::endl;
    }

    for (auto pair_ : inst_file_map_) {
      std::cout << "Closing File: " << pair_.first << std::endl;
      pair_.second->Close();
      delete pair_.second;
    }

    inst_file_map_.clear();
    yyyymmdd_ = yyyymmdd;
  }

  int GetDate() { return yyyymmdd_; }

  bool isRunning() { return !stop_thread; }

  void stop() { closeFiles(); }

  void thread_main(void) {
    std::cout << "THREAD_MAIN" << std::endl;
    if (!should_affine_) {
      AffinInitCores();
    } else {
      setName(name_.c_str());
      AllocateCPUOrExit();
    }

    mds_logger_settings_.init(MDS_LOGGER_CONFIG_FILE, exch);
    timeout_sec_ = mds_logger_settings_.getCurrentTolerance();

    int msg_len = sizeof(T);
    T cstr_;

    while (!stop_thread) {

      while (!queue.isEmpty()) {
        if (queue.pop(cstr_) == false) continue;
        std::string amr_code_filename_;
        const char* inst (is_bardata_ ? cstr_.getShortcode() : cstr_.getContract());
        if (strstr(inst, "INVALID")) continue;

        if (inst == NULL) {
          std::cerr << " NULL Instrument For : " << cstr_.ToString() << std::endl;
          continue;
        }

        if (is_liffe_ || is_generic_ || is_ice_) {
          amr_code_filename_ = inst;
          std::replace(amr_code_filename_.begin(), amr_code_filename_.end(), ' ', '~');

          inst = amr_code_filename_.c_str();
        }

        std::string quincy_added_name_;

        if (is_generic_) {
          if (((HFSAT::MDS_MSG::GenericMDSMessage*)(&cstr_))->IsQuincyFeed()) {
            quincy_added_name_ = std::string("QUI_") + std::string(inst);
            inst = quincy_added_name_.c_str();
          }

          amr_code_filename_ = inst;
          std::replace(amr_code_filename_.begin(), amr_code_filename_.end(), '/', '~');
          inst = amr_code_filename_.c_str();
        }

        if (is_ebs_) {
          amr_code_filename_ = inst;
          std::replace(amr_code_filename_.begin(), amr_code_filename_.end(), '/', '~');

          inst = amr_code_filename_.c_str();
        }

        if (inst == NULL) {
          fprintf(stderr, "Weird message in Logger::ProcessMsg\n");
          fprintf(stderr, cstr_.ToString().c_str());
        }

        if (inst_file_map_.find(inst) == inst_file_map_.end()) {
          AddToMap(inst);
        }

        HFSAT::BulkFileWriter* my_file(NULL);

        if ( exch == "BARDATA" ) {
          std::string symbol, file_key, shortcode;
          std::vector<char*> tokens_;
          char readline_buffer_copy_[1024];
          memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
          strcpy(readline_buffer_copy_, inst);
          HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, "_", tokens_);
          shortcode = inst;
          symbol = tokens_[1];
          uint16_t token_size = tokens_.size();
          if ( shortcode.find("_FUT") != std::string::npos ) {
            file_key = symbol + "_FUT";
          } else if ( std::string(tokens_[token_size - 1] ) == "W" ) {
            file_key = symbol + "_W";
          } else {
            file_key = symbol;
          }
          my_file = inst_file_map_[file_key];
        } else {
          my_file = inst_file_map_[inst];
        }
        if (my_file == NULL) continue;
        my_file->Write(&cstr_, msg_len);
        if ( exch == "BARDATA" ) {
          my_file->DumpCurrentBuffer();
        } else {
          my_file->CheckToFlushBuffer();
        }
      }
    }
  }
};
