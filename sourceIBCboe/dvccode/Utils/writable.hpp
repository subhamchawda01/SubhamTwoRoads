#ifndef WRITABLE_HPP
#define WRITABLE_HPP 1
#include <cstdint>
#include <dvccode/CDef/fwd_decl.hpp>

struct Writable {
  virtual ~Writable() {}
  virtual int write(void* src, int len) = 0;
};

typedef int64_t i8;

class ShmChannel {
  friend ShmMultiChannel;
  friend ShmChannelReader;
  char* ptr;
  volatile i8 bytesWritten;
  i8 len;
  int wrapmask;

  void UpdateBytesWritten() {
    // todo: use a barrier; otherwise subsequent reads maybe incorrrect
    auto end = ((volatile i8*)(ptr + len + sizeof(i8)));
    *end = bytesWritten;
  }
  i8 BytesWritten() {
    // todo: use a barrier; otherwise subsequent reads maybe incorrrect
    auto end = ((volatile i8*)(ptr + len + sizeof(i8)));
    return *end;
  }
  i8 Position() const { return bytesWritten & wrapmask; }
  int SegmentSize() const { return len + 2 * sizeof(i8); }
  ShmChannel(char* ptr_, int len_);

  static void insertWraparoundMarker(char* pos) {
    i8* p = (i8*)pos;
    *p = -1;
  }

 public:
  static const int kHeaderLen = 2 * sizeof(int);

  static ShmChannel* AllocateInMemory(int len, char** ptr);

  ShmChannel(int key, int count, int size, int _wrapmask);
  int write(ChannelId channel, void* src, int len);
};

class ShmChannelReader {
  ShmChannel chan;
  i8 bytesRead, bytesWrittenCached;
  char* ptr;

  bool wraparoundMarkerRead() const {
    i8* p = (i8*)ptr;
    return *p == -1;
  }

 public:
  ShmChannelReader(char* ptr_, int len_);
  ShmChannelReader(int key, int count, int size, int _wrapmask);

  ChannelId id() const { return ((int*)ptr)[0]; }

  i8 Position() const { return bytesRead & chan.wrapmask; }
  int size() const { return ((int*)ptr)[1]; }
  char* data() const { return ptr + ShmChannel::kHeaderLen; }

  int read();
  int tryread(int attempts = 1);
};

class ShmMultiChannelFactory {
  ShmChannel* chan;

 public:
  ShmMultiChannelFactory(int len, char** pptr);
  ShmMultiChannelFactory(int key, int count, int size, int _wrapmask);
  Writable* allocate(ChannelId channelid);
};

#endif  // !WRITABLE_HPP
