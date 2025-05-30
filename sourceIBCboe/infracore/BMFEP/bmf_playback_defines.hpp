#ifndef _BMF_PLAYBACK_DEFINES_
#define _BMF_PLAYBACK_DEFINES_

#include "infracore/BasicOrderRoutingServer/playback_defines.hpp"

namespace HFSAT {
namespace ORS {

class BMFPlaybackStruct : public BasePlaybackStruct {
 public:
  BMFPlaybackStruct() : BasePlaybackStruct() {}
  BMFPlaybackStruct(int _side, int _size, unsigned long long _id) : BasePlaybackStruct(_side, _size, _id) {}
};
}
}

#endif
