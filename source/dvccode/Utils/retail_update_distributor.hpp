// =====================================================================================
//
//       Filename:  retail_update_distributor.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  05/05/2014 08:34:54 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include "dvccode/LiveSources/retail_trading_listener.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/CDef/retail_data_defines.hpp"
#include "dvccode/CDef/assumptions.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"

namespace HFSAT {
namespace Utils {

class RetailUpdateDistributor : public RetailTradingListener {
 private:
  static RetailUpdateDistributor* p_unique_instance_;

  HFSAT::CDef::RetailDataStruct* retail_data_struct_;
  HFSAT::TCPClientSocket* tcp_client_socket_;

  RetailUpdateDistributor()
      : retail_data_struct_(new HFSAT::CDef::RetailDataStruct()), tcp_client_socket_(new HFSAT::TCPClientSocket()) {
    tcp_client_socket_->Connect(RETAIL_CO_LO_IP, RETAIL_SERVER_LISTENER_TO_QUERY_PORT);

    if (-1 == tcp_client_socket_->socket_file_descriptor()) {
      std::cerr << " CONNECTION TO DATA SERVER FAILED \n";
      exit(-1);
    }
  }

 public:
  static RetailUpdateDistributor* GetUniqueInstance() {
    if (p_unique_instance_ == NULL) {
      p_unique_instance_ = new RetailUpdateDistributor();
    }
    return p_unique_instance_;
  }

  inline void OnRetailOfferUpdate(const std::string& _secname_, const CDef::RetailOffer& _retail_offer_) {
    memset((void*)(retail_data_struct_), 0, sizeof(HFSAT::CDef::RetailDataStruct));

    memcpy((void*)(retail_data_struct_->security_name_), (void*)(_secname_.c_str()),
           std::min(sizeof(retail_data_struct_->security_name_) - 1, _secname_.length()));
    // memcpy ( ( void * ) ( retail_data_struct_ -> shortcode_ ), ( void * ) ( _secname_.c_str ( ) ), std::min ( sizeof
    // ( retail_data_struct_ -> shortcode_ ) - 1, _secname_.length ( ) ) ) ;

    retail_data_struct_->retail_offer_ = _retail_offer_;

    int32_t write_len = tcp_client_socket_->WriteN(sizeof(HFSAT::CDef::RetailDataStruct), (void*)(retail_data_struct_));

    if (write_len < (int)(sizeof(HFSAT::CDef::RetailDataStruct))) {
      std::cerr << " Write Failed : " << write_len << " " << strerror(errno) << "\n";
      exit(-1);
    }
  }

  inline void OnRetailOfferUpdate(unsigned int _security_id_, const std::string& _shortcode_,
                                  const std::string& _security_name_, const int _server_assigned_client_id_,
                                  const CDef::RetailOffer& _retail_offer_) {
    memset((void*)(retail_data_struct_), 0, sizeof(HFSAT::CDef::RetailDataStruct));

    memcpy((void*)(retail_data_struct_->security_name_), (void*)(_security_name_.c_str()),
           std::min(sizeof(retail_data_struct_->security_name_) - 1, _security_name_.length()));
    // memcpy ( ( void * ) ( retail_data_struct_ -> shortcode_ ), ( void * ) ( _shortcode_.c_str () ), std::min ( sizeof
    // ( retail_data_struct_ -> shortcode_ ) - 1, _shortcode_.length () ) ) ;

    retail_data_struct_->retail_offer_ = _retail_offer_;
    int32_t write_len = tcp_client_socket_->WriteN(sizeof(HFSAT::CDef::RetailDataStruct), (void*)(retail_data_struct_));
    if (write_len < (int)(sizeof(HFSAT::CDef::RetailDataStruct))) {
      std::cerr << " Write Failed : " << write_len << " " << strerror(errno) << "\n";
      exit(-1);
    }

    //        mcast_sender_socket_ -> WriteN ( sizeof ( HFSAT::CDef::RetailDataStruct ), ( void * ) (
    //        retail_data_struct_ ) ) ;
  }
};
}
}
