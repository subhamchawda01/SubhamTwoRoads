/**
   \file Tools/bandwidth_monitor.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
*/
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <map>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/ttime.hpp"

#define NO_USEC_PER_SEC 1000000.0
#define NO_BITS_PER_MEGA_BIT 1000000.0

// This is not a very accurate way of monitoring bandwidth in our timeframe (microbursts).
// This factor is experimentally determined and used to offset some of these innacuracies.
#define CALIBRATION_FACTOR 100.0

#define PROC_NW_FILE "/proc/net/dev"
#define PROC_NW_BOND_FILE \
  "/proc/net/bonding/bond0"  // hard-coded for now assuming all servers uses only one bonding or not using it

struct interface_summary {
  unsigned long initial_rx_packet_count_;
  unsigned long initial_tx_packet_count_;
  unsigned long initial_rx_usage_count_;
  unsigned long initial_tx_usage_count_;
  unsigned long total_packets_count_;
  unsigned long total_usage_count_;

  bool first_run_;

  interface_summary() {
    initial_rx_packet_count_ = initial_tx_packet_count_ = initial_rx_usage_count_ = initial_tx_usage_count_ =
        total_usage_count_ = total_packets_count_ = 0;
    first_run_ = true;
  }

  void initialize() {
    initial_rx_packet_count_ = initial_tx_packet_count_ = initial_rx_usage_count_ = initial_tx_usage_count_ =
        total_usage_count_ = total_packets_count_ = 0;
    first_run_ = true;
  }
};

void print_usage(char **argv) {
  std::cerr << " This is a simple system bandwidth monitoring tool. makes use of the /proc/net/dev information "
            << std::endl;
  std::cerr << " USAGE : " << argv[0]
            << " <timeout_stats_sec> <max_bw_limit_mbps> <interfaces [i.e bond0 eth4 eth5] [SYSTEM [EXT]]> "
            << std::endl;
  std::cerr << " @Arg interfaces : when given one interface[ i.e eth5 ] can monitor interface independently "
            << std::endl;
  std::cerr << " @Arg interfaces : when given multiple interfaces[ i.e eth5 eth4 ] sums up things, doesn't detect "
               "bonding of interfaces " << std::endl;
  std::cerr << " @Arg interfaces : when given SYSTEM detects all interfaces needed except lo, takes care of bonding"
            << std::endl;
  std::cerr
      << " @Arg interfaces : when given SYSTEM EXT detects all interfaces needed except lo, excludes bonding interfaces"
      << std::endl;
}

void updateStats(std::map<std::string, interface_summary *> &interface_stats_, unsigned long &overall_usage_count_,
                 unsigned long &total_packets_) {
  overall_usage_count_ = total_packets_ = 0;

  std::ifstream in;
  in.open(PROC_NW_FILE, std::ios::in);

  if (!in.is_open()) {
    std::cerr << " Could not open file : " << PROC_NW_FILE << " to read network traffic stats " << std::endl;
    exit(-1);
  }

  while (in.good()) {
    char line_buffer_[1024];
    std::string line_read_;

    memset((void *)&line_buffer_, 0, 1024);
    in.getline(line_buffer_, 1024);
    line_read_ = line_buffer_;

    if (line_read_.find_first_of('|') != std::string::npos) continue;

    for (auto i = 0u; i < strlen(line_buffer_); i++) {
      if (line_buffer_[i] == ':') {
        line_buffer_[i] = ' ';  // separate interface name with rx usage
        break;
      }
    }

    HFSAT::PerishableStringTokenizer st_(line_buffer_, 1024);
    const std::vector<const char *> &tokens_ = st_.GetTokens();

    if (tokens_.size() >= 11) {
      std::string if_name_ = tokens_[0];
      if_name_ = if_name_.substr(0, if_name_.find_first_of(':'));

      if (interface_stats_.find(if_name_) == interface_stats_.end()) continue;

      if (interface_stats_[if_name_]->first_run_) {
        interface_stats_[if_name_]->initialize();
        interface_stats_[if_name_]->initial_rx_packet_count_ = strtoul(tokens_[2], NULL, 0);
        interface_stats_[if_name_]->initial_tx_packet_count_ = strtoul(tokens_[10], NULL, 0);
        interface_stats_[if_name_]->initial_rx_usage_count_ = strtoul(tokens_[1], NULL, 0);
        interface_stats_[if_name_]->initial_tx_usage_count_ = strtoul(tokens_[9], NULL, 0);
        interface_stats_[if_name_]->first_run_ = false;
      } else {
        interface_stats_[if_name_]->total_usage_count_ =
            strtoul(tokens_[1], NULL, 0) - interface_stats_[if_name_]->initial_rx_usage_count_ +
            strtoul(tokens_[9], NULL, 0) - interface_stats_[if_name_]->initial_tx_usage_count_;

        interface_stats_[if_name_]->total_packets_count_ =
            strtoul(tokens_[2], NULL, 0) - interface_stats_[if_name_]->initial_rx_packet_count_ +
            strtoul(tokens_[10], NULL, 0) - interface_stats_[if_name_]->initial_tx_packet_count_;

        interface_stats_[if_name_]->initial_rx_usage_count_ = strtoul(tokens_[1], NULL, 0);
        interface_stats_[if_name_]->initial_tx_usage_count_ = strtoul(tokens_[9], NULL, 0);

        overall_usage_count_ += interface_stats_[if_name_]->total_usage_count_;
        total_packets_ += interface_stats_[if_name_]->total_packets_count_;
      }
    }
  }

  in.close();
}

