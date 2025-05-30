// =====================================================================================
//
//       Filename:  dvccode/Tests/dvccode/CommonDataStructures/perishable_string_tokenizer_tests.cpp
//
//    Description:  Tests for Utility Functions for fetching the Sample Data values from the last 30 days
//
//        Version:  1.0
//        Created:  01/14/2016 04:31:28 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include <stdio.h>
#include <string.h>
#include "dvccode/Tests/CommonDataStructures/perishable_string_tokenizer_tests.hpp"

namespace HFTEST {
// using namespace HFSAT;

/**
 * Test for plain tokenizor with delim ' '
 */
void PerishableStringTokenizerTests::TestTokenizer() {
  int kTokenLength = 1024;
  char string_to_be_tokenized[1024];
  strcpy(string_to_be_tokenized, "Hi I am a string. I need to be tokenized till this # and not this");

  // First tokenize it till #
  HFSAT::PerishableStringTokenizer st(string_to_be_tokenized, kTokenLength, true);
  const std::vector<const char*>& tokens = st.GetTokens();
  CPPUNIT_ASSERT(tokens.size() == 12);

  // ignore # and tokenize all strings
  strcpy(
      string_to_be_tokenized,
      "Hi I am a string. I need to be tokenized till this # and not this");  // copy again because it has been modified.
  HFSAT::PerishableStringTokenizer st2(string_to_be_tokenized, kTokenLength, false);
  const std::vector<const char*>& tokens2 = st2.GetTokens();
  CPPUNIT_ASSERT(tokens2.size() == 16);
}

void PerishableStringTokenizerTests::TestNonConstTokenizer(void) {
  /*
   * Test for tokenizor with delim ','
*/
  char string_to_be_tokenized[1024];
  std::vector<char*> tokens_;
  strcpy(string_to_be_tokenized, "FGBM,FGBL,XT,YT,ZN,ZF,ZT.Tell the no. of products");
  HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(string_to_be_tokenized, ",", tokens_);
  CPPUNIT_ASSERT(tokens_.size() == 7);
}

void PerishableStringTokenizerTests::TestConstTokenizer(void) {
  /* Check that input string doesnt change along with tokenization check
   */

  char string_to_be_tokenized[1024];
  char test_string[1024];
  std::vector<char*> tokens_;
  strcpy(string_to_be_tokenized, "FGBM,FGBL,XT,YT,ZN,ZF,ZT.Tell the no. of products");
  strcpy(test_string, string_to_be_tokenized);

  HFSAT::PerishableStringTokenizer::ConstStringTokenizer(string_to_be_tokenized, ",", tokens_);
  CPPUNIT_ASSERT(tokens_.size() == 7);
  CPPUNIT_ASSERT(strcmp(test_string, string_to_be_tokenized) == 0);
}
}
