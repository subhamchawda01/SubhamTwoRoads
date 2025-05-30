// =====================================================================================
// 
//       Filename:  IBKRDataManager.cpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  10/22/2024 06:02:10 AM
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


/* Copyright (C) 2019 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "dvccode/IBUtils/StdAfx.hpp"

#include <boost/program_options.hpp>
#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <thread>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/IBUtils/contract_manager.hpp"
#include "dvccode/IBUtils/TestCppClient.hpp"
#include "dvccode/Utils/cboe_daily_token_symbol_handler.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/IBUtils/combo_product_data_request_ibkr.hpp"
#include "dvccode/Utils/connection_handler.hpp"

#include "dvccode/Utils/allocate_cpu.hpp"
const unsigned MAX_ATTEMPTS = 50;
const unsigned SLEEP_TIME = 30;
HFSAT::DebugLogger dbglogger_(4 * 1024 * 1024, 1);
HFSAT::Utils::ConnectionHandler* p_global_connection_handler_ = nullptr;
TestCppClient* p_test_cpp_client_ = nullptr;
class IBKRDataManager : public IBDataListener{

  private:
    static const int sizeOfLoggerQueue = 1024*1024;
    int64_t mcast_pkt_seq_;
    int32_t mode;
    MDSLogger<IBL1UpdateTick,sizeOfLoggerQueue> *mds_logger_thread_;
    HFSAT::MulticastSenderSocket *sender_socket_;

    static inline constexpr HFSAT::FastMdConsumerMode_t combineModes(HFSAT::FastMdConsumerMode_t lhs, HFSAT::FastMdConsumerMode_t rhs) {
        return static_cast<HFSAT::FastMdConsumerMode_t>(
            static_cast<std::underlying_type<HFSAT::FastMdConsumerMode_t>::type>(lhs) |
            static_cast<std::underlying_type<HFSAT::FastMdConsumerMode_t>::type>(rhs)
        );
    }
  public:
    IBKRDataManager(std::string this_mode){
      // Get the current time as seconds since the epoch
      auto now = std::chrono::system_clock::now();
      auto seconds_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

      // Store the time in int32_t
      mcast_pkt_seq_ = static_cast<int64_t>(seconds_since_epoch);
      std::cout<<"The starting packet seq is :"<<mcast_pkt_seq_<<std::endl;
      if("LOGGER" == this_mode){
       
        mode = HFSAT::kLogger;
        mds_logger_thread_ = new MDSLogger<IBL1UpdateTick,sizeOfLoggerQueue>("IBKR");
        mds_logger_thread_->run();
      }else if("MCAST" == this_mode){
        mode = HFSAT::kMcast;
        HFSAT::NetworkAccountInfoManager network_account_info_manager;
        HFSAT::DataInfo data_info_mkt_data = network_account_info_manager.GetSrcDataInfo(HFSAT::kExchSourceCBOE, "CBOE_SPXW_C0_A");
        sender_socket_ = new HFSAT::MulticastSenderSocket(data_info_mkt_data.bcast_ip_, data_info_mkt_data.bcast_port_, HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceCBOE, HFSAT::k_MktDataMcast));
      }else if("HYBRID" == this_mode){

        mode = HFSAT::kLogger | HFSAT::kMcast;
        mds_logger_thread_ = new MDSLogger<IBL1UpdateTick,sizeOfLoggerQueue>("IBKR");
        mds_logger_thread_->run();
        HFSAT::NetworkAccountInfoManager network_account_info_manager;
        HFSAT::DataInfo data_info_mkt_data = network_account_info_manager.GetSrcDataInfo(HFSAT::kExchSourceCBOE, "CBOE_SPXW_C0_A");
        sender_socket_ = new HFSAT::MulticastSenderSocket(data_info_mkt_data.bcast_ip_, data_info_mkt_data.bcast_port_, HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceCBOE, HFSAT::k_MktDataMcast));
      }

    }

    void OnIBKRRawDataUpdate(IBL1UpdateTick* market_event){
      switch(mode){
        case HFSAT::kLogger:
          {
            market_event->packet_seq = ++mcast_pkt_seq_;
            mds_logger_thread_->log(*market_event);
          }break;
        case HFSAT::kMcast:
          {
            if(IBUpdateType::IB_GREEKS_UPDATE == market_event->ib_update_type){
              market_event->packet_seq = mcast_pkt_seq_;
              break;
            }
            market_event->packet_seq = ++mcast_pkt_seq_;
            int write_len = sender_socket_->WriteN(sizeof(IBL1UpdateTick), market_event);
            if(write_len < (int)sizeof(IBL1UpdateTick)){
              std::cout << "FAILED TO WRITE COMPLETE MSG ON SOCKET, EXPECTED WRITE SIZE : " << sizeof(IBL1UpdateTick) << " WRITTEN : " << write_len << std::endl;
            }
          }break;
        case combineModes(HFSAT::kLogger , HFSAT::kMcast):
          {
            if(IBUpdateType::IB_GREEKS_UPDATE == market_event->ib_update_type){
              market_event->packet_seq = mcast_pkt_seq_;
              mds_logger_thread_->log(*market_event);
              break;
            }
            market_event->packet_seq = ++mcast_pkt_seq_;
            int write_len = sender_socket_->WriteN(sizeof(IBL1UpdateTick), market_event);
            if(write_len < (int)sizeof(IBL1UpdateTick)){
              std::cout << "FAILED TO WRITE COMPLETE MSG ON SOCKET, EXPECTED WRITE SIZE : " << sizeof(IBL1UpdateTick) << " WRITTEN : " << write_len << std::endl;
            }
            mds_logger_thread_->log(*market_event);
          }break;
        default:{}break;
      }
    }

};

void termination_handler_segv(int signum) {
  dbglogger_.DumpCurrentBuffer();
  dbglogger_.Close();

  // Save Fix Sequences

  if (p_global_connection_handler_) {
    p_global_connection_handler_->DisconnectSocket();
  }
  if(p_test_cpp_client_){
    p_test_cpp_client_->cancelAllMktDataReq();
    p_test_cpp_client_->disconnect();
  }
  delete p_test_cpp_client_;  // Deallocates memory
  p_test_cpp_client_ = nullptr;  // Prevents a dangling pointer
  //e.sendMail();
  usleep(5000000);  // Sleep to receive logout confirmations.
  signal(signum, SIG_DFL);
  kill(getpid(), signum);
}

void termination_handler(int signum) {
  dbglogger_.DumpCurrentBuffer();

  // Save Fix Sequences

  if (p_global_connection_handler_) {
    p_global_connection_handler_->DisconnectSocket();
  }
  if(p_test_cpp_client_){
    p_test_cpp_client_->cancelAllMktDataReq();
    p_test_cpp_client_->disconnect();
  }
  delete p_test_cpp_client_;  // Deallocates memory
  p_test_cpp_client_ = nullptr;  // Prevents a dangling pointer

  signal(signum, SIG_DFL);
  kill(getpid(), signum);

  exit(-1);

  dbglogger_.Close();
  //  tradelogger_.Close ( );
  usleep(5000000);  // Sleep to receive logout confirmations.
  exit(0);
}
/* IMPORTANT: always use your paper trading account. The code below will submit orders as part of the demonstration. */
/* IB will not be responsible for accidental executions on your live account. */
/* Any stock or option symbols displayed are for illustrative purposes only and are not intended to portray a recommendation. */
/* Before contacting our API support team please refer to the available documentation. */
int main(int argc, char** argv)
{
  // signal handling, Interrupts and seg faults
  signal(SIGINT, termination_handler);
  signal(SIGTERM, termination_handler);
  signal(SIGSEGV, termination_handler_segv);
  signal(SIGPIPE, SIG_IGN);


  std::cout<<"Start making option_desc"<<std::endl;
  boost::program_options::options_description desc("Allowed Options");
  std::cout<<"Start adding options option_desc"<<std::endl;

  desc.add_options()("help", "produce help message.")("host", boost::program_options::value<std::string>()->default_value("127.0.0.1"))("port", boost::program_options::value<std::string>()->default_value("7496"))("clientId", boost::program_options::value<std::string>()->default_value("1"))("mode", boost::program_options::value<std::string>()->default_value("LOGGER"))("productfile", boost::program_options::value<std::string>()->default_value("/home/pengine/prod/live_configs/required_shortcodes.txt"))
      ("worker_control_port", boost::program_options::value<std::string>()->default_value("51217"))
      ("useAffinity", boost::program_options::value<bool>()->default_value(true));
      // ("config", boost::program_options::value<std::string>()->default_value("/USER/sanskar/ConfigFiles/cboe_config.cfg"));
  
  std::cout<<"Added options option_desc"<<std::endl;

  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);

  std::string host = vm["host"].as<std::string>();
  std::cout<<host<<std::endl;
  int port = std::atoi((vm["port"].as<std::string>()).c_str());
  std::cout<<port<<std::endl;

  int clientId = std::atoi((vm["clientId"].as<std::string>()).c_str());
  std::cout<<clientId<<std::endl;

  std::string mode = vm["mode"].as<std::string>();
  std::cout<<mode<<std::endl;

  std::string filename = vm["productfile"].as<std::string>();
  std::cout<<filename<<std::endl;

  //This is the port where it listens request for comboProducts data
  int worker_control_port=std::atoi((vm["worker_control_port"].as<std::string>()).c_str());

  bool use_affinity_ = vm["useAffinity"].as<bool>();

  std::cout<<"Settings done"<<std::endl;
  if (use_affinity_) {
    HFSAT::AllocateCPUUtils::GetUniqueInstance().AllocateCPUOrExit("IBKRDataManagerWithListenReq");
  }
  const char* connectOptions = "";
  unsigned attempt = 0;

  IBKRDataManager ib_data_manager(mode);
  int today = HFSAT::DateTime::GetCurrentIsoDateLocal();


  HFSAT::ExchangeSymbolManager::SetUniqueInstance(today);
  HFSAT::SecurityDefinitions::GetUniqueInstance(today).LoadCBOESecurityDefinitions();
  std::vector<Contract> req_contracts;
  std::vector<std::string> contract_sym;

  std::ifstream sub_file;
  sub_file.open(filename.c_str());
  if(!sub_file.is_open()){
    std::cout << "Failed To Load The Product Subscription File" << filename << std::endl;
    std::exit(-1);
  }

  HFSAT::Utils::CBOEDailyTokenSymbolHandler &cboe_token_handler = HFSAT::Utils::CBOEDailyTokenSymbolHandler::GetUniqueInstance(today);

  // Contract contract_comb;
	// contract_comb.symbol = "SPX";
	// contract_comb.secType = "BAG";
	// contract_comb.currency = "USD";
	// contract_comb.exchange = "CBOE";

  // contract_comb.comboLegs.reset(new Contract::ComboLegList());
  // {
  //   ComboLegSPtr leg(new ComboLeg);
  //   leg->conId = cboe_token_handler.GetTokenFromInternalSymbol(HFSAT::CBOESecurityDefinitions::GetDatasourcenameCBOE("CBOE_SPXW_C0_A").c_str(),'F');
  //   leg->action = "SELL";
  //   leg->ratio = 1;
  //   leg->exchange = "CBOE";
	//   contract_comb.comboLegs->push_back(leg);
  // }
    
  // {
  //   ComboLegSPtr leg(new ComboLeg);
  //   leg->conId = cboe_token_handler.GetTokenFromInternalSymbol(HFSAT::CBOESecurityDefinitions::GetDatasourcenameCBOE("CBOE_SPXW_P0_A").c_str(),'F');
  //   leg->action = "SELL";
  //   leg->ratio = 1;
  //   leg->exchange = "CBOE";
	//   contract_comb.comboLegs->push_back(leg);
  // }

  // {
  //   ComboLegSPtr leg(new ComboLeg);
  //   leg->conId = cboe_token_handler.GetTokenFromInternalSymbol(HFSAT::CBOESecurityDefinitions::GetDatasourcenameCBOE("CBOE_SPXW_C0_A").c_str(),'F');
  //   leg->action = "BUY";
  //   leg->ratio = 1;
  //   leg->exchange = "CBOE";
	//   contract_comb.comboLegs->push_back(leg);
  // }

  // {
  //   ComboLegSPtr leg(new ComboLeg);
  //   leg->conId = cboe_token_handler.GetTokenFromInternalSymbol(HFSAT::CBOESecurityDefinitions::GetDatasourcenameCBOE("CBOE_SPXW_P0_A").c_str(),'F');
  //   leg->action = "BUY";
  //   leg->ratio = 1;
  //   leg->exchange = "CBOE";
	//   contract_comb.comboLegs->push_back(leg);
  // }
  // std::string contract_comb_str="CBOE_SPXW_COMBO_0";
  // req_contracts.push_back(contract_comb);
  // contract_sym.push_back("CBOE_SPXW_COMBO_1");

  Contract contract_idx;
  contract_idx.symbol = "SPX";
  contract_idx.secType = "IND";
  contract_idx.exchange = "CBOE";
  contract_idx.currency = "USD";
  req_contracts.push_back(contract_idx);
  contract_sym.push_back("CBOE_SPX");

  char buffer[1024];
  int num=0;
  int tokErr=0;
  while(sub_file.good()){
    sub_file.getline(buffer,1024);
    std::string line_buffer = buffer;

    // Comments
    if (line_buffer.find("#") != std::string::npos) continue;

    HFSAT::PerishableStringTokenizer pst(buffer, 1024);
    std::vector<char const*> const& tokens = pst.GetTokens();

    // We expect to read StreamId, StreamIP, StreamPort
    if (tokens.size() != 1){
      tokErr++;
      continue;
    }

    std::string shortcode = tokens[0];
    std::string int_sym = HFSAT::CBOESecurityDefinitions::GetDatasourcenameCBOE(shortcode);
    int con_id = cboe_token_handler.GetTokenFromInternalSymbol(int_sym.c_str(),'F');
    if(con_id <= 0){
      num++;
      continue;
    }

    Contract contract;
    contract.conId = con_id;
    contract.exchange = "CBOE";
    req_contracts.push_back(contract);
    contract_sym.push_back(int_sym);

  }
  sub_file.close();

  std::cout<<"Negative cid for :"<<num<<std::endl;
  std::cout<<"Token size error :"<<tokErr<<std::endl;

  if(0 == req_contracts.size()){
    std::cout << "No contracts requested from file : " << filename << std::endl;
    std::exit(-1);
  }

  printf( "Start of C++ Socket Client Test %u\n", attempt);

  for (;;) {
    ++attempt;
    printf( "Attempt %u of %u\n", attempt, MAX_ATTEMPTS);
    // TestCppClient client;
    p_test_cpp_client_ = new TestCppClient();
    HFSAT::ComboProductDataRequestIBKR::AddTCPClient(p_test_cpp_client_);
    HFSAT::Utils::ConnectionHandler connection_handler_(dbglogger_, worker_control_port);
    p_global_connection_handler_ = &connection_handler_;
    // Run time error will occur (here) if TestCppClient.exe is compiled in debug mode but TwsSocketClient.dll is compiled in Release mode
    // TwsSocketClient.dll (in Release Mode) is copied by API installer into SysWOW64 folder within Windows directory

    if( connectOptions) {
      p_test_cpp_client_->setConnectOptions(connectOptions);
    }

    std::cout << "HOST : " << host << " PORT : " << port << " " << clientId << std::endl;
    p_test_cpp_client_->connect( host.c_str(), port, clientId);
    p_test_cpp_client_->AddIBDataListener(&ib_data_manager);

    p_test_cpp_client_->RequestMarketData(req_contracts,contract_sym);
    // client.RequestMarketDataSingleContract(contract_comb,contract_comb_str);
    // std::cout<<"The combo Product size: "<<contract_comb.comboLegs->size()<<std::endl;
    connection_handler_.run();

    while( p_test_cpp_client_->isConnected()) {
      p_test_cpp_client_->processMessages();
    }
    p_global_connection_handler_->DisconnectSocket();
    p_global_connection_handler_->stop();
    if( attempt >= MAX_ATTEMPTS) {
            break;
    }
    p_test_cpp_client_->cancelAllMktDataReq();
    delete p_test_cpp_client_;  // Deallocates memory
    p_test_cpp_client_ = nullptr;  // Prevents a dangling pointer

    printf( "Sleeping %u seconds before next attempt\n", SLEEP_TIME);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TIME));
  }


  printf ( "End of C++ Socket Client Test\n");
}

