#include <iostream>
#include <string>
#include <unistd.h>

#include "basetrade/SqlCpp/SqlCppUtils.hpp"

using namespace HFSAT;

int main(int argc, char** argv) {
  int mode = 0;
  int min_date = 0;
  int max_date = 0;
  std::vector<int> date_vec;

  if (argc < 4) {
    std::cerr << "USAGE: " << argv[0] << " mode[0(test_dates)/1(train_dates)] min_date max_date" << std::endl;
    exit(0);
  }
  mode = atoi(argv[1]);
  min_date = atoi(argv[2]);
  max_date = atoi(argv[3]);
  ResultsAccessManager& ram_ = ResultsAccessManager::GetUniqueInstance();
  ram_.FetchDates(mode, min_date, max_date, date_vec);
  for (std::vector<int>::const_iterator i = date_vec.begin(); i != date_vec.end(); ++i) {
    std::cerr << *i << std::endl;
  }
}
