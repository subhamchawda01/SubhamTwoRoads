
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <sstream>


#include "dvccode/Utils/writable.hpp"
#include "dvccode/Utils/misc.hpp"
#include "dvccode/Utils/async_writer.hpp"

using namespace std;  // Our coding standards specifies using std:: and not "using namespace std"

AsyncWriter::AsyncWriter(bool _allocate_core_, int _len_) : allocate_core(_allocate_core_), qlen(_len_), ptr(nullptr), continuelogging(0), allowaddition(1){
  shm = ShmChannel::AllocateInMemory(_len_, &ptr);
  assert(ptr);
}

void AsyncWriter::stop() { continuelogging = false; }

void AsyncWriter::thread_main() {
  if (!continuelogging) {
    allowaddition = 0;
    continuelogging = 1;
  }
  if ( allocate_core ) {
    setName("AsyncWriter");
    AllocateCPUOrExit();
  }
  else {
    Misc::AffineMe2Init();  
  }
  ShmChannelReader &rdr = *(new ShmChannelReader(ptr, qlen));

  // we are using a nested while loop because opening a try block generally has a big overhead;
  // so we dont want to put a try block in the inner loop;
  while (1) {
    try {
      while (1) {
        auto len = rdr.tryread(10);
        if (len) {
          auto id = rdr.id();
          // cout << "<<" << id << "\t" << len << "\n";
          auto f = files.at(id);
          assert(f);  // this is true because we are not allowed to remove a channel; once added, it always remains
                      // there
          f->write(rdr.data(), len);
        } else {
          // make sure we have not stopped;
          if (!continuelogging) {
            for (auto f : files) {
              f->close();
              delete f;
            }
            files.clear();
	    Thread::stop();
          }
          // since we are planning to sleep, might as well flush the files
          for (auto f : files) f->flush();
          // usleep(5000);
        }
      }
    } catch (out_of_range &err) {
      // todo: serious error; log somewhere/send email;
    }
  }
}

ChannelId AsyncWriter::add(Cstr &fname) {
  // if the file already exists, return the same channel id
  auto fid = fname2id.find(fname);
  if (fid != fname2id.end()) return fid->second;

  if (!allowaddition)  // todo: use a lock and remove this restriction
    throw runtime_error("AsyncWriter: cannot call add after starting the thread");

  if (files.size() == kMaxChannelCount) throw runtime_error("AsyncWriter: exceeded max channel count");

  // todo: use boost::iostream/rdbuf for better control on buffer size
  // since we dont check for overflow, we may have an overrun situation due to disk stalls
  // especially if we are bound to the init core
  auto f = new ofstream(fname, ios::binary | ios::app);
  if (f->bad()) throw runtime_error("async writer cannot open log file: " + fname);

  files.push_back(f);
  auto id = files.size() - 1;
  fname2id[fname] = id;
  return id;
}

int AsyncWriter::log(ChannelId id, const char *msg, int len) { return shm->write(id, (void *)msg, len); }

int AsyncWriter::log(ChannelId id, const char *msg1, int len1, const char *msg2, int len2) {
  memcpy(log_buffer, msg1, len1);
  memcpy(log_buffer + len1, msg2, len2);
  return shm->write(id, (void *)log_buffer, len1+len2); 
}