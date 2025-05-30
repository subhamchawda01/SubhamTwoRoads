/**
    \file dvccode/CDef/debug_logger.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_CDEF_DEBUG_LOGGER_H
#define BASE_CDEF_DEBUG_LOGGER_H

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <typeinfo>  // for typeid

#include <fstream>
#include <iostream>
#include <vector>

#include <tr1/unordered_map>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/shared_logging_defines.hpp"
#include "dvccode/CDef/defines.hpp"
#define BASEVEC_LOGLEVELS_SIZE 512

// the following defines are some predefined loglevels
// it is the responsibility of the coder to maintain uniqueness if required
// *ERROR = Essential, should be logged at all times
// *INFO not essential
// *TEST Excessive printouts, should be removed before production
#define WATCH_ERROR 10
#define WATCH_INFO 110
#define OM_ERROR 11
#define OM_INFO 111
#define PLSMM_ERROR 12
#define PLSMM_INFO 112
#define PLSMM_TEST 212
#define TRADING_ERROR 13
#define TRADING_INFO 113
#define TRADING_TEST 213
#define BOOK_ERROR 14
#define BOOK_INFO 114
#define BOOK_TEST 214
#define ORS_ERROR 15
#define ORS_INFO 115
#define MDS_ERROR 16
#define MDS_INFO 116
#define MDS_TEST 216
#define LRDB_ERROR 17
#define LRDB_INFO 117
#define DBG_MODEL_ERROR 18
#define DBG_MODEL_INFO 118
#define PROM_OM_INFO 219
#define BOOK_MINIMAL_TEST 314
#define MUM_ERROR 20
#define MUM_INFO 120
#define SEEM_ERROR 21
#define SEEM_INFO 121
#define HPM_ERROR 22
#define HPM_INFO 122
#define SMVSELF_ERROR 23
#define SMVSELF_INFO 123
#define PVM_ERROR 24
#define PVM_INFO 124
#define INDICATOR_PRICE_TEST 220
#define TRADEINIT_INFO 300
#define OLSMM_INFO 125
#define RETAIL_INFO 301
#define ATEX_INFO 133
#define SIM_ORDER_INFO 126  // to generate logs in ors logs format
#define DBG_PARAM_INFO 19   // to provide thresholds value on UpdateTarget
#define OPTIONS_INFO 400    // to provide options variables on every calculation of implied vol
#define ORS_DATA_INFO 333
#define FILL_TIME_INFO 299
#define INDICATOR_INFO 177
#define VOL_INFO 302
#define SKIP_PACKET_DROPS_INFO 303
#define SKIP_SANITIZE_INFO 304
// end ... if adding more please update TextToLogLevel function as well

#define DBGLOG_TIME_CLASS_FUNC_LINE \
  dbglogger_ << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' '
#define DBGLOG_TIME_CLASS_FUNC dbglogger_ << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << ' '
#define DBGLOG_TIME_CLASS dbglogger_ << watch_.tv() << ' ' << typeid(*this).name() << ' '
#define DBGLOG_TIME dbglogger_ << watch_.tv() << ' '

#define DBGLOG_TIME_CLASS_FUNC_ _dbglogger__ << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << ' '
#define DBGLOG_CLASS_FUNC_LINE dbglogger_ << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' '
#define DBGLOG_CLASS_FUNC dbglogger_ << typeid(*this).name() << ':' << __func__ << ' '
#define DBGLOG_CLASS dbglogger_ << typeid(*this).name() << ' '

#define DBGLOG_CLASS_FUNC_LINE_ERROR \
  dbglogger_ << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << "< ERROR >" << ' '
#define DBGLOG_CLASS_FUNC_LINE_FATAL \
  dbglogger_ << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << "< FATAL >" << ' '
#define DBGLOG_CLASS_FUNC_LINE_INFO \
  dbglogger_ << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << "< INFO >" << ' '

#define DBGLOG_FUNC dbglogger_ << __func__ << ' '

#define DBGLOG_ENDL_FLUSH \
  '\n';                   \
  dbglogger_.CheckToFlushBuffer()
#define DBGLOG_ENDL_FLUSH_ \
  '\n';                    \
  _dbglogger__.CheckToFlushBuffer()
#define DBGLOG_ENDL_NOFLUSH '\n'
#define DBGLOG_DUMP dbglogger_.DumpCurrentBuffer()
#define DBGLOG_ENDL_DUMP \
  '\n';                  \
  dbglogger_.DumpCurrentBuffer()
#define DBGLOG_ENDL_DUMP_AND_EXIT \
  '\n';                           \
  dbglogger_.DumpCurrentBuffer(); \
  std::exit(-1)

namespace HFSAT {

#define ADDING_CHECK_TO_FLUSH_TO_ALL_MEM_ACCESSES

/** \brief Class to minimize number of disk writes. It writes data in memory till it needs to dump it in file
 * Meant to be used in error logging
 * Assumes inputs to output operator << are : char, int, double, const char *
 * Has support for loglevels. If very few ( < BASEVEC_LOGLEVELS_SIZE ) loglevels then loglevels kept as a vector-map
 * After that loglevels are kept as a hash_map
 */
