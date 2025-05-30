// =====================================================================================
//
//       Filename:  MarketAdapter/indexed_book_oputils.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/24/2012 07:10:03 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 353, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {

#define SHORTCODE_TICKS_INFO_FILENAME "/spare/local/files/shortcodes_to_ticks_info.dat"

// can be configured per product
#define BASE_LEVEL 1
#define MIN_INITIAL_TICK_BASE 512
#define DEFAULT_MAX_TICK_RANGE 1000000

#define SET_LIMIT_PRICE_ALONG_WITH_LIMIT_INT_PRICE

class IndexedBookOputils {
 private:
  /// Made copy constructor private to disable
  IndexedBookOputils(const IndexedBookOputils&);

  std::map<std::string, unsigned int> shortcode_to_initial_ticks_size_;
  std::map<std::string, unsigned int> shortcode_to_max_tick_range_;

 public:
  explicit IndexedBookOputils() { LoadIndexedBookInitialSizes(); }

  static inline IndexedBookOputils& GetUniqueInstance() {
    static IndexedBookOputils uniqueinstance_;
    return uniqueinstance_;
  }

 public:
  unsigned int GetMaxTickRange(const std::string this_shortcode) {
    unsigned int this_initial_max_tick_range = DEFAULT_MAX_TICK_RANGE;
    if (shortcode_to_max_tick_range_.find(this_shortcode) != shortcode_to_max_tick_range_.end()) {
      this_initial_max_tick_range = shortcode_to_max_tick_range_[this_shortcode];
      if (this_initial_max_tick_range < GetInitialBaseBidIndex(this_shortcode) * 4) {
        this_initial_max_tick_range = GetInitialBaseBidIndex(this_shortcode) * 4;
      }
    }
    return this_initial_max_tick_range;
  }

  unsigned int GetInitialBaseBidIndex(const std::string& this_shortcode_) {
    unsigned int this_initial_tick_base_ = MIN_INITIAL_TICK_BASE;
    if (shortcode_to_initial_ticks_size_.find(this_shortcode_) != shortcode_to_initial_ticks_size_.end()) {
      this_initial_tick_base_ = shortcode_to_initial_ticks_size_[this_shortcode_];
      if (this_initial_tick_base_ < MIN_INITIAL_TICK_BASE) {  // Dont know how this will ever occur;
        this_initial_tick_base_ = MIN_INITIAL_TICK_BASE;
      }
    }
    return this_initial_tick_base_;
  }

 private:
  void LoadIndexedBookInitialSizes() {
#define MAX_LINE_LENGTH 1024

    // Collect the size of book tick_range_for_shortcode_ from the file /spare/local/files/shortcodes_to_ticks_info.dat.
    std::string shortcode_to_ticks_info_filename_ = SHORTCODE_TICKS_INFO_FILENAME;
    std::ifstream shortcode_to_ticks_info_file_(shortcode_to_ticks_info_filename_.c_str());
    if (!shortcode_to_ticks_info_file_.is_open()) {
      std::cerr << " Failed to Initialize Index EUREX MVM, Unable to open file : " << shortcode_to_ticks_info_filename_
                << "\n";
      exit(-1);
    }

    char line_buffer_[MAX_LINE_LENGTH];

    while (shortcode_to_ticks_info_file_.good()) {
      memset(line_buffer_, 0, MAX_LINE_LENGTH);
      shortcode_to_ticks_info_file_.getline(line_buffer_, MAX_LINE_LENGTH);
      HFSAT::PerishableStringTokenizer st_(line_buffer_, MAX_LINE_LENGTH);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() != 2 || tokens_[0][0] == '#') continue;  // ignore comments o empty line

      std::string shortcode_ = tokens_[0];
      int tick_range_for_shortcode_ = std::max(1, atoi(tokens_[1]));

      if (tick_range_for_shortcode_ <= 0) {
        tick_range_for_shortcode_ = MIN_INITIAL_TICK_BASE;
        std::cerr << " Initial Tick Range Assumed to be Even, Found Invalid Entry For : " << shortcode_
                  << " To be : " << tick_range_for_shortcode_ << " Adjusted To : " << tick_range_for_shortcode_ << "\n";
      }

      if (tick_range_for_shortcode_ % 2 != 0) {  // may not be that necessary ??.
        std::cerr << " Initial Tick Range Assumed to be Even, Found Invalid Entry For : " << shortcode_
                  << " To be : " << tick_range_for_shortcode_ << " Adjusted To : " << tick_range_for_shortcode_ + 1
                  << "\n";
        tick_range_for_shortcode_++;
      }

      int max_tick_range_ = DEFAULT_MAX_TICK_RANGE;
      if (tokens_.size() >= 3) {
        max_tick_range_ = std::min(tick_range_for_shortcode_ * 4, atoi(tokens_[2]));
        shortcode_to_max_tick_range_[shortcode_] = max_tick_range_;
      }
      shortcode_to_initial_ticks_size_[shortcode_] = tick_range_for_shortcode_;
    }

    shortcode_to_ticks_info_file_.close();

#undef MAX_LINE_LENGTH
  }
};
}
