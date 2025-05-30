#include <iostream>
int main(int argc, char** arv) {
#if __GNUC__ >= 4
#if __GNUC_MINOR__ >= 3
  std::cout << __VERSION__ << std::endl;
#endif
#endif
  std::cout << __GNUC__ << "-" << __GNUC_MINOR__ << "-" << __GNUC_PATCHLEVEL__ << std::endl;
  return 0;
}
