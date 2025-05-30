#include <iostream>

#include "dvccode/Utils/CPUAffinity.hpp"

int main() {
  std::cout << "Enter PID" << std::endl;
  int pid = -1;
  int option = -1;

  std::cin >> pid;

  std::cout << "Enter Option. 1 for set 0 for get " << std::endl;
  std::cin >> option;

  if (option) {
    int core = -1;
    std::cout << "Enter core " << std::endl;
    std::cin >> core;

    std::cout << "Affinity set returned " << CPUManager::setAffinity(core, pid) << std::endl;
  }

  else
    std::cout << "Affinity get returned " << CPUManager::getAffinity(pid) << std::endl;
  return 0;
}
