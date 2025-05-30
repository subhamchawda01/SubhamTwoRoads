#include "dvccode/ExternalData/dump_raw_data_listener.hpp"

namespace HFSAT {

DumpRawDataListener::DumpRawDataListener(const std::string& md_udp_ip, const int md_udp_port, const int ip_code,
                                         std::string exch, HFSAT::BulkFileWriter* bfw) {
  multicast_receiver_socket_ = new HFSAT::MulticastReceiverSocket(
      md_udp_ip, md_udp_port, NetworkAccountInterfaceManager::instance().GetInterface(exch, HFSAT::k_MktDataRaw));
  multicast_receiver_socket_->setBufferSize(N);

  bfw_ = bfw;
  multicast_receiver_socket_->SetNonBlocking();
  ip_code_ = ip_code;
}

void DumpRawDataListener::ProcessAllEvents(int this_socket_fd_) {
  while (true) {
    msg_len = multicast_receiver_socket_->ReadN(72000, (void*)msg_buf);
    if (msg_len < 0) break;
    gettimeofday(&time_, NULL);

    bfw_->Write(&ip_code_, sizeof(int));
    bfw_->Write(&time_, sizeof(time_));
    bfw_->Write(&msg_len, sizeof(int));
    bfw_->Write((void*)msg_buf, msg_len);
    bfw_->DumpCurrentBuffer();
  }
  bfw_->DumpCurrentBuffer();
}

MulticastReceiverSocket* DumpRawDataListener::GetMulticastReceiverSocket() { return multicast_receiver_socket_; }

void DumpRawDataListener::CloseMulticastReceiverSocket() { multicast_receiver_socket_->Close(); }

void DumpRawDataListener::CleanUp() {
  CloseMulticastReceiverSocket();
  if (bfw_->is_open()) bfw_->Close();
}
}
