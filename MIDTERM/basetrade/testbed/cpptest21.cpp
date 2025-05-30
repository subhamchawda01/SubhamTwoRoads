// range heap example
#include <iostream>
#include <algorithm>
#include <vector>
#include "basetrade/testbed/ticks.hpp"
using namespace std;

#define LEN 10240
void showvec(std::vector<int>& v) {
  cout << "sorted range :";
  for (unsigned i = 0; i < v.size(); i++) cout << " " << v[i];
  cout << endl;
}

int main() {
  int myints[] = {10, 170, 30, 5, 15, 23, 110, 89, 41, 23, 7, 6};
  vector<int> u(myints, myints + 12);
  vector<int> v = u;

  showvec(v);
  make_heap(v.begin(), v.end(), std::greater<int>());  // 1031
  showvec(v);

  pop_heap(v.begin(), v.end(), std::greater<int>());  // 416
  showvec(v);

  v.back() = 16;
  push_heap(v.begin(), v.end(), std::greater<int>());  // 551
  showvec(v);

  sort_heap(v.begin(), v.end(), std::less<int>());  // 2086
  showvec(v);

  return 0;
}
