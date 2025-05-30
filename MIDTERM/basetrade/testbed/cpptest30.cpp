#include <string>
#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>

#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/lock.hpp"

/// to compile g++ cpptest30.cpp -I$HOME/infrabase_install -L$HOME/infrabase_install/libdebug -lpthread
/// #-I/apps/boost/include/ -L/apps/boost/lib

using namespace HFSAT;

class SimpleThread : public Thread {
 public:
  SimpleThread(Lock& _stmutex_lock_, int _start_counter_)
      : stmutex_lock_(_stmutex_lock_), start_counter_(_start_counter_) {}

  void thread_main() {
    for (int i = 0; i < 5; i++) {
      stmutex_lock_.LockMutex();
      std::cout << start_counter_++ << std::endl;
      sleep(1);
      std::cout << start_counter_++ << std::endl;
      sleep(1);
      std::cout << start_counter_++ << std::endl;
      sleep(1);
      stmutex_lock_.UnlockMutex();
      sleep(1);
    }
  }

 private:
  Lock& stmutex_lock_;
  int start_counter_;
};

int main(int argc, char** argv) {
  Lock stmutex_lock_;

  SimpleThread st1(stmutex_lock_, 0);
  SimpleThread st2(stmutex_lock_, 1000);
  st1.run();
  st2.run();

  int abc;
  std::cin >> abc;
  std::cout << " Calling join on threads" << std::endl;
  st1.stop();
  st2.stop();
  std::cout << " Thread::stop returned " << std::endl;
}