typedef std::tr1::unordered_map<unsigned int, bool> IntToBoolUnorderedMap;
typedef std::tr1::unordered_map<unsigned int, bool>::const_iterator IntToBoolUnorderedMapCIter;

class DebugLogger {
 protected:
  const size_t buf_capacity_;
  const size_t flush_trigger_size_;
  size_t front_marker_;
  uint32_t loglevels_basevec_[BASEVEC_LOGLEVELS_SIZE];
  IntToBoolUnorderedMap loglevels_highervec_;  ///< levels greater than 512 are stored as a unordered_map

  std::ofstream local_ofstream_;
  std::ios_base::openmode open_mode_;

  char* in_memory_buffer_;
  bool no_log_mode_;
  std::string logfilename_;

 public:
  DebugLogger(size_t t_buf_capacity_, size_t t_flush_trigger_size_ = 0)
      : buf_capacity_(t_buf_capacity_),
        flush_trigger_size_(
            (t_flush_trigger_size_ > 0)
                ? (std::min(t_flush_trigger_size_, (size_t)((int)((double)t_buf_capacity_ * (double)0.80))))
                : ((size_t)((double)t_buf_capacity_ * (double)0.80))),
        front_marker_(0),
        loglevels_basevec_(),
        loglevels_highervec_(),
        open_mode_(std::ios_base::out),
        in_memory_buffer_((char*)calloc(t_buf_capacity_ * 2, sizeof(char))),
        no_log_mode_(false),
        logfilename_("") {
    bzero(in_memory_buffer_, buf_capacity_ * 2);
  }

  virtual ~DebugLogger() {
    logfilename_.clear();
    loglevels_highervec_.clear();

    if (local_ofstream_.is_open()) {
      local_ofstream_.write(in_memory_buffer_, front_marker_);
      local_ofstream_.flush();
      local_ofstream_.close();
    }

    if (NULL != in_memory_buffer_) {
      free(in_memory_buffer_);
      in_memory_buffer_ = NULL;
    }
  }

  /*
   * Return if the no logs mode is set
   */
  const bool IsNoLogs(void) const { return no_log_mode_; }

  /*
   * Closes the debug logger file handle
   */
  virtual void Close() {
    logfilename_.clear();
    loglevels_highervec_.clear();

    if (local_ofstream_.is_open()) {
      DumpCurrentBuffer();
      local_ofstream_.close();
    }

    if (NULL != in_memory_buffer_) {
      free(in_memory_buffer_);
      in_memory_buffer_ = NULL;
    }
  }

  /*
   * Check if size exceeds to flush to disk from memory
   */
  inline void CheckToFlushBuffer() {
    if (no_log_mode_) return;
    if (front_marker_ > flush_trigger_size_) {
      DumpCurrentBuffer();  // also sets front_marker_ = 0;
    }
  }

