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
  std::map<std::string, long double> shc_to_symbol_map_;
  long double total_time_ = 0;
  long double tv;
  std::string name;
  while (filename_1 >> name >> token >> msg_seq_no >> tv >> aprice >> aquantity >> bprice >> bquantity) {
    token = "";
    std::string id_ = token + msg_seq_no;
    shc_to_symbol_map_[id_] = tv;
   //  std::cout << std::fixed << std::setprecision(9) << id_ << " "<< tv  <<  " " <<  aprice << " " << tv - aprice <<  std::endl;
//     std::cout << std::fixed<< "ID1: " << id_ << " "<< tv  << " " << shc_to_symbol_map_[id_]<< std::endl;
    // std::cout << "READ1" << tv << std::endl;
  }
  std::cout << "Read Entries " << shc_to_symbol_map_.size() << std::endl;
 //  for(auto it = shc_to_symbol_map_.begin();
 //   it != shc_to_symbol_map_.end(); ++it)
// std::cout << it->first << " " << it->second << "\n";
  int count_faster_1 = 0, count_faster_2 = 0;
  while (filename_2 >> name >> token >> msg_seq_no >> tv >> aprice >> aquantity >> bprice >> bquantity) {
    token = "";
    std::string id_ = token + msg_seq_no;
    std::cout << "ID2: " << id_ << std::endl;
    if (shc_to_symbol_map_.find(id_) != shc_to_symbol_map_.end()) {
      long double val = shc_to_symbol_map_[id_] - tv;
      total_time_ += val;
//      if (val != 0) std::cout <<std::fixed<< id_ << " " << shc_to_symbol_map_[id_] << " - " << tv << " "<< std::fixed << val << std::endl;
    //  std::cout << "Latency Number for: " << id_ << " " << shc_to_symbol_map_[id_] << " - " << tv << " " << val
    //            << std::endl;
      latency_numbers.push_back(val);
      if (abs(val) > 0.01) 
        std::cout<<"Toverify " << msg_seq_no << " Val " << val <<std::endl;
      if (val > 0)
        count_faster_2++;
      else if (val < 0)
        count_faster_1++;
    }
  }
  std::cout<<"Total Time" << total_time_ << "Size " << latency_numbers.size() << std::endl;
  std::sort(latency_numbers.begin(), latency_numbers.end());
  // std::cout << " Token :" << token << "  ";
  std::cout << "Sample Size:\t"
            << "Mean:\t"
            << "01%:\t"
            << "05%:\t"
            << "10%:\t"
            << "25%:\t"
            << "50%:\t"
            << "75%:\t"
            << "90%:\t"
            << "95%:\t"
            << "99%:\t"
            << "Min:\t"
            << "Max:\t"
            << "1st_Fast\t"
            << "2nd_Fast\t" << std::endl;
  std::cout << std::fixed << std::setprecision(10) << latency_numbers.size() << "\t";

  std::cout << ((long double)total_time_ / latency_numbers.size()) * 1000000 << "\t";
  std::cout << latency_numbers[(latency_numbers.size() * 1) / 100] * 1000000 << "\t";
  std::cout << latency_numbers[(latency_numbers.size() * 5) / 100] * 1000000 << "\t";
  std::cout << latency_numbers[(latency_numbers.size() * 10) / 100] * 1000000 << "\t";
  std::cout << latency_numbers[(latency_numbers.size() * 25) / 100] * 1000000 << "\t";
  std::cout << latency_numbers[(latency_numbers.size() * 50) / 100] * 1000000 << "\t";
  std::cout << latency_numbers[(latency_numbers.size() * 75) / 100] * 1000000 << "\t";
  std::cout << latency_numbers[(latency_numbers.size() * 90) / 100] * 1000000 << "\t";
  std::cout << latency_numbers[(latency_numbers.size() * 95) / 100] * 1000000 << "\t";
  std::cout << latency_numbers[(latency_numbers.size() * 99) / 100] * 1000000 << "\t";
  std::cout << latency_numbers[0] * 1000000 << "\t";
  std::cout << latency_numbers[latency_numbers.size() - 1] * 1000000 << "\t";
  std::cout << count_faster_1 << "\t";
  std::cout << count_faster_2 << "\n";
  // shc_to_symbol_map_[token + msg_seq_no] = tv;
  return 0;
}