int main(int argc, char **argv) {
  if (argc < 4) {
    print_usage(argv);
    exit(-1);
  }

  std::map<std::string, interface_summary *> interface_stats_;

  int timeout_stats_secs_ = atoi(argv[1]);
  double max_bw_limit_ = atof(argv[2]);

  // detect what interfaces are needed
  if (!strcmp(argv[3], "SYSTEM")) {
    std::ifstream in;
    std::map<std::string, bool> interfaces_map_;

    // TODO_OPT eliminate interfaces which are down to save a few computations
    in.open(PROC_NW_FILE, std::ios::in);  // read file to get all interfaces

    if (!in.is_open()) {
      std::cerr << " Could not open file : " << PROC_NW_FILE << " to read network traffic stats " << std::endl;
      exit(-1);
    }

    char line_buffer_[1024];
    std::string line_read_;

    while (in.good()) {
      memset((void *)&line_buffer_, 0, 1024);
      in.getline(line_buffer_, 1024);
      line_read_ = line_buffer_;

      if (line_read_.find_first_of('|') != std::string::npos) continue;  // exclude info headers

      for (auto i = 0u; i < strlen(line_buffer_); i++) {
        if (line_buffer_[i] == ':') {
          line_buffer_[i] = ' ';  // separate interface name with rx usage
          break;
        }
      }

      HFSAT::PerishableStringTokenizer st_(line_buffer_, 1024);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() >= 1) {
        std::string if_name_ = tokens_[0];
        if (if_name_ != "lo")  // exclude loopback interface
          interfaces_map_[if_name_] = true;
      }
    }

    in.close();

    in.open(PROC_NW_BOND_FILE, std::ios::in);  // read bonding file to detect backup interfaces

    if (!in.is_open()) {
      std::cerr << " Could not open file : " << PROC_NW_BOND_FILE << " to read interface bonding information "
                << std::endl;
    }

    while (in.good()) {
      memset((void *)&line_buffer_, 0, 1024);
      in.getline(line_buffer_, 1024);
      line_read_ = line_buffer_;

      if (line_read_.find("Slave Interface") == std::string::npos) continue;

      HFSAT::PerishableStringTokenizer st_(line_buffer_, 1024);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() >= 3) {
        std::string if_name_ = tokens_[2];
        std::cerr << " Slave Interface : " << if_name_ << "\n";
        if (interfaces_map_.find(if_name_) != interfaces_map_.end()) interfaces_map_[if_name_] = false;
      }
    }

    in.close();

    if (argc >= 5 && !strcmp(argv[4], "EXT")) {
      if (interfaces_map_.find("bond0") != interfaces_map_.end()) interfaces_map_["bond0"] = false;
    }

    std::map<std::string, bool>::iterator interface_map_iterator_;

    for (interface_map_iterator_ = interfaces_map_.begin(); interface_map_iterator_ != interfaces_map_.end();
         interface_map_iterator_++) {
      if ((interface_map_iterator_->second)) {
        interface_summary *if_stats_ = new interface_summary();
        std::cerr << " Interface Included for Stats : " << (interface_map_iterator_->first) << "\n";
        interface_stats_[(interface_map_iterator_->first)] = if_stats_;
      }
    }

  }

  else {
    for (int interface_counter_ = 3; interface_counter_ < argc; interface_counter_++) {
      interface_summary *if_stats_ = new interface_summary();
      interface_stats_[argv[interface_counter_]] = if_stats_;
    }
  }

  std::cerr << "Time\t\tMax_Bandwidth (Mbps)\t\tAverage_Bandwith (Mbps)\t\tLimit_Exceeded" << std::endl;

  HFSAT::ttime_t last_print_time_(0, 0);

  while (true) {  // main () loop
    HFSAT::ttime_t last_bw_time_(0, 0);
    double max_bandwidth_bps_(0.0);

    unsigned long overall_total_usage_count_(0);
    unsigned long total_packets_count_(0);

    // Reset all interface stats.
    for (std::map<std::string, interface_summary *>::iterator interface_stats_iterator_ = interface_stats_.begin();
         interface_stats_iterator_ != interface_stats_.end(); interface_stats_iterator_++) {
      interface_stats_iterator_->second->initialize();
    }

    unsigned long bw_usage_count_(0);

    while (true) {  // Loop till a second expires or bandwidth exceeds max allowable bandwidth
      unsigned long this_usage_count_(0);
      unsigned long this_total_packets_(0);

      updateStats(interface_stats_, this_usage_count_, this_total_packets_);

      overall_total_usage_count_ += this_usage_count_;
      bw_usage_count_ += this_usage_count_;
      total_packets_count_ += this_total_packets_;

      {  // Compute bandwidth
        HFSAT::ttime_t current_time_ = HFSAT::GetTimeOfDay();

        if (last_bw_time_.tv_sec != 0) {
          HFSAT::ttime_t time_diff_ = current_time_ - last_bw_time_;
          double secs_diff_ = time_diff_.tv_sec + time_diff_.tv_usec / NO_USEC_PER_SEC;

          if (secs_diff_ > 0.010) {  // Atleast 10 msecs.
            double t_max_bandwidth_bps_ = (bw_usage_count_ * 8) / secs_diff_;

            max_bandwidth_bps_ = t_max_bandwidth_bps_ > max_bandwidth_bps_ ? t_max_bandwidth_bps_ : max_bandwidth_bps_;

            last_bw_time_ = current_time_;
            bw_usage_count_ = 0;
          }
        } else {  // last_bw_time_.tv_sec == 0
          last_bw_time_ = current_time_;
          bw_usage_count_ = 0;
        }
      }

      {  // Print if limits exceeded.
        HFSAT::ttime_t current_time_ = HFSAT::GetTimeOfDay();

        if (max_bandwidth_bps_ / (NO_BITS_PER_MEGA_BIT * CALIBRATION_FACTOR) > max_bw_limit_) {
          HFSAT::ttime_t time_diff_ = current_time_ - last_print_time_;
          double secs_diff_ = time_diff_.tv_sec + time_diff_.tv_usec / NO_USEC_PER_SEC;

          double avg_bandwidth_bps_ = (overall_total_usage_count_ * 8) / secs_diff_;

          std::cout << current_time_ << "\t" << (max_bandwidth_bps_ / (NO_BITS_PER_MEGA_BIT * CALIBRATION_FACTOR))
                    << "\t\t" << (avg_bandwidth_bps_ / NO_BITS_PER_MEGA_BIT) << "\t\t"
                    << "\t\t10"
                    << std::endl;  // 10 signifies that this data point corresponds to an exceeded bw scenario.

          last_print_time_ = current_time_;
          break;
        }

        if (last_print_time_.tv_sec != 0) {  // Print on timeout.
          HFSAT::ttime_t time_diff_ = current_time_ - last_print_time_;
          double secs_diff_ = time_diff_.tv_sec + time_diff_.tv_usec / NO_USEC_PER_SEC;

          if (secs_diff_ > timeout_stats_secs_ && max_bandwidth_bps_ > 0.001) {  // Atleast this many seconds.
            double avg_bandwidth_bps_ = (overall_total_usage_count_ * 8) / secs_diff_;

            std::cout
                << current_time_ << "\t" << (max_bandwidth_bps_ / (NO_BITS_PER_MEGA_BIT * CALIBRATION_FACTOR)) << "\t\t"
                << (avg_bandwidth_bps_ / NO_BITS_PER_MEGA_BIT) << "\t\t"
                << "\t\t0"
                << std::endl;  // 0 signifies that this data point does not corresponds to an exceeded bw scenario.

            last_print_time_ = current_time_;
            break;
          }
        } else {  // last_print_time_.tv_sec == 0
          last_print_time_ = current_time_;
        }
      }
    }
  }

  return 0;
}