  /*
   * Writes char to buffer
   */
  inline DebugLogger& operator<<(const char& t_item_) {
    if (no_log_mode_) return *this;
    in_memory_buffer_[front_marker_] = t_item_;
    front_marker_++;
#ifdef ADDING_CHECK_TO_FLUSH_TO_ALL_MEM_ACCESSES
    CheckToFlushBuffer();
#endif
    return *this;
  }

  /*
   * Writes unsigned int to buffer
   */
  inline DebugLogger& operator<<(const unsigned int& t_item_) {
    if (no_log_mode_) return *this;
    int gcount = sprintf(in_memory_buffer_ + front_marker_, "%u", t_item_);
    if (gcount > 0) {
      front_marker_ += gcount;
    }
#ifdef ADDING_CHECK_TO_FLUSH_TO_ALL_MEM_ACCESSES
    CheckToFlushBuffer();
#endif
    return *this;
  }

  /*
   * Writes size_t to buffer
   */
  inline DebugLogger& operator<<(const size_t& t_item_) {
    if (no_log_mode_) return *this;
    int gcount = sprintf(in_memory_buffer_ + front_marker_, "%lu", (unsigned long)t_item_);
    if (gcount > 0) {
      front_marker_ += gcount;
    }
#ifdef ADDING_CHECK_TO_FLUSH_TO_ALL_MEM_ACCESSES
    CheckToFlushBuffer();
#endif
    return *this;
  }

  /*
   * Writes unsigned long long to buffer
   */
  inline DebugLogger& operator<<(const unsigned long long& _item_) {
    if (no_log_mode_) return *this;
    int gcount = sprintf(in_memory_buffer_ + front_marker_, "%lld", _item_);
    if (gcount > 0) {
      front_marker_ += gcount;
    }
#ifdef ADDING_CHECK_TO_FLUSH_TO_ALL_MEM_ACCESSES
    CheckToFlushBuffer();
#endif
    return *this;
  }

  /*
   * Writes long long to buffer
   */
  inline DebugLogger& operator<<(const long long& _item_) {
    if (no_log_mode_) return *this;
    int gcount = sprintf(in_memory_buffer_ + front_marker_, "%lld", _item_);
    if (gcount > 0) {
      front_marker_ += gcount;
    }
#ifdef ADDING_CHECK_TO_FLUSH_TO_ALL_MEM_ACCESSES
    CheckToFlushBuffer();
#endif
    return *this;
  }

  /*
   * Writes int64 to buffer
   */
  inline DebugLogger& operator<<(const int64_t& _item_) {
    if (no_log_mode_) return *this;
    int gcount = sprintf(in_memory_buffer_ + front_marker_, "%lld", (long long)_item_);
    if (gcount > 0) {
      front_marker_ += gcount;
    }
#ifdef ADDING_CHECK_TO_FLUSH_TO_ALL_MEM_ACCESSES
    CheckToFlushBuffer();
#endif
    return *this;
  }

  /*
   * Writes int to buffer
   */
  inline DebugLogger& operator<<(const int& t_item_) {
    if (no_log_mode_) return *this;
    int gcount = sprintf(in_memory_buffer_ + front_marker_, "%d", t_item_);
    if (gcount > 0) {
      front_marker_ += gcount;
    }
#ifdef ADDING_CHECK_TO_FLUSH_TO_ALL_MEM_ACCESSES
    CheckToFlushBuffer();
#endif
    return *this;
  }

  /*
   * Writes ttime_t to buffer
   */
  inline DebugLogger& operator<<(const ttime_t& r_ttime_t_) {
    if (no_log_mode_) return *this;
    int gcount = sprintf(in_memory_buffer_ + front_marker_, "%d.%06d", r_ttime_t_.tv_sec, r_ttime_t_.tv_usec);
    if (gcount > 0) {
      front_marker_ += gcount;
    }
#ifdef ADDING_CHECK_TO_FLUSH_TO_ALL_MEM_ACCESSES
    CheckToFlushBuffer();
#endif
    return *this;
  }

