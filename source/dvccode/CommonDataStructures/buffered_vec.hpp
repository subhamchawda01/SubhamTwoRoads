/**
    \file buffered_vec.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#ifndef BASE_COMMONDATASTRUCTURES_BUFFERED_VEC_HPP_
#define BASE_COMMONDATASTRUCTURES_BUFFERED_VEC_HPP_

#include <inttypes.h>
#include <deque>
#include <vector>
#include "dvccode/CommonDataStructures/simple_mempool.hpp"

/**
 * A set of classes to get several iterators to a file like structure. Typical usage will be to transform timed data to
 * reg data.
 * This is based on the assumption that the stream that we are operaing on typically will exhibit locality. Its not that
 * we want to
 * operate on very large apart set of data.
 * Typical use case will be conversion of timed data to reg data
 * advantages: i) buffering, ii) single parsing/processing of raw data, iii) probably better set of API exposed
 */
namespace HFSAT {

//////////////////////////////////////////

template <class T>
class BufferedDataSource {
 public:
  virtual bool getNextData(T* data) = 0;
};

template <class T>
class BufferedVec;
template <class T>
class BufferedVecIter;

//////////////////////////////////////////

template <class T>
class BufferedVec {
  BufferedDataSource<T>* data_src;
  SimpleMempool<T>* mempool;
  std::deque<T*> data_store;
  std::vector<BufferedVecIter<T>*> all_iters;

  uint64_t sz_cleared;
  bool src_read_fully;

  void clearBuffer() {
    for (uint32_t i = 0; i < all_iters.size(); ++i) {
      if (all_iters.at(i)->getPos() == sz_cleared) {
        // std::cerr << "clear buffer not applicable:: ("<< i << " " << all_iters.at(i)->getPos () << ") \n";
        return;
      }
    }
    mempool->DeAlloc(data_store.at(0));
    data_store.pop_front();
    sz_cleared++;
    // std::cerr << "clear buffer applicable: sz_cleared " << sz_cleared << "\n";
  }

  T* fetchItem(uint64_t& pos) {
    if (pos < sz_cleared) {
      // std::cerr << "BufferedVec::fetchItem called for position " << pos << " which has already been removed from
      // buffer. Exiting\n";
      exit(-1);
    }

    while (!src_read_fully && data_store.size() + sz_cleared <= pos) {
      T* tmp = mempool->Alloc();
      bool success = data_src->getNextData(tmp);
      if (!success) {
        src_read_fully = true;
        break;
      }
      data_store.push_back(tmp);
    }
    if (pos >= sz_cleared + data_store.size()) return NULL;

    T* toRet = data_store[pos - sz_cleared];
    ++pos;
    // std::cerr << "position incremented after fetch to " << pos << "\n";
    if (pos == 1 + sz_cleared) {
      clearBuffer();
    }
    return toRet;
  }

 public:
  BufferedVec(BufferedDataSource<T>* src_)
      : data_src(src_), data_store(), all_iters(), sz_cleared(0), src_read_fully(false) {
    mempool = new SimpleMempool<T>();
  }

  ~BufferedVec() {
    all_iters.clear();
    data_store.clear();
    delete mempool;
  }

  /**
   * returns the lowest possible data index that is still in the buffer
   */
  BufferedVecIter<T>* getIter() {
    all_iters.push_back(new BufferedVecIter<T>(this, sz_cleared));
    return all_iters.back();
  }

  friend class BufferedVecIter<T>;
};

//////////////////////////////////////////

template <class T>
class BufferedVecIter {
  BufferedVec<T>* vec;
  uint64_t pos;
  BufferedVecIter(BufferedVec<T>* vec_, uint64_t pos_) : vec(vec_), pos(pos_) {}
  BufferedVecIter(const BufferedVecIter&);  // disable copy
  void operator=(const BufferedVecIter&);   // disable copy
 public:
  ~BufferedVecIter() { vec = NULL; }
  uint64_t getPos() { return pos; }

  /// Make sure that this pointer is consumed immediately after calling
  /// If we want to make use of the data, we need to copy it to some variable in the caller
  /// The design is such that once the iterator reads a data, it is marked for deletion in the
  /// sense that any new items to be stored in the buffer can make use of this location
  T* getItem() { return vec->fetchItem(pos); }

  void ResetTo(const BufferedVecIter& it) { this->pos = it.pos; }
  friend class BufferedVec<T>;
};
}

#endif /* BASE_COMMONDATASTRUCTURES_BUFFERED_VEC_HPP_ */
