#include <stdio.h>
#include <iostream>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

int main(int argc, char** argv) {
  char _readline_buffer_[10] = {0};
  sprintf(_readline_buffer_, "%d\t%s\n", 23, "xy");

  // for ( int i = 0 ; i < 10 ; i ++ )
  //   {
  //     int int_value_char_ = (int)_readline_buffer_[i] ;

  //     if ( isspace ( int_value_char_ ) )
  // 	{
  // 	  std::cout << " isspace " << (int)_readline_buffer_[i] << std::endl;
  // 	}
  //     else
  // 	{
  // 	  std::cout << " NOTspace " << (int)_readline_buffer_[i] << std::endl;
  // 	}
  //   }

  HFSAT::PerishableStringTokenizer st_(_readline_buffer_, 10);
  const std::vector<const char*>& tokens_ = st_.GetTokens();
  std::cout << tokens_.size() << std::endl;
  return 0;
}