  /*
   * Writes double to buffer
   */
  inline DebugLogger& operator<<(const double& t_item_) {
    if (no_log_mode_) return *this;
    int gcount = ::sprintf(in_memory_buffer_ + front_marker_, "%f", t_item_);
    if (gcount > 0) {
      front_marker_ += gcount;
    }
#ifdef ADDING_CHECK_TO_FLUSH_TO_ALL_MEM_ACCESSES
    CheckToFlushBuffer();
#endif
    return *this;
  }

  /// Added a function to print a char string that might not be properly NULL terminated.
  /// Only used in infracore/BasicOrderRoutingServer/ControlThread
  inline void snprintf(unsigned int r_max_len_, const char* t_item_) {
    if (no_log_mode_) return;
    int gcount = ::snprintf(in_memory_buffer_ + front_marker_, r_max_len_, "%s", t_item_);
    if (gcount > 0) {
      front_marker_ += gcount;
    }
#ifdef ADDING_CHECK_TO_FLUSH_TO_ALL_MEM_ACCESSES
    CheckToFlushBuffer();
#endif
    return;
  }

  /// scary, since we are using sprintf and not snprintf
  inline DebugLogger& operator<<(const char* t_item_) {
    if (no_log_mode_) return *this;
    int gcount = ::snprintf(in_memory_buffer_ + front_marker_, (buf_capacity_ - front_marker_ - 1), "%s", t_item_);
    if (gcount > 0) {
      front_marker_ += gcount;
    }
#ifdef ADDING_CHECK_TO_FLUSH_TO_ALL_MEM_ACCESSES
    CheckToFlushBuffer();
#endif
    return *this;
  }

  inline DebugLogger& operator<<(const std::string& t_item_) {
    if (no_log_mode_) return *this;
    int gcount =
        ::snprintf(in_memory_buffer_ + front_marker_, (buf_capacity_ - front_marker_ - 1), "%s", t_item_.c_str());
    if (gcount > 0) {
      front_marker_ += gcount;
    }
#ifdef ADDING_CHECK_TO_FLUSH_TO_ALL_MEM_ACCESSES
    CheckToFlushBuffer();
#endif
    return *this;
  }

  /*
   * Dumps from in memory buffer to disk
   */
  inline virtual void DumpCurrentBuffer() {
    if (local_ofstream_.is_open()) {
      local_ofstream_.write(in_memory_buffer_, front_marker_);
      local_ofstream_.flush();
      // bzero ( in_memory_buffer_, buf_capacity_*2 ) ;
    }
    front_marker_ = 0;
  }

  /*
   * open the debug logger filename
   */
  void OpenLogFile(const std::string& fname) { OpenLogFile(fname.c_str(), std::ofstream::app); }

  std::string GetLogFileName() { return logfilename_; }

  /*
   * open the handle to logfilename
   */
  virtual void OpenLogFile(const char* _logfilename_, std::ios_base::openmode _open_mode_) {
    if (local_ofstream_.is_open()) {
      DumpCurrentBuffer();
      local_ofstream_.close();
    }

    logfilename_ = _logfilename_;
    open_mode_ = _open_mode_;

    TryOpeningFile();
  }

  virtual void TryOpeningFile() {
    // if directory does not exist create it
    FileUtils::MkdirEnclosing(logfilename_);
    local_ofstream_.open(logfilename_.c_str(), open_mode_);
  }

  /// Adds a _loglevel_ to allow printing on that level
  /// @param _loglevel_ is a level for which DebugLogger should allow printing
  void AddLogLevel(unsigned int _loglevel_) {
    if (no_log_mode_) return;
    if (_loglevel_ < BASEVEC_LOGLEVELS_SIZE) {
      loglevels_basevec_[_loglevel_] = 1;
    } else {
      loglevels_highervec_[_loglevel_] = true;
    }
  }

