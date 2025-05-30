/**
 \file ORSUtils/broadcast_strat_cmds.cpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite 217, Level 2, Prestige Omega,
 No 104, EPIP Zone, Whitefield,
 Bangalore - 560066, India
 +91 80 4060 0717
 */

#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/send_alert.hpp"
#include "infracore/ORSUtils/broadcast_strat_cmds.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

namespace HFSAT {

BroadcastStratCmds::BroadcastStratCmds(DebugLogger& t_dbglogger_)
    : dbglogger_(t_dbglogger_),
      network_account_info_manager_(),
      control_recv_data_info_(network_account_info_manager_.GetControlRecvDataInfo()),
      sock_(control_recv_data_info_.bcast_ip_, control_recv_data_info_.bcast_port_,
      HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_Control)){
  
  gcrs_ = (HFSAT::GenericControlRequestStruct*)calloc(sizeof(HFSAT::GenericControlRequestStruct), 1);
}

BroadcastStratCmds::~BroadcastStratCmds() {}

void BroadcastStratCmds::sendStratCmd(ControlMessageCode_t msg_code_) {
  if (msg_code_ == HFSAT::kControlMessageCodeGetflat) {
    bzero((gcrs_->control_message_).strval_1_, 20);
  } else if (msg_code_ == HFSAT::kControlMessageCodeAggGetflat) {
    bzero((gcrs_->control_message_).strval_1_, 20);
  } else if (msg_code_ == HFSAT::kControlMessageCodeStartTrading) {
    bzero((gcrs_->control_message_).strval_1_, 20);
  } else {
	dbglogger_ << "Strat Cmd Not Recoginesd In BroadcastStratCmds.. Not Sending " << "\n";
	return;
  }
  sock_.WriteN(sizeof(HFSAT::GenericControlRequestStruct), gcrs_);
}
}
