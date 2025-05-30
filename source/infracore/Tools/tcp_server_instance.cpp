#include <time.h>
#include <iostream>
#include "dvccode/Utils/tcp_server_socket.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/CPUAffinity.hpp"
#include "dvccode/CDef/ttime.hpp"

class SendPackets : public HFSAT::Thread {
 private:
  HFSAT::TCPServerSocket& server_socket_;
  int32_t& client_socket_fd_;

 public:
  SendPackets(HFSAT::TCPServerSocket& server_socket, int32_t& client_socket_fd)
      : server_socket_(server_socket), client_socket_fd_(client_socket_fd) {}

  void thread_main() {
    setAffinityCore(19);

    char send_buf[48];

    for (int32_t count = 0; count < 100000; count++) {
      std::ostringstream tempstr;
      tempstr << "ZYXWVUTS "
              << " PLUS " << count + 1;

      memset((void*)send_buf, 0, sizeof(send_buf));
      memcpy((void*)send_buf, (void*)tempstr.str().c_str(), tempstr.str().length());

      int32_t wrote = server_socket_.WriteNToSocket(client_socket_fd_, sizeof(send_buf), send_buf);
      std::cout << " WROTE : " << wrote << std::endl;

      sleep(30);
      //        HFSAT::usleep(10);
    }
  }
};

int main(int argc, char** argv) {
  int tcp_port_;

  if (argc < 2) {
    std::cerr << " Usage: " << argv[0] << " <port> " << std::endl;
    exit(0);
  }

  tcp_port_ = atoi(argv[1]);

  uint32_t pid = getpid();

  CPUManager::setAffinity(21, pid);

  HFSAT::TCPServerSocket abc_(tcp_port_);

  int nfd_ = abc_.Accept();

  std::cout << " ACCEPTED : " << nfd_ << std::endl;

  char buf[200000] = {0};

  SendPackets send_packets(abc_, nfd_);
  send_packets.run();

  for (int32_t count = 0; count < 100000; count++) {
    int read_size = abc_.ReadNFromSocket(nfd_, 200000, buf);
    std::cout << count << " " << read_size << std::endl;
    std::cout << buf << std::endl;
    //    sleep(5);
  }

  std::exit(0);

  //  std::cout << "OUT OF LOOP: " <<

  send_packets.stop();
}
