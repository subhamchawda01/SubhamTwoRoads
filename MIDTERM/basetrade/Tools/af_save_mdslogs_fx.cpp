// =====================================================================================
//
//       Filename:  af_save_mdslogs.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Thursday 28 May 2015 04:06:41  IST
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

#include <iostream>
#include <sstream>
#include <locale>
#include <iomanip>
#include <string>
#include <map>
#include <time.h>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/af_xmlspecs.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

std::map<int, AFLASH_MDS::AFlashCommonStruct *> packet_map_;
std::map<std::string, int> name2cat_map_;
std::map<std::string, int> name2fid_map_;
std::map<std::string, int> name2ind_map_;
std::map<std::string, double> name2scale_map_;
std::map<int, float> id2latency_map_;
std::vector<std::pair<time_t, int> > msg_time_pairs_;
std::string yyyy_;
std::string mm_;
std::string dd_;

void fill_latency(std::string fname_);
void fill_cat_names(std::string fname_);
void read_fx_file(std::string fname_);
void fill_packet(std::string ev_name_, time_t unixtime_, std::string val_str_);
void write_mdslog();
void read_and_Fill(std::string date_);

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

void fill_latency(std::string fname_) {
  std::ifstream fx_read_;
  fx_read_.open(fname_, std::ifstream::in);

  if (!fx_read_.is_open()) {
    std::cerr << "File: " << fname_ << " does NOT exist" << std::endl;
    return;
  }

  char line_buffer_[1024];
  std::string line_read_ = "";

  while (fx_read_.good()) {
    memset(line_buffer_, 0, 1024);
    line_read_ = "";

    fx_read_.getline(line_buffer_, 1024);
    line_read_ = line_buffer_;
    if (line_read_.find("#") != std::string::npos) continue;  // comments

    std::vector<std::string> tokens_;
    split(line_read_, ':', tokens_);
    if (tokens_.size() >= 2) {
      id2latency_map_[stoi(tokens_[0])] = stof(tokens_[1]);
    }
  }
}

void fill_cat_names(std::string fname_) {
  std::ifstream fx_read_;
  fx_read_.open(fname_, std::ifstream::in);

  if (!fx_read_.is_open()) {
    std::cerr << "File: " << fname_ << " does NOT exist" << std::endl;
    return;
  }

  char line_buffer_[1024];
  std::string line_read_ = "";

  while (fx_read_.good()) {
    memset(line_buffer_, 0, 1024);
    line_read_ = "";

    fx_read_.getline(line_buffer_, 1024);
    line_read_ = line_buffer_;
    if (line_read_.find("#") != std::string::npos) continue;  // comments

    std::vector<std::string> tokens_;
    split(line_read_, ',', tokens_);
    if (tokens_.size() < 2) continue;

    int cat_id_ = atoi(tokens_[0].c_str());

    for (unsigned i = 1; i < tokens_.size(); i++) {
      if (tokens_[i] == "") continue;
      std::vector<std::string> t_tokens_;
      split(tokens_[i], ':', t_tokens_);
      if (t_tokens_.size() < 2) continue;
      name2cat_map_[t_tokens_[1]] = cat_id_;
      name2fid_map_[t_tokens_[1]] = stoi(t_tokens_[0]);
      name2ind_map_[t_tokens_[1]] = (int)(i - 1);
      if (t_tokens_.size() > 2) {
        name2scale_map_[t_tokens_[1]] = stod(t_tokens_[2]);
      }
    }
  }
}

void read_fx_file(std::string fname_) {
  std::ifstream fx_read_;
  fx_read_.open(fname_, std::ifstream::in);

  if (!fx_read_.is_open()) {
    std::cerr << "File: " << fname_ << " does NOT exist" << std::endl;
    return;
  }

  std::string date_str_ = mm_ + "/" + dd_ + "/" + yyyy_;
  char line_buffer_[1024];
  std::string line_read_ = "";

  while (fx_read_.good()) {
    memset(line_buffer_, 0, 1024);
    line_read_ = "";

    fx_read_.getline(line_buffer_, 1024);
    line_read_ = line_buffer_;

    if (line_read_.find("#") != std::string::npos) continue;  // comments

    std::vector<std::string> tokens_;
    split(line_read_, ',', tokens_);
    if (tokens_.size() < 5) continue;

    std::vector<std::string> datetime_;
    split(tokens_[0], ' ', datetime_);
    if (datetime_[0] != date_str_) {
      continue;
    }

    std::string &ev_name_ = tokens_[1];
    if (name2cat_map_.find(ev_name_) == name2cat_map_.end()) {
      continue;
    }

    std::tm t_;
    if (!strptime(tokens_[0].c_str(), "%m/%d/%Y %H:%M:%S", &t_)) {
      std::cerr << " The Time for " << tokens_[0] << " could NOT be parsed" << std::endl;
    }
    fill_packet(ev_name_, mktime(&t_), tokens_[4]);
  }
}

