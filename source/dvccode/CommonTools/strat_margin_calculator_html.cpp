#include "dvccode/SpanMargin/SpanMargin.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <ctime> 

// void read_from_strat_file(std::string file_name, HFSAT::SPANMargin& margin) {
//   std::ifstream file;
//   file.open(file_name, std::ios::in);
// 
//   if( !file )
//     std::cerr << "Cant open " << file_name << std::endl;
// 
//   std::string line;
//   
//   while(std::getline(file, line)) {
//     std::stringstream ss(line);
//     std::string dummy;
//     std::string sc = "";
//     std::string time_;
//     std::string buysell;
//     int positions = 0;
//     int t_positions = 0;
// 
//     if (ss >> time_ >> dummy >> sc >> buysell >> positions >> dummy >> t_positions) {
//       if (buysell == "S")
//         positions *= -1;
//       
//       std::stringstream ss_sc(sc);
//       std::getline(ss_sc, dummy, '.');
//       sc = dummy;
// 
//       margin.add(sc, positions);      
//     }
//   }
// }
// 
// void get_positions(HFSAT::SPANMargin& margin) {
//   margin.getPositions();
// }

int main(int argc, char** argv) {
//   // tradingdate as int.
//   int tradingdate_ = atoi(argv[1]);
// 
//   HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
// 
//   HFSAT::SPANMargin margin(tradingdate_);
// 
//   for (int i = 2; i < argc; i++) {
//     std::istringstream ss(argv[i]);
//     std::string token;
//     std::string strat_id_;
//     std::string strat_id;
// 
//     std::getline(ss, strat_id_, '.');
// 
//     std::istringstream ss_(strat_id_);
//     
//     while(std::getline(ss_, strat_id, '/'));
//     
//     while(std::getline(ss, token, '.'));
// 
//     read_from_strat_file(argv[i], margin);
//     // std::cout << token << ": " << std::fixed << std::setprecision(2) << margin.getMargin() / 10000000.00 << "\n";
//     
//     std::cout << "{\"timestamp\": " << token
//               << ", \"strat_id\": " << strat_id
//               << ", \"margin\": " << std::fixed << std::setprecision(4) << margin.getMargin() / 10000000.00
//               << "},\n";
// 
//     margin.clearPositionsMap();
//   }

  return 0;
}
