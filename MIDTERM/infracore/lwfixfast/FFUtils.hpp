/**
 \file ParseFFStream.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 162, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551

 Created on: Nov 2, 2011

 */

#pragma once

#include <iostream>
#include <inttypes.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "infracore/lwfixfast/fields/ByteArrayPool.hpp"

namespace FFUtils {
#define FF_SIGN_BIT 0x40
#define FF_STOP_BIT 0x80

// first element won't be used from negative pow.
// positive and negative powers separated to avoid multiplication with double if positive
static double quick_pow_negative_[11] = {0,        0.1,       0.01,       0.001,       0.0001,      0.00001,
                                         0.000001, 0.0000001, 0.00000001, 0.000000001, 0.0000000001};
static unsigned int quick_pow_positive_[10] = {1,      10,      100,      1000,      10000,
                                               100000, 1000000, 10000000, 100000000, 1000000000};
struct PMap {
  char* ch;
  int len;
  int currIndex;

  PMap() : ch(NULL), len(0), currIndex(0) {}

  bool nextBit() {
    if (len == 0) return false;
    bool toRet = (*(ch)&0x40);
    ++currIndex;
    *ch <<= 1;

    if (currIndex == 7) {
      currIndex = 0;
      --len;
      ++ch;
    }
    return toRet;
  }
};

struct Decimal {
  int32_t exponent;
  int64_t mantissa;

  double value() {
    if (exponent < -63 || exponent > 63) {
      std::cerr << "decimal decoding: invalid exponent in decimal field " << exponent << " " << mantissa << "\n";
    }

    if (exponent < -10 || exponent > 9) {
      std::cerr << "decimal decoding: not an error, but returning ZERO (exponent, mantissa) = " << exponent << " "
                << mantissa << "\n";
      return 0;
    }

    return exponent < 0 ? (mantissa * quick_pow_negative_[-exponent]) : (mantissa * quick_pow_positive_[exponent]);
  }

  // to allow decimal value calculation for cme when we have exponent and mantissa explicitly specified
  static double value(int32_t _exponent_, int64_t _mantissa_) {
    if (_exponent_ < -63 || _exponent_ > 63) {
      std::cerr << "decimal decoding: invalid exponent in decimal field " << _exponent_ << " " << _mantissa_ << "\n";
    }

    if (_exponent_ < -10 || _exponent_ > 9) {
      std::cerr << "decimal decoding: not an error, but returning ZERO (exponent, mantissa) = " << _exponent_ << " "
                << _mantissa_ << "\n";
      return 0;
    }

    return _exponent_ < 0 ? (_mantissa_ * quick_pow_negative_[-_exponent_])
                          : (_mantissa_ * quick_pow_positive_[_exponent_]);
  }

  Decimal() : exponent(0), mantissa(0){};
  Decimal(double d) {
    exponent = 0;
    while (true) {
      if ((int)d == d) {
        mantissa = (int)d;
        break;
      } else {
        --exponent;
        d *= 10;
      }
    }
  }
  void operator+=(const Decimal& arg) {
    mantissa += arg.mantissa;
    exponent += arg.exponent;
  }
};

struct ByteArr {
 public:
  ByteArr() : bytes(NULL), len(0), isnull(true) {}  // default construtor

  ByteArr(std::string str) {
    len = str.length();
    bytes = new char[str.length()];
    strncpy(bytes, str.c_str(), str.size());
    isnull = false;
  }

  char* bytes;
  uint32_t len;
  bool isnull;

  std::string toString() {
    char* c = ByteArrayPool::get();
    if (len > 0) memcpy(c, bytes, len);
    c[len] = '\0';
    return c;
  }

  // does not look for errors, supposed to be quick
  uint64_t parse_uint64_t_without_errors() {
    uint64_t val = 0;
    for (uint32_t i = 0; i < len; ++i) val = val * 10 + (bytes[i] - '0');
    return val;
  }

  bool contains_char(const char c) {
    for (uint32_t i = 0; i < len; ++i)
      if (c == bytes[i]) return true;

    return false;
  }

  void copyToDest(char* dest) { strncpy(dest, bytes, len); }
};

inline std::ostream& operator<<(std::ostream& output, const ByteArr& p) {
  static char* buf = new char[64];
  strncpy(buf, p.bytes, std::min(p.len, 64u));
  buf[std::min(p.len, 64u)] = 0;
  output << buf;
  return output;  // for multiple << operators.
}

inline std::ostream& operator<<(std::ostream& output, const Decimal& p) {
  output << "mantissa:" << p.mantissa << ", exponent:" << p.exponent << ", value:" << p.mantissa * pow(10, p.exponent);
  return output;  // for multiple << operators.
}

struct ByteVec : public ByteArr {
  ByteVec() : ByteArr() {}
  ByteVec(const ByteArr& copy) {
    len = (copy.len);
    bytes = (copy.bytes);
    isnull = (copy.isnull);
  }
};

class ByteStreamReader {
  char* startPointer;
  uint64_t len;
  uint64_t bytes_read;
  char* currPointer;

