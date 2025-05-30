/**
    \file dvccode/CommonDataStructures/perishable_string_tokenizer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_COMMONDATASTRUCTURES_PERISHABLE_STRING_TOKENIZER_H
#define BASE_COMMONDATASTRUCTURES_PERISHABLE_STRING_TOKENIZER_H

#include <vector>
#include <map>
#include <ctype.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <sstream>

namespace HFSAT {

class PerishableStringTokenizer {
 public:
  std::vector<const char *> tokens_;
  PerishableStringTokenizer(char *_readline_buffer_, unsigned int _bufferlen_, bool _ignore_after_hash_ = false);
  const std::vector<const char *> &GetTokens() const { return tokens_; }

  // Tokenizing functions : splits _input_string_ into tokens based on the delimiters specified in _delim_string_,
  // each char in _delim_string_ is treated as a separate delimiter

  // responsibility of freeing the allocated memory is on the calling function,
  // it is better to use NonConstStringTokenizer by making a non-const copy of input string
  static void ConstStringTokenizer(const char *_input_string_, const char *_delim_string_,
                                   std::vector<char *> &_tokens_);

  // same as ConstStringTokenizer but it modifies the input string, saving the new memory allocation and strcpy
  // this can be used if modifying input string won't create problems
  static void NonConstStringTokenizer(char *_input_string_, const char *_delim_string_, std::vector<char *> &_tokens_);

  static void StringSplit(const std::string &s, char delim, std::vector<std::string> &_tokens_);

  static void TrimString(const char *_input_str_, std::string &_trimmed_str_, const char _delim_ = ' ');

  // config format
  // Key1\nVal1\nVal2\n\nKey2...
  static void ParseConfig(const std::string &_config_filename_,
                          std::map<std::string, std::vector<std::string> > &_key_valvec_map_);

  // config format
  // Key1 Val1 Val2\nKey2...
  static void ParseConfigLines(const std::string &_config_filename_,
                               std::vector<std::vector<std::string> > &_val_vec_vec_, const char *_delim_str_ = " ");
};
}

#endif  // BASE_COMMONDATASTRUCTURES_PERISHABLE_STRING_TOKENIZER_H
