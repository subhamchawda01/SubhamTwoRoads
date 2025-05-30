/**
   \file Tools/mds_logger.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <fstream>
#include <map>
#include <set>
#include "dvccode/CDef/ttime.hpp"
#include <string>
#include "dvccode/CDef/bse_mds_defines.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/sgx_mds_defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/generic_l1_data_struct.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
// data type to store entries for cme_trade order type
typedef std::pair<long int,
                  std::pair<long int, std::pair<uint32_t, std::pair<uint32_t, std::pair<double, uint32_t> > > > >
    TRADE;
// data type to store entries for cme_delta order type
typedef std::pair<
    long int,
    std::pair<
        long int,
        std::pair<
            uint32_t,
            std::pair<int32_t,
                      std::pair<uint32_t,
                                std::pair<double, std::pair<char, std::pair<char, std::pair<char, bool> > > > > > > > >
    DELTA;
std::set<DELTA> hash_delta;
std::set<TRADE> hash_trade;

const boost::gregorian::date_duration one_day_date_duration(1);
int start_month, start_year, start_date, end_month, end_date, end_year;
namespace fs = boost::filesystem;

// functionality to convert int to string
std::string itoa(int num) {
  std::string result = "";

  while (num) {
    result += '0' + (num % 10);
    num /= 10;
  }

  reverse(result.begin(), result.end());
  if (result.size() > 3) return result;

  if (result.size() == 1) {
    return ("0" + result);
  }

  return result;
}

template <class T>
class MDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_,
                             std::vector<std::pair<long int, long int> >& data) {
    T next_event_;
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(T));
        if (available_len_ < sizeof(next_event_)) break;

        data.push_back(std::make_pair(next_event_.time_.tv_sec, next_event_.time_.tv_usec));
      }
      bulk_file_reader_.close();
    }
  }

  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_, std::vector<CME_MDS::CMECommonStruct>& data,
                             std::vector<CME_MDS::CMECommonStruct>& bug) {
    T next_event_, prev_event_;
    bool first = true;
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(T));
        if (available_len_ < sizeof(next_event_)) break;

        if (first) {
          first = false;
          data.push_back(next_event_);
          prev_event_ = next_event_;
        } else {
          if (next_event_.time_.tv_sec > prev_event_.time_.tv_sec ||
              (next_event_.time_.tv_sec == prev_event_.time_.tv_sec &&
               next_event_.time_.tv_usec >= prev_event_.time_.tv_usec)) {
            data.push_back(next_event_);
            prev_event_ = next_event_;
          } else {
            bug.push_back(next_event_);
          }
        }
      }
      bulk_file_reader_.close();
    }
  }
};
// the method builds a hash table out of all the entries in 'data'
void build_hash(std::vector<CME_MDS::CMECommonStruct>& data) {
  for (auto& el : data) {
    if (el.isTradeMsg()) {
      TRADE ord;
      ord =
          std::make_pair(el.time_.tv_sec,
                         std::make_pair(el.time_.tv_usec,
                                        std::make_pair(el.data_.cme_trds_.trd_qty_,
                                                       std::make_pair(el.data_.cme_trds_.tot_qty_,
                                                                      std::make_pair(el.data_.cme_trds_.trd_px_,
                                                                                     el.data_.cme_trds_.agg_side_)))));
      hash_trade.insert(ord);
    } else {
      DELTA ord;
      ord = std::make_pair(
          el.time_.tv_sec,
          std::make_pair(
              el.time_.tv_usec,
              std::make_pair(
                  el.data_.cme_dels_.level_,
                  std::make_pair(
                      el.data_.cme_dels_.size_,
                      std::make_pair(
                          el.data_.cme_dels_.num_ords_,
                          std::make_pair(
                              el.data_.cme_dels_.price_,
                              std::make_pair(el.data_.cme_dels_.type_,
                                             std::make_pair(el.data_.cme_dels_.status_,
                                                            std::make_pair(el.data_.cme_dels_.action_,
                                                                           el.data_.cme_dels_.intermediate_)))))))));
      hash_delta.insert(ord);
    }
  }
  return;
}

bool comp(CME_MDS::CMECommonStruct& a, CME_MDS::CMECommonStruct& b) {
  return (a.time_.tv_sec < b.time_.tv_sec || (a.time_.tv_sec == b.time_.tv_sec && a.time_.tv_usec < b.time_.tv_usec));
}
// The method checks if given entry is already present in the hash table, if no, it makes an entry for it
bool check_if_repeated(CME_MDS::CMECommonStruct& el) {
  if (el.isTradeMsg()) {
    TRADE ord;
    ord = std::make_pair(
        el.time_.tv_sec,
        std::make_pair(el.time_.tv_usec, std::make_pair(el.data_.cme_trds_.trd_qty_,
                                                        std::make_pair(el.data_.cme_trds_.tot_qty_,
                                                                       std::make_pair(el.data_.cme_trds_.trd_px_,
                                                                                      el.data_.cme_trds_.agg_side_)))));
    if (hash_trade.find(ord) != hash_trade.end()) {
      return true;
    } else {
      hash_trade.insert(ord);
      return false;
    }
  } else {
    DELTA ord;
    ord = std::make_pair(
        el.time_.tv_sec,
        std::make_pair(
            el.time_.tv_usec,
            std::make_pair(
                el.data_.cme_dels_.level_,
                std::make_pair(
                    el.data_.cme_dels_.size_,
                    std::make_pair(
                        el.data_.cme_dels_.num_ords_,
                        std::make_pair(
                            el.data_.cme_dels_.price_,
                            std::make_pair(el.data_.cme_dels_.type_,
                                           std::make_pair(el.data_.cme_dels_.status_,
                                                          std::make_pair(el.data_.cme_dels_.action_,
                                                                         el.data_.cme_dels_.intermediate_)))))))));
    if (hash_delta.find(ord) != hash_delta.end()) {
      return true;
    } else {
      hash_delta.insert(ord);
      return false;
    }
  }
}

inline unsigned int YYYYMMDD_from_date(const boost::gregorian::date& d1) {
  boost::gregorian::date::ymd_type ymd = d1.year_month_day();
  return (((ymd.year * 100 + ymd.month) * 100) + ymd.day);
}

bool increment_date(std::string date_end, std::string& date_start) {
  if (date_end == date_start) return false;

  boost::gregorian::date curr_date(start_year, start_month, start_date);
  curr_date += one_day_date_duration;

  int next_date = YYYYMMDD_from_date(curr_date);

  date_start = "";
  date_start = std::string(itoa(next_date / 10000)) + "/" + std::string(itoa((next_date / 100) % 100)) + "/" +
               std::string(itoa(next_date % 100));

  return true;
}

void find_buggy_CME_files(std::string exch, std::string date_start, std::string date_end) {
  // Creating an output file for debugging
  std::ofstream error;
  error.open("error");

  // Iterate over dates between start and end dates
  bool flag = true;
  while (flag) {
    error << date_start << " " << std::endl;
    // boolean variable to control number of times output is written to the output log
    bool write = true;

    start_year = atoi((date_start.substr(0, 4)).c_str());
    start_month = atoi((date_start.substr(5, 2)).c_str());
    start_date = atoi((date_start.substr(8, 2)).c_str());

    // Check if there are files for the current date and exchange
    if (!(HFSAT::FileUtils::exists(std::string("/NAS1/data/CMELoggedData/" + exch + "/" + date_start + "/")))) {
      flag = increment_date(date_end, date_start);
      continue;
    }

    // Read files for input date and exchange
    std::vector<std::string> filenames;
    fs::path path(std::string("/NAS1/data/CMELoggedData/" + exch + "/" + date_start + "/"));
    fs::directory_iterator it(path), eod;

    BOOST_FOREACH (fs::path const& p, std::make_pair(it, eod)) {
      if (fs::is_regular_file(p)) {
        filenames.push_back(p.string());
      }
    }

    for (std::string file_ : filenames) {
      HFSAT::BulkFileReader reader;
      reader.open(file_);
      std::vector<std::pair<long int, long int> > data;
      // Read each file using BulkFileReader() into vector 'data'
      MDSLogReader<CME_MDS::CMECommonStruct>::ReadMDSStructs(reader, data);

      // Iterate over orders in the file to detect anomaly
      bool buggy = false;
      for (int itr = 0; itr != (int)data.size(); itr++) {
        if (itr == 0) continue;
        if (data[itr].first < data[itr - 1].first ||
            (data[itr].first == data[itr - 1].first && data[itr].second < data[itr - 1].second)) {
          buggy = true;

          if (data[itr].first < data[itr - 1].first)
            error << file_ << " " << data[itr].first - data[itr - 1].first << std::endl;
          else
            error << file_ << " " << data[itr].second - data[itr - 1].second << std::endl;

          break;
        }
      }
      if (buggy) {
        if (write) {
          write = false;
          std::cout << date_start << std::endl;
        }
        std::cout << file_ << std::endl;
      }
      reader.close();
    }

    flag = increment_date(date_end, date_start);
  }

  error.close();
}

void fix_buggy_CME_files(std::string file_) {
  int len = file_.size();
  std::string suffix = file_.substr(len - 3, 3);

  // to skip corrupt files
  if (suffix != ".gz") {
    std::cout << "Invalid file " << file_ << std::endl;
    return;
  }

  start_year = atoi((file_.substr(29, 4)).c_str());
  start_month = atoi((file_.substr(34, 2)).c_str());
  start_date = atoi((file_.substr(37, 2)).c_str());

  std::vector<std::pair<std::string, std::string> > filenames;
  filenames.push_back(std::make_pair(file_, ""));

  for (auto file_ : filenames) {
    if (!(HFSAT::FileUtils::exists(file_.first))) {
      std::cout << "Skipping " << file_.first << std::endl;
      continue;
    }

    std::cout << "fixing " << file_.first << std::endl;
    std::string date_start = file_.second;

    HFSAT::BulkFileReader reader;
    reader.open(file_.first);

    std::vector<CME_MDS::CMECommonStruct> data, bug;

    // Read out of entries into vector 'bug' and
    // subset of entries in increasing order in vector 'data'
    MDSLogReader<CME_MDS::CMECommonStruct>::ReadMDSStructs(reader, data, bug);

    // build hash of entries in 'data' to avoid repetition of orders
    build_hash(data);

    std::sort(bug.begin(), bug.end(), comp);
    std::sort(data.begin(), data.end(), comp);
    HFSAT::BulkFileWriter* bulk_file_writer_ = new HFSAT::BulkFileWriter();

    std::string date_ =
        std::string(itoa(start_year)) + std::string(itoa((start_month))) + std::string(itoa(start_date));
    std::string product = std::string(data[0].getContract());
    // create an output file which has entries in correct sequence
    bulk_file_writer_->Open(("/home/mgupta/repos/dvccode/output/log/" + product + "_" + date_).c_str(),
                            std::ofstream::binary | std::ofstream::out);

    int s1 = data.size(), s2 = bug.size(), i = 0, j = 0;
    // merge the 2 sorted vectors 'bug' and 'data'
    while (i < s1 && j < s2) {
      if (data[i].time_.tv_sec < bug[j].time_.tv_sec ||
          (data[i].time_.tv_sec == bug[j].time_.tv_sec && data[i].time_.tv_usec <= bug[j].time_.tv_usec)) {
        bulk_file_writer_->Write(&data[i], sizeof(data[i]));
        bulk_file_writer_->CheckToFlushBuffer();
        i++;
      } else {
        // check if the current order is not already present in hash to avoid repetition
        if (check_if_repeated(bug[j]) == false) {
          bulk_file_writer_->Write(&bug[j], sizeof(bug[j]));
          bulk_file_writer_->CheckToFlushBuffer();
        }
        j++;
      }
    }

    while (i < s1 || j < s2) {
      if (i < s1) {
        bulk_file_writer_->Write(&data[i], sizeof(data[i]));
        bulk_file_writer_->CheckToFlushBuffer();
        i++;
      }
      if (j < s2) {
        if (check_if_repeated(bug[j]) == false) {
          bulk_file_writer_->Write(&bug[j], sizeof(bug[j]));
          bulk_file_writer_->CheckToFlushBuffer();
        }
        j++;
      }
    }
    if (bulk_file_writer_) bulk_file_writer_->Close();

    reader.close();
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << " USAGE: EXEC MODE" << std::endl;
    exit(0);
  }

  std::string MODE = argv[1];

  if (MODE == "FIND") {
    if (argc < 5) {
      std::cout << " USAGE: EXEC <MODE> <EXCHANGE> <START DATE> <END DATE> (date format : yyyy/mm/dd)" << std::endl;
      exit(0);
    }
    // Method to run the sanity check the CME files on specified locations and during specified duration
    find_buggy_CME_files(argv[2], argv[3], argv[4]);
  } else if (MODE == "FIX") {
    if (argc < 3) {
      std::cout << " USAGE: EXEC <MODE> <FILE_TO_BE_FIXED> " << std::endl;
      exit(0);
    }
    // Method to fix the discovered buggy CME log files.
    fix_buggy_CME_files(argv[2]);
  } else {
    std::cout << "Invalid mode\n";
  }

  return 0;
}
