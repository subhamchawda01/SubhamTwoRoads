/**
   \file Tools/realign_to_quantiles.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
    Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include <iostream>
#include <stdlib.h>

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
using namespace HFSAT;

void GetQuantileSeparators(const std::vector<double>& data, std::vector<double>& toRet) {
  size_t numQ = toRet.size() + 1;

  for (size_t i = 0; i < toRet.size(); ++i) {
    unsigned int data_idx_ = (unsigned int)std::max(0.0, ((double)(data.size() * (i + 1)) / (double)numQ));
    if (data_idx_ < data.size()) {
      toRet[i] = data[data_idx_];
    }
  }
}

void GetQuantileMedians(const std::vector<double>& data, std::vector<double>& toRet) {
  size_t numQ = toRet.size();

  for (size_t i = 0; i < toRet.size(); ++i) {
    unsigned int data_idx_ = (unsigned int)std::max(0.0, ((double)(data.size() * (i + 0.5)) / (double)numQ));
    if (data_idx_ < data.size()) {
      toRet[i] = data[data_idx_];
    }
  }
}

double Realign(double t_value_, const std::vector<double>& t_quantile_separator_vec_,
               const std::vector<double>& t_quantile_median_vec_) {
  if (t_quantile_median_vec_.empty() || (t_quantile_median_vec_.size() <= t_quantile_separator_vec_.size())) {
    return t_value_;
  } else {
    for (size_t i = 0; i < t_quantile_separator_vec_.size(); i++) {
      if (t_value_ < t_quantile_separator_vec_[i]) {
        return t_quantile_median_vec_[i];
      }
    }
    return t_quantile_median_vec_[t_quantile_separator_vec_.size()];  // last one
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << " Usage: " << argv[0] << " input_file output_file num_quantiles\n";
    exit(0);
  }
  std::string input_data_filename_ = argv[1];
  std::string output_data_filename_ = argv[2];
  size_t numQ = (size_t)std::max(2, atoi(argv[3]));  // at least 2 quantiles or 1 separator

  std::vector<std::vector<double> > quantile_separator_vec_;
  std::vector<std::vector<double> > quantile_median_vec_;

  {  // first block to compute quantile separator and median
    HFSAT::BulkFileReader reader;
    reader.open(input_data_filename_.c_str());
    const int BUF_LEN = 8 * 1024;
    char line[BUF_LEN];
    std::vector<std::vector<double> > data;
    while (reader.GetLine(line, BUF_LEN) > 0) {
      std::vector<const char*> toks = PerishableStringTokenizer(line, BUF_LEN).GetTokens();
      if (data.size() == 0) {
        data.resize(toks.size(), std::vector<double>(0));
      }
      while (data.size() < toks.size()) {
        data.push_back(std::vector<double>(0));  // works for non tabular data as well
      }

      for (size_t i = 0; i < toks.size(); ++i) {
        data[i].push_back(atof(toks[i]));
      }
    }

    for (size_t i = 0; i < data.size(); ++i) {
      std::sort(data[i].begin(), data[i].end());

      // for each column compute separator values
      std::vector<double> this_quantile_separator_vec_(numQ - 1, 0);
      GetQuantileSeparators(data[i], this_quantile_separator_vec_);
      quantile_separator_vec_.push_back(this_quantile_separator_vec_);

      // for each column get median values
      std::vector<double> this_quantile_median_vec_(numQ, 0);
      GetQuantileMedians(data[i], this_quantile_median_vec_);
      quantile_median_vec_.push_back(this_quantile_median_vec_);
    }
  }

  {  // process data and realign
    HFSAT::BulkFileReader second_reader_;
    second_reader_.open(input_data_filename_.c_str());
    HFSAT::BulkFileWriter output_writer_(output_data_filename_.c_str(), 1024 * 1024);

    const int BUF_LEN = 8 * 1024;
    char line[BUF_LEN];
    while (second_reader_.GetLine(line, BUF_LEN) > 0) {
      std::vector<const char*> toks = PerishableStringTokenizer(line, BUF_LEN).GetTokens();
      for (size_t i = 0; i < toks.size(); ++i) {
        if (i > 0) {
          output_writer_ << ' ';
        }

        double realigned_value_ = Realign(atof(toks[i]), quantile_separator_vec_[i], quantile_median_vec_[i]);
        output_writer_ << realigned_value_;
      }
      output_writer_ << '\n';
      output_writer_.CheckToFlushBuffer();
    }

    output_writer_.Close();
  }
}
