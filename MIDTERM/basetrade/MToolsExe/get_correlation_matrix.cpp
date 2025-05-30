/**
    \file MToolsExe/get_correlation_matrix.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include "basetrade/Math/matrix_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "basetrade/MToolsExe/call_read_reg_data.hpp"

/// Exec used to read a matrix of data and print the correlation matrix
/// Reads command line arguments
/// Compute mean & stdev of all independants
/// Normalize independants
/// Print CorrelationMatrix
int main(int argc, char** argv) {
  // local variables
  std::vector<std::vector<double> > t_in_matrix_;
  std::string infilename_ = "";

  // command line processing
  if (argc < 2) {
    std::cerr << argv[0] << " input_file_name " << std::endl;
    exit(0);
  }

  infilename_ = argv[1];

  // read data
  HFSAT::ReadInData(infilename_, t_in_matrix_);
  HFSAT::VectorUtils::CalcAndRemoveMeanFromSeriesVec(t_in_matrix_);
  HFSAT::SquareMatrix<double> covar_matrix_(t_in_matrix_.size());

  HFSAT::MatrixUtils::GetCovarianceMatrix(t_in_matrix_, covar_matrix_);

  HFSAT::MatrixUtils::CovarToCorrMatrix(covar_matrix_);

  std::cout << covar_matrix_.ToString4();
}
