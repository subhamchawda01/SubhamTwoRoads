/**
    \file MToolsExe/get_dep_median_corr.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */

#include <stdio.h>

#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "dvccode/CDef/math_utils.hpp"
#include "basetrade/MTools/iterative_regress.hpp"

#include "basetrade/MToolsExe/call_read_reg_data.hpp"
#include "basetrade/MToolsExe/mtools_utils.hpp"

using namespace HFSAT;

/// Exec used to read a matrix of data and print the vector of median correlations with the first column
/// Reads command line arguments
/// Print Median Correlations with Dependant
int main(int argc, char** argv) {
  // local variables
  std::vector<double> train_data_dependant_;
  std::vector<std::vector<double> > train_data_independants_;

  std::string infilename_ = "";

  unsigned int num_folds_ = 5;

  // command line processing
  if (argc < 3) {
    std::cerr << "USAGE: " << argv[0] << " input_reg_data_file_name  nfolds" << std::endl;
    exit(0);
  }

  infilename_ = argv[1];
  num_folds_ = std::max(2u, std::min(30u, (unsigned int)atoi(argv[2])));

  // read data
  ReadData(infilename_, train_data_dependant_, train_data_independants_);

  std::vector<double> correlation_dep_indep_(train_data_independants_.size(), 0.0);

  for (unsigned int indep_index_ = 0; indep_index_ < correlation_dep_indep_.size(); indep_index_++) {
    double this_median_correlation_ =
        GetMedianNoMeanCorrelation(train_data_dependant_, train_data_independants_[indep_index_], num_folds_);

    std::cout << this_median_correlation_;
    if ((indep_index_ + 1) < correlation_dep_indep_.size()) {
      std::cout << ' ';
    }
  }
  std::cout << std::endl;
}
