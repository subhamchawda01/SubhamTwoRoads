#include <iostream>
#include <sstream>
#include <algorithm>

#include "basetrade/Tools/symbol_allocator.hpp"

const char CMEMonthCode[] = {
    'F',  // January
    'G',  // February
    'H',  // March
    'J',  // April
    'K',  // May
    'M',  // June
    'N',  // July
    'Q',  // August
    'U',  // September
    'V',  // October
    'X',  // November
    'Z'   // December
};

//#define MAX_MONTHS_TO_CONSIDER_GC 15;

const char* CME_ZT = "ZT";
const char* CME_ZF = "ZF";
const char* CME_ZN = "ZN";
const char* CME_ZB = "ZB";
const char* CME_UB = "UB";
const char* CME_GE = "GE";
const char* CME_ES = "ES";
const char* CME_NQ = "NQ";
const char* CME_YM = "YM";
const char* CME_6A = "6A";
const char* CME_6B = "6B";
const char* CME_6C = "6C";
const char* CME_6E = "6E";
const char* CME_6J = "6J";
const char* CME_6M = "6M";
const char* CME_6S = "6S";
const char* CME_GC = "GC";
const char* CME_CL = "CL";
const char* EUREX_FGBS = "FGBS";
const char* EUREX_FGBM = "FGBM";
const char* EUREX_FGBL = "FGBL";
const char* EUREX_FGBX = "FGBX";
const char* EUREX_FBTS = "FBTS";
const char* EUREX_FBTP = "FBTP";
const char* EUREX_FESX = "FESX";
const char* EUREX_FDAX = "FDAX";
const char* TMX_SXF = "SXF";
const char* TMX_CGB = "CGB";
const char* TMX_BAX = "BAX";
const char* BMF_DOL = "BR_DOL";
const char* BMF_IND = "BR_IND";
const char* BMF_WIN = "BR_WIN";

void VolumeSymbolManager::populate_cme_base_symbols() {
  cme_base_symbols_.clear();

  // For now push only these basee base_symbols
  cme_base_symbols_.push_back(CME_ZT);
  cme_base_symbols_.push_back(CME_ZF);
  cme_base_symbols_.push_back(CME_ZN);
  cme_base_symbols_.push_back(CME_ZB);
  cme_base_symbols_.push_back(CME_UB);
  cme_base_symbols_.push_back(CME_GE);
  cme_base_symbols_.push_back(CME_ES);
  cme_base_symbols_.push_back(CME_NQ);
  cme_base_symbols_.push_back(CME_YM);
  cme_base_symbols_.push_back(CME_6A);
  cme_base_symbols_.push_back(CME_6B);
  cme_base_symbols_.push_back(CME_6C);
  cme_base_symbols_.push_back(CME_6E);
  cme_base_symbols_.push_back(CME_6J);
  cme_base_symbols_.push_back(CME_6M);
  cme_base_symbols_.push_back(CME_6S);

  // These are newly added symbols
  cme_base_symbols_.push_back(CME_GC);
  cme_base_symbols_.push_back(CME_CL);
}

