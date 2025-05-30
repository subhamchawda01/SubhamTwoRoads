/**
    \file dvccode/Utils/bulk_file_writer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_UTILS_BULK_FILE_WRITER_H
#define BASE_UTILS_BULK_FILE_WRITER_H

#include <string.h>
#include <string>
// #include <cstring> // in case we want o move to std::memcpy
#include <iostream>
#include <fstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/ttime.hpp"

namespace HFSAT {

// 128KB
#define DEFAULT_BUFFER_SIZE 131072

// #define NOT_CLOSING_AFTER_OPEN

/** \brief Class to minimize number of disk writes. It writes data in memory till it needs to dump it in file.
 * Used in IndicatorLogger, timed_data_toreg_data.cpp, split_eurex_logged_message_file.cpp,
 * add_YYYYMMDD_to_eurex_timeval_and_split.cpp.
 * Assumes inputs to output operator '<<' are primary data types ... char,unit,int,double,char *
 */
class BulkFileWriter {
 protected:
  size_t buf_capacity_;        ///< maximum size of buffer used to adaptively "grow"
  size_t flush_trigger_size_;  ///< this is the point if reached then the buffer is flushed. This is 80% of the full
  /// buffer size to allow enough space left in the buffer at all times
  size_t front_marker_;     ///< first point in buffer at which to start appending text
  char* in_memory_buffer_;  ///< memory storage for the text to be output. Only set in constructor and destructor
  std::ofstream local_ofstream_;
  std::string file_name_;
  std::ios_base::openmode flags_;
  bool first_open_;
  bool can_open_;

 public:
  BulkFileWriter(const std::string& _file_name_, size_t t_buf_capacity_ = DEFAULT_BUFFER_SIZE,
                 std::ios_base::openmode _flags_ =
                     std::ios::out |
                     std::ios::binary)  // Added flags_ in case some one wants to specify them differently
      : buf_capacity_(t_buf_capacity_),
        flush_trigger_size_((int)(t_buf_capacity_ * 0.90)),
        front_marker_(0),
        in_memory_buffer_(NULL),
        local_ofstream_(),
        file_name_(_file_name_),
        flags_(_flags_),
        first_open_(true),
        can_open_(false) {
    in_memory_buffer_ = new char[buf_capacity_];

    // Writes are defered till needed.
    // File open is also defered till needed.
    // Probably help us deal with too many simultaneous file descriptors for CME instruments.

    // if directory does not exist create it
    FileUtils::MkdirEnclosing(file_name_.c_str());

    local_ofstream_.open(file_name_.c_str(), flags_);
    if (is_ofstream_open()) {
      can_open_ = true;
    }
    first_open_ = false;
    local_ofstream_.close();
  }

  BulkFileWriter(size_t t_buf_capacity_ = DEFAULT_BUFFER_SIZE)
      : buf_capacity_(t_buf_capacity_),
        flush_trigger_size_((int)(t_buf_capacity_ * 0.90)),
        front_marker_(0),
        in_memory_buffer_(NULL),
        local_ofstream_(),
        file_name_(),
        flags_(std::ios::out | std::ios::binary),
        first_open_(true),
        can_open_(false) {
    in_memory_buffer_ = new char[buf_capacity_];
  }

  ~BulkFileWriter() {
    Close();
    if (in_memory_buffer_ != NULL) {
      delete[] in_memory_buffer_;
      in_memory_buffer_ = NULL;
    }
  }

  void Close() {
    if (is_open()) {
      if (front_marker_ > 0) {
        DumpCurrentBuffer();  // also sets front_marker_ = 0;
      }
      local_ofstream_.close();
    }

    if (local_ofstream_.is_open()) {
      local_ofstream_.close();
    }
  }

  void Open(const std::string& _file_name_,
            const std::ios_base::openmode t_open_mode_ = std::ios::out | std::ios::binary) {
    Close();
    // Again file is not really opened till needed.
    file_name_ = _file_name_;
    flags_ = t_open_mode_;
    front_marker_ = 0;

    // if directory does not exist create it
    FileUtils::MkdirEnclosing(file_name_.c_str());

    local_ofstream_.open(file_name_.c_str(), flags_);
    if (is_ofstream_open()) {
      can_open_ = true;
    }
    first_open_ = false;
    local_ofstream_.close();
  }

  /// \brief returns file is writeable or not
  inline bool is_open() const { return can_open_; }

  bool is_ofstream_open() const { return (local_ofstream_.is_open() && local_ofstream_.good()); }

