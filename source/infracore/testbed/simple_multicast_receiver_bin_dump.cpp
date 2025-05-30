#include <time.h>
#include <signal.h>
#include "dvccode/Utils/multicast_receiver_socket.hpp"

void to_binary_(char buf_) {
  int n_bits_[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  for (int i = 0; i < 8; buf_ /= 2, ++i) {
    n_bits_[i] = buf_ % 2;
  }

  for (int i = 7; i >= 0; i--) {
    fprintf(stdout, "%d", (n_bits_[i] < 0) ? -n_bits_[i] : n_bits_[i]);
    if (i == 4) fprintf(stdout, " ");
  }
}

FILE *bin_file_;
FILE *size_file_;

void signal_handler(int sig_num_) {
  fclose(size_file_);
  fclose(bin_file_);

  exit(0);
}

int main(int argc, char **argv) {
  std::string mcast_ip_;
  int mcast_port_;

  if (argc < 3) {
    std::cerr << " Usage: " << argv[0] << " <ip> <port>" << std::endl;
    std::cerr << "Read data from <ip> <port> and dump data to file in binary format.\n";
    exit(0);
  }

  signal(SIGINT, signal_handler);
  signal(SIGSEGV, signal_handler);

  mcast_ip_ = argv[1];
  mcast_port_ = atoi(argv[2]);

  HFSAT::MulticastReceiverSocket abc_(mcast_ip_, mcast_port_);

  char buf_[1024 * 1024];
  while (1) {
    int received_size_ = abc_.ReadN(1024 * 1024, (void *)buf_);

    fprintf(stdout, "SIZE: %d\n", received_size_);
    for (int i = 0; i < received_size_; ++i) {
      fprintf(stdout, "<%d>", i + 1);
      to_binary_(buf_[i]);
      fprintf(stdout, " ");
    }
    fprintf(stdout, "\n\n");
  }
}