void VolumeSymbolManager::populateAllSymbolsWithContractsCME() {
  int num_cme_base_symbols = cme_base_symbols_.size();

  std::vector<SymbolInfo> all_symbols;

  for (int ii = 0; ii < num_cme_base_symbols; ii++) {
    all_symbols.clear();

    if (strcmp(cme_base_symbols_[ii].c_str(), CME_GC) == 0) {
      std::string symbol(CME_GC);
      std::vector<std::string> gc_next_23_month_vec;  // Temp variables
      std::vector<std::string> gc_next_72_month_vec;  // Temp variables

      std::string this_month_sym = ThisMonthCME(symbol, current_trading_month_, current_trading_year_);
      std::string next_month_sym = NextNthMonthCME(symbol, current_trading_month_, current_trading_year_, 1);
      std::string next_next_month_sym = NextNthMonthCME(symbol, current_trading_month_, current_trading_year_, 2);
      std::cout << "This mon: " << this_month_sym << " Next: " << next_next_month_sym << "NN : " << next_next_month_sym
                << std::endl;
      Next23MonthsGCcontract(symbol, current_trading_month_, current_trading_year_, gc_next_23_month_vec);
      Next72MonthsGCcontract(symbol, current_trading_month_, current_trading_year_, gc_next_72_month_vec);

      // Write to the final all symbol vector
      all_symbols.push_back(SymbolInfo(symbol, this_month_sym, 0));
      all_symbols.push_back(SymbolInfo(symbol, next_month_sym, 0));
      all_symbols.push_back(SymbolInfo(symbol, next_next_month_sym, 0));

      for (unsigned int ii = 0; ii < gc_next_72_month_vec.size(); ii++)
        all_symbols.push_back(SymbolInfo(symbol, gc_next_72_month_vec[ii], 0));
      for (unsigned int ii = 0; ii < gc_next_23_month_vec.size(); ii++)
        all_symbols.push_back(SymbolInfo(symbol, gc_next_23_month_vec[ii], 0));

      // Push to the original map
      cme_base_symbol_to_all_symbols_map[symbol] = all_symbols;
    }

    // Coal Future
    if (strcmp(cme_base_symbols_[ii].c_str(), CME_CL) == 0) {
      std::string symbol(cme_base_symbols_[ii]);

      // consecutive months are listed for the current year and the next five years;
      // in addition, the June and December contract months are listed beyond the sixth year.
      // Take 15 months
      for (int ii = 0; ii < 15; ii++) {
        std::string sym = NextNthMonthCME(symbol, current_trading_month_, current_trading_year_, ii);
        all_symbols.push_back(SymbolInfo(symbol, sym, 0));
      }
      // Push to the original map
      cme_base_symbol_to_all_symbols_map[symbol] = all_symbols;

    } else {
      // Consider  5 contracts
      std::string symbol(cme_base_symbols_[ii]);
      std::vector<std::string> just_symbols;
      GetNextNCMEContracts(symbol, current_trading_month_, current_trading_year_, just_symbols, 5);
      for (unsigned int ii = 0; ii < just_symbols.size(); ii++)
        all_symbols.push_back(SymbolInfo(symbol, just_symbols[ii], 0));

      // Push to the original map
      cme_base_symbol_to_all_symbols_map[symbol] = all_symbols;
    }
  }
}

std::string VolumeSymbolManager::NextMonthCME(const std::string& baseename, int current_trading_month,
                                              int current_trading_year) {
  if (current_trading_month == 12) {
    current_trading_month = 1;
    current_trading_year++;

  } else
    current_trading_month++;

  std::stringstream ss;
  ss << baseename << CMEMonthCode[current_trading_month - 1] << current_trading_year;
  return ss.str();
}

std::string VolumeSymbolManager::NextNthMonthCME(const std::string& baseename, int current_trading_month,
                                                 int current_trading_year, int N) {
  int ii = 1;
  do {
    if (current_trading_month == 12) {
      current_trading_month = 1;
      current_trading_year++;

    } else
      current_trading_month++;

    ii++;
  } while (ii <= N);

  std::stringstream ss;
  ss << baseename << CMEMonthCode[current_trading_month - 1] << current_trading_year;
  return ss.str();
}
std::string VolumeSymbolManager::ThisMonthCME(const std::string& baseename, int current_trading_month,
                                              int current_trading_year) {
  std::stringstream ss;
  ss << baseename << CMEMonthCode[current_trading_month - 1] << current_trading_year;
  return ss.str();
}

// Any Feb, April, August, OCtober in next 23 months Used only for GC Contracts
void VolumeSymbolManager::Next23MonthsGCcontract(const std::string& baseename, int current_trading_month,
                                                 int current_trading_year, std::vector<std::string>& sym_name)

