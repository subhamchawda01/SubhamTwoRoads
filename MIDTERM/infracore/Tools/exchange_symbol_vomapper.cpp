/**
   \file Tools/exchange_symbol_mapper

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <map>
#include <iostream>
#include <algorithm>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

struct SymbolInfo {
  std::string base_symbol_;
  std::string symbol_;
  int traded_volume_;
  SymbolInfo() : base_symbol_(" "), symbol_(" "), traded_volume_(0) {}
  SymbolInfo(const std::string& t_base_symbol_, const std::string& t_symbol_, const int& t_traded_volume_)
      : base_symbol_(t_base_symbol_), symbol_(t_symbol_), traded_volume_(t_traded_volume_) {}

  bool operator()(const SymbolInfo& first, const SymbolInfo& second) {
    return (first.traded_volume_ > second.traded_volume_);
  }
  bool operator<(const SymbolInfo& other) const {
    if (traded_volume_ > other.traded_volume_) {
      return true;
    }
    return false;
  }
};

inline bool equate_symbol(const SymbolInfo& first, const SymbolInfo& second) {
  return (first.symbol_.compare(second.symbol_) == 0);
}

struct RawSym {
  std::string sym_;
  int volume_;
};

class SymbolMapper {
 public:
  SymbolMapper(int _YYYYMMDD_);
  ~SymbolMapper();
  void LoadVolumeBasedInitialFile(int _YYYYMMDD_);
  void SearchMaxVolumeforBaseSymbol(std::string base_symbol, int& volume, std::string& max_symbol);
  void SearchMaxNVolumeforBaseSymbol(std::string base_symbol, unsigned int numbers_to_list, std::vector<int>& volume,
                                     std::vector<std::string>& max_symbol);

 private:
  int trading_date_;
  // Map like {FDAX -> [{FDAX201112, 100},{FDAX201203, 200}]}etc
  std::map<std::string, std::vector<SymbolInfo> > base_symbol_to_exchange_symbol_vec_;
  std::map<std::string, std::vector<SymbolInfo> >::iterator base_symbol_to_exchange_symbol_vec_iter;
  // Input from the volume_file_genearted {FDAX2011, 100}, {ZFZ1, 200}, etc
  std::vector<RawSym> raw_sym_vec_;
};

SymbolMapper::SymbolMapper(int _YYYYMMDD_) : trading_date_(_YYYYMMDD_) { LoadVolumeBasedInitialFile(_YYYYMMDD_); }

SymbolMapper::~SymbolMapper() {
  // NOThing to free right now
}

void SymbolMapper::LoadVolumeBasedInitialFile(int _YYYYMMDD_) {
  int this_YYYYMMDD_ = _YYYYMMDD_;
  std::string _volume_symbol_filename_ = "";
  // Try to load most recent file in the last 30 days
  for (unsigned int ii = 0; ii < 30; ii++) {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/"
                << "VolumeBasedSymbol/VOSymbol_" << this_YYYYMMDD_ << ".basecodes.txt";
    _volume_symbol_filename_ = t_temp_oss_.str();

    if (HFSAT::FileUtils::exists(_volume_symbol_filename_))
      break;
    else {
      // Try preivous day
      this_YYYYMMDD_ = HFSAT::DateTime::CalcPrevDay(this_YYYYMMDD_);
    }
  }

  if (!HFSAT::FileUtils::exists(_volume_symbol_filename_)) {
  }

  std::ifstream volume_symbol_file_;
  volume_symbol_file_.open(_volume_symbol_filename_.c_str(), std::ifstream::in);
  if (volume_symbol_file_.is_open()) {
    const int kVolSymbolLen = 1024;
    char readline_buffer_[kVolSymbolLen];
    bzero(readline_buffer_, kVolSymbolLen);

    while (volume_symbol_file_.good()) {
      bzero(readline_buffer_, kVolSymbolLen);
      volume_symbol_file_.getline(readline_buffer_, kVolSymbolLen);
      HFSAT::PerishableStringTokenizer st_(readline_buffer_, kVolSymbolLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      // expect lines of the form :
      // EXCHSYMBOL VOLUME
      if (tokens_.size() == 2) {
        RawSym _this_volsymbol_pair_;
        _this_volsymbol_pair_.sym_ = tokens_[0];
        _this_volsymbol_pair_.volume_ = atoi(tokens_[1]);
        raw_sym_vec_.push_back(_this_volsymbol_pair_);
        //	std::cout << " SYM: "<< _this_volsymbol_pair_.sym_ << " VOL: "<< _this_volsymbol_pair_.volume_<<
        // std::endl;
      }
    }
    volume_symbol_file_.close();
  }
}

// const char * ExchangeSymbolManager::fromRolloverOverrideFile ( const std::string & _shortcode_ ) const
// {
//   for ( unsigned int i = 0 ; i < orpair_vec_.size ( ) ; i ++ )
//     {
// 	if ( orpair_vec_[i].shortcode_.compare ( _shortcode_ ) == 0 )
// 	  {
// 	    return orpair_vec_[i].exchange_symbol_ ;
// 	  }
//     }

//   return NULL ;
// }

void SymbolMapper::SearchMaxVolumeforBaseSymbol(std::string base_symbol, int& volume, std::string& max_symbol) {
  int len = base_symbol.length();
  volume = -1;
  std::vector<SymbolInfo> temp_sym_info_;
  for (unsigned int jj = 0; jj < (unsigned int)raw_sym_vec_.size(); jj++) {
    if (raw_sym_vec_[jj].sym_.compare(0, len, base_symbol) == 0) {
      SymbolInfo syminfo(base_symbol, raw_sym_vec_[jj].sym_, raw_sym_vec_[jj].volume_);
      temp_sym_info_.push_back(syminfo);
    }
  }
  // Made it this way in case in future we will be interested to trade second, third largest volume
  // etc
  std::sort(temp_sym_info_.begin(), temp_sym_info_.end());

  if (temp_sym_info_.empty())
    max_symbol = "SYMNOTFOUND";
  else {
    volume = temp_sym_info_[0].traded_volume_;
    max_symbol = temp_sym_info_[0].symbol_;
    if (volume == 0) max_symbol = "ZEROMAXVOLUME";
  }
}

// Search max N specified volumes of various traded contracts of the same product
// contract_combinations_to_look (say = 3 for DI) will probe max 3 traded DI contracts
void SymbolMapper::SearchMaxNVolumeforBaseSymbol(std::string base_symbol, unsigned int contracts_combinations_to_look,
                                                 std::vector<int>& volume, std::vector<std::string>& max_symbol) {
  volume.clear();
  max_symbol.clear();

  int len = base_symbol.length();
  std::vector<SymbolInfo> temp_sym_info_;
  for (unsigned int jj = 0; jj < (unsigned int)raw_sym_vec_.size(); jj++) {
    if (raw_sym_vec_[jj].sym_.compare(0, len, base_symbol) == 0) {
      SymbolInfo syminfo(base_symbol, raw_sym_vec_[jj].sym_, raw_sym_vec_[jj].volume_);
      temp_sym_info_.push_back(syminfo);
    }
  }
  // Made it this way in case in future we will be interested to trade second, third largest volume
  // etc
  std::sort(temp_sym_info_.begin(), temp_sym_info_.end());

  if (temp_sym_info_.empty())
    return;
  else {
    for (unsigned int ii = 0; ii < contracts_combinations_to_look; ii++) {
      volume.push_back(temp_sym_info_[ii].traded_volume_);
      if (volume[ii] == 0) {
        max_symbol.push_back("ZEROMAXVOLUME");
      } else {
        max_symbol.push_back(temp_sym_info_[ii].symbol_);
      }
    }
  }
}

void sendEmailNotification(std::string email_body_) {
  HFSAT::Email e;
  e.setSubject("Subject: Data Not Available");
  e.addRecepient("nseall@tworoads.co.in");
  e.addSender("nseall@tworoads.co.in");
  e.content_stream << email_body_;

  e.sendMail();
}

int main(int argc, char** argv) {
  std::vector<std::string> sec_list_vec;
  if (argc < 3) {
    std::cerr << " usage : Input file name containing the Symbols E.g "
                 "~/infracore_install/files/volume_based_symbols_to_eval.txt DATE(YYYYMMDD)"
              << std::endl;
    exit(0);
  }

  std::string filename_input(argv[1]);

  /////////FILE INPUT OF SHORTCODES///////////
  char line[1024];
  std::ifstream sec_file_;
  sec_file_.open(filename_input.c_str(), std::ifstream::in);
  if (!sec_file_.is_open()) {
    std::cerr << filename_input << "  FILE DOESNOT EXIST " << std::endl;
    exit(-1);
  }

  while (!sec_file_.eof()) {
    bzero(line, 1024);
    sec_file_.getline(line, 1024);
    if (strstr(line, "#") || strlen(line) == 0) continue;
    HFSAT::PerishableStringTokenizer st(line, 1024);
    const std::vector<const char*>& tokens = st.GetTokens();
    if (tokens.size() < 1) {
      std::cerr << " Bad file..See #entries " << std::endl;
      exit(-1);
    }
    // std::cout << " SEC_SHORTCODE " << tokens[ 0 ] << std::endl;
    sec_list_vec.push_back(std::string(tokens[0]));
  }
  ////////@end FILE INPUT////////////////

  //  int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal ( );
  int process_date_ = atoi(argv[2]);
  SymbolMapper symMapper(process_date_);

  /// Open A file for writing volume based Symbol result
  std::ostringstream t_temp_stream;
  t_temp_stream << "/spare/local/"
                << "VolumeBasedSymbol/VOSymbol_" << process_date_ << ".txt";
  std::string volume_based_exchnage_symbol_filename = t_temp_stream.str();

  std::ofstream volume_based_exchnage_symbol_file;
  volume_based_exchnage_symbol_file.open(volume_based_exchnage_symbol_filename.c_str(), std::ios::out);

  std::ostringstream symbol_with_space_stream;
  symbol_with_space_stream << "/spare/local/"
                           << "VolumeBasedSymbol/LIFFEVOSymbol_" << process_date_ << ".txt";

  std::string symbol_with_space_filename_ = symbol_with_space_stream.str();

  std::ofstream symbol_with_space_file_;
  symbol_with_space_file_.open(symbol_with_space_filename_.c_str(), std::ofstream::out);

  //  std::cout << "File to Write : " << volume_based_exchnage_symbol_filename << std::endl;

  std::string mail_notif_ = "Data Not Copied ( or Copied Later ) For : " + std::string("<b>") + std::string(argv[2]) +
                            std::string("</b> <br/> <br/> Instruments List : ");

  bool data_not_found_ = false;
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(process_date_);

  std::ofstream vfile;
  vfile.open(volume_based_exchnage_symbol_filename.c_str(), std::ofstream::out);

  std::cout << " FIle to write: " << volume_based_exchnage_symbol_filename << std::endl;
  std::cout << " FIle to write: " << symbol_with_space_filename_ << std::endl;

  if (vfile.is_open()) {
    // Inefficient serach O(n^2), not concerned right now

    for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
      // For other such contracts compare them here
      if (sec_list_vec[ii].compare("DI1") == 0) {
        unsigned int kMaxNumbersToList = 7;
        // List first kMaxNumbersToList max vols contract in case of DI
        for (unsigned int jj = 0; jj < kMaxNumbersToList; jj++) {
          int volume = 0;
          std::string max_symbol = "";
          std::ostringstream shortcode_stream_;
          shortcode_stream_ << "BR_DI_" << jj;
          std::string _this_shortcode_ = shortcode_stream_.str();
          std::string this_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(_this_shortcode_);
          symMapper.SearchMaxVolumeforBaseSymbol(this_exchange_symbol_, volume, max_symbol);

          if (volume > 0 && max_symbol.compare(0, 11, "SYMNOTFOUND") != 0) {
            vfile << _this_shortcode_ << "\t" << max_symbol << "\t" << volume << "\n";
          }
        }
      } else if (sec_list_vec[ii].compare("LFI") == 0 || sec_list_vec[ii].compare("LFL") == 0) {
        unsigned int kMaxNumbersToList = 7;
        for (unsigned int jj = 0; jj < kMaxNumbersToList; jj++) {
          int volume = 0;
          std::string max_symbol = "";
          std::ostringstream shortcode_stream_;
          shortcode_stream_ << sec_list_vec[ii] << "_" << jj;
          std::string _this_shortcode_ = shortcode_stream_.str();
          std::string this_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(_this_shortcode_);
          std::replace(this_exchange_symbol_.begin(), this_exchange_symbol_.end(), ' ', '~');
          symMapper.SearchMaxVolumeforBaseSymbol(this_exchange_symbol_, volume, max_symbol);
          if (volume > 0 && max_symbol.compare(0, 11, "SYMNOTFOUND") != 0) {
            symbol_with_space_file_ << _this_shortcode_ << "\t" << max_symbol << "\t" << volume << "\n";
          }
        }
      } else if (sec_list_vec[ii].compare("TPX") == 0 || sec_list_vec[ii].compare("JGB") == 0) {
        int volume = 0;
        std::string max_symbol = "";
        std::string t_temp_symbol_ = "FFTPX";
        if (sec_list_vec[ii] == "JGB") {
          t_temp_symbol_ = "GFJGB";
        }
        symMapper.SearchMaxVolumeforBaseSymbol(t_temp_symbol_, volume, max_symbol);
        std::stringstream ss;
        if (volume > 0 && max_symbol.compare(0, 11, "SYMNOTFOUND") != 0) {
          std::ostringstream shortcode_stream_;
          shortcode_stream_ << sec_list_vec[ii] << "_0";
          std::string _this_shortcode_ = shortcode_stream_.str();
          // HFSAT::ExchSource_t _this_exch_source_ = HFSAT::SecurityDefinitions::GetContractExchSource (
          // _this_shortcode_ , process_date_ ) ;
          ss << sec_list_vec[ii] << "_" << 0;
          vfile << ss.str() << "\t" << max_symbol << "\t" << volume << "\n";
        }
      } else {
        // Serach the max volume for this basesymbol
        int volume = 0;
        std::string max_symbol = "";
        symMapper.SearchMaxVolumeforBaseSymbol(sec_list_vec[ii], volume, max_symbol);
        std::stringstream ss;
        if (volume > 0 && max_symbol.compare(0, 11, "SYMNOTFOUND") != 0 && (sec_list_vec[ii] != "ONX") &&
            (sec_list_vec[ii] != "LGB")) {
          std::ostringstream shortcode_stream_;

          if (sec_list_vec[ii] == "DOL" || sec_list_vec[ii] == "WIN" || sec_list_vec[ii] == "WDO" ||
              sec_list_vec[ii] == "IND") {
            shortcode_stream_ << "BR_" << sec_list_vec[ii] << "_0";
            std::ostringstream ss_;
            ss_ << "BR_" << sec_list_vec[ii];
            sec_list_vec[ii] = ss_.str();
          } else if (sec_list_vec[ii] == "NK1") {
            shortcode_stream_ << "NK_0";
            sec_list_vec[ii] = "NK";
          } else {
            shortcode_stream_ << sec_list_vec[ii] << "_0";
          }

          std::string _this_shortcode_ = shortcode_stream_.str();

          HFSAT::ExchSource_t _this_exch_source_ =
              HFSAT::SecurityDefinitions::GetContractExchSource(_this_shortcode_, process_date_);

          if (_this_exch_source_ == HFSAT::kExchSourceLIFFE) {
            ss << sec_list_vec[ii] << "_" << 0;
            symbol_with_space_file_ << ss.str() << "\t" << max_symbol << "\t" << volume << "\n";

          } else {
            ss << sec_list_vec[ii] << "_" << 0;
            vfile << ss.str() << "\t" << max_symbol << "\t" << volume << "\n";
          }
        } else if (max_symbol.compare(0, 11, "SYMNOTFOUND") == 0 && (sec_list_vec[ii] != "ONX") &&
                   (sec_list_vec[ii] != "LGB")) {
          mail_notif_ += (sec_list_vec[ii] + "  ");

          if (!data_not_found_) data_not_found_ = true;
        }
      }
    }

    vfile.close();

    if (data_not_found_) {
      mail_notif_ += std::string(
          "<br/><br/> /home/dvcinfra/LiveExec/scripts/volume_symbol_vomapper.sh needs to be rerun on Ny Machines");
      sendEmailNotification(mail_notif_);
    }
  }
}
