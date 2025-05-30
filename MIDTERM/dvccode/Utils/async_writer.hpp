
#pragma once
#include <vector>
#include <string>
#include <iosfwd>
#include <map>
#include "dvccode/CDef/fwd_decl.hpp"

class AsyncWriter {
  int qlen;
  char *ptr;
  volatile int continuelogging, allowaddition;
  ShmChannel *shm;
  boost::thread *worker;
  std::vector<std::ofstream *> files;
  std::map<std::string, int> fname2id;
  void log_main();

 public:
  static const size_t kMaxChannelCount = 255;

  AsyncWriter(int len = 16 * 1024 * 1024);

  void start();
  void stop();
  ChannelId add(Cstr &fname);
  int log(ChannelId id, const char *msg, int len);
};
