/**
    \file MTools/covariance_mat_inverse.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */

//#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "basetrade/MTools/covariance_mat_inverse.hpp"
#include "dvctrade/linal/linal_util.hpp"
#include "basetrade/MTools/data_processing.hpp"
#include "basetrade/MTools/iterative_regress.hpp"
#include "basetrade/MToolsExe/call_read_reg_data.hpp"
#include "basetrade/MToolsExe/mtools_utils.hpp"

using namespace HFSAT;

int main(int argc, char** argv) {
  // local variables
  std::vector<double> train_data_dependant_;
  std::vector<std::vector<double> > train_data_independants_;
  std::string infilename_ = argv[1];
  // read data
  ReadData(infilename_, train_data_dependant_, train_data_independants_);

  CovMatInv cvm(train_data_independants_);

  for (int i = 0; i < 15; ++i) {
    cvm.AddIndex(i);
    std::cout << "==========\n";
    std::cout << "COV_MAT\n";
    cvm.cov_mat.printToConsole();
    std::cout << "COV_MAT_INV\n";
    cvm.inv_mat.printToConsole();
  }

  return 0;
}
