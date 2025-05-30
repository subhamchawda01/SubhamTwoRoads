/**
    \file MToolsExe/get_dep_corr.cpp

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

/// Exec used to read a matrix of data and print the correlation matrix
/// Reads command line arguments
/// Compute mean & stdev of all independants
/// Normalize independants
/// Print CorrelationMatrix
/// USAGE ~/basetrade_install/bin/get_dep_corr <filename> fshr_3_10
void UseFSHLRfilterAndFindCorrMatrix(std::string filename_, std::string filter_arg) {
  // read regfile
  std::vector<double> dependant_;
  std::vector<std::vector<double> > independants_;
  ReadData(filename_, dependant_, independants_);

  float min_cutoff = 0.0, max_cutoff = 10.0;
  // parse filter_arg
  sscanf(filter_arg.c_str(), "fshlr_%f_%f", &min_cutoff, &max_cutoff);

  int num_ind = independants_.size();
  std::vector<double> independants_mean_(num_ind, 0), independants_stdev_(num_ind, 1);
  // for analysis only
  std::vector<int> count(independants_.size(), 0);
  VectorUtils::CalcNonZeroMeanStdevVec(independants_, independants_mean_, independants_stdev_);
  VectorUtils::CalcAndRemoveMeanFromSeriesVec(independants_);
  VectorUtils::CalcAndRemoveMeanFromSeries(dependant_);
  std::vector<double> indep_dep_prod_(independants_.size(), 0), indep_filtered_sumsquare_(independants_.size(), 0);
  std::vector<double> dep_filtered_sumsquare_(independants_.size(), 0), corr_vec(independants_.size(), 0);
  for (auto i = 0u; i < dependant_.size(); i++)
    for (unsigned int j = 0; j < independants_.size(); j++)
      if (fabs(independants_[j][i]) > independants_stdev_[j] * min_cutoff &&
          fabs(independants_[j][i]) < independants_stdev_[j] * max_cutoff) {
        indep_dep_prod_[j] += independants_[j][i] * dependant_[i];
        indep_filtered_sumsquare_[j] += independants_[j][i] * independants_[j][i];
        dep_filtered_sumsquare_[j] += dependant_[i] * dependant_[i];
        count[j]++;
      }
  for (auto i = 0u; i < independants_.size(); i++) {
    corr_vec[i] =
        indep_dep_prod_[i] / sqrt(indep_filtered_sumsquare_[i] *
                                  dep_filtered_sumsquare_[i]);  // have to check the formula. divded by n or n-1
    if (std::isnan(corr_vec[i])) corr_vec[i] = 0;
  }

  auto i = 0u;
  for (i = 0; i < corr_vec.size() - 1; i++) std::cout << corr_vec[i] << " ";
  std::cout << corr_vec[i] << std::endl;

  for (i = 0; i < corr_vec.size() - 1; i++) printf("%-8d ", count[i]);
  printf("%-8d\n", count[i]);

  return;
}

int main(int argc, char** argv) {
  // local variables
  int lines_read_ = 0;

  double dep_sum_ = 0;
  std::vector<double> indep_sum_;
  double dep_squared_sum_ = 0;
  std::vector<double> indep_squared_sum_;
  std::vector<double> dep_indep_sum_;

  std::string infilename_ = "";

  // command line processing
  if (argc < 2) {
    std::cerr << argv[0] << " input_file_name " << std::endl;
    exit(0);
  }

  infilename_ = argv[1];

  // use FSHLR filter
  if (argc > 2) {
    UseFSHLRfilterAndFindCorrMatrix(infilename_, argv[2]);
    return 0;
  }

  // read data
  {
    std::ifstream infile_;
    infile_.open(infilename_.c_str());
    if (!infile_.is_open()) {
      std::cerr << " Input data file " << infilename_ << " did not open " << std::endl;
      exit(kCallIterativeRegressInFileOpenError);
    }

    const unsigned int kLineBufferLen = 10240;
    char readline_buffer_[kLineBufferLen];
    bzero(readline_buffer_, kLineBufferLen);
    int num_independants_ = -1;

    while (infile_.good()) {
      bzero(readline_buffer_, kLineBufferLen);
      infile_.getline(readline_buffer_, kLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      // simple file format : "DEPENDANT INDEP_1 INDEP_2 ... INDEP_V"

      if (tokens_.size() <= 0) continue;

      if (num_independants_ == -1) {  // first line
        num_independants_ = tokens_.size() - 1;
        if (num_independants_ < 1) {
          infile_.close();

          std::cerr << "First line has very few tokens " << tokens_.size() << " should be at least 2 " << std::endl;
          exit(kCallIterativeRegressFewTokens);
        }

        indep_sum_.resize(num_independants_, 0);
        indep_squared_sum_.resize(num_independants_, 0);
        dep_indep_sum_.resize(num_independants_, 0);
      }

      if ((1 + num_independants_) !=
          (int)(tokens_.size()))  // make sure this line has the number of words we are expecting
      {
        infile_.close();
        std::cerr << " A line in the input data has a different number of tokens " << tokens_.size()
                  << " than the first " << (1 + num_independants_) << " for " << infilename_ << std::endl;
        exit(kCallIterativeRegressDiffNumTokensError);
      } else {
        lines_read_++;
        double this_dep = atof(tokens_[0]);
        dep_sum_ += this_dep;
        dep_squared_sum_ += HFSAT::GetSquareOf(this_dep);

        for (auto i = 0u; i < indep_sum_.size(); i++) {
          double this_indep = atof(tokens_[i + 1]);
          indep_sum_[i] += this_indep;
          indep_squared_sum_[i] += HFSAT::GetSquareOf(this_indep);
          dep_indep_sum_[i] += this_dep * this_indep;
        }
      }
    }
    infile_.close();
  }
  std::vector<double> correlation_dep_indep_(indep_squared_sum_.size(), 0);
  for (auto i = 0u; i < correlation_dep_indep_.size(); i++) {
    correlation_dep_indep_[i] =
        ((double(lines_read_) * dep_indep_sum_[i]) - (dep_sum_ * indep_sum_[i])) /
        (sqrt((double(lines_read_) * dep_squared_sum_) - HFSAT::GetSquareOf(dep_sum_)) *
         sqrt((double(lines_read_) * indep_squared_sum_[i]) - HFSAT::GetSquareOf(indep_sum_[i])));

    if (std::isnan(correlation_dep_indep_[i])) {
      correlation_dep_indep_[i] = 0;
    }
  }

  for (auto i = 0u; i < correlation_dep_indep_.size(); i++) {
    std::cout << correlation_dep_indep_[i];
    if ((i + 1) < correlation_dep_indep_.size()) {
      std::cout << ' ';
    }
  }
  std::cout << std::endl;
}
