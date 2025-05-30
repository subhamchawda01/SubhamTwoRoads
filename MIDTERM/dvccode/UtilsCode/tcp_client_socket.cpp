
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "dvccode/Utils/async_writer.hpp"

using namespace HFSAT;
using namespace std;

TcpClientSocketWithLogging::TcpClientSocketWithLogging(bool use_ethnet5, AsyncWriter *pWriter_, AsyncWriter *pReader_,
                                                       string logfile)
    : TCPClientSocket(use_ethnet5), pReader(pReader_), pWriter(pWriter_) {
  if (pReader != nullptr) {
    readChannel = pReader->add(logfile + ".in");
  }
  if (pWriter != nullptr) {
    writeChannel = pWriter->add(logfile + ".out");
  }
}

int TcpClientSocketWithLogging::WriteN(const unsigned int len, const void *src) {
  auto res = TCPClientSocket::WriteN(len, src);
  if (res > 0) {
    if (pWriter != nullptr) {
      pWriter->log(writeChannel, (const char *)src, res);
    }
  }
  return res;
}

int TcpClientSocketWithLogging::ReadN(const unsigned int len, void *dst) {
  auto res = TCPClientSocket::ReadN(len, dst);
  if (res > 0) {
    if (pReader != nullptr) {
      pReader->log(readChannel, (const char *)dst, res);
    }
  }
  return res;
};
