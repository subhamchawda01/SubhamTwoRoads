
#include <xmmintrin.h>

#include <vector>
#include <iostream>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "dvccode/Utils/writable.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"

using namespace std;

static char* createshm(int key, int len) {
  int shmid = shmget(key, len, IPC_CREAT | 0666);
  if (shmid < 0) {
    cout << "Size of segment = " << len << " bytes key = " << key << endl;
    cout << "Failed to shmget error = " << strerror(errno) << endl;
    if (errno == EINVAL)
      printf("Invalid segment size specified\n");
    else if (errno == EEXIST)
      printf("Segment exists, cannot create it\n");
    else if (errno == EIDRM)
      printf("Segment is marked for deletion or was removed\n");
    else if (errno == ENOENT)
      printf("Segment does not exist\n");
    else if (errno == EACCES)
      printf("Permission denied\n");
    else if (errno == ENOMEM)
      printf("Not enough memory to create segment\n");
    return nullptr;
  }

  char* ptr = (char*)shmat(shmid, NULL, 0);
  if ((char*)-1 == ptr) {
    perror("shmat");
    return nullptr;
  }
  struct shmid_ds shm_ds;
  if (shmctl(shmid, IPC_STAT, &shm_ds) == -1) {
    perror("shmctl");
    return nullptr;
  }
  memset(ptr, 0, len);
  return ptr;
}

ShmChannel* ShmChannel::AllocateInMemory(int len, char** pptr) {
  int seg_size = len + 2 * sizeof(i8);  // Adding extra 8 bytes such that wrap around marker doesn't get overwritten by
                                        // 'bytesWritten' update which is done at last 8 bytes of ptr.
  char* ptr = (char*)calloc(seg_size, 1);
  if (!ptr) throw std::runtime_error("failed to create shm segment");
  *pptr = ptr;
  return new ShmChannel(ptr, len);
}

ShmChannel::ShmChannel(char* ptr_, int len_) {
  wrapmask = len_ - 1;
  bytesWritten = 0;
  len = len_;
  ptr = ptr_;
}

ShmChannel::ShmChannel(int key, int count, int size, int _wrapmask) {
  // int at last to read the msg id
  size += kHeaderLen;
  // assert(len == (1 << logf(len)));
  wrapmask = _wrapmask;
  bytesWritten = 0;
  len = count * size;
  int seg_size = len + 2 * sizeof(i8);  // Adding extra 8 bytes such that wrap around marker doesn't get overwritten by
                                        // 'bytesWritten' update which is done at last 8 bytes of ptr.
  ptr = createshm(key, seg_size);
  if (!ptr) throw std::runtime_error("failed to create shm segment");
  memset(ptr, 0, seg_size);
}

int ShmChannel::write(ChannelId id, void* src, int size) {
  if (size < 0 || size >= len)  // todo: can remove this check and make sure everyone else respects this
    return 0;

  int bytesleft = len - Position(), bytesneeded = size + kHeaderLen;

  auto bytesWrittenLocal = bytesWritten;  // this local is necessary; without this, the compiler can update the memory
                                          // and the other thread can see inconsistent state
  if (bytesleft < bytesneeded + kHeaderLen) {  // the second header len is for the wraparound marker if required. this
                                               // does not permit, perfect fit
    insertWraparoundMarker(ptr + (bytesWrittenLocal & wrapmask));
    bytesWrittenLocal += bytesleft;
    assert(0 == (bytesWrittenLocal & wrapmask));
  }

  auto cur = ptr + (bytesWrittenLocal & wrapmask);
  auto p = (int*)cur;
  p[0] = id;
  p[1] = size;
  memcpy(cur + kHeaderLen, src, size);
  bytesWrittenLocal += bytesneeded;
  bytesWritten = bytesWrittenLocal;
  UpdateBytesWritten();
  return len;
}

ShmChannelReader::ShmChannelReader(char* ptr_, int len_) : chan(ptr_, len_) {
  bytesWrittenCached = bytesRead = 0;
  ptr = chan.ptr;
}

ShmChannelReader::ShmChannelReader(int key, int count, int size, int _wrapmask) : chan(key, count, size, _wrapmask) {
  bytesWrittenCached = bytesRead = 0;
  ptr = chan.ptr;
}

int ShmChannelReader::read() { return tryread(1024 * 1024 * 1024); }

int ShmChannelReader::tryread(int attempts) {
  if (bytesWrittenCached == bytesRead) {
    bytesWrittenCached = chan.BytesWritten();
    while (bytesWrittenCached == bytesRead) {
      if (0 >= --attempts) return 0;
      _mm_pause();
      bytesWrittenCached = chan.BytesWritten();
    }
  }
  ptr = chan.ptr + Position();
  if (wraparoundMarkerRead()) {
    int bytesLeft = chan.len - Position();
    bytesRead += bytesLeft;
    assert(0 == Position());
    ptr = chan.ptr;
    assert(bytesRead <= bytesWrittenCached);
  }
  auto sz = size();
  int readableBytesLeft = chan.len - (Position() + ShmChannel::kHeaderLen);
  bytesRead += sz + ShmChannel::kHeaderLen;
  return sz > readableBytesLeft ? 0 : sz;
}

class ShmMultiChannel : public Writable {
  int id;
  ShmChannel* chan;

 public:
  ShmMultiChannel(ShmChannel* chan_, int id_) : id(id_), chan(chan_) {}
  virtual int write(void* src, int len) final override { return chan->write(id, src, len); }
};
ShmMultiChannelFactory::ShmMultiChannelFactory(int len, char** pptr) { chan = ShmChannel::AllocateInMemory(len, pptr); }

ShmMultiChannelFactory::ShmMultiChannelFactory(int key, int count, int size, int _wrapmask) {
  chan = new ShmChannel(key, count, size, _wrapmask);
}

Writable* ShmMultiChannelFactory::allocate(int channelid) { return new ShmMultiChannel(chan, channelid); }
