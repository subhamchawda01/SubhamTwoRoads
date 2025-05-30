#include <iostream>
#include "dvccode/IBUtils/combo_product_data_request_ibkr.hpp"
namespace HFSAT{
  TestCppClient* ComboProductDataRequestIBKR::client=nullptr;
  void ComboProductDataRequestIBKR::AddTCPClient(TestCppClient* _client){
    if(ComboProductDataRequestIBKR::client!=nullptr){
      std::cerr<<"Already set the TCP client overwritting it"<<std::endl;
    }
    ComboProductDataRequestIBKR::client=_client;
  }
  TestCppClient* ComboProductDataRequestIBKR::GetTCPClient(){
    if(ComboProductDataRequestIBKR::client==nullptr)std::cerr<<"The tcp client is null"<<std::endl;
    return ComboProductDataRequestIBKR::client;
  }
  void ComboProductDataRequestIBKR::RequestDataFor(Contract& contract_,std::string& uniqueID){
    client->RequestMarketDataSingleContract(contract_,uniqueID);
  }
}


