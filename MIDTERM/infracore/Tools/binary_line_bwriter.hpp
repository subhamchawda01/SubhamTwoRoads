/**
   \file Tools/binary_line_bwriter.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/
#ifndef BASE_TOOLS_BINARY_LINE_BWRITER_HPP
#define BASE_TOOLS_BINARY_LINE_BWRITER_HPP

#include "dvccode/Utils/bulk_file_writer.hpp"
#include "infracore/Tools/simple_line_processor.hpp"

namespace HFSAT {

class BinaryLineBWriter : public class BinaryLineProcessor {
 public:
  BinaryLineBWriter(const std::string& output_data_filename_)
      : bulk_file_writer_(output_data_filename_), writing_first_line_(true) {}

  inline void AddWord(double _item_) {
    if (writing_first_line_) {
      flstore_.push_back(_item_);
    } else {
      bulk_file_writer_.Write(&_item_, sizeof(double));
    }
  }

  inline void FinishLine() {
    if (writing_first_line_) {
      writing_first_line_ = false;
      unsigned int flstore_size_ = flstore_.size();
      bulk_file_writer_.Write(&flstore_size_, sizeof(unsigned int));
      for (unsigned int i = 0; i < flstore_.size(); i++) {
        bulk_file_writer_.Write(&(flstore_[i]), sizeof(double));
      }
      flstore_.clear();
    }
    bulk_file_writer_.CheckToFlushBuffer();
  }

  inline void Close() { bulk_file_writer_.Close(); }

 private:
  BulkFileWriter bulk_file_writer_;
  bool writing_first_line_;
  std::vector<double> flstore_;
};
}

#endif  // BASE_TOOLS_BINARY_LINE_BWRITER_HPP
