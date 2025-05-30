/**
    \file testbed/cpptest53.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include <math.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <memory>

////////////////

namespace HFSAT {
template <class T>
struct dynarray {
  // types:
  typedef T value_type;
  typedef T& reference;
  typedef const T& const_reference;
  typedef T* iterator;
  typedef const T* const_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef size_t size_type;

  // fields:
 private:
  T* store;
  size_type count;

  // helper functions:
  T* alloc(size_type n) { return reinterpret_cast<T*>(new char[n * sizeof(T)]); }

 public:
  // construct and destruct:
  dynarray() = delete;
  const dynarray operator=(const dynarray&) = delete;

  explicit dynarray(size_type c) : store(alloc(c)), count(c) {
    size_type i;
    try {
      for (size_type i = 0; i < count; ++i) new (store + i) T;
    } catch (...) {
      for (; i > 0; --i) (store + (i - 1))->~T();
      throw;
    }
  }

  dynarray(const dynarray& d) : store(alloc(d.count)), count(d.count) {
    try {
      uninitialized_copy(d.begin(), d.end(), begin());
    } catch (...) {
      delete store;
      throw;
    }
  }

  ~dynarray() {
    for (size_type i = 0; i < count; ++i) (store + i)->~T();
    delete[] store;
  }

  // iterators:
  iterator begin() { return store; }
  const_iterator begin() const { return store; }
  const_iterator cbegin() const { return store; }
  iterator end() { return store + count; }
  const_iterator end() const { return store + count; }
  const_iterator cend() const { return store + count; }

  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const { return reverse_iterator(begin()); }

  // capacity:
  size_type size() const { return count; }
  size_type max_size() const { return count; }
  bool empty() const { return count == 0; }

  // element access:
  reference operator[](size_type n) { return store[n]; }
  const_reference operator[](size_type n) const { return store[n]; }

  reference front() { return store[0]; }
  const_reference front() const { return store[0]; }
  reference back() { return store[count - 1]; }
  const_reference back() const { return store[count - 1]; }

  const_reference at(size_type n) const { return store[n]; }
  reference at(size_type n) { return store[n]; }

  // data access:
  T* data() { return store; }
  const T* data() const { return store; }
};

}  // namespace HFSAT

////////////////

typedef unsigned long cyclecount_t;

class CpucycleProfiler {
 public:
  /** @brief return the curent cpu cyclecount */
  static __inline__ cyclecount_t GetCpucycleCount(void) {
    uint32_t lo, hi;
    __asm__ __volatile__(  // serialize
        "xorl %%eax,%%eax \n        cpuid" ::
            : "%rax", "%rbx", "%rcx", "%rdx");
    /* We cannot use "=A", since this would use %rax on x86_64 and return only the lower 32bits of the TSC */
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return (cyclecount_t)hi << 32 | lo;
  }

  /// \brief returns the difference in cyclecount_t
  static __inline__ cyclecount_t Diff(cyclecount_t t1, cyclecount_t t0) { return (t1 - t0); }

  /// \brief returns the difference in cyclecount_t in Kilo ( or multiples of 2^10 = 1024 )
  static __inline__ cyclecount_t KDiff(cyclecount_t t1, cyclecount_t t0) { return (t1 - t0) >> 10; }

  /// \brief returns the difference in cyclecount_t in Mega ( or multiples of 2^20 = 1024*1024 )
  static __inline__ cyclecount_t MDiff(cyclecount_t t1, cyclecount_t t0) { return (t1 - t0) >> 20; }

  static __inline__ cyclecount_t ConvertCycleCountToUsec(cyclecount_t t) { return (t >> 10) / 3; }
};

int main(int argc, char** argv) {
  if (argc < 2) {
    exit(0);
  }
  int num_secids_ = atoi(argv[1]);
  std::cout << num_secids_ << std::endl;

  double arbit_retval_ = 0.0;
  size_t num_trips_ = 100000000;

  int counted_msecs_ = 0;

  //  std::vector < double > partial_sums_ ( num_secids_, 0.0 );
  HFSAT::dynarray<double> partial_sums_(num_secids_);
  std::fill_n(partial_sums_.begin(), num_secids_, 0.0);

  struct timeval start_loop_time_;
  gettimeofday(&start_loop_time_, nullptr);
  cyclecount_t start_cycle_count_ = CpucycleProfiler::GetCpucycleCount();
  for (size_t i = 0; i < num_trips_; i++) {
    double this_val_ = pow((double)i, 0.1);
    partial_sums_[i % num_secids_] += this_val_;
  }

  cyclecount_t end_cycle_count_ = CpucycleProfiler::GetCpucycleCount();
  struct timeval end_loop_time_;
  gettimeofday(&end_loop_time_, NULL);

  printf("Arbit Retval %f timediff_usec %ld cyclediff %ld cycletimediff %ld microseconds counted_msecs_ = %d\n",
         arbit_retval_, ((end_loop_time_.tv_sec - start_loop_time_.tv_sec) * 1000000) +
                            (end_loop_time_.tv_usec - start_loop_time_.tv_usec),
         CpucycleProfiler::Diff(end_cycle_count_, start_cycle_count_),
         CpucycleProfiler::ConvertCycleCountToUsec(end_cycle_count_ - start_cycle_count_), counted_msecs_);

  int print_index_ = 0;
  std::cin >> print_index_;
  std::cout << partial_sums_[print_index_ % num_secids_];
}
