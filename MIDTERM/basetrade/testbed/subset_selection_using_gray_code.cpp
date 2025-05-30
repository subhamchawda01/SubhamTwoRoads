#include <iostream>
#include <vector>
#include <map>
#include <stdio.h>
#include <algorithm>

int combine(int* c, int k, int n) {
  for (int i = k; --i >= 0;) {
    if (++c[i] <= n - (k - i)) {
      while (++i < k) c[i] = c[i - 1] + 1;
      return 1;
    }
  }
  return 0;
}

void PrintSubsets() {
  int source[3] = {1, 2, 3};
  int currentSubset = 7;
  int tmp;
  while (currentSubset) {
    printf("(");
    tmp = currentSubset;
    for (int i = 0; i < 3; i++) {
      if (tmp & 1) printf("%d ", source[i]);
      tmp >>= 1;
    }
    printf(")\n");
    currentSubset--;
  }
}

void print_subset(std::vector<bool> bit_mask, std::size_t req_size, std::vector<int>& oput) {
  if (std::count(bit_mask.begin(), bit_mask.end(), true) == req_size) {
    static int cnt = 0;
    std::cout << ++cnt << ". [ ";
    for (std::size_t i = 0; i < bit_mask.size(); ++i)
      if (bit_mask[i]) {
        oput.push_back(i + 1);
        std::cout << i + 1 << ' ';
      }
    std::cout << "]\n";
  }
}

// generate the next Gray code (in reverse)
// http://en.wikipedia.org/wiki/Gray_code
bool next_bitmask(std::vector<bool>& bit_mask) {
  std::size_t i = 0;
  for (; (i < bit_mask.size()) && bit_mask[i]; ++i) bit_mask[i] = false;

  if (i < bit_mask.size()) {
    bit_mask[i] = true;
    return true;
  } else
    return false;
}

int factorial(int n) { return (n == 1 || n == 0) ? 1 : factorial(n - 1) * n; }

int main() {
  std::size_t k, n;
  std::cout << "n? " && std::cin >> n;
  std::cout << "k? " && std::cin >> k;

  int size = factorial(n) / (factorial(k) * factorial(n - k));
  size = size * k;
  std::vector<int> output;
  std::vector<bool> bit_mask(n);

  int m = 0;
  do {
    print_subset(bit_mask, k, output);
  } while (next_bitmask(bit_mask));

  for (int i = 0; i < output.size();) {
    for (int j = 0; j < k; j++, i++) {
      std::cout << output[i] << " ";
    }
    std::cout << "\n";
  }
}

/*int main ( int argc, char ** argv )
{
  int n = 0;
  int *c ;
  int k = 0;

  n = atoi ( argv[0] );
  k = atoi ( argv[1] );

  combine ( c , k , n ) ;
  PrintSubsets ( ) ;


}
*/
