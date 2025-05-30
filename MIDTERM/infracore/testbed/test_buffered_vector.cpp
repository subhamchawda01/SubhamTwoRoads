/**
    \file test_buffered_vector.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/
#include "dvccode/CommonDataStructures/buffered_vec.hpp"

struct MyData {
  int i;
};

int glob_cntr;

template <class MyData>
class DummyBufferedDataSrc : public HFSAT::BufferedDataSource<MyData> {
  virtual bool getNextData(MyData* data) {
    data->i = ++glob_cntr;
    std::cerr << "data src called " << glob_cntr << "\n";
    if (data->i == 1000) return false;  // marks end of list
    return true;
  }
};

int main(int argc, char** argv) {
  glob_cntr = 0;
  HFSAT::BufferedVec<MyData>* bv = new HFSAT::BufferedVec<MyData>(new DummyBufferedDataSrc<MyData>());

  HFSAT::BufferedVecIter<MyData>* it1 = bv->getIter();
  HFSAT::BufferedVecIter<MyData>* it2 = bv->getIter();

  for (int k = 0; k < 51; k++) {
    for (int i = 0; i < 10; i++) {
      MyData* t = it1->getItem();
      if (t == NULL) {
        std::cout << "end of t1 reached\n";
      } else
        std::cout << "reading from iter 1 " << t->i << "\n";
    }

    for (int i = 0; i < 20; i++) {
      MyData* t = it2->getItem();
      if (t == NULL) {
        std::cout << "end of t2 reached\n";
      } else
        std::cout << "reading from iter 2 " << t->i << "\n";
    }

    for (int i = 0; i < 10; i++) {
      MyData* t = it1->getItem();
      if (t == NULL) {
        std::cout << "end of t1 reached\n";
      } else
        std::cout << "reading from iter 1 " << t->i << "\n";
    }
  }
}
