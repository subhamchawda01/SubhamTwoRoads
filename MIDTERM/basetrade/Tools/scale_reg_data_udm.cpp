#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_codes.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "basetrade/Tools/simple_line_bwriter.hpp"
#include "dvccode/CDef/file_utils.hpp"

int main(int argc, char** argv) {
  std::string input_data_filename_;
  std::string output_data_filename_;
  const int kDataFileLineBufferLen = 10240;

  if (argc != 3) {
    std::cerr << "USAGE :: " << argv[0] << " input_data_filename output_data_filename \n";
    exit(0);
  }

  input_data_filename_ = argv[1];
  output_data_filename_ = argv[2];

  std::ifstream input_data_file_;
  input_data_file_.open(input_data_filename_.c_str(), std::ifstream::in);
  HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);

  char line_buffer_[kDataFileLineBufferLen];
  while (input_data_file_.good()) {
    bzero(line_buffer_, kDataFileLineBufferLen);
    input_data_file_.getline(line_buffer_, kDataFileLineBufferLen);
    HFSAT::PerishableStringTokenizer st_(line_buffer_, kDataFileLineBufferLen);
    const std::vector<const char*>& tokens_ = st_.GetTokens();

    if (tokens_.size() < 1) {
      continue;
    }

    float factor_ = fabs(atof(tokens_[0]));
    for (auto i = 0u; i < tokens_.size(); i++) {
      // std::cerr << factor_ << " " << atof ( tokens_[i] ) << " " << factor_ * atof ( tokens_[i] ) << "\n";
      simple_line_bwriter_.AddWord(atof(tokens_[i]) * factor_);
    }
    simple_line_bwriter_.FinishLine();
  }

  simple_line_bwriter_.Close();
  if (input_data_file_.is_open()) {
    input_data_file_.close();
  }
}
