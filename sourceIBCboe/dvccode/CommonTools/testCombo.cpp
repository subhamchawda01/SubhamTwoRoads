#include "dvccode/IBUtils/TestCppClient.hpp"
#include "dvccode/IBUtils/combo_product_data_request_ibkr.hpp"
int main(){
    TestCppClient t1;
    HFSAT::ComboProductDataRequestIBKR::AddTCPClient(&t1);
    // ComboProductDataRequestIBKR::GetTCPClient();
}