{
  sym_name.clear();
  for (unsigned int ii = 1; ii <= 23; ii++) {
    current_trading_month++;

    if (current_trading_month == 13) {
      current_trading_month = 1;
      current_trading_year++;
    }

    if (current_trading_month == 2 || current_trading_month == 4 || current_trading_month == 8 ||
        current_trading_month == 10) {
      {
        std::stringstream ss;
        ss << baseename << CMEMonthCode[current_trading_month - 1] << current_trading_year;
        sym_name.push_back(ss.str());
      }
    }
  }
}

// Any June and December for next 72 months Used only for GC contracts
void VolumeSymbolManager::Next72MonthsGCcontract(const std::string& baseename, int current_trading_month,
                                                 int current_trading_year, std::vector<std::string>& sym_name)

{
  sym_name.clear();
  for (unsigned int ii = 1; ii <= 72; ii++) {
    current_trading_month++;

    if (current_trading_month == 13) {
      current_trading_month = 1;
      current_trading_year++;
    }

    if (current_trading_month == 6 || current_trading_month == 12) {
      {
        std::stringstream ss;
        ss << baseename << CMEMonthCode[current_trading_month - 1] << current_trading_year;
        sym_name.push_back(ss.str());
      }
    }
  }
}

// Take N next contracts assuming the normal CME cycle of MAR, JUN, SEPT, DEC
void VolumeSymbolManager::GetNextNCMEContracts(const std::string& baseename, int current_trading_month,
                                               int current_trading_year, std::vector<std::string>& sym_name,
                                               int num_contracts)

{
  sym_name.clear();
  unsigned int ii = 1;
  while (ii <= (unsigned int)num_contracts) {
    // We want to consider if the current month is the contract month
    // E.g for 20110601 or 20110619 consider say ESM1 and not skip to ESU1
    if (current_trading_month == 3 || current_trading_month == 6 || current_trading_month == 12 ||
        current_trading_month == 9) {
      {
        std::stringstream ss;
        ss << baseename << CMEMonthCode[current_trading_month - 1] << current_trading_year;
        sym_name.push_back(ss.str());
        //	    fprintf(stdout, "Consider : %s \n",  ss. str ( ).c_str() );
      }
      ii++;
    }

    current_trading_month++;

    if (current_trading_month == 13) {
      current_trading_month = 1;
      current_trading_year++;
    }
  }
}

// The input vector contains ESU1, ESZ1, ESU2, etc
void VolumeSymbolManager::computeVolumesForSymbol(const std::string this_base_symbol, const int input_date_,
                                                  const int begin_secs_from_midnight_,
                                                  const int end_secs_from_midnight_) {
  // VolToSymMap volume_to_symbol_map;
  // VolToSymMapIter it;

  std::vector<SymbolInfo> allsyms_for_this_base_sym;
  cme_base_symbol_to_all_symbols_map_iter = cme_base_symbol_to_all_symbols_map.find(this_base_symbol);
  if (cme_base_symbol_to_all_symbols_map_iter != cme_base_symbol_to_all_symbols_map.end()) {
    allsyms_for_this_base_sym = cme_base_symbol_to_all_symbols_map_iter->second;
  } else {
    std::cout << " Base Symbol : " << this_base_symbol << " Not in the Map ?? " << std::endl;
    return;
  }

  for (unsigned int ii = 0; ii < allsyms_for_this_base_sym.size(); ii++) {
    HFSAT::TradingLocation_t trading_location_file_read_;
    HFSAT::BulkFileReader bulk_file_reader_;

    int traded_volume_ = 0;

    const char* t_exchange_symbol_ = allsyms_for_this_base_sym[ii].symbol_.c_str();

    std::string t_cme_filename_ =
        HFSAT::CMELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
    std::cout << " Filename : " << t_cme_filename_ << std::endl;
    CME_MDS::CMECommonStruct next_event_;
    bulk_file_reader_.open(t_cme_filename_);

    if (!bulk_file_reader_.is_open())
      traded_volume_ = -1;
    else {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMECommonStruct));
        if (available_len_ >= sizeof(CME_MDS::CMECommonStruct)) {  // data found

          if (next_event_.msg_ == CME_MDS::CME_TRADE) {  // This is a trade message, update the total_traded_volume_
                                                         // based on this trd vol, if this falls within our time range.
            if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&  // 86400 = 24 hrs * 60 mins * 60 secs.
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              traded_volume_ += next_event_.data_.cme_trds_.trd_qty_;
            }
          }
        } else {
          break;
        }
      }
    }

    allsyms_for_this_base_sym[ii].traded_volume_ = traded_volume_;
  }

  if (allsyms_for_this_base_sym.size() > 0) {
    std::sort(allsyms_for_this_base_sym.begin(), allsyms_for_this_base_sym.end());
  }

  // Put back the sorted vector as new updated map
  cme_base_symbol_to_all_symbols_map[this_base_symbol] = allsyms_for_this_base_sym;

  // Printing info
  for (unsigned int ii = 0; ii < allsyms_for_this_base_sym.size(); ii++) {
    std::cout << allsyms_for_this_base_sym[ii].symbol_ << " : " << allsyms_for_this_base_sym[ii].traded_volume_
              << std::endl;
  }
  std::cout << "*************************************" << std::endl;
}