void read_bbg_file(std::string fname_) {
  std::ifstream fx_read_;
  fx_read_.open(fname_, std::ifstream::in);

  if (!fx_read_.is_open()) {
    std::cerr << "File: " << fname_ << " does NOT exist" << std::endl;
    return;
  }

  std::string date_str_ = yyyy_ + mm_ + dd_;
  char line_buffer_[1024];
  std::string line_read_ = "";

  while (fx_read_.good()) {
    memset(line_buffer_, 0, 1024);
    line_read_ = "";

    fx_read_.getline(line_buffer_, 1024);
    line_read_ = line_buffer_;

    if (line_read_.find("#") != std::string::npos) continue;  // comments

    std::vector<std::string> tokens_;
    split(line_read_, ',', tokens_);
    if (tokens_.size() < 7 || tokens_[0] != date_str_) continue;

    time_t unix_time_ = HFSAT::DateTime::GetTimeFromTZHHMM(stoi(date_str_), stoi(tokens_[1]), "EST");

    std::string &ev_name_ = tokens_[3];
    if (name2cat_map_.find(ev_name_) != name2cat_map_.end()) {
      fill_packet(ev_name_, unix_time_, tokens_[6]);
    }
    std::string ev_rev_name_ = tokens_[3] + " Revised";
    if (name2cat_map_.find(ev_rev_name_) != name2cat_map_.end() && tokens_.size() > 8) {
      fill_packet(ev_rev_name_, unix_time_, tokens_[8]);
    }
  }
}

void fill_packet(std::string ev_name_, time_t unixtime_, std::string val_str_) {
  AFLASH_MDS::AFlashCommonStruct *this_packet_;
  int cat_id_ = name2cat_map_[ev_name_];
  int fid_ = name2fid_map_[ev_name_];
  int scale_ = 1;
  if (name2scale_map_.find(ev_name_) != name2scale_map_.end()) {
    scale_ = name2scale_map_[ev_name_];
  }
  val_str_.erase(std::remove(val_str_.begin(), val_str_.end(), '%'), val_str_.end());

  AF_MSGSPECS::AF_MsgParser &af_msgparser_ = AF_MSGSPECS::AF_MsgParser::GetUniqueInstance();
  AF_MSGSPECS::Category *catg_ = af_msgparser_.getCategoryforId(cat_id_);
  if (catg_ == NULL) {
    std::cerr << " No Category for Id: " << cat_id_ << std::endl;
    return;
  }
  AF_MSGSPECS::Message *msg_ = af_msgparser_.getMsgFromCatg(catg_, 1);
  if (msg_ == NULL) {
    std::cerr << "No release msg for Id: " << cat_id_ << std::endl;
    return;
  }

  if (packet_map_.find(cat_id_) == packet_map_.end()) {
    this_packet_ = new AFLASH_MDS::AFlashCommonStruct();
    /* Adding latency to this message */
    if (id2latency_map_.find(cat_id_) == id2latency_map_.end()) {
      id2latency_map_[cat_id_] = 0;
    }
    int latency_secs_ = (int)id2latency_map_[cat_id_];
    this_packet_->time_.tv_sec = unixtime_ + latency_secs_;
    this_packet_->time_.tv_usec = (id2latency_map_[cat_id_] - latency_secs_) * 1000000l;
    std::cout << "Time: " << this_packet_->time_.tv_sec << ", " << this_packet_->time_.tv_usec << std::endl;
    this_packet_->uid_ = 0;  // any random unique id
    this_packet_->type_ = (uint8_t)AF_MSGSPECS::kRelease;
    this_packet_->version_ = msg_->msg_version_;
    this_packet_->category_id_ = (uint16_t)cat_id_;
    this_packet_->nfields_ = 0;
    packet_map_[cat_id_] = this_packet_;
    msg_time_pairs_.push_back(std::make_pair(unixtime_, cat_id_));
  } else {
    this_packet_ = packet_map_[cat_id_];
  }
  int pind_ = this_packet_->nfields_;

  AF_MSGSPECS::Field *t_field_ = af_msgparser_.getFieldFromMsg(msg_, fid_);

  this_packet_->fields[pind_].field_id_ = (uint8_t)fid_;
  this_packet_->nfields_ = std::max(this_packet_->nfields_, (uint8_t)(pind_ + 1));
  switch (t_field_->field_type_) {
    case AF_MSGSPECS::kShort_value_enumeration:
    case AF_MSGSPECS::kShort:
    case AF_MSGSPECS::kInt:
    case AF_MSGSPECS::kLong: {
      this_packet_->fields[pind_].data_.vInt = (long int)(scale_ * stod(val_str_));
      break;
    }
    case AF_MSGSPECS::kDouble:
    case AF_MSGSPECS::kFloat: {
      this_packet_->fields[pind_].data_.vFloat = scale_ * stod(val_str_);
      break;
    }
    default: {
      std::cerr << "The Value for this msg NOT INTERPRETABLE" << std::endl;
      this_packet_->fields[pind_].data_.vInt = 0;
    }
  }
  // std::cout << this_packet_->ToString ( ) << std::endl;
  return;
}

