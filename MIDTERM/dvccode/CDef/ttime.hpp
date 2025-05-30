/**
    \file dvccode/CDef/ttime.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_CDEF_TTIME_H
#define BASE_CDEF_TTIME_H

#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <sys/time.h>

#include <sstream>

namespace HFSAT {

inline int usleep(long usec) {
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = usec % 1000000L;
  return select(0, 0, 0, 0, &tv);
}

#define USING_CONCISE_TIME_OBJECT
// #undef USING_CONCISE_TIME_OBJECT

#ifdef USING_CONCISE_TIME_OBJECT

/// A class to encapsulate time, similar to timeval, except using 8 bytes not 16.
/// Not sure which is faster
struct ttime_t {
  union {
    struct {
      int tv_usec;
      int tv_sec;
    };
    unsigned long val;
  };

  /* only added as a version of setnull */
  ttime_t() { val = 0; }
  ttime_t(const timeval &tv_)
      : tv_usec(tv_.tv_usec), tv_sec(tv_.tv_sec) {}  ///< added to pacify OnTimeReceived called in LiveListeners
  explicit ttime_t(int ts, int tu) : tv_usec(tu % 1000000), tv_sec(ts + (tu / 1000000)) {}
  explicit ttime_t(time_t ts, int tu) : tv_usec(tu), tv_sec((int)ts) {}
  //    explicit ttime_t ( const ttime_t & o ) : val ( o.val ) {}

  inline ttime_t &operator=(const timeval &tv) {
    tv_usec = tv.tv_usec;
    tv_sec = tv.tv_sec;
    return *this;
  }

  inline bool operator==(const ttime_t &tv) const { return (val == tv.val); }

  inline bool operator!=(const ttime_t &tv) const { return (val != tv.val); }

  ttime_t operator-(const ttime_t &b) const {
    ttime_t t_ttm_;
    t_ttm_.tv_sec = tv_sec - b.tv_sec;
    t_ttm_.tv_usec = tv_usec - b.tv_usec;
    if (t_ttm_.tv_usec <
        0) {  // Since usec comes after decimal, it should always be positive to avoid non-integer numbers
      t_ttm_.tv_sec -= 1;
      t_ttm_.tv_usec += 1000000;
    }
    return t_ttm_;
  }

  ttime_t operator+(const ttime_t &b) const {
    ttime_t t_ttm_;
    t_ttm_.tv_sec = tv_sec + b.tv_sec;
    t_ttm_.tv_usec = tv_usec + b.tv_usec;
    if (t_ttm_.tv_usec >= 1000000) {
      t_ttm_.tv_sec += 1;
      t_ttm_.tv_usec -= 1000000;
    }

    return t_ttm_;
  }

  ttime_t operator*(const double t_factor_) const {
    ttime_t t_time_;
    double micro_seconds_ = (tv_sec * 1000000.0 + tv_usec) * t_factor_;

    t_time_.tv_sec = (int)(micro_seconds_ / 1000000);
    t_time_.tv_usec = (int)(((unsigned long long)micro_seconds_) % 1000000);

    return t_time_;
  }

  ttime_t operator/(const double t_factor_) const {
    ttime_t t_time_;

    double micro_seconds_ = (tv_sec * 1000000.0 + tv_usec) / t_factor_;

    t_time_.tv_sec = (int)(micro_seconds_ / 1000000);
    t_time_.tv_usec = (int)(((unsigned long long)micro_seconds_) % 1000000);

    return t_time_;
  }

  inline int getmfm() const {
    unsigned long t_msecs_ = long(tv_sec) * 1000 + tv_usec / 1000;
    return (t_msecs_ % 86400000);
  }

  inline void addmsecs(int msecs) {
    tv_usec += msecs * 1000;
    if (tv_usec >= 1000000) {
      tv_sec += tv_usec / 1000000;
      tv_usec = tv_usec % 1000000;
    } else if (tv_usec < 0) {  // For -ve msecs value.
      while (tv_usec < 0) {
        tv_sec--;
        tv_usec += 1000000;
      }
    }
  }
  inline void addusecs(int usecs_) {
    tv_usec += usecs_;
    if (tv_usec >= 1000000) {
      tv_sec += tv_usec / 1000000;
      tv_usec = tv_usec % 1000000;
    } else if (tv_usec < 0) {  // For -ve usecs_ value.
      while (tv_usec < 0) {
        tv_sec--;
        tv_usec += 1000000;
      }
    }
  }

  inline std::string ToString() {
    std::ostringstream t_oss_;
    t_oss_ << tv_sec << "." << std::setw(6) << std::setfill('0') << tv_usec;
    return t_oss_.str();
  }

  friend std::ostream &operator<<(std::ostream &out, const ttime_t &tv_ttt);
};

