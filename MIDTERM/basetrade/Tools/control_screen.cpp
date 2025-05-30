/**
    \file Tools/control_screen.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717

     Control Screen sets up a multicast UDP socket to send messages to
     the trading application. All messages are of the form of set field=value.
     We also listen to the multicast stream sent by the trading clients and
     for the messages which have the same trade id as the one we are monitoring,
     we read in values.
*/

void ParseCommandLineParams(int argc, char** argv) {
  // expect :
  // 1. $controlscreenexec CONTROLINFOFILENAME

  // TODO
}

int main(int argc, char** argv) {
  // parse command line parameters
  ParseCommandLineParams(argc, argv);

  // Load the info file
  //    BCAST_INFO BCAST_IP BCAST_PORT ... should be loaded from NetworkAccountInfoManager
  //    SEC_INFO security_short_code unique_trading_id trading_machine_ip_expected )
  // maintain a vector of all strategies listening to
  return 0;
}
