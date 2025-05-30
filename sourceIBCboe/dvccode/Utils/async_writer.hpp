
#pragma once
#include <vector>
#include <string>
#include <iosfwd>
#include <map>
#include "dvccode/CDef/fwd_decl.hpp"
#include "dvccode/Utils/thread.hpp"

class AsyncWriter : public HFSAT::Thread {
  bool allocate_core;
  int qlen;
  char *ptr;
  volatile int continuelogging, allowaddition;
  ShmChannel *shm;
  std::vector<std::ofstream *> files;
  std::map<std::string, int> fname2id;
  void thread_main();

 public:
  static const size_t kMaxChannelCount = 255;
  char log_buffer[65536];
  AsyncWriter(bool allocate_core, int len = 16 * 1024 * 1024);
  ~AsyncWriter() {}
  void stop();
  ChannelId add(Cstr &fname);
  int log(ChannelId id, const char *msg, int len);
  int log(ChannelId id, const char *msg1, int len1,const char *msg2, int len2);
};
