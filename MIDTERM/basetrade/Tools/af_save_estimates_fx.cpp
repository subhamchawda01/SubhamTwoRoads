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
//#include <boost/algorithm/string.hpp>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/af_xmlspecs.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

//#define LOGGED_DATA_PREFIX "/spare/local/MDSlogs/"
#define LOGGED_DATA_PREFIX "/home/hagarwal/af_data/eventdates/"

const uint32_t MAX_FIELDS = 4;
struct EstmPacket {
  timeval time_;
  uint16_t category_id_;
  uint8_t nfields_;
  double fields[MAX_FIELDS];
  uint8_t field_ids[MAX_FIELDS];
};

std::map<int, EstmPacket *> packet_map_;
std::map<std::string, int> name2cat_map_;
std::map<std::string, int> name2fid_map_;
std::map<std::string, int> name2ind_map_;
std::map<std::string, int> name2scale_map_;
std::vector<std::pair<time_t, int> > msg_time_pairs_;
std::string yyyy_;
std::string mm_;
std::string dd_;

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
        name2scale_map_[t_tokens_[1]] = stoi(t_tokens_[2]);
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

    if (tokens_[6] == "") {
      continue;
    }

    std::tm t_;
    if (!strptime(tokens_[0].c_str(), "%m/%d/%Y %H:%M:%S", &t_)) {
      std::cerr << " The Time for " << tokens_[0] << " could NOT be parsed" << std::endl;
    }
    fill_packet(ev_name_, mktime(&t_), tokens_[6]);
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
    if (tokens_.size() < 6 || tokens_[0] != date_str_) continue;

    time_t unix_time_ = HFSAT::DateTime::GetTimeFromTZHHMM(stoi(date_str_), stoi(tokens_[1]), "EST");

    std::string &ev_name_ = tokens_[3];
    if (name2cat_map_.find(ev_name_) != name2cat_map_.end()) {
      fill_packet(ev_name_, unix_time_, tokens_[5]);
    }
    std::string ev_rev_name_ = tokens_[3] + " Revised";
    if (name2cat_map_.find(ev_rev_name_) != name2cat_map_.end() && tokens_.size() > 7) {
      fill_packet(ev_rev_name_, unix_time_, tokens_[7]);
    }
  }
}

void fill_packet(std::string ev_name_, time_t unixtime_, std::string val_str_) {
  EstmPacket *this_packet_;
  int cat_id_ = name2cat_map_[ev_name_];
  int fid_ = name2fid_map_[ev_name_];
  int scale_ = 1;
  if (name2scale_map_.find(ev_name_) != name2scale_map_.end()) {
    scale_ = name2scale_map_[ev_name_];
  }
  val_str_.erase(val_str_.find_last_not_of(" \n\r\t") + 1);
  val_str_.erase(std::remove(val_str_.begin(), val_str_.end(), '%'), val_str_.end());

  double val_ = 0;
  try {
    val_ = scale_ * stod(val_str_);
  } catch (const std::invalid_argument &ia) {
    std::cerr << "Invalid Value for Event: " << ev_name_ << ", Cat: " << cat_id_ << ",fid: " << fid_
              << ", val: " << val_str_ << ", " << ia.what() << "\n";
    return;
  } catch (const std::out_of_range &oor) {
    std::cerr << "Out of Range Value for Event: " << ev_name_ << ", Cat: " << cat_id_ << ",fid: " << fid_
              << ", val: " << val_str_ << ", " << oor.what() << '\n';
    return;
  }
  //  int pind_ = name2ind_map_[ ev_name_ ];

  if (packet_map_.find(cat_id_) == packet_map_.end()) {
    this_packet_ = new EstmPacket();
    this_packet_->time_.tv_sec = unixtime_;
    this_packet_->category_id_ = (uint16_t)cat_id_;
    this_packet_->nfields_ = 0;
    packet_map_[cat_id_] = this_packet_;
    msg_time_pairs_.push_back(std::make_pair(unixtime_, cat_id_));
  } else {
    this_packet_ = packet_map_[cat_id_];
  }
  int pind_ = this_packet_->nfields_;
  this_packet_->nfields_++;
  this_packet_->fields[pind_] = val_;
  this_packet_->field_ids[pind_] = (uint8_t)fid_;
  return;
}

void write_estimatelog(std::string data_loggin_dir_) {
  std::string date_str_ = yyyy_ + mm_ + dd_;
  std::string fname_ = data_loggin_dir_ + "/estimates_" + date_str_;

  if ( msg_time_pairs_.size() == 0 ) {
    return;
  }

  std::ifstream estm_read_;
  estm_read_.open(fname_, std::ifstream::in);
  std::map<uint16_t, bool> already_catid_vec_;

  if ( estm_read_.is_open() ) {
    std::string line_;
    const int line_length_ = 1024;
    char readline_buffer_[line_length_];

    while (estm_read_.good()) {
      bzero(readline_buffer_, line_length_);
      estm_read_.getline(readline_buffer_, line_length_);
      line_ = readline_buffer_;

      std::vector<std::string> tokens_;
      split( line_, ' ', tokens_ );

      if (tokens_.size() >= 3u) {
        already_catid_vec_[ (uint16_t)stoi(tokens_[0]) ] = true;
      }
    }
  }
  estm_read_.close();

  std::ofstream estm_write_;
  estm_write_.open(fname_, std::ofstream::out | std::ofstream::app | std::ofstream::ate);

  std::tm t_;
  memset((void*)&t_, 0, sizeof(std::tm));
  if (!strptime(date_str_.c_str(), "%Y%m%d", &t_)) {
    std::cerr << " The Time for " << date_str_ << " could NOT be parsed" << std::endl;
  }
  time_t unixtime_midnight_ = mktime(&t_);

  for (auto it = msg_time_pairs_.begin(); it != msg_time_pairs_.end(); it++) {
    EstmPacket *this_packet_ = packet_map_[it->second];
    if (!this_packet_) continue;

    if ( already_catid_vec_.find( this_packet_->category_id_ ) == already_catid_vec_.end() ) { 
      estm_write_ << this_packet_->category_id_ << " " << (this_packet_->time_.tv_sec - unixtime_midnight_) * 1000;
      for (unsigned indx_ = 0; indx_ < this_packet_->nfields_; indx_++) {
        estm_write_ << " " << (int)this_packet_->field_ids[indx_] << ":" << this_packet_->fields[indx_];
      }
      estm_write_ << "\n";
    }
  }
  estm_write_.close();
}

void read_and_Fill(std::string date_) {}

int main(int argc, char **argv) {
  if (argc < 4) {
    std::cout << "USAGE: <FXS/BBG> <dataloggingdir> <date> " << std::endl;
    exit(1);
  }
  std::string ev_opt_ = std::string(argv[1]);
  std::string data_loggin_dir_ = std::string(argv[2]);
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
  // const std::string fx_fname_ = "/home/dvctrader/modelling/alphaflash/fxstreet_us_eventdates.csv";
  // const data_loggin_dir_pref_ = ""/home/hagarwal/af_data/eventdates";

  if (ev_opt_ == "FXS") {
    fill_cat_names(ev_map_fname_fx_);
    read_fx_file(fx_fname_);
  } else if (ev_opt_ == "BBG") {
    fill_cat_names(ev_map_fname_bbg_);
    read_bbg_file(bbg_fname_);
  }
  write_estimatelog(data_loggin_dir_);
}