inline std::ostream &operator<<(std::ostream &out, const ttime_t &tv_ttt) {
  out << tv_ttt.tv_sec << "." << std::setw(6) << std::setfill('0') << tv_ttt.tv_usec;
  return out;
}

inline ttime_t GetTimeFromSec(const time_t _tvsec_) { return ttime_t(_tvsec_, 0); }

/// Returns ttime_t instead of timeval
inline ttime_t GetTimeOfDay() {
  ttime_t tv_ttt;
  timeval tv_;
  gettimeofday(&tv_, NULL);
  tv_ttt.val =
      ((unsigned long)tv_.tv_usec) |
      (((unsigned long)tv_.tv_sec) << 32);  // same as tv_ttt.tv_sec = tv_.tv_sec; tv_ttt.tv_usec = tv_.tv_usec;
  return tv_ttt;
}

// inline int GetMsecsFrom ( ttime_t _tv_ttt_, time_t _tvsec_ )
// {
//   return ( ( _tv_ttt_.tv_sec - _tvsec_ ) * 1000 ) + ( _tv_ttt_.tv_usec / 1000 ) ;
// }

inline bool operator<(const ttime_t &t1, const ttime_t &t2) {
  return (t1.val < t2.val);  // same as line below, since usec is before sec in struct
  //    return ( t1.tv_sec < t2.tv_sec ) || ( ( t1.tv_sec == t2.tv_sec ) && ( t1.tv_usec < t2.tv_usec ) ) ;
}

inline bool operator<=(const ttime_t &t1, const ttime_t &t2) {
  return (t1.val <= t2.val);  // same as line below, since usec is before sec in struct
  //    return ( t1.tv_sec <= t2.tv_sec ) || ( ( t1.tv_sec == t2.tv_sec ) && ( t1.tv_usec <= t2.tv_usec ) ) ;
}

inline bool operator>(const ttime_t &t1, const ttime_t &t2) {
  return (t1.val > t2.val);  // same as line below, since usec is before sec in struct
  //    return ( t1.tv_sec > t2.tv_sec ) || ( ( t1.tv_sec == t2.tv_sec ) && ( t1.tv_usec > t2.tv_usec ) ) ;
}

inline bool operator>=(const ttime_t &t1, const ttime_t &t2) {
  return (t1.val >= t2.val);  // same as line below, since usec is before sec in struct
  //    return ( t1.tv_sec >= t2.tv_sec ) || ( ( t1.tv_sec == t2.tv_sec ) && ( t1.tv_usec >= t2.tv_usec ) ) ;
}

#else  // USING_CONCISE_TIME_OBJECT

typedef timeval ttime_t;

inline ttime_t GetTimeFromSec(const time_t _tvsec_) { return ttime_t(_tvsec_, 0); }

inline ttime_t GetTimeOfDay() {
  timeval tv_;
  gettimeofday(&tv_, NULL);
  return tv_;
}

// inline int GetMsecsFrom ( ttime_t _tv_ttt_, time_t _tvsec_ )
// {
//   return ( ( _tv_ttt_.tv_sec - _tvsec_ ) * 1000 ) + ( _tv_ttt_.tv_usec / 1000 ) ;
// }

inline bool operator<(const ttime_t &t1, const ttime_t &t2) {
  return (t1.tv_sec < t2.tv_sec) || ((t1.tv_sec == t2.tv_sec) && (t1.tv_usec < t2.tv_usec));
}

inline bool operator<=(const ttime_t &t1, const ttime_t &t2) {
  return (t1.tv_sec < t2.tv_sec) || ((t1.tv_sec == t2.tv_sec) && (t1.tv_usec <= t2.tv_usec));
}

#endif  // USING_CONCISE_TIME_OBJECT
}

#endif  // BASE_CDEF_TTIME_H
