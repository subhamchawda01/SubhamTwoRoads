/**
    \file dvccode/Utils/data_daemon_config.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include "dvccode/Utils/data_daemon_config.hpp"

namespace HFSAT {

DataDaemonConfig *DataDaemonConfig::unique_instance_ = nullptr;

DataDaemonConfig &DataDaemonConfig::GetUniqueInstance(std::string config_) {
  if (unique_instance_ == nullptr) {
    unique_instance_ = new DataDaemonConfig(config_);
  }

  return (*unique_instance_);
}

DataDaemonConfig &DataDaemonConfig::GetUniqueInstance() {
  if (unique_instance_ != nullptr) {
    return (*unique_instance_);
  }
  std::cerr << "DataDaemonConfig::GetUniqueInstance() called. No Instance Created. Exiting." << std::endl;
  exit(1);
}

DataDaemonConfig::DataDaemonConfig(std::string config_) : config_file_(config_), exch_mode_() { ParseConfig(); }

FastMdConsumerMode_t DataDaemonConfig::GetModeFromString(std::string mode_str_) {
  if (mode_str_ == "CombinedWriter") {
    return kComShm;
  } else if (mode_str_ == "Logger") {
    return kLogger;
  } else if (mode_str_ == "ShmWriter") {
    return kProShm;
  } else if (mode_str_ == "Multicast") {
    return kMcast;
  } else if (mode_str_ == "Raw") {
    return kRaw;
  } else if (mode_str_ == "MCastL1") {
    return kMcastL1;
  } else if (mode_str_ == "Reference") {
    return kReference;
  } else if (mode_str_ == "PriceFeedLogger") {
    return kPriceFeedLogger;
  }

  return kLogger;
}

void DataDaemonConfig::AddExchModePair(std::string exchange_, FastMdConsumerMode_t mode_) {
  if (exch_mode_.find(exchange_) == exch_mode_.end()) {
    exch_mode_[exchange_] = std::vector<FastMdConsumerMode_t>();
  }

  VectorUtils::UniqueVectorAdd(exch_mode_[exchange_], mode_);
}

void DataDaemonConfig::ParseConfig() {
  std::ifstream config_stream_;
  config_stream_.open(config_file_.c_str(), std::ifstream::in);

  if (config_stream_.is_open()) {
    const int config_len_ = 1024;
    char readline_buffer_[config_len_];
    bzero(readline_buffer_, config_len_);

    while (config_stream_.good()) {
      bzero(readline_buffer_, config_len_);
      config_stream_.getline(readline_buffer_, config_len_);
      PerishableStringTokenizer st_(readline_buffer_, config_len_);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.empty()) {
        continue;
      }

      if (strcmp(tokens_[0], "Exchange") == 0) {
        std::string exchange_ = tokens_[1];
        FastMdConsumerMode_t mode_ = kComShm;

        if (tokens_.size() >= 3) {
          mode_ = GetModeFromString(tokens_[2]);
        }

        AddExchModePair(exchange_, mode_);
      }
    }

    config_stream_.close();
  } else {
    std::cerr << "ConfigFile: " << config_file_ << std::endl;
    std::cerr << "Exiting." << std::endl;
    exit(-1);
  }
}

bool DataDaemonConfig::ExchModeExists(std::string exchange_, FastMdConsumerMode_t mode_) {
  if (exch_mode_.find(exchange_) == exch_mode_.end()) {
    return false;
  }

  std::vector<FastMdConsumerMode_t> &mode_vec_ = exch_mode_[exchange_];
  for (auto exch_mode_ : mode_vec_) {
    if (exch_mode_ == mode_) {
      return true;
    }
  }

  return false;
}

bool DataDaemonConfig::IsComShm(std::string exchange_) { return ExchModeExists(exchange_, kComShm); }

bool DataDaemonConfig::IsLogger(std::string exchange_) { return ExchModeExists(exchange_, kLogger); }

std::set<std::string> DataDaemonConfig::GetExchSet() {
  std::set<std::string> exch_set_;
  for (auto pair_ : exch_mode_) {
    exch_set_.insert(pair_.first);
  }

  return exch_set_;
}

std::string DataDaemonConfig::GetOperatingMode() {
  for (auto pair_ : exch_mode_) {
    if (IsLogger(pair_.first)) {
      return "HYBRID";
    }
  }

  return "WRITER";
}
}
