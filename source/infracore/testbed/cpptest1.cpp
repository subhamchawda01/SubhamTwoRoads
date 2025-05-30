/**
    \file testbed/cpptest1.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/math/special_functions/fpclassify.hpp>

struct mystruc {
  int abc;
  double dbl;
  //  mystruc ( int _abc_, double _dbl_ ) : abc ( _abc_ ), dbl ( _dbl_ ) {}
  bool operator==(const mystruc& _newmystruc_) { return (abc == _newmystruc_.abc); }
  //  bool operator < ( const mystruc & _newmystruc_ ) const { return ( abc < _newmystruc_.abc ) ; }
  static bool mycompare(const mystruc& _s1, const mystruc& _s2) { return (_s1.abc < _s2.abc); }
};

int main() {
  mystruc a[5] = {{19, 2}, {17, 5}, {16, 8}, {13, 11}, {20, 7}};
  std::vector<mystruc> v(a, a + 5);  // copy of a

  std::sort(v.begin(), v.end(), mystruc::mycompare);
  std::cout << v[0].abc << std::endl;
  mystruc _tn_ = {17, 0};
  std::vector<mystruc>::iterator _it_ = std::lower_bound(v.begin(), v.end(), _tn_, mystruc::mycompare);
  std::cout << _it_->abc << std::endl;

  _tn_ = *_it_;
  std::cout << _tn_.dbl << std::endl;

  double b = 5;
  if (boost::math::isnan(b) || boost::math::isinf(b)) {
    std::cout << " __" << std::endl;
  }
}
