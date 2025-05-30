/**
   \file Tools/reply_audit_reader.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */
#include <iostream>
#include <stdlib.h>
#include <sys/stat.h>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/mds_messages.hpp"

#include "infracore/Tools/bse_reply_audit_reader.hpp"

int main(int argc, char** argv) {
  if (argc != 7 && argc != 8 && argc != 9 && argc != 10) {
    	std::cout
	<< " USAGE: EXEC <GET_ORSREPLY_USER_IND> <ORSREPLY_DIR> <AUDIT_FILE_DIR> <BSE_FO/BSE_EQ> <YYYYMMDD> <INDB11> <start_time[GMT_0345]> <end_time[GMT_1000]>\n" 
	<< " USAGE: EXEC <GET_MODFYCOUNT_USER_IND> <ORSREPLY_DIR> <AUDIT_FILE_DIR> <BSE_FO/BSE_EQ> <YYYYMMDD> <start_time[GMT_0345]> <end_time[GMT_1000]>\n" 
	<< " USAGE: EXEC <GET_ORDERID_SACI> <ORSREPLY_DIR> <AUDIT_FILE_DIR> <BSE_EQ> <YYYYMMDD> <INDB11> <SYMBOL[SBIN]> <start_time[GMT_0345]> <end_time[GMT_1000]>" << std::endl;
    	exit(0);
  }
  std::string exch = argv[1];
  std::string start_time = "GMT_0345";
  std::string end_time = "GMT_1000";

  if(exch == "GET_ORSREPLY_USER_IND") {
        if (argc == 9) {
          start_time = argv[7];
          end_time = argv[8];
        }
        HFSAT::ReplyAuditReader<HFSAT::GenericORSReplyStructLiveProShm>::ReadOrsReplyAndAuditFile(argv[2],argv[3],argv[4],argv[5],argv[6],start_time,end_time);
  }
  else if(exch == "GET_ORDERID_SACI") {
        if (argc == 10) {
          start_time = argv[8];
          end_time = argv[9];
        }
        HFSAT::ReplyAuditReader<HFSAT::GenericORSReplyStructLiveProShm>::ReadOrderIdSaciFromReplyAndAuditFile(argv[2],argv[3],argv[4],argv[5],argv[6],argv[7],start_time,end_time);
  }
  else if(exch == "GET_MODFYCOUNT_USER_IND") {
        if (argc == 8) {
          start_time = argv[6];
          end_time = argv[7];
        }
        HFSAT::ReplyAuditReader<HFSAT::GenericORSReplyStructLiveProShm>::GetModifyCountFromReply(argv[2],argv[3],argv[4],argv[5],start_time,end_time);
        
  }

  else{
    std::cout << "Wrong exchange...!!" << exch << std::endl;
  }
  return 0;
}

