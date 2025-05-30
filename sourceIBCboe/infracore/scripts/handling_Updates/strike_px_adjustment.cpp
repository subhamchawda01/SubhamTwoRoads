#include<bits/stdc++.h>

using namespace std;


class PerishableStringTokenizer {
 public:
  std::vector<const char *> tokens_;
  const std::vector<const char *> &GetTokens() const { return tokens_; }
  PerishableStringTokenizer(char *_readline_buffer_, unsigned int _bufferlen_, bool _ignore_after_hash_ = false){
    bool reading_word_ = false;
  for (auto i = 0u; i < _bufferlen_; i++) {
    int int_value_char_ = (int)_readline_buffer_[i];

    if (iscntrl(int_value_char_) &&
        (_readline_buffer_[i] != '\t')) {  // end of string .. should be replaced with == NUL ?
      _readline_buffer_[i] = '\0';
      break;
    }

    if (_ignore_after_hash_ && (_readline_buffer_[i] == '#')) {  // not reading anything after #
      _readline_buffer_[i] = '\0';
      break;
    }

    if (!isspace(int_value_char_) && !reading_word_) {
      tokens_.push_back(&(_readline_buffer_[i]));
      reading_word_ = true;
    }

    if (isspace(int_value_char_) && reading_word_) {  // first space char after word ... replace with NUL
      _readline_buffer_[i] = '\0';
      reading_word_ = false; 
    }
  }
  }
};

int main(int argc, char **argv){
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " file_name value"
      << std::endl;
    exit(0);
  }
  std::string file_name_ = argv[1];
  double multiple = 0.05;
  double value = std::stof(argv[2]);
  std::ifstream file_(file_name_, std::ifstream::in);
  if(file_.is_open()){
        const int kBufferLen = 1024;
        char readline_buffer_[kBufferLen];
        bzero(readline_buffer_, kBufferLen);
        while(file_.good()){
          bzero(readline_buffer_, kBufferLen);
          file_.getline(readline_buffer_, kBufferLen);
          PerishableStringTokenizer st_(readline_buffer_, kBufferLen);
          const std::vector<const char *> &tokens_ = st_.GetTokens();
          if(tokens_.size() == 2){
            double price = stof(tokens_[1]) * value;
            double strike_ = std::round(price / multiple) * multiple;
            std::cout << tokens_[0] << "_" << strike_ << std::endl;
          }
        }
      }else{
        std::cerr << "FILE for " << file_name_ << " is not READABLE or does not exist under DIR: . EXITING ............\n";
        exit(-1);
      }


  return 0;
}
