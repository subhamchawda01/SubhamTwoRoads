/**
    \file dvccode/CDef/random_number_generator.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_CDEF_RANDOM_NUMBER_GENERATOR_H
#define BASE_CDEF_RANDOM_NUMBER_GENERATOR_H

#include <iostream>
#include <stdlib.h>
#include <math.h>

namespace HFSAT {

/// Class to generate random numbers
class RandomNumberGenerator {
 private:
  // to disable copy constructor
  RandomNumberGenerator(const RandomNumberGenerator &);

 protected:
  unsigned int seed;
  RandomNumberGenerator() { _SetSeed(); }

  inline bool _GetSuccess(double _prob_ = 0.5) { return (rand() < _prob_ * RAND_MAX); }

  inline void _SetSeed(unsigned int _seed_ = time(NULL)) {
    seed = _seed_;
    srand(seed);
  }

  inline unsigned int _GetRandNum(unsigned int _max_val_ = RAND_MAX) {
    return rand() % (std::max(1u, _max_val_)) + 1;  // random no from 1-_max_val_
  }

 public:
  static inline RandomNumberGenerator &GetUniqueInstance() {
    static RandomNumberGenerator uniqueinstance_;
    return uniqueinstance_;
  }

  static inline bool GetSuccess(double _prob_ = 0.5) { return GetUniqueInstance()._GetSuccess(_prob_); }

  static inline void SetSeed(unsigned int _seed_ = time(NULL)) { return GetUniqueInstance()._SetSeed(_seed_); }

  static inline unsigned int GetSeed() { return GetUniqueInstance().seed; }

  static inline unsigned int GetRandNum(unsigned int _max_val_ = RAND_MAX) {
    return GetUniqueInstance()._GetRandNum(_max_val_);
  }
};
}

#endif  // BASE_CDEF_RANDOM_NUMBER_GENERATOR_H
