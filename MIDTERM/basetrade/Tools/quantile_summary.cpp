#include <iostream>
#include <stdlib.h>

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
using namespace HFSAT;

void getQuantiles(std::vector<double>& data, std::vector<double>& toRet, size_t numQ) {
  if (numQ >= toRet.size()) {
    std::sort(data.begin(), data.end());
    for (size_t i = 0; i < numQ; ++i) {
      unsigned int data_idx_ = (unsigned int)std::max(0.0, ((double)(data.size() * (i + 0.5)) / (double)numQ));
      if (data_idx_ < data.size()) {
        toRet[i] = data[data_idx_];
      }
    }
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << " print usage: " << argv[0] << " input_file num_quantiles\n";
    exit(0);
  }

  HFSAT::BulkFileReader reader;
  reader.open(argv[1]);
  size_t numQ = (size_t)atoi(argv[2]);
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
  std::vector<std::vector<double> > quantiles;
  for (size_t i = 0; i < data.size(); ++i) {
    std::vector<double> toRet(numQ, 0);
    getQuantiles(data[i], toRet, numQ);
    quantiles.push_back(toRet);
  }
  if (quantiles.size() > 0) {
    for (size_t i = 0; i < quantiles[0].size(); ++i) {
      std::cout << "Quantile: " << i;
      for (size_t j = 0; j < quantiles.size(); ++j) {
        std::cout << " " << quantiles[j][i];
      }
      std::cout << "\n";
    }
  }
}
