/**
    \file dvccode/CommonDataStructures/unordered_vector.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/

#ifndef BASE_COMMONDATASTRUCTURES_UNORDERED_VECTOR_H
#define BASE_COMMONDATASTRUCTURES_UNORDERED_VECTOR_H

#include <string.h>
#include <stdlib.h>

/**
 * A generic class that is optimized for add, remove and access. when we remove an element,
 * we do not reassign the index, but simply replace it with the last element
 * for example if we delete 3 from the list {1, 2, 3, 4, 5, 6} we get {1, 2, 6, 4, 5}
 * since the order is disturbed when we delete, hence the name unordered vector
 *
 * This can be used for instance when we have n listeners to an event and none of them are more important than the
 * other,
 * and during the run time any one of them could be removed
 */

namespace HFSAT {
template <class T>
class UnOrderedVec {
  unsigned int capacity;
  unsigned int curr_size;
  T* data;

  void double_capacity() {
    T* new_data = (T*)calloc(capacity * 2, sizeof(T));
    memcpy(new_data, data, capacity * sizeof(T));
    free(data);
    capacity *= 2;
    data = new_data;
  }

 public:
  UnOrderedVec(unsigned int def_capacity = 16)
      : capacity(def_capacity), curr_size(0), data((T*)calloc(capacity, sizeof(T))) {}

  unsigned int size() { return curr_size; }
  /**
   * unchecked index
   */
  inline T at(unsigned int i) { return data[i]; }

  /**
   * unchecked index
   * the last element is moved to this position. we are not bothered about order.
   */
  inline void remove_and_delete(unsigned int i) {
    data[i] = data[--curr_size];
    // note memory only expands, never contracts
  }

  inline void replace_element(unsigned int i, const T& item){
    data[i] = item;
  }

  inline void push_back(const T& item) {
    curr_size++;
    if (curr_size == capacity) {
      double_capacity();
    }
    data[curr_size - 1] = item;
  }
};
}

#endif /* BASE_COMMONDATASTRUCTURES_UnOrderedVec_H */
