/**
    \file Tools/symbol_driver_main.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717

*/

// Volume Based Symbol Manager
// Idea is to count the volumes for day-1
// sort by volume and then 0-points to highest, 1-points to second higest etc

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <vector>

#include "basetrade/Tools/symbol_allocator.hpp"

void ParseCommandLineParams(const int argc, const char** argv, int& input_date_, int& begin_secs_from_midnight_,
                            int& end_secs_from_midnight_) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " input_date_YYYYMMDD [ start_tm_utc_hhmm ] [ end_tm_utc_hhmm ]" << std::endl;
    exit(0);
  } else {
    input_date_ = atoi(argv[1]);

    if (argc > 3) {
      begin_secs_from_midnight_ = (atoi(argv[3]) / 100) * 60 * 60 + (atoi(argv[2]) % 100) * 60;
    }
    if (argc > 4) {
      end_secs_from_midnight_ = (atoi(argv[4]) / 100) * 60 * 60 + (atoi(argv[3]) % 100) * 60;
    }
  }
}

/// input arguments : input_date
int main(int argc, char** argv) {
  std::string shortcode_ = "";
  int input_date_ = 20110101;
  int begin_secs_from_midnight_ = 0;
  int end_secs_from_midnight_ = 24 * 60 * 60;
  ParseCommandLineParams(argc, (const char**)argv, input_date_, begin_secs_from_midnight_, end_secs_from_midnight_);

  /// Open A file for writing volume based Symbol result
  std::ostringstream t_temp_stream;
  t_temp_stream << "/spare/local/"
                << "VolumeBasedSymbol/VOSymbol_" << input_date_ << ".txt";
  std::string volume_based_exchnage_symbol_filename = t_temp_stream.str();

  std::ofstream volume_based_exchnage_symbol_file;
  volume_based_exchnage_symbol_file.open(volume_based_exchnage_symbol_filename.c_str(), std::ios::out);
  std::cout << "File to Write : " << volume_based_exchnage_symbol_filename << std::endl;

  // Just doing for CME Products for now
  {
    VolumeSymbolManager volume_sym_manager(input_date_, begin_secs_from_midnight_, end_secs_from_midnight_,
                                           volume_based_exchnage_symbol_filename);
    volume_sym_manager.populate_cme_base_symbols();           // Put all the base symbols
    volume_sym_manager.populateAllSymbolsWithContractsCME();  // For each  base symbol compute all possible contracts

    volume_sym_manager.computeVolumesForAllSymbolsCME(input_date_, begin_secs_from_midnight_, end_secs_from_midnight_);

    volume_sym_manager.dumpToFileCMEContracts(3);  // Dump _0, _1, _2 for each base symbol
  }

  // TODO for EUREX, TMX, BMF etc
}
