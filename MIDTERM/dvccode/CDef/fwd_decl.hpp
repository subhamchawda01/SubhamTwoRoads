#ifndef FWD_DECL_HPP
#define FWD_DECL_HPP
#include <string>
#include <cstdint>

namespace HFSAT {
class SimpleSecuritySymbolIndexer;
class DebugLogger;

class NewAsxRawLiveDataSource;
class NewOSEItchRawLiveDataSource;

namespace ASXD {
class ASXDecoder;
}

namespace NSE {
class NSEEngine;
}

namespace ORS {
class MarginChecker;
class AccountThread;
class ControlReceiver;
class ControlThread;
class Settings;
class BCaster;
class ICAPHeartBeatManager;
class RTSHeartBeatManager;

class BaseEngine;
class BMFEPEngine;
class CMEEngine;
class TMXEngine;
class LIFFECCGEngine;
class ICEEngine;
class ASXEngine;
class SGXEngine;

namespace BMFEPFIX {
class BMFEPClientThread;
}

namespace ICAPFIX {
class ICAPClientThread;
}

namespace CMEFIX {
class CMEClientThread;
}

namespace TMXSAIL {
class TMXClientThread;
}

namespace LIFFECCG {
class LIFFECCGClientThread;
}

namespace ICEFIX {
class ICEClientThread;
}

namespace ASXFIX {
class ASXClientThread;
}
}
}

struct Writable;

class ShmMultiChannel;
class ShmChannelReader;
class ShmChannel;

class AsyncWriter;
class TcpClientSocketWithLogging;
typedef int ChannelId;
typedef const std::string Cstr;
typedef int64_t i8;
typedef uint64_t ui8;

// fwd decl for compile speed
namespace boost {
class thread;
}
#endif
