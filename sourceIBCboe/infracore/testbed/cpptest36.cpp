#include <fstream>
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

void ReadNumbers(std::string model_filename_, std::vector<double>& sequence_) {
  std::ifstream model_infile_;
  model_infile_.open(model_filename_.c_str(), std::ifstream::in);
  if (model_infile_.is_open()) {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);

    while (model_infile_.good()) {
      bzero(readline_buffer_, kModelLineBufferLen);
      model_infile_.getline(readline_buffer_, kModelLineBufferLen);
      if (model_infile_.gcount() > 0) {
        HFSAT::PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string
                                                                                      // readline_buffer_ .. in
                                                                                      // tokenizing the string contents
                                                                                      // are changed
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if (tokens_.size() == 2) {
          sequence_.push_back(atof(tokens_[1]));
        }
      }
    }

    model_infile_.close();
  }
}

int main(int argc, char** argv) {
  std::string model_filename_ = argv[1];
  HFSAT::FastPriceConvertor fast_price_convertor_(1.0);
  std::vector<double> sequence_;
  std::vector<int> answers_;
  ReadNumbers(model_filename_, sequence_);

  for (unsigned int i = 0; i < sequence_.size(); i++) {
    answers_.push_back(fast_price_convertor_.GetFastIntPx(sequence_[i]));
  }

  return 0;
}
