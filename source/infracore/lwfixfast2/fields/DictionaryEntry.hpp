/**
    \file DictionaryEntry.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

     Created on: Nov 3, 2011

*/
#pragma once
#include <map>
#include <string.h>

namespace FF_GLOB {
enum PrevValStatus { UNDEFINED_PREV_VAL, EMPTY_PREV_VAL, ASSIGNED_PREV_VAL };

template <class T>
struct PreviousVal {
  T val;
  PrevValStatus status;
  PreviousVal() : status(UNDEFINED_PREV_VAL) {}
  void reset() { status = UNDEFINED_PREV_VAL; }
};

template <class T>
class DictionaryStore {
  std::map<std::string, PreviousVal<T> > dictionaryEntries;

  DictionaryStore() : dictionaryEntries() {}

 public:
  static DictionaryStore<T>& getUniqInstance() {
    static DictionaryStore<T>* instance = NULL;
    if (instance == NULL) {
      instance = new DictionaryStore<T>();
    }
    return *instance;
  }

  PreviousVal<T>& Add(std::string uniqName) {
    if (dictionaryEntries.find(uniqName) == dictionaryEntries.end()) dictionaryEntries[uniqName] = PreviousVal<T>();
    return dictionaryEntries[uniqName];
  }
};
}
