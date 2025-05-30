// =====================================================================================
// 
//       Filename:  process.cpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  09/05/2024 07:55:55 AM
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

#include <cstdlib>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include <iostream>
#include <algorithm>
#include <set>


int main ( int argc, char *argv[] )
{

  std::cout << argv[1] << " " << argv[2] << std::endl;

  std::ifstream map_file;

  map_file.open(argv[1], std::ifstream::in);
  std::map<std::string, std::string> exch_to_datasym;

  char buffer[1024];
  while(map_file.good()){
    map_file.getline(buffer, 1024);

    HFSAT::PerishableStringTokenizer st(buffer, sizeof(buffer));
    const std::vector<const char *> &tokens = st.GetTokens();

    if(tokens.size() < 2)continue;

    exch_to_datasym[tokens[0]] = tokens[1];

    std::cout << tokens[0] << " " << tokens[1] << std::endl;
  }
  map_file.close();

  std::cout << "Size of symbols: " << exch_to_datasym.size() << std::endl;

  std::ifstream data_file;
  data_file.open(argv[2],std::ifstream::in);

  std::map<std::string, int> sym_to_netpos;
  std::set<uint64_t> second_seen;

  int total_long_calls = 0;
  int total_short_calls = 0;
  int total_long_puts = 0;
  int total_short_puts = 0;

  while(data_file.good()){
    data_file.getline(buffer, 1024);

    HFSAT::PerishableStringTokenizer st(buffer, sizeof(buffer));
    const std::vector<const char *> &tokens = st.GetTokens();

    if(tokens.size() != 8) continue;
    double time = std::atof(tokens[6]);
    uint64_t int_time = time;

    if(std::string(tokens[0]) == std::string("NSE3432352") || std::string(tokens[0]) == std::string("NSE3432722")) continue;

    //init
    if(sym_to_netpos.find(exch_to_datasym[tokens[0]]) == sym_to_netpos.end()){
      sym_to_netpos[exch_to_datasym[tokens[0]]] = 0;
    }

    //buy
    if(std::atoi(tokens[1]) == 0){
      sym_to_netpos[exch_to_datasym[tokens[0]]] += std::atoi(tokens[2]);
    }else if(std::atoi(tokens[1]) == 1){
      sym_to_netpos[exch_to_datasym[tokens[0]]] -= std::atoi(tokens[2]);
    }

    total_long_calls = 0;
    total_short_calls = 0;
    total_long_puts = 0;
    total_short_puts = 0;

    for(auto itr : sym_to_netpos){

      //call
      if(itr.first.find("_CE_") != std::string::npos){

        if(itr.second>0){
          total_long_calls += itr.second;
        }else{
          total_short_calls += std::abs(itr.second);
        }

      }else if(itr.first.find("_PE_") != std::string::npos){

        if(itr.second>0){
          total_long_puts += itr.second;
        }else{
          total_short_puts += std::abs(itr.second);
        }

      }

    }

    if(second_seen.find(int_time) != second_seen.end()) continue;
    std::cout << "Time : " << int_time << " L_CALL " << total_long_calls/25 << " S_CALL " << total_short_calls/25 << " L_PUT "  << total_long_puts/25 << " S_PUT " << total_short_puts/25 << std::endl;
    second_seen.insert(int_time);

  }
  data_file.close();

  return EXIT_SUCCESS;
}				// ----------  end of function main  ----------
