/**
    \file dvccode/Utils/bulk_file_reader.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_UTILS_BULK_FILE_READER_H
#define BASE_UTILS_BULK_FILE_READER_H

#include <string.h>
#include <string>
#include <iostream>
#include <fstream>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "dvccode/CDef/file_utils.hpp"

namespace HFSAT {

/** \brief Class to minimize number of disk reads. It prefetches a lot of data */
class BulkFileReader {
 protected:
  size_t buf_capacity_;
  size_t buf_size_;     /**actual number of chars left in buffer */
  size_t front_marker_; /**first point in buffer which has not been sent to client yet */
  char* buffer_;
  std::ifstream local_ifstream_;
  std::string last_open_filename_;

  bool isGzStream;
  boost::iostreams::filtering_streambuf<boost::iostreams::input> gz_stream;

  bool tried_downloading_;

 public:
  BulkFileReader(size_t _buf_capacity_ = 4 * 1024 * 1024)
      : buf_capacity_(_buf_capacity_),
        buf_size_(0),
        front_marker_(0),
        buffer_(NULL),
        local_ifstream_(),
        last_open_filename_(""),
	isGzStream(true),
        tried_downloading_(false) {
    buffer_ = new char[buf_capacity_];
  }

  ~BulkFileReader() {
    if (buffer_ != NULL) {
      delete[] buffer_;
      buffer_ = NULL;
    }

    close();
  }

  /**
   * @param _file_name_ if the file name ends with .gz, open a gzstream reader
   * Else if _file_name_ does not end with .gz, look for a file <_file_name_>.gz and if it exists, open a gzstream
   * reader
   * Else try opening the file with a normal stream reader and the passed parameter
   */
  void open(std::string _file_name_) {
    isGzStream = false;

    if (_file_name_.length() > 3 && 0 == _file_name_.compare(_file_name_.length() - 3, 3, ".gz")) {
      isGzStream = true;
      // gz_stream.push(boost::iostreams::gzip_decompressor(), 4 * 1024 * 1024);
    } else {
      // append .gz to the file name and check if it exists
      std::string gz_file_name = _file_name_ + ".gz";
      if (FileUtils::IsFile(gz_file_name)) {
        _file_name_ = gz_file_name;
        isGzStream = true;
      }
    }

    // usual opening and add to buffer
    if (local_ifstream_.is_open()) {
      fprintf(stderr, "Init BulkFileReader closing already open file %s to open %s \n", last_open_filename_.c_str(),
              _file_name_.c_str());
      local_ifstream_.close();
    }

    if (!FileUtils::IsFile(_file_name_)) {
      fprintf(stderr, "File (%s) does not exist, returning \n", _file_name_.c_str());
      return;
    }

    local_ifstream_.open(_file_name_.c_str(), std::ios::in | std::ios::binary);
    if (isGzStream) {
      gz_stream.push(boost::iostreams::gzip_decompressor(), buf_capacity_);
      gz_stream.push(local_ifstream_, buf_capacity_);
      last_open_filename_ = _file_name_;
    } else {
      if (local_ifstream_.is_open() && local_ifstream_.good()) {
        last_open_filename_ = _file_name_;
        buf_size_ = 0;
        local_ifstream_.read(buffer_, buf_capacity_);
        buf_size_ = local_ifstream_.gcount();
        if (!local_ifstream_.good()) {
          local_ifstream_.close();
        }
      }
    }
  }

  void close() {
    if (local_ifstream_.is_open()) {
      local_ifstream_.close();
      if (isGzStream) {
        gz_stream.reset();
      }
    }
  }

  /** \brief returns if more data is there to be read. not necessarily if the ifstream is open or not */
  bool is_open() const { return ((buf_size_ > 0) || (local_ifstream_.is_open() && local_ifstream_.good())); }

  bool IsNYMachine() {
    char hostname_[128];
    hostname_[127] = '\0';
    gethostname(hostname_, 127);
    return strncmp(hostname_, "ip-10-0-1", 9);  // If not worker => NY machine
  }

  void FetchFileFromHS1(std::string filename) {
    // escape & in filename
    size_t start_pos = 0;
    while ((start_pos = filename.find("&", start_pos)) != std::string::npos) {
      filename.insert(start_pos, 3, '\\');
      start_pos += 4;  // skip the new \ and current &
    }
    system(("ssh dvcinfra@10.23.74.41 LiveExec/scripts/download_file_from_hs1.sh '" + filename + "'").c_str());
  }

  size_t read(void* p_dest_, const size_t _len_) {
    if (isGzStream) {
      try {
        return gz_stream.sgetn((char*)p_dest_, _len_);
      } catch (...) {
        std::string error_file = last_open_filename_ + ".error";
        rename(last_open_filename_.c_str(), error_file.c_str());
        // If this is NY machine, then try downloading file from HS1 (this version could be corrupt)
        if ((!tried_downloading_) && IsNYMachine()) {
          std::cerr << "Error reading in bulk file reader for file " << last_open_filename_
                    << ". Could be corrupt. Trying to fetch it again!\n";
          close();
          FetchFileFromHS1(last_open_filename_);
          open(last_open_filename_);
          tried_downloading_ = true;
          return read(p_dest_, _len_);
        } else {
          std::cerr << "error reading in bulk file reader for file " << last_open_filename_ << " . Renamed to "
                    << error_file << "\n"
                    << "Exiting ...\n";
          exit(-1);
        }
      }
    } else if (buf_size_ < _len_) { /* not enough left to cater to this request from prefetched data */
      if (local_ifstream_.is_open() && local_ifstream_.good()) {
        if ((front_marker_ > 0) &&
            (buf_size_ >
             0)) {  // some data left at the end, and there is a gap at the beginning, so copy it to the beginning
          memcpy(buffer_, (buffer_ + front_marker_), buf_size_);
          front_marker_ = 0;
        }
        front_marker_ = 0;  // next unread data is at index 0

        local_ifstream_.read(
            (buffer_ + buf_size_),
            (buf_capacity_ -
             buf_size_));  // fill data from index buf_size_ and remainign space is ( buf_capacity_ - buf_size_ )
        buf_size_ += local_ifstream_.gcount();
      }
      size_t available_len_ = std::min(_len_, buf_size_);
      if (available_len_ > 0) {
        memcpy(p_dest_, buffer_, available_len_);
        front_marker_ += available_len_;
        buf_size_ -= available_len_;
      }
      return available_len_;  // if no data then return 0;
    } else {
      memcpy(p_dest_, (buffer_ + front_marker_), _len_);
      front_marker_ += _len_;
      buf_size_ -= _len_;
      return _len_;
    }
  }

  /// arguments: the destination memory, the maximum size of the destination-char-block
  /// Reads upto d_maxlen_ OR till an occurrence of newline ('\n') OR end of stream
  unsigned int GetLine(char* const p_dest_, const unsigned int d_maxlen_) {
    unsigned int length_read = 0;
    while (length_read < d_maxlen_) {
      int retVal = read(p_dest_ + length_read, 1);
      if (retVal <= 0) break;
      if (p_dest_[length_read++] == '\n') break;
    }
    return length_read;
  }

 private:
};
}

#endif  // BASE_UTILS_BULK_FILE_READER_H