  /// Removes this debuglevel
  void RemLogLevel(unsigned int _loglevel_) {
    if (_loglevel_ < BASEVEC_LOGLEVELS_SIZE) {
      loglevels_basevec_[_loglevel_] = 0;
    } else {
      loglevels_highervec_.erase(_loglevel_);
    }
  }

  /// Checks if debug printing is allowed at this debuglevel
  inline bool CheckLoggingLevel(unsigned int _loglevel_) {
    if (_loglevel_ < BASEVEC_LOGLEVELS_SIZE) {
      return (loglevels_basevec_[_loglevel_] == 1);
    } else {
      // if chexcking for loglevel higher than BASEVEC_LOGLEVELS_SIZE then check the Datastructure of levels higher than
      // that
      IntToBoolUnorderedMapCIter _iter_ = loglevels_highervec_.find(_loglevel_);
      if (_iter_ != loglevels_highervec_.end()) {
        return _iter_->second;
      }
      return false;
    }
    return false;
  }

  /*
   * Set the debug logger to clear all debug logging levels
   */
  inline void SetNoLogs() {
    no_log_mode_ = true;
    for (auto i = 0u; i < BASEVEC_LOGLEVELS_SIZE; i++) {
      loglevels_basevec_[i] = 0;
    }
    loglevels_highervec_.clear();
  }

