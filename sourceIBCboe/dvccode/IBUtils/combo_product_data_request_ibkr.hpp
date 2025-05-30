#pragma once

#include <iostream>
#include "dvccode/IBUtils/TestCppClient.hpp"
#include "dvccode/IBUtils/Contract.hpp"

namespace HFSAT{
    class ComboProductDataRequestIBKR{
        public:
        ComboProductDataRequestIBKR()=default;
        ComboProductDataRequestIBKR(ComboProductDataRequestIBKR& obj1)=delete;
        ~ComboProductDataRequestIBKR()=default;
        static void AddTCPClient(TestCppClient *_client);
        static TestCppClient* GetTCPClient();
        static void RequestDataFor(Contract& contract_,std::string& uniqueID);
        private:
        static TestCppClient* client;
    };
}