void VolumeSymbolManager::computeVolumesForAllSymbolsCME(const int input_date_, const int begin_secs_from_midnight_,
                                                         const int end_secs_from_midnight_) {
  for (unsigned int ii = 0; ii < cme_base_symbols_.size(); ii++) {
    computeVolumesForSymbol(cme_base_symbols_[ii], input_date_, begin_secs_from_midnight_, end_secs_from_midnight_);
  }
}

void VolumeSymbolManager::dumpToFileCMEContracts(int num_imp_contracts) {
  std::ofstream vfile;
  vfile.open(volume_based_exchnage_symbol_filename_.c_str(), std::ofstream::out);

  std::cout << " FIle: " << volume_based_exchnage_symbol_filename_ << std::endl;
  if (vfile.is_open()) {
    std::cout << "File Good to Write..Dumpinggg Volume Infos" << std::endl;
    for (unsigned int ii = 0; ii < cme_base_symbols_.size(); ii++) {
      cme_base_symbol_to_all_symbols_map_iter = cme_base_symbol_to_all_symbols_map.find(cme_base_symbols_[ii]);

      if (cme_base_symbol_to_all_symbols_map_iter != cme_base_symbol_to_all_symbols_map.end()) {
        std::vector<SymbolInfo> this_symbols_vec = cme_base_symbol_to_all_symbols_map_iter->second;

        std::map<int, std::string> this_symbols_map;
        pushInMapForSorting(this_symbols_vec, this_symbols_map);

        std::map<int, std::string>::reverse_iterator
            this_symbols_map_iter;  // This is so that we can traverse the map from highest vol. to lowest vol.
        int jj = 0;
        for (this_symbols_map_iter = this_symbols_map.rbegin();
             this_symbols_map_iter != this_symbols_map.rend() && (jj < num_imp_contracts);
             jj++, this_symbols_map_iter++) {
          {
            std::stringstream ss;
            ss << cme_base_symbols_[ii] << "_" << jj;
            vfile << ss.str() << "\t" << this_symbols_map_iter->second << "\t" << this_symbols_map_iter->first << "\n";
          }
        }
      }
    }
  }

  vfile.close();
}

void VolumeSymbolManager::pushInMapForSorting(std::vector<SymbolInfo>& sym_vec, std::map<int, std::string>& sym_map) {
  for (unsigned int ii = 0; ii < sym_vec.size(); ii++) {
    sym_map[sym_vec[ii].traded_volume_] = sym_vec[ii].symbol_;
  }
}
