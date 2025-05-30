/**
   \file results_access_manager.hpp
   Manages access to results mysql database

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "basetrade/SqlCpp/results_db_access_manager.hpp"

// Example Usage
int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "USAGE: exec <SHORTCODE> <DATE>" << std::endl;
    return 0;
  }

  HFSAT::ResultsAccessManager& rsm = HFSAT::ResultsAccessManager::GetUniqueInstance();
  std::vector<std::vector<std::string> > temp_vec;
  rsm.FetchResults(argv[1], atoi(argv[2]), temp_vec);

  for (auto i = 0u; i < temp_vec.size(); i++) {
    /*
    std::cout << temp_vec[i].size() << " ";
    for ( unsigned int j=0 ; j<temp_vec[i].size(); j++)
    {
      std::cout << temp_vec[i][j] << " ";
    }
    std::cout << "\n";
    */

    std::string sname_ = temp_vec[i].front();
    temp_vec[i].erase(temp_vec[i].begin(), temp_vec[i].begin() + 1);

    rsm.InsertResults(sname_, 20150520, temp_vec[i]);
  }

  return EXIT_SUCCESS;
}