  static inline int TextToLogLevel(const char* log_level_text_) {
    int log_level_ = 0;

    if (!strncmp(log_level_text_, "WATCH_ERROR", strlen("WATCH_ERROR"))) {
      log_level_ = WATCH_ERROR;
    } else if (!strncmp(log_level_text_, "WATCH_INFO", strlen("WATCH_INFO"))) {
      log_level_ = WATCH_INFO;
    } else if (!strncmp(log_level_text_, "OM_ERROR", strlen("OM_ERROR"))) {
      log_level_ = OM_ERROR;
    } else if (!strncmp(log_level_text_, "OM_INFO", strlen("OM_INFO"))) {
      log_level_ = OM_INFO;
    } else if (!strncmp(log_level_text_, "SIM_ORDER_INFO", strlen("SIM_ORDER_INFO"))) {
      log_level_ = SIM_ORDER_INFO;
    } else if (!strncmp(log_level_text_, "PLSMM_ERROR", strlen("PLSMM_ERROR"))) {
      log_level_ = PLSMM_ERROR;
    } else if (!strncmp(log_level_text_, "PLSMM_INFO", strlen("PLSMM_INFO"))) {
      log_level_ = PLSMM_INFO;
    } else if (!strncmp(log_level_text_, "OLSMM_INFO", strlen("OLSMM_INFO"))) {
      log_level_ = OLSMM_INFO;
    } else if (!strncmp(log_level_text_, "PLSMM_TEST", strlen("PLSMM_TEST"))) {
      log_level_ = PLSMM_TEST;
    } else if (!strncmp(log_level_text_, "TRADING_ERROR", strlen("TRADING_ERROR"))) {
      log_level_ = TRADING_ERROR;
    } else if (!strncmp(log_level_text_, "TRADING_INFO", strlen("TRADING_INFO"))) {
      log_level_ = TRADING_INFO;
    } else if (!strncmp(log_level_text_, "TRADING_TEST", strlen("TRADING_TEST"))) {
      log_level_ = TRADING_TEST;
    } else if (!strncmp(log_level_text_, "BOOK_ERROR", strlen("BOOK_ERROR"))) {
      log_level_ = BOOK_ERROR;
    } else if (!strncmp(log_level_text_, "BOOK_INFO", strlen("BOOK_INFO"))) {
      log_level_ = BOOK_INFO;
    } else if (!strncmp(log_level_text_, "BOOK_TEST", strlen("BOOK_TEST"))) {
      log_level_ = BOOK_TEST;
    } else if (!strncmp(log_level_text_, "ORS_ERROR", strlen("ORS_ERROR"))) {
      log_level_ = ORS_ERROR;
    } else if (!strncmp(log_level_text_, "ORS_INFO", strlen("ORS_INFO"))) {
      log_level_ = ORS_INFO;
    } else if (!strncmp(log_level_text_, "MDS_ERROR", strlen("MDS_ERROR"))) {
      log_level_ = MDS_ERROR;
    } else if (!strncmp(log_level_text_, "MDS_INFO", strlen("MDS_INFO"))) {
      log_level_ = MDS_INFO;
    } else if (!strncmp(log_level_text_, "MDS_TEST", strlen("MDS_TEST"))) {
      log_level_ = MDS_TEST;
    } else if (!strncmp(log_level_text_, "LRDB_ERROR", strlen("LRDB_ERROR"))) {
      log_level_ = LRDB_ERROR;
    } else if (!strncmp(log_level_text_, "LRDB_INFO", strlen("LRDB_INFO"))) {
      log_level_ = LRDB_INFO;
    } else if (!strncmp(log_level_text_, "DBG_MODEL_ERROR", strlen("DBG_MODEL_ERROR"))) {
      log_level_ = DBG_MODEL_ERROR;
    } else if (!strncmp(log_level_text_, "DBG_MODEL_INFO", strlen("DBG_MODEL_INFO"))) {
      log_level_ = DBG_MODEL_INFO;
    } else if (!strncmp(log_level_text_, "PROM_OM_INFO", strlen("PROM_OM_INFO"))) {
      log_level_ = PROM_OM_INFO;
    } else if (!strncmp(log_level_text_, "BOOK_MINIMAL_TEST", strlen("BOOK_MINIMAL_TEST"))) {
      log_level_ = BOOK_MINIMAL_TEST;
    } else if (!strncmp(log_level_text_, "MUM_ERROR", strlen("MUM_ERROR"))) {
      log_level_ = MUM_ERROR;
    } else if (!strncmp(log_level_text_, "MUM_INFO", strlen("MUM_INFO"))) {
      log_level_ = MUM_INFO;
    } else if (!strncmp(log_level_text_, "SEEM_ERROR", strlen("SEEM_ERROR"))) {
      log_level_ = SEEM_ERROR;
    } else if (!strncmp(log_level_text_, "SEEM_INFO", strlen("SEEM_INFO"))) {
      log_level_ = SEEM_INFO;
    } else if (!strncmp(log_level_text_, "HPM_ERROR", strlen("HPM_ERROR"))) {
      log_level_ = HPM_ERROR;
    } else if (!strncmp(log_level_text_, "HPM_INFO", strlen("HPM_INFO"))) {
      log_level_ = HPM_INFO;
    } else if (!strncmp(log_level_text_, "SMVSELF_INFO", strlen("SMVSELF_INFO"))) {
      log_level_ = SMVSELF_INFO;
    } else if (!strncmp(log_level_text_, "SMVSELF_ERROR", strlen("SMVSELF_ERROR"))) {
      log_level_ = SMVSELF_ERROR;
    } else if (!strncmp(log_level_text_, "PVM_ERROR", strlen("PVM_ERROR"))) {
      log_level_ = PVM_ERROR;
    } else if (!strncmp(log_level_text_, "PVM_INFO", strlen("PVM_INFO"))) {
      log_level_ = PVM_INFO;
    } else if (!strncmp(log_level_text_, "INDICATOR_PRICE_TEST", strlen("INDICATOR_PRICE_TEST"))) {
      log_level_ = INDICATOR_PRICE_TEST;
    } else if (!strncmp(log_level_text_, "TRADEINIT_INFO", strlen("TRADEINIT_INFO"))) {
      log_level_ = TRADEINIT_INFO;
    } else if (!strncmp(log_level_text_, "RETAIL_INFO", strlen("RETAIL_INFO"))) {
      log_level_ = RETAIL_INFO;
    } else if (!strncmp(log_level_text_, "ATEX_INFO", strlen("ATEX_INFO"))) {
      log_level_ = ATEX_INFO;
    } else if (!strncmp(log_level_text_, "DBG_PARAM_INFO", strlen("DBG_PARAM_INFO"))) {
      log_level_ = DBG_PARAM_INFO;
    } else if (!strncmp(log_level_text_, "OPTIONS_INFO", strlen("OPTIONS_INFO"))) {
      log_level_ = OPTIONS_INFO;
    } else if (!strncmp(log_level_text_, "ORS_DATA_INFO", strlen("ORS_DATA_INFO"))) {
      log_level_ = ORS_DATA_INFO;
    } else if (!strncmp(log_level_text_, "FILL_TIME_INFO", strlen("FILL_TIME_INFO"))) {
      log_level_ = FILL_TIME_INFO;
    } else if (!strncmp(log_level_text_, "INDICATOR_INFO", strlen("INDICATOR_INFO"))) {
      log_level_ = INDICATOR_INFO;
    } else if (!strncmp(log_level_text_, "VOL_INFO", strlen("VOL_INFO"))) {
      log_level_ = VOL_INFO;
    } else if (!strncmp(log_level_text_, "SKIP_PACKET_DROPS_INFO", strlen("SKIP_PACKET_DROPS_INFO"))) {
      log_level_ = SKIP_PACKET_DROPS_INFO;
    } else {
      log_level_ = atoi(log_level_text_);
    }

    return log_level_;
  }