  inline void CheckToFlushBuffer() {
    if (front_marker_ > flush_trigger_size_) {
      DumpCurrentBuffer();  // also sets front_marker_ = 0;

      if (buf_capacity_ <
          DEFAULT_BUFFER_SIZE)  // Double the buffer capacity and trigger size if less than DEFAULT_BUFFER_SIZE
      {
        buf_capacity_ *= 2;
        flush_trigger_size_ *= 2;
      }

      delete[] in_memory_buffer_;
      in_memory_buffer_ = new char[buf_capacity_];  // Free & realloc more memory to buffer.
    }
  }

  template <typename T>
  inline void Write(const T* const _ptr_, const size_t _size_to_write_) {
    memcpy(in_memory_buffer_ + front_marker_, _ptr_, _size_to_write_);
    front_marker_ += _size_to_write_;  /// check if this will work if T is not 'char'
  }

  inline BulkFileWriter& operator<<(const char& _item_) {
    in_memory_buffer_[front_marker_] = _item_;
    front_marker_++;
    return *this;
  }

  inline BulkFileWriter& operator<<(const unsigned int& _item_) {
    int gcount = sprintf(in_memory_buffer_ + front_marker_, "%u", _item_);
    if (gcount > 0) {
      front_marker_ += gcount;
    }
    return *this;
  }

  inline BulkFileWriter& operator<<(const unsigned long long& _item_) {
    int gcount = sprintf(in_memory_buffer_ + front_marker_, "%lld", _item_);
    if (gcount > 0) {
      front_marker_ += gcount;
    }
    return *this;
  }

  inline BulkFileWriter& operator<<(const int& _item_) {
    int gcount = sprintf(in_memory_buffer_ + front_marker_, "%d", _item_);
    if (gcount > 0) {
      front_marker_ += gcount;
    }
    return *this;
  }

  inline BulkFileWriter& operator<<(const ttime_t& r_ttime_t_) {
    int gcount = sprintf(in_memory_buffer_ + front_marker_, "%d.%06d", r_ttime_t_.tv_sec, r_ttime_t_.tv_usec);
    if (gcount > 0) {
      front_marker_ += gcount;
    }
    return *this;
  }

  inline BulkFileWriter& operator<<(const double& _item_) {
    int gcount = sprintf(in_memory_buffer_ + front_marker_, "%f", _item_);
    if (gcount > 0) {
      front_marker_ += gcount;
    }
    return *this;
  }

  /// scary, since we are using sprintf and not snprintf
  inline BulkFileWriter& operator<<(const char* t_item_) {
    int gcount = ::snprintf(in_memory_buffer_ + front_marker_, (buf_capacity_ - front_marker_ - 1), "%s", t_item_);
    if (gcount > 0) {
      front_marker_ += gcount;
    }
    return *this;
  }

  inline BulkFileWriter& operator<<(const std::string& t_item_) {
    int gcount =
        ::snprintf(in_memory_buffer_ + front_marker_, (buf_capacity_ - front_marker_ - 1), "%s", t_item_.c_str());
    if (gcount > 0) {
      front_marker_ += gcount;
    }
    return *this;
  }

  inline void DumpCurrentBuffer() {
    if (front_marker_ > 0) {
      // Open a file if not already opened.
      if (!local_ofstream_.is_open()) {
#ifdef NOT_CLOSING_AFTER_OPEN
        // if directory does not exist create it
        FileUtils::MkdirEnclosing(file_name_.c_str());

        local_ofstream_.open(file_name_.c_str(), flags_);
#else
        if (first_open_) {
          // if directory does not exist create it
          FileUtils::MkdirEnclosing(file_name_.c_str());

          local_ofstream_.open(file_name_.c_str(), flags_);
          first_open_ = false;
        } else {
          local_ofstream_.open(file_name_.c_str(),
                               flags_ | std::ios_base::app);  // append if we are opening again after closing it
        }
#endif
      }

      // Always a good idea to check.
      if (!local_ofstream_.is_open()) {
        fprintf(stderr, "BulkFileWriter could not open file %s for writing \n", file_name_.c_str());
      } else {
        local_ofstream_.write(in_memory_buffer_, front_marker_);
        local_ofstream_.flush();
      }
      front_marker_ = 0;

#ifndef NOT_CLOSING_AFTER_OPEN
      if (local_ofstream_.is_open()) {
        local_ofstream_.close();
      }
#endif
    }
  }
#undef NOT_CLOSING_AFTER_OPEN
};
}

#endif  // BASE_UTILS_BULK_FILE_WRITER_H