  ByteStreamReader(const ByteStreamReader& copy);  // disable it
 public:
  ByteStreamReader(char* buffer, uint32_t size)
      : startPointer(buffer), len(size), bytes_read(0), currPointer(startPointer) {}
  bool canRead() { return bytes_read < len; }

  PMap extractPmap() {
    PMap p;
    p.ch = this->currPointer;
    while ((*currPointer++ & FF_STOP_BIT) == 0)
      ;
    p.len = (currPointer - startPointer) - bytes_read;
    bytes_read += p.len;
    return p;
  }

  // No error checking on the size of bytes read
  void skipNBytes(int n) {
    bytes_read += n;
    if (bytes_read > len) {
      std::cerr << "error decoding from null stream\n";
      bytes_read = n;
    }
    currPointer = startPointer + bytes_read;
  }

  char* getCurrCharPointer() { return currPointer; }

  int getCurrPointer() { return bytes_read; }

  int getNumBytesToRead() { return len - bytes_read; }

  template <class T>
  bool interpret_int(T& val, bool isMandatory) {
    while ((*currPointer & FF_STOP_BIT) == 0) val = (val << 7) | *currPointer++;
    val = (val << 7) | (0x7f & *currPointer++);
    bytes_read = currPointer - startPointer;

    if (bytes_read > len) {
      std::cerr << "error decoding from null stream\n";
      return !isMandatory;
    }
    if (isMandatory) return false;
    if (val == 0)        // if optional and zero, it means null
      return true;       // NULL
    if (val > 0) --val;  // OPTIONAL and POSITIVE, subtract 1
    return false;        // not null
  }

  bool interpret(uint32_t& val, bool isMandatory) {
    val = 0;
    return interpret_int(val, isMandatory);
  }

  bool interpret(uint64_t& val, bool isMandatory) {
    val = 0;
    return interpret_int(val, isMandatory);
  }

  bool interpret(int32_t& val, bool isMandatory) {
    val = (*currPointer & FF_SIGN_BIT) ? -1 : 0;
    return interpret_int(val, isMandatory);
  }

  bool interpret(int64_t& val, bool isMandatory) {
    val = (*currPointer & FF_SIGN_BIT) ? -1 : 0;
    return interpret_int(val, isMandatory);
  }

  bool interpret(ByteArr& val, bool isMandatory) {
    val.bytes = currPointer;
    while ((*currPointer++ & FF_STOP_BIT) == 0)
      ;  // No check for overflow
    val.len = (currPointer - startPointer) - bytes_read;
    val.isnull = false;
    bytes_read += val.len;

    if (bytes_read > len) {  // already messed up. remove stray chars from the byte array
      std::cerr << "error decoding from null stream\n";

      currPointer = startPointer + len;
      val.len = currPointer - val.bytes;
      bytes_read = len;
    }

    *(currPointer - 1) &= 0x7f;  // we don't mind modifying the input stream itself.

    if (isMandatory) {
      if (val.len == 1 && val.bytes[0] == 0) {
        val.len = 0;
        return false;  // empty string
      }
      if (val.len == 2 && val.bytes[0] == 0 && val.bytes[1] == 0) {
        val.len = 1;
        return false;  //"\0"
      }
    } else {
      if (val.len == 1 && val.bytes[0] == 0) {
        val.isnull = true;
        return true;  // null
      }

      if (val.len == 2 && val.bytes[0] == 0 && val.bytes[1] == 0) {
        val.len = 0;
        return false;  // empty string
      }

      if (val.len == 3 && val.bytes[0] == 0 && val.bytes[1] == 0 && val.bytes[2] == 0) {
        val.len = 1;
        return false;  //"\0"
      }
    }
    return false;
  }

  bool interpret(ByteVec& val, bool isMandatory) {
    val.isnull = interpret(val.len, isMandatory);
    if (val.isnull) return true;
    val.bytes = getCurrCharPointer();
    skipNBytes(val.len);
    return false;
  }

  bool interpret(Decimal& val, bool isMandatory) {
    bool exponentNull = interpret(val.exponent, isMandatory);
    // if exponent is null we don't decode mantissa
    if (exponentNull) return true;
    interpret(val.mantissa, true);  // mantissa is never null if exponent is non-null
    return false;
  }
};
}
