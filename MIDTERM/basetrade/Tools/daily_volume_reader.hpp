/**
    \file daily_volume_reader.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include "dvccode/Utils/bulk_file_reader.hpp"

class DailyTradeVolumeReader {
  static const int TRADE_AVERAGING_WINDOW_SIZE_SECS = 180;
  std::string file_name_;
  std::vector<int> timestamps_;
  std::vector<int> trades_;

  double avg_trades_per_sec;

  void readFile() {
    HFSAT::BulkFileReader reader_;
    reader_.open(file_name_);

    char buf[64];
    bzero(buf, 64);

    int time = 0;
    int trade = 0;
    while (reader_.GetLine(buf, 64)) {
      sscanf(buf, "%d %d", &time, &trade);
      // std::cout << "value read " << time << " " << trade << "\n";
      if (timestamps_.size() > 0 && timestamps_.back() == time) {
        trades_[trades_.size() - 1] += trade;
      } else {
        timestamps_.push_back(time);
        if (trades_.size() > 0) trade += trades_.back();
        trades_.push_back(trade);
      }
      bzero(buf, 64);
    }
    reader_.close();
    if (trades_.size() > 1)
      avg_trades_per_sec = (double)trades_.back() / (timestamps_.back() - timestamps_[0]);
    else {
      avg_trades_per_sec = 1;
    }
  }

  /**
   *
   * @param time
   * @return tries to bin_search the argument. if the argument is not present, returns the highest index whose values is
   * lower than the argument.
   * If no such index exists, i.e. argument less than least element in the list, we return index 0
   */
  int bin_search(int time) {
    int low = 0;
    int high = timestamps_.size() - 1;

    while (true) {
      int mid = (low + high) / 2;

      if (high <= low) return 0 >= high ? high : low;

      if (timestamps_[mid] == time) return mid;

      if (timestamps_[mid] < time) {
        if (timestamps_[mid + 1] > time) return mid;
        low = mid + 1;
      } else {
        high = mid - 1;
      }
    }
  }

 public:
  /**
   * @param file_name = expects a text file. each line has 2 numbers, timestamp in seconds and traded volume
   * Also assumes file is sorted based on timestamp
   */
  DailyTradeVolumeReader(std::string file_name) : file_name_(file_name), timestamps_(), trades_() { readFile(); }

  /**
   * @param time_secs = unix_time_in_seconds
   * @return
   */
  double getTradedVolRatioAtTime(int time_secs) {
    if (trades_.size() <= 1)  // handle trades_ of size <= 1
      return 1;
    int indx = bin_search(time_secs + TRADE_AVERAGING_WINDOW_SIZE_SECS);
    int indx2 = bin_search(time_secs);
    int trades_in_period = trades_[indx] - trades_[indx2];
    double period = timestamps_[indx] - timestamps_[indx2];
    if (period < 0.000001) {
      return 0;
    }
    double ratio = (trades_in_period / period) / avg_trades_per_sec;
    return ratio;
  }

  /**
   *
   * @param time_secs
   * @return tries to find the cumulative volume at or before time_secs, in case the trades file is sparse
   */
  int getCumTradesVolAtTime(int time_secs) {
    int indx = bin_search(time_secs);
    return trades_[indx];
  }
};