   virtual void LogQueryOrder(HFSAT::CDef::LogBuffer& buffer, char const *sec_shortcode_, char const *identifier_, ttime_t tv, HFSAT::TradeType_t side, bool is_eye, bool is_consec,
                             bool is_mod, bool is_new, double price, int32_t size, double theo_bid_px, double theo_ask_px,
                             double mkt_bid_px, double mkt_ask_px, double prim_bid_px, double prim_ask_px, double refprim_bid_px,
                             double refprim_ask_px, int32_t opp_sz, int32_t caos, int32_t unique_exec_id,
                             double prev_px, int32_t curr_int_price = -1, int32_t dimer_int_price = -1, bool is_dimer = false) {}

   virtual void LogQueryExec(HFSAT::CDef::LogBuffer &buffer, ttime_t tv, TradeType_t trade_type, int32_t position, int32_t caos,
                                     double trade_price, double theo_bid_px, double theo_ask_px,
                                     double mkt_bid_px, double mkt_ask_px, double primkt_bid_px,
                                     double primkt_ask_px, double refprim_bid_px, double refprim_ask_px,
                                     int32_t volume, int32_t target_pos, double total_traded_val, bool fill_type) {

     buffer.content_type_ = HFSAT::CDef::QueryExec;

     HFSAT::CDef::QueryExecStruct &exec = buffer.buffer_data_.query_exec_;
     exec.watch_tv_sec_ = tv.tv_sec;
     exec.watch_tv_usec_ = tv.tv_usec;

     exec.position_ = position;
     exec._caos_ = caos;
     exec.trade_price_ = trade_price;

     exec.theo_bid_px_ = theo_bid_px;
     exec.theo_ask_px_ = theo_ask_px;

     exec.mkt_bid_px_ = mkt_bid_px;
     exec.mkt_ask_px_ = mkt_ask_px;

     exec.primkt_bid_px_ = primkt_bid_px;
     exec.primkt_ask_px_ = primkt_ask_px;

     exec.refprim_bid_px_ = refprim_bid_px ;
     exec.refprim_ask_px_ = refprim_ask_px ;

     exec.volume_ = volume;
     exec.target_pos_ = target_pos;

     exec.total_traded_val_ = total_traded_val;
     exec.fill_type_ = fill_type;

     exec.trade_type_ = (trade_type == (HFSAT::TradeType_t::kTradeTypeBuy))
                       ? 'B'
                       : ((trade_type == HFSAT::TradeType_t::kTradeTypeSell) ? 'S' : 'N');

     (*this) << buffer.ToString() << "\n";
 
   }
};
}

#endif  // BASE_CDEF_DEBUG_LOGGER_H
