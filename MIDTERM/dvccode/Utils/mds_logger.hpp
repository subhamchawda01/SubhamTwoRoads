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
  HFSAT::MDSLoggerSettings mds_logger_settings_;
  int timeout_sec_;
  bool is_liffe_;
  bool is_ice_;
  bool is_ose_;
  bool is_cme_;
  bool is_ebs_;
  bool is_generic_;
  int yyyymmdd_;
  bool should_affine_;
  std::string name_;

 private:
  void AddToMap(std::string inst) {
    char date_str[9] = {'\0'};
    sprintf(date_str, "%d", yyyymmdd_);
    std::string fname = data_loggin_dir + "/" + inst + "_" + date_str;  // Save into corresponding exchange directory

    // 4 KB - most files are atleast this much.
    HFSAT::BulkFileWriter* new_file_ = new HFSAT::BulkFileWriter(
        fname.c_str(), 4 * 1024, std::ofstream::binary | std::ofstream::app | std::ofstream::ate);

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
        timeout_sec_(600),
        is_liffe_(exch_ == "LIFFE"),
        is_ice_(exch_ == "ICE" || exch_ == "ICE_PL" || exch_ == "ICE_FOD" || exch_ == "ICE_CF"),
        is_ose_(exch_ == "OSE" || exch_ == "OSE_L1" || exch_ == "OSEPriceFeed"),
        is_cme_(exch_ == "CME"),
        is_ebs_(exch_ == "EBS"),
        is_generic_(exch_ == "GENERIC"),
        yyyymmdd_(0),
        should_affine_(false) {
    if (exch_ == "OSE" || exch_ == "OSE_L1" || exch_ == "OSEPriceFeed") {
      // hack to avoid frequent alerts from OSE, mds config changes doesn't
      // apply here, starts on previous day
      timeout_sec_ = 1200;
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

  void set_log_dir(const std::string& new_dir) { data_loggin_dir = new_dir; }

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
    if (cnt > 0)
      std::cerr << "queue size inefficient, consider increasing size. (times "
                   "inside while loop: "
                << cnt << ")\n";

    queue.push(msg);
    gettimeofday(&tv_last_data, NULL);

    if (is_generic_) {
      HFSAT::ExchSource_t msg_exch_src_ =
          ((HFSAT::MDS_MSG::GenericMDSMessage*)(&msg))->GetExchangeSourceFromGenericStruct();

      // This check is to ensure that its a real exchange and not a
      // CONTROL/ORS_REPLY message
      if (exch_to_last_data_map_.find(msg_exch_src_) != exch_to_last_data_map_.end()) {
        exch_to_last_data_map_[msg_exch_src_] = tv_last_data;

        if (exch_to_silent_period_map_[msg_exch_src_] == true) {
          (*exch_to_notification_file_map_[msg_exch_src_])
              << "RECOVERY : " << HFSAT::ExchSourceStringForm(msg_exch_src_) << " -- GENERIC Started Receiving Data Now"
              << std::endl;
          exch_to_notification_file_map_[msg_exch_src_]->flush();

          exch_to_silent_period_map_[msg_exch_src_] = false;
          exch_to_timeout_map_[msg_exch_src_] = exch_to_mds_settings_map_[msg_exch_src_].getCurrentTolerance();
        }
      }
    }
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
    if (!should_affine_) {
      AffinInitCores();
    } else {
      setName(name_.c_str());
      AllocateCPUOrExit();
    }

    mds_logger_settings_.init(MDS_LOGGER_CONFIG_FILE, exch);
    timeout_sec_ = mds_logger_settings_.getCurrentTolerance();

    bool silentPeriod = false;
    int msg_len = sizeof(T);
    T cstr_;

    while (!stop_thread) {
      while (queue.isEmpty() && !stop_thread) {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 1;
        select(0, NULL, NULL, NULL, &tv);  // sleep for 1 micro

        gettimeofday(&current_time, NULL);

        timeout_sec_ = std::max(timeout_sec_, mds_logger_settings_.getCurrentTolerance());

        if (current_time.tv_sec - tv_last_data.tv_sec > timeout_sec_) {
          (*exch_to_notification_file_map_[HFSAT::StringToExchSource(exch)])
              << "ALERT : " << exch << " -- Logger Not Receiving Data For Last " << timeout_sec_ << " Seconds"
              << std::endl;
          exch_to_notification_file_map_[HFSAT::StringToExchSource(exch)]->flush();

          timeout_sec_ = std::min(timeout_sec_ * 2, 30 * 60 /* 30 min */);
          gettimeofday(&tv_last_data, NULL);

          silentPeriod = true;
          continue;
        }
      }

      /*
       * For every exchange this combined source is looking at, check whether
       * the difference between curr_time and last data received for that
       * exchange is greater than timeout , send an alert and set the silent
       * period to be true and send alert mail. When the data is received again
       * in
       * log () function for this exchange , set silent period false and send
       * recovery mail. We have mds_settings object per exchange, which tracks
       * the
       * tolerance value of that particular exchange.
       */
      if (is_generic_) {
        for (std::map<HFSAT::ExchSource_t, struct timeval>::iterator iter = exch_to_last_data_map_.begin();
             iter != exch_to_last_data_map_.end(); ++iter) {
          struct timeval tv;
          tv.tv_sec = 0;
          tv.tv_usec = 1;
          select(0, NULL, NULL, NULL, &tv);  // sleep for 1 micro

          gettimeofday(&current_time, NULL);

          HFSAT::ExchSource_t exch_src_ = iter->first;

          int time_out_ =
              std::max(exch_to_timeout_map_[exch_src_], exch_to_mds_settings_map_[exch_src_].getCurrentTolerance());

          if (current_time.tv_sec - iter->second.tv_sec > time_out_ && iter->second.tv_sec != 0) {
            if (exch_to_notification_file_map_[exch_src_] == NULL) continue;
            (*exch_to_notification_file_map_[exch_src_]) << "ALERT : " << HFSAT::ExchSourceStringForm(exch_src_)
                                                         << " -- GENERIC Not Receiving Data For Last "
                                                         << exch_to_timeout_map_[exch_src_] << " Seconds" << std::endl;
            exch_to_notification_file_map_[exch_src_]->flush();

            exch_to_timeout_map_[exch_src_] = std::min(time_out_ * 2, 30 * 60 /* 30 min */);
            gettimeofday(&(iter->second), NULL);

            exch_to_silent_period_map_[exch_src_] = true;
          }
        }
      }
      // we received some message, send out the recovery notifications
      if (silentPeriod && exch_to_notification_file_map_[HFSAT::StringToExchSource(exch)] != NULL) {
        (*exch_to_notification_file_map_[HFSAT::StringToExchSource(exch)])
            << "RECOVERY : " << exch << " -- Logger Started Receiving Data Now" << std::endl;
        exch_to_notification_file_map_[HFSAT::StringToExchSource(exch)]->flush();
        silentPeriod = false;
      }

      // finally we dump the messages into files
      while (!queue.isEmpty()) {
        if (queue.pop(cstr_) == false) continue;
        std::string amr_code_filename_;
        const char* inst = cstr_.getContract();

        if (inst == NULL) {
          std::cerr << " NULL Instrument For : " << cstr_.ToString() << "\n";
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

        // Wait for the correct date to be set from SetDate function
        // which would be running from a different thread
        if (yyyymmdd_ == 0) {
          usleep(500000);

          while (yyyymmdd_ == 0) {
            std::cerr << "MDSLogger: ERROR: Date is still 0." << std::endl;
            sleep(1);
          }
        }

        if (inst_file_map_.find(inst) == inst_file_map_.end()) {
          AddToMap(inst);
        }

        HFSAT::BulkFileWriter* my_file = inst_file_map_[inst];
        if (my_file == NULL) continue;
        my_file->Write(&cstr_, msg_len);
        my_file->CheckToFlushBuffer();
      }
    }
  }
};
