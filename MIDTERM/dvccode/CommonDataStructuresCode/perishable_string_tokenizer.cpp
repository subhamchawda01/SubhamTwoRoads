/**
    \file CommonDataStructuresCode/perishable_string_tokenizer.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {

PerishableStringTokenizer::PerishableStringTokenizer(char *_readline_buffer_, unsigned int _bufferlen_,
                                                     bool _ignore_after_hash_) {
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

// Tokenizing functions : splits _input_string_ into tokens based on the delimiters specified in _delim_string_,
// each char in _delim_string_ is treated as a separate delimiter

// responsibility of freeing the allocated memory is on the calling function,
// it is better to use NonConstStringTokenizer by making a non-const copy of input string
void PerishableStringTokenizer::ConstStringTokenizer(const char *_input_string_, const char *_delim_string_,
                                                     std::vector<char *> &_tokens_) {
  if (_input_string_ == NULL) {
    return;
  }
  const size_t t_len_ = strlen(_input_string_);
  char *temp_inp_str_ = new char[t_len_ + 1];
  strncpy(temp_inp_str_, _input_string_, t_len_);
  temp_inp_str_[t_len_] = '\0';
  char *this_token_ = strtok(temp_inp_str_, _delim_string_);
  while (this_token_ != NULL) {
    _tokens_.push_back(this_token_);
    this_token_ = strtok(NULL, _delim_string_);
  }
}

// same as ConstStringTokenizer but it modifies the input string, saving the new memory allocation and strcpy
// this can be used if modifying input string won't create problems
void PerishableStringTokenizer::NonConstStringTokenizer(char *_input_string_, const char *_delim_string_,
                                                        std::vector<char *> &_tokens_) {
  if (_input_string_ == NULL) {
    return;
  }
  char *this_token_ = strtok(_input_string_, _delim_string_);
  while (this_token_ != NULL) {
    _tokens_.push_back(this_token_);
    this_token_ = strtok(NULL, _delim_string_);
  }
}

// tokenize std::string and the tokens are of std::string class as well
void PerishableStringTokenizer::StringSplit(const std::string &s, char delim, std::vector<std::string> &_tokens_) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    _tokens_.push_back(item);
  }
}

void PerishableStringTokenizer::TrimString(const char *_input_str_, std::string &_trimmed_str_,
                                           const char _delim_ /* = ' '*/) {
  if (_input_str_ == NULL) {
    _trimmed_str_ = "";
  }
  if (_delim_ == '\0') {
    _trimmed_str_ = _input_str_;
  }

  int i = 0;
  while (_input_str_[i] == _delim_) {
    i++;
  }
  int start_idx_ = i;

  int end_idx_ = i;
  while (_input_str_[i] != '\0') {
    if (_input_str_[i] != _delim_) {
      end_idx_ = i;
    }
    i++;
  }

  _trimmed_str_ = std::string(&(_input_str_[start_idx_]), end_idx_ - start_idx_ + 1);
}

void PerishableStringTokenizer::ParseConfig(const std::string &_config_filename_,
                                            std::map<std::string, std::vector<std::string> > &_key_valvec_map_) {
  std::ifstream ifs_(_config_filename_.c_str(), std::ifstream::in);
  if (ifs_.is_open()) {
    std::string curr_key_ = "";
    bool curr_key_set_ = false;

    const int kBufferLength = 1024;
    char buffer_[kBufferLength];
    while (ifs_.good()) {
      bzero(buffer_, kBufferLength);
      ifs_.getline(buffer_, kBufferLength);
      std::string t_val_;
      TrimString(buffer_, t_val_);

      PerishableStringTokenizer pst_(buffer_, kBufferLength);
      const std::vector<const char *> &tokens_ = pst_.GetTokens();

      if (tokens_.empty()) {
        if (curr_key_set_) {
          curr_key_set_ = false;
        }
      } else if (tokens_[0][0] == '#') {
        // skip lines commented with #
      } else if (!curr_key_set_) {
        curr_key_ = tokens_[0];
        curr_key_set_ = true;
      } else {
        _key_valvec_map_[curr_key_].push_back(t_val_);
      }
    }
    ifs_.close();
  } else {
    std::cerr << "Can't open " << _config_filename_ << " for reading\n";
  }
}

void PerishableStringTokenizer::ParseConfigLines(const std::string &_config_filename_,
                                                 std::vector<std::vector<std::string> > &_val_vec_vec_,
                                                 const char *_delim_str_ /*= ' '*/) {
  std::ifstream ifs_(_config_filename_.c_str(), std::ifstream::in);
  if (ifs_.is_open()) {
    const int kBufferLength = 1024;
    char buffer_[kBufferLength];
    while (ifs_.good()) {
      bzero(buffer_, kBufferLength);
      ifs_.getline(buffer_, kBufferLength);

      std::vector<char *> tokens_;
      NonConstStringTokenizer(buffer_, _delim_str_, tokens_);

      if (tokens_.empty() || tokens_[0][0] == '#')  // ignore empty lines and lines starting with #
      {
        continue;
      } else {
        _val_vec_vec_.emplace_back();
        std::vector<std::string> &t_vec_ = _val_vec_vec_.back();
        for (auto i = 0u; i < tokens_.size(); i++) {
          if (tokens_[i][0] == '#') {
            break;
          }  // ignore after #
          t_vec_.emplace_back(tokens_[i]);
        }
      }
    }
    ifs_.close();
  } else {
    std::cerr << "Can't open " << _config_filename_ << " for reading\n";
  }
}
}
