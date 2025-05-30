// =====================================================================================
//
//       Filename:  sim_trade_filesource.cpp
//
//    Description:
//
//        Version:  1.0
//        Cre  03/18/2016 10:31:11 AM
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
#include "midterm/MidTerm/sim_trade_filesource.hpp"

namespace HFSAT {

void SimTradeFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_,
                                               bool &rw_hasevents_) {
  // std::cout << "Seeking.." << std::endl;
  // dbglogger_ << "Seeking..\n";
  if (file_reader_.good()) {
    // std::cout << "Seeking.." << std::endl;
    // dbglogger_ << "Seeking..\n";
    std::string line;
    do {
      std::getline(file_reader_, line);
      std::vector<std::string> tokens_;
      HFSAT::PerishableStringTokenizer::StringSplit(line, '|', tokens_);
      next_event_timestamp_.tv_sec = std::atoi(tokens_[3].c_str());
      next_event_ = line;
    } while (next_event_timestamp_ < r_start_time_);
  } else {
    rw_hasevents_ = false;
  }
}

inline bool SimTradeFileSource::SetNextTimeStamp() {
  if (file_reader_.good()) {
    std::string line;
    std::getline(file_reader_, line);
    std::vector<std::string> tokens_;
    HFSAT::PerishableStringTokenizer::StringSplit(line, '|', tokens_);
    next_event_timestamp_.tv_sec = std::atoi(tokens_[3].c_str());
    next_event_ = line;
    // dbglogger_ << __func__ << " " << next_event_timestamp_.tv_sec << "\n";
    return true;
  } else {
    next_event_timestamp_ = ttime_t(
        time_t(0),
        0); // to indicate to calling process that we don't have any more data
    return false;
  }
}

void SimTradeFileSource::ComputeEarliestDataTimestamp(bool &rw_hasevents_) {
  if (file_reader_.good()) {
    std::string line;
    std::getline(file_reader_, line);
    std::vector<std::string> tokens_;
    HFSAT::PerishableStringTokenizer::StringSplit(line, '|', tokens_);
    next_event_timestamp_.tv_sec = std::atoi(tokens_[3].c_str());
    // dbglogger_ << __func__ << " " << next_event_timestamp_.tv_sec << "\n";
    next_event_ = line;
  } else {
    next_event_timestamp_ = ttime_t(
        time_t(0),
        0); // to indicate to calling process that we don't have any more data
    rw_hasevents_ = false;
  }
}

void SimTradeFileSource::ProcessAllEvents() {
  while (1) {
    ProcessThisEvent();
    if (!SetNextTimeStamp())
      return;
  }
}

void SimTradeFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  while (next_event_timestamp_ <= _endtime_) {
    // std::cout << _endtime_ << '\t' << __func__ << std::endl;
    ProcessThisEvent();
    if (!SetNextTimeStamp())
      return;
  }
}

void SimTradeFileSource::AddListener(
    HFSAT::Utils::TCPServerSocketListener *listener) {
  HFSAT::VectorUtils::UniqueVectorAdd(order_listeners, listener);
}

void SimTradeFileSource::ProcessThisEvent() {
  // first alert Watch -- now removed
  // dbglogger_ << next_event_timestamp_ << '\t' << __func__ << "\n";
  p_time_keeper_->OnTimeReceived(next_event_timestamp_);
  NotifyListeners();
}

void SimTradeFileSource::NotifyListeners() {
  for (auto listener : order_listeners) {
    int32_t len_ = next_event_.length();
    // dbglogger_ << __func__ << " " << next_event_ << "\n";
    char *buf_ = new char[len_ + 1];
    strcpy(buf_, next_event_.c_str());
    listener->OnClientRequest(-1, buf_, len_);
  }
}
}
