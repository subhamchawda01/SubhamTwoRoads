/**
   \file Tools/volume_symbol_reconcilation.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <map>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/send_alert.hpp"

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#define MAX_LINE_LENGTH 1024

#define IN_USE_VOLUMEBASED_SHORTCODE_SYMBOL_FILE \
  "/spare/local/VolumeBasedSymbol/VOSymbol_"  // this file has the volume based mapping
#define LIVE_SYMBOL_VOLUME_FILE \
  "/spare/local/VolumeBasedSymbol/current_volume_sorted.txt"  // this will be generated through a script after
                                                              // collecting all volumes for all inst.

class VolumeSymbolManager {
  std::map<std::string, std::string>
      volume_based_shortcode_to_symbol_map_;              // holds existing volume based shortcode to symbol mapping
  std::vector<std::string> all_symbols_vec_;              // all the symbols we have insterest in
  std::map<std::string, int> live_symbol_to_volume_map_;  // holds live symbols to volume mapping
  std::map<std::string, std::string>
      live_shortcode_to_symbol_map_;  // will eventually hold shortcode to highest volume symbol mapping

 public:
  void loadSymbolEvaluationFile(std::string _filename_) {
    std::ifstream symbol_evaluation_file_;

    symbol_evaluation_file_.open(_filename_.c_str(), std::ifstream::in);

    if (!symbol_evaluation_file_.is_open()) {
      std::cerr << " File : " << _filename_ << " Doesn't Exists " << std::endl;
      exit(-1);
    }

    char line_[MAX_LINE_LENGTH];
    std::string symbol_ = "";

    while (symbol_evaluation_file_.good()) {
      memset(line_, 0, MAX_LINE_LENGTH);
      symbol_ = "";

      symbol_evaluation_file_.getline(line_, MAX_LINE_LENGTH);

      if (strstr(line_, "#") || !strlen(line_)) continue;  // comments etc.

      symbol_ = line_;
      all_symbols_vec_.push_back(symbol_);
    }

    symbol_evaluation_file_.close();
  }

  void loadVolumeBasedShortcodeToSymbolFile(int _processdate_) {
    int processdate = _processdate_;

    std::string filename = "";

    // try to load most recent file in last 30 days
    for (unsigned int dayCounter = 0; dayCounter < 30; dayCounter++) {
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << IN_USE_VOLUMEBASED_SHORTCODE_SYMBOL_FILE << processdate << ".txt";

      filename = t_temp_oss_.str();

      if (HFSAT::FileUtils::exists(filename))
        break;

      else
        processdate = HFSAT::DateTime::CalcPrevDay(processdate);
    }

    if (!HFSAT::FileUtils::exists(filename)) {
      std::cerr << " File : " << filename << " Doesn't Exists " << std::endl;
      exit(-1);
    }

    std::ifstream in_use_volume_based_shortcode_to_symbol_file_;

    in_use_volume_based_shortcode_to_symbol_file_.open(filename.c_str(), std::ifstream::in);

    if (!in_use_volume_based_shortcode_to_symbol_file_.is_open()) {
      std::cerr << " File : " << filename << " Doesn't Exists " << std::endl;
      exit(-1);
    }

    char line_[MAX_LINE_LENGTH];
    std::string symbol_ = "";

    while (in_use_volume_based_shortcode_to_symbol_file_.good()) {
      memset(line_, 0, MAX_LINE_LENGTH);
      symbol_ = "";

      in_use_volume_based_shortcode_to_symbol_file_.getline(line_, MAX_LINE_LENGTH);

      if (strstr(line_, "#") || !strlen(line_)) continue;

      HFSAT::PerishableStringTokenizer st_(line_, MAX_LINE_LENGTH);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() == 3) {  // haven't handled bad line formation, skipping for now
        volume_based_shortcode_to_symbol_map_[tokens_[0]] = tokens_[1];
      }
    }

    in_use_volume_based_shortcode_to_symbol_file_.close();
  }

  void loadLiveSymbolsToVolumeFile(std::string _filename_) {
    std::ifstream live_symbol_to_volume_file_;

    live_symbol_to_volume_file_.open(_filename_.c_str(), std::ifstream::in);

    if (!live_symbol_to_volume_file_.is_open()) {
      std::cerr << " File : " << _filename_ << " Doesn't Exists " << std::endl;
      exit(-1);
    }

    char line_[MAX_LINE_LENGTH];
    std::string symbol_ = "";

    while (live_symbol_to_volume_file_.good()) {
      memset(line_, 0, MAX_LINE_LENGTH);
      symbol_ = "";

      live_symbol_to_volume_file_.getline(line_, MAX_LINE_LENGTH);

      if (strstr(line_, "#") || !strlen(line_)) continue;

      HFSAT::PerishableStringTokenizer st_(line_, MAX_LINE_LENGTH);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() == 2) {
        live_symbol_to_volume_map_[tokens_[0]] = atoi(tokens_[1]);
      }
    }

    live_symbol_to_volume_file_.close();
  }

  void sendEmailNotification(HFSAT::Email &e) {
    e.setSubject("Subject: Volume-Symbol Map Discrepancy");
    e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
    e.addSender("ravi.parikh@tworoads.co.in");
    e.sendMail();
  }

  // Checks wheather the products are actually same or not. Currently discards spreads only.
  bool areSimilarProduct(std::string ourSymbol, std::string exchSymbol) {
    // hack where substring causing problems, not a clean fix, change later
    if (ourSymbol.find("NK") != std::string::npos && exchSymbol.find("NKM") != std::string::npos) return false;

    if (ourSymbol.size() > exchSymbol.size()) return false;
    std::string postfix = exchSymbol.substr(ourSymbol.size());
    if (!exchSymbol.compare(0, ourSymbol.size(), ourSymbol) && postfix.find(ourSymbol.c_str()) == postfix.npos)
      return true;
    else
      return false;
  }

  // this method populates the live_shortcode_to_symbol_map_ after finding all the symbols with max volume for each
  // shortcode available
  void evaluateLiveShortcodeMapping() {
    std::map<std::string, int>::iterator live_symbol_to_volume_map_iterator_;
    int max_volume_ = 0;

    for (unsigned int all_symbols_vec_counter = 0; all_symbols_vec_counter < all_symbols_vec_.size();
         all_symbols_vec_counter++) {
      max_volume_ = 0;

      for (live_symbol_to_volume_map_iterator_ = live_symbol_to_volume_map_.begin();
           live_symbol_to_volume_map_iterator_ != live_symbol_to_volume_map_.end();
           live_symbol_to_volume_map_iterator_++) {
        //         if( !live_symbol_to_volume_map_iterator_->first.compare( 0, all_symbols_vec_[
        //         all_symbols_vec_counter].length(),all_symbols_vec_[ all_symbols_vec_counter] ) && max_volume_ <
        //         live_symbol_to_volume_map_iterator_->second ){
        if (areSimilarProduct(all_symbols_vec_[all_symbols_vec_counter], live_symbol_to_volume_map_iterator_->first) &&
            max_volume_ < live_symbol_to_volume_map_iterator_->second) {
          live_shortcode_to_symbol_map_[all_symbols_vec_[all_symbols_vec_counter] + "_0"] =
              live_symbol_to_volume_map_iterator_->first;
          max_volume_ = live_symbol_to_volume_map_iterator_->second;
        }
      }
    }
  }
  // for each shortcode, figures out whether in-use mapping matches with the live mapping or not
  // if not sends an email notification / alert
  void reconcileShortcodeMapping() {
    bool alert_flag_ = false;
    HFSAT::Email e;
    std::string shortcode_ = "";
    std::string shortcode_to_symbol_ = "";

    for (unsigned int all_symbols_vec_counter = 0; all_symbols_vec_counter < all_symbols_vec_.size();
         all_symbols_vec_counter++) {
      // only compare when volume available
      if ((live_shortcode_to_symbol_map_.find(all_symbols_vec_[all_symbols_vec_counter] + "_0") ==
           live_shortcode_to_symbol_map_.end()))
        continue;

      shortcode_ = all_symbols_vec_[all_symbols_vec_counter];

      // brazilian instruments have BR in front
      if (shortcode_ == "DOL" || shortcode_ == "WIN" || shortcode_ == "WDO" || shortcode_ == "IND" ||
          shortcode_ == "DI")
        shortcode_ = "BR_" + shortcode_;

      shortcode_ += "_0";

      // Get In-Use Mapping
      shortcode_to_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

      std::replace(shortcode_to_symbol_.begin(), shortcode_to_symbol_.end(), ' ', '~');

      // Compare current highest volume based symbol with in-use mapping
      if (live_shortcode_to_symbol_map_[all_symbols_vec_[all_symbols_vec_counter] + "_0"] != shortcode_to_symbol_) {
        if (shortcode_ == "GE_0" || shortcode_ == "BAX_0" || shortcode_ == "LFI_0" || shortcode_ == "LFL_0" ||
            shortcode_ == "BR_DI_0" || (shortcode_.find("YFEBM") != std::string::npos) ||
            (shortcode_.find("XFC") != std::string::npos) || (shortcode_.find("XFRC") != std::string::npos) ||
            (shortcode_.find("XFW") != std::string::npos))
          continue;

        alert_flag_ = true;

        // notify all three mapping - InUse, BasedOnCurrentVolume, BasedOnLastVolume
        e.content_stream
            << " SHORTCODE : " << all_symbols_vec_[all_symbols_vec_counter] + "_0"
            << " LOGICAL : " << shortcode_to_symbol_ << " -> " << live_symbol_to_volume_map_[shortcode_to_symbol_]
            << " HIGHEST_VOL_TODAY : "
            << live_shortcode_to_symbol_map_[all_symbols_vec_[all_symbols_vec_counter] + "_0"] << " -> "
            << live_symbol_to_volume_map_[live_shortcode_to_symbol_map_[all_symbols_vec_[all_symbols_vec_counter] +
                                                                        "_0"]]
            << " HIGHEST_VOL_YESTERDAY : "
            << volume_based_shortcode_to_symbol_map_[all_symbols_vec_[all_symbols_vec_counter] + "_0"] << " -> "
            << live_symbol_to_volume_map_
                   [volume_based_shortcode_to_symbol_map_[all_symbols_vec_[all_symbols_vec_counter] + "_0"]]
            << "<br/>";

        //       std::cerr << " SHORTCODE : " << all_symbols_vec_[ all_symbols_vec_counter ] + "_0"
        //                 << " LOGICAL : " << shortcode_to_symbol_ << " -> " << live_symbol_to_volume_map_ [
        //                 shortcode_to_symbol_ ]
        //                 << " HIGHEST_VOL_TODAY : " << live_shortcode_to_symbol_map_[ all_symbols_vec_[
        //                 all_symbols_vec_counter ] + "_0" ] << " -> " << live_symbol_to_volume_map_[
        //                 live_shortcode_to_symbol_map_[ all_symbols_vec_[ all_symbols_vec_counter ] + "_0" ] ]
        //                 << " HIGHEST_VOL_YESTERDAY : " << volume_based_shortcode_to_symbol_map_[ all_symbols_vec_[
        //                 all_symbols_vec_counter ] + "_0" ] << " -> " << live_symbol_to_volume_map_[
        //                 volume_based_shortcode_to_symbol_map_[ all_symbols_vec_[ all_symbols_vec_counter ] + "_0" ] ]
        //                 << "<br/>";
      }
    }

    if (alert_flag_) sendEmailNotification(e);
  }
};

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << " usage : Input file name containing the Symbols E.g "
                 "~/infracore_install/files/volume_based_symbols_to_eval.txt DATE(YYYYMMDD)"
              << std::endl;
    exit(0);
  }

  std::string symbol_eval_filename_ = argv[1];
  int processdate_ = atoi(argv[2]);

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(processdate_);

  VolumeSymbolManager vsm;

  vsm.loadSymbolEvaluationFile(symbol_eval_filename_);
  vsm.loadVolumeBasedShortcodeToSymbolFile(processdate_);
  vsm.loadLiveSymbolsToVolumeFile(LIVE_SYMBOL_VOLUME_FILE);

  vsm.evaluateLiveShortcodeMapping();
  vsm.reconcileShortcodeMapping();

  return 0;
}