void write_mdslog(std::string data_loggin_dir_pref_) {
  std::string exch_ = "AFLASH";
  std::string inst_ = "AFL";
  std::string data_loggin_dir_ = data_loggin_dir_pref_ + "/" + exch_;
  std::string date_str_ = yyyy_ + mm_ + dd_;

  std::string fname_ = data_loggin_dir_ + "/" + inst_ + "_" + date_str_;  // Save into corresponding exchange directory
  // 4 KB - most files are atleast this much.
  HFSAT::BulkFileWriter *new_file_ = new HFSAT::BulkFileWriter(
      fname_.c_str(), 4 * 1024, std::ofstream::binary | std::ofstream::app | std::ofstream::ate);

  std::sort(msg_time_pairs_.begin(), msg_time_pairs_.end());
  int msg_len = sizeof(AFLASH_MDS::AFlashCommonStruct);

  for (auto it = msg_time_pairs_.begin(); it != msg_time_pairs_.end(); it++) {
    std::pair<time_t, int> t_pair_ = *it;
    AFLASH_MDS::AFlashCommonStruct *this_packet_ = packet_map_[t_pair_.second];
    if (!this_packet_) continue;
    std::cout << this_packet_->ToString() << std::endl;
    new_file_->Write(this_packet_, msg_len);
    new_file_->CheckToFlushBuffer();
    delete this_packet_;
  }
  new_file_->Close();
}

int main(int argc, char **argv) {
  if (argc < 4) {
    std::cout << "USAGE: <FXS/BBG> <dataloggingdir> <date> " << std::endl;
    exit(1);
  }
  std::string ev_opt_ = std::string(argv[1]);
  std::string data_loggin_dir_pref_ = std::string(argv[2]);
  std::string date_ = std::string(argv[3]);

  std::string fx_fname_ = "/home/dvctrader/modelling/alphaflash/fxstreet_us_eventdates.csv";
  std::string bbg_fname_ = "/home/dvctrader/modelling/alphaflash/bbg_us_eco.csv";
  if (!(ev_opt_ == "FXS" || ev_opt_ == "BBG")) {
    std::cerr << "Arg.1 has to be FXS/BBG.. Exiting.." << std::endl;
    exit(1);
  }

  yyyy_ = date_.substr(0, 4);
  mm_ = date_.substr(4, 2);
  dd_ = date_.substr(6, 2);

  const std::string ev_map_fname_fx_ = "/spare/local/tradeinfo/Alphaflash/event_fxstreet_map";
  const std::string ev_map_fname_bbg_ = "/spare/local/tradeinfo/Alphaflash/event_bbg_map";
  const std::string ev_latency_fname_ = "/spare/local/tradeinfo/Alphaflash/ev_latency";
  // const std::string fx_fname_ = "/home/dvctrader/modelling/alphaflash/fxstreet_us_eventdates.csv";
  // const data_loggin_dir_pref_ = ""/home/hagarwal/af_data/eventdates";

  fill_latency(ev_latency_fname_);
  if (ev_opt_ == "FXS") {
    fill_cat_names(ev_map_fname_fx_);
    read_fx_file(fx_fname_);
  } else if (ev_opt_ == "BBG") {
    fill_cat_names(ev_map_fname_bbg_);
    read_bbg_file(bbg_fname_);
  }
  write_mdslog(data_loggin_dir_pref_);
}
