#ifndef _PLAYBACK_DEFINES_
#define _PLAYBACK_DEFINES_

#include <sstream>
#include <string>

namespace HFSAT {
namespace ORS {

class BasePlaybackStruct {
 public:
  BasePlaybackStruct() : side(0), size(0), origClOrdID(0) {}
  BasePlaybackStruct(int _side, int _size, unsigned long long _id) : side(_side), size(_size), origClOrdID(_id) {}
  const std::string ToString() const {
    std::ostringstream temp_oss;

    temp_oss << "ClOrdId : " << origClOrdID << "\n";
    temp_oss << "Side : " << side << "\n";
    temp_oss << "Size : " << size << "\n";

    return temp_oss.str();
  }

  int side;
  int size;
  unsigned long long origClOrdID;
};
}
}

#endif
