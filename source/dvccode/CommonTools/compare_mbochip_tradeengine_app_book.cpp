#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cout << "Usage : <exec> <file1> <file2> \n";
    exit(0);
  }
  std::string file1 = argv[1];
  std::string file2 = argv[2];
  std::ifstream filename_1(file1);
  std::ifstream filename_2(file2);
  std::string token, msg_seq_no,aprice, aquantity, bprice, bquantity;
  std::vector<long double> latency_numbers;
  std::map<std::string, std::string> shc_to_symbol_map_;
  long double tv;
  std::string name;
  while (filename_1 >> name >> token >> msg_seq_no >> tv >> aprice >> aquantity >> bprice >> bquantity) {
    std::string id_ = token + msg_seq_no;
    shc_to_symbol_map_[id_] = aprice + " " + aquantity + " " + bprice + " " + bquantity;
   //  std::cout << std::fixed << std::setprecision(9) << id_ << " "<< tv  <<  " " <<  aprice << " " << tv - aprice <<  std::endl;
//     std::cout << std::fixed<< "ID1: " << id_ << " "<< tv  << " " << shc_to_symbol_map_[id_]<< std::endl;
    // std::cout << "READ1" << tv << std::endl;
  }
  std::cout << "Read Entries " << shc_to_symbol_map_.size() << std::endl;
 //  for(auto it = shc_to_symbol_map_.begin();
 //   it != shc_to_symbol_map_.end(); ++it)
// std::cout << it->first << " " << it->second << "\n";
  int count  = 0;
  while (filename_2 >> name >> token >> msg_seq_no >> tv >> aprice >> aquantity >> bprice >> bquantity) {
    std::string id_ = token + msg_seq_no;
//  std::cout << "ID2: " << id_ << std::endl;
    if (shc_to_symbol_map_.find(id_) != shc_to_symbol_map_.end()) {
      std::string compare_string = aprice + " " + aquantity + " " + bprice + " " + bquantity;
      if (shc_to_symbol_map_[id_] != compare_string){
          std::cout << "Not Equal " << token << " " << msg_seq_no << " " << std::fixed << tv << " Book1 " << shc_to_symbol_map_[id_] << " Book2 " << compare_string << std::endl;
      }
      count++;
    }
  }
  std::cout << "Book Compared " << count << std::endl;
  return 0;
}
