/**
    \file MToolsExe/remove_outliers.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include <math.h>
#include <algorithm>

#include "basetrade/MToolsExe/call_read_reg_data.hpp"

using namespace HFSAT;

/// Exec used to remove outliers
/// Reads command line arguments ( reg_data_filename reg_data_output_filename dep_outlier_percentile
/// indep_outlier_percentile )
int main(int argc, char** argv) {
  // local variables
  std::vector<double> train_data_dependant_;
  std::vector<std::vector<double> > train_data_independants_;

  std::string reg_data_input_filename_ = "";
  bool first_indep_is_weight_ = false;
  double dep_outlier_percentile_ = 0.90;
  double indep_outlier_percentile_ = 0.90;
  std::string reg_data_output_filename_ = "";

  // command line processing
  if (argc < 6) {
    std::cerr << argv[0] << " reg_data_input_filename first_indep_is_weight_ dep_outlier_percentile "
                            "indep_outlier_percentile reg_data_output_filename " << std::endl;
    exit(0);
  }

  reg_data_input_filename_ = argv[1];
  first_indep_is_weight_ = (atoi(argv[2]) != 0);
  dep_outlier_percentile_ = (std::max(0.1, std::min(99.9, atof(argv[3]))) / 100.00);
  indep_outlier_percentile_ = (std::max(0.1, std::min(99.9, atof(argv[4]))) / 100.00);
  reg_data_output_filename_ = argv[5];

  // read data
  ReadData(reg_data_input_filename_, train_data_dependant_, train_data_independants_);

  unsigned int dep_min_acceptable_index_ = (int)round((1 - dep_outlier_percentile_) * train_data_dependant_.size());
  unsigned int dep_max_acceptable_index_ = (int)round((dep_outlier_percentile_)*train_data_dependant_.size());

  unsigned int indep_min_acceptable_index_ = (int)round((1 - indep_outlier_percentile_) * train_data_dependant_.size());
  unsigned int indep_max_acceptable_index_ = (int)round((indep_outlier_percentile_)*train_data_dependant_.size());

  // for dependant, and independants
  // sort all data
  sort(train_data_dependant_.begin(), train_data_dependant_.end());
  for (auto i = 0u; i < train_data_independants_.size(); i++) {
    if (first_indep_is_weight_)
      if (i == 0) continue;
    sort(train_data_independants_[i].begin(), train_data_independants_[i].end());
  }

  // find the value of 100*(1-dep_outlier_percentile_) and (100*dep_outlier_percentile_)
  double dep_min_acceptable_value_ = train_data_dependant_[dep_min_acceptable_index_];
  double dep_max_acceptable_value_ = train_data_dependant_[dep_max_acceptable_index_];

  std::vector<double> indep_min_acceptable_value_(train_data_independants_.size(), 0);
  std::vector<double> indep_max_acceptable_value_(train_data_independants_.size(), 0);

  for (auto i = 0u; i < train_data_independants_.size(); i++) {
    if (first_indep_is_weight_)
      if (i == 0) continue;
    indep_min_acceptable_value_[i] = train_data_independants_[i][indep_min_acceptable_index_];
    indep_max_acceptable_value_[i] = train_data_independants_[i][indep_max_acceptable_index_];
  }

  // read data
  {
    std::ifstream infile_;
    std::ofstream outfile_(reg_data_output_filename_.c_str());

    infile_.open(reg_data_input_filename_.c_str());
    if (!infile_.is_open()) {
      std::cerr << " Input data file " << reg_data_input_filename_ << " did not open " << std::endl;
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
      }

      if ((1 + num_independants_) !=
          (int)(tokens_.size()))  // make sure this line has the number of words we are expecting
      {
        infile_.close();
        std::cerr << " A line in the input data has a different number of tokens " << tokens_.size()
                  << " than the first " << (1 + num_independants_) << std::endl;
        exit(kCallIterativeRegressDiffNumTokensError);
      } else {
        double this_dep_value_ = atof(tokens_[0]);
        if (this_dep_value_ > dep_max_acceptable_value_) {
          outfile_ << dep_max_acceptable_value_;
        } else if (this_dep_value_ < dep_min_acceptable_value_) {
          outfile_ << dep_min_acceptable_value_;
        } else {
          outfile_ << this_dep_value_;
        }

        for (int i = 0; i < num_independants_; i++) {
          if (i == 0) {
            if (first_indep_is_weight_) {
              outfile_ << ' ' << tokens_[i + 1];
              continue;
            }
          }

          double this_indep_value_ = atof(tokens_[i + 1]);

          if (this_indep_value_ > indep_max_acceptable_value_[i]) {
            outfile_ << ' ' << indep_max_acceptable_value_[i];
          } else if (this_indep_value_ < indep_min_acceptable_value_[i]) {
            outfile_ << ' ' << indep_min_acceptable_value_[i];
          } else {
            outfile_ << ' ' << tokens_[i + 1];
          }
        }
        outfile_ << std::endl;
      }
    }
    infile_.close();
    outfile_.close();
  }

  return 0;
}
