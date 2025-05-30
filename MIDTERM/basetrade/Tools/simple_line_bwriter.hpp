/**
   \file Tools/simple_line_bwriter.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/
#ifndef BASE_TOOLS_SIMPLE_LINE_BWRITER_HPP
#define BASE_TOOLS_SIMPLE_LINE_BWRITER_HPP

#include "dvccode/Utils/bulk_file_writer.hpp"
#include "basetrade/Tools/simple_line_processor.hpp"

namespace HFSAT {

class SimpleLineBWriter : public SimpleLineProcessor {
 public:
  SimpleLineBWriter(const std::string& output_data_filename_)
      : bulk_file_writer_(output_data_filename_), first_word_in_line_(true) {}

  inline void AddWord(double _item_) {
    if (first_word_in_line_) {
      first_word_in_line_ = false;
    } else {
      bulk_file_writer_ << ' ';
    }
    bulk_file_writer_ << _item_;
    bulk_file_writer_.CheckToFlushBuffer();
  }

  inline void FinishLine() {
    bulk_file_writer_ << '\n';
    bulk_file_writer_.CheckToFlushBuffer();
    first_word_in_line_ = true;
  }

  inline void Close() { bulk_file_writer_.Close(); }

 private:
  BulkFileWriter bulk_file_writer_;
  bool first_word_in_line_;
};
}

#endif  // BASE_TOOLS_SIMPLE_LINE_BWRITER_HPP
