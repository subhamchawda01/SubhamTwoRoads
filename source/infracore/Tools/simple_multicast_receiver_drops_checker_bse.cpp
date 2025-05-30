#include <vector>
#include <iostream>
#include <signal.h>
#include <math.h>
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/online_debug_logger.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "infracore/Tools/simple_multicast_test_data.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/Utils/mds_shm_interface.hpp"
#include "dvccode/Utils/load_our_defined_products.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/Utils/data_daemon_config.hpp"
#include "dvccode/Utils/allocate_cpu.hpp"
#include "infracore/Tools/live_products_manager.hpp"
#include "infracore/LiveSources/multi_shm_live_data_source.hpp"
#include "dvccode/Utils/bse_daily_token_symbol_handler.hpp"
#include "infracore/BSEMD/bse_tbt_raw_md_handler.hpp"
#include "infracore/BSEMD/bse_raw_md_handler.hpp"
#include "infracore/BSEMD/bse_md_processor.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

int no_packets_recv_ = 0;
std::vector<HFSAT::ttime_t> times_vec_;
std::unordered_map<int32_t, uint32_t> mkt_seg_id_to_last_seen_seq_;
void termination_handler(int signal_num_) {
  int sum_ = 0;
  int max_ = -1, min_ = 65553;
  no_packets_recv_ = 0;

  for (unsigned int i = 0; i < times_vec_.size(); ++i) {
    if (times_vec_[i].tv_sec == 0 && times_vec_[i].tv_usec > 0) {
      sum_ += times_vec_[i].tv_usec;
      ++no_packets_recv_;

      max_ = (max_ > times_vec_[i].tv_usec) ? max_ : times_vec_[i].tv_usec;
      min_ = (min_ < times_vec_[i].tv_usec) ? min_ : times_vec_[i].tv_usec;
    }
  }

  double avg_ = sum_ / (double)no_packets_recv_;

  double std_dev_ = 0.0;
  for (unsigned int i = 0; i < times_vec_.size(); ++i) {
    if (times_vec_[i].tv_sec == 0 && times_vec_[i].tv_usec > 0) {
      std_dev_ += ((avg_ - times_vec_[i].tv_usec) * (avg_ - times_vec_[i].tv_usec));
    }
  }

  std_dev_ /= no_packets_recv_;

  std_dev_ = sqrt(std_dev_);

  // std::cout << "TimeStamp  No_Of_Packets  Min  Max  Avg  StdDev" << std::endl;
  std::cout << HFSAT::GetTimeOfDay() << " " << no_packets_recv_ << " " << (min_ / 1000.0) << " " << (max_ / 1000.0)
            << " " << (avg_ / 1000.0) << " " << (std_dev_ / 1000.0) << std::endl;

  exit(0);
}

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << " Usage: " << argv[0] << "  <interface>  <mcast-ip>  <mport>" << std::endl;
    exit(0);
  }
  signal(SIGINT, termination_handler);
  std::string interface_ = argv[1];
  std::string mcast_ip_ = argv[2];
  int mcast_port_ = atoi(argv[3]);

  HFSAT::DebugLogger dbglogger_(1024000, 1);
  std::string logfilename = "/spare/local/MDSlogs/simple_multicast_receiver_drops_checker_bse_dbg" + std::string("_") +
                            HFSAT::DateTime::GetCurrentIsoDateLocalAsString() + ".log";
  dbglogger_.OpenLogFile(logfilename.c_str(), std::ios::out | std::ios::app);
  std::cout << "dbglogger_.OpenLogFile done" << logfilename << std::endl;
  dbglogger_ << "OpenDbglogger\n";
  dbglogger_.DumpCurrentBuffer();

  HFSAT::AllocateCPUUtils::GetUniqueInstance().AllocateCPUOrExit("BSEShmWriter");
  std::cout << "dbglogger_.DumpCurrentBuffer DONE" << std::endl;
  int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadBSESecurityDefinitions(); 
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  HFSAT::Utils::BSERefDataLoader::GetUniqueInstance(tradingdate_);
  std::cout << "HFSAT::ExchangeSymbolManager::SetUniqueInstance" << std::endl;
  std::cout << "HFSAT::SecurityDefinitions::GetUniqueInstance DONE" << std::endl;
  HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(tradingdate_);
  HFSAT::SecurityNameIndexer::GetUniqueInstance();
  std::cout << "HFSAT::SecurityNameIndexer& done" << std::endl;
/*    std::map<int32_t, BSE_UDP_MDS::BSERefData> eq_bse_ref_data;
   std::cout << "Get Token Details: " << std::endl;
   for (auto& itr : ref_data_loader.GetBSERefData(BSE_EQ_SEGMENT_MARKING)) { 
     eq_bse_ref_data[itr.first] = itr.second;
     
     std::ostringstream internal_symbol_str;
     internal_symbol_str << "BSE" << "_" << (itr.second).symbol;
     std::string exchange_symbol = HFSAT::BSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol_str.str());
    int64_t token_ = HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetTokenFromInternalSymbol(internal_symbol_str.str().c_str(), BSE_EQ_SEGMENT_MARKING);
     std::cout << "internal_symbol_str: " << internal_symbol_str.str()
	       << " exchange_symbol: " << exchange_symbol 
	       << " TOKEN: " << token_ << std::endl;

   }
  return 0;
*/
  HFSAT::MulticastReceiverSocket abc_(mcast_ip_, mcast_port_, interface_);
  char* data_buffer = new char[65536];
  while (1) {
    int32_t msg_length = abc_.ReadN(sizeof(TestOrderDatagram), data_buffer);

    char* msg_ptr = data_buffer;

    if(msg_length <= 0) continue;
    uint16_t body_len_ = *(uint16_t*)(msg_ptr + 0);   
    uint16_t template_id_ = *(uint16_t*)(msg_ptr + 2);
    // dbglogger_ << "template_id_:" << template_id_ <<"\n";
    if (template_id_ != 13002) {
      continue;
    }

    // uint32_t msg_seq_no_not_used = *(uint32_t*)(msg_ptr + 4); //appl_seq_num_
    uint32_t msg_seq_no = *(uint32_t*)(msg_ptr + 8); //appl_seq_num_
    int32_t mkt_seg_id_ = *(int32_t*)(msg_ptr + 12);
    // uint64_t TransactTime_ = *(uint64_t*)(msg_ptr + 24);
 //      dbglogger_ << "MarketEvents:" << " body_len_: " << body_len_ << " template_id_: " << template_id_ << " msg_seq_no_not_used: " << msg_seq_no_not_used
  //            << " msg_seq_no: " << msg_seq_no << " mkt_seg_id_: " << mkt_seg_id_
  //            << " TransactTime_: " << TransactTime_ << "\n";
    mkt_seg_id_ = 1;
    if(mkt_seg_id_to_last_seen_seq_[mkt_seg_id_] <= 0){ // First Packet Seen From The Stream

         dbglogger_ << "STARTED RECEIVING DATA" << DBGLOG_ENDL_NOFLUSH;
        mkt_seg_id_to_last_seen_seq_[mkt_seg_id_] = msg_seq_no;

      } else if (mkt_seg_id_to_last_seen_seq_[mkt_seg_id_] >= msg_seq_no) {
        // return;  // Nothing to do

      } else if ((1 + mkt_seg_id_to_last_seen_seq_[mkt_seg_id_]) == msg_seq_no) {
	mkt_seg_id_to_last_seen_seq_[mkt_seg_id_] = msg_seq_no;
      } else {
        dbglogger_ << "DROPPED PACKETS FOR SEGMENT ID" << DBGLOG_ENDL_NOFLUSH;
        mkt_seg_id_to_last_seen_seq_[mkt_seg_id_] = msg_seq_no;
      }

/*
    dbglogger_ << "BSERawMDHandler::ProcessAllEvents: \n"
	      << "body_len_: " << body_len_ << "\n"
	      << "template_id_: " << template_id_ << "\n"
	      << "msg_seq_no_not_used: " << msg_seq_no_not_used << "\n"
	      << "msg_seq_no: " << msg_seq_no << " "
	      << "mkt_seg_id_: " << mkt_seg_id_
	      << " = > " << HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(mkt_seg_id_, BSE_EQ_SEGMENT_MARKING) << "\n"
	      << "partition_id_: " << partition_id_ << "\n"
	      << "completion_indicator_: " << completion_indicator_ << "\n"
	      << "AppSeqResetIndicator_: " << AppSeqResetIndicator_ << "\n"
	      << "TransactTime_: " << TransactTime_ << "\n"; 
*/
    msg_length -=body_len_;
    msg_ptr += body_len_;
    int i = 0;
    while (msg_length > 0) {
      body_len_ = *(uint16_t *)(msg_ptr + 0);     // Read unsigned int of size 2 bytes at offset 0 for body length
      template_id_ = *(uint16_t *)(msg_ptr + 2);  // Read unsigned int of size 2 bytes at offset 2 for template id
      // dbglogger_ << "LENGHT " << body_len_ << " ID  " << template_id_ <<"\n";
       int64_t security_id_ =0;
/*       if ( template_id_== 13100) security_id_ = *(int64_t *)(msg_ptr + 16);
       else if ( template_id_ == 13100) security_id_ = *(int64_t *)(msg_ptr + 16);
       else if ( template_id_ == 13101) security_id_ = *(int64_t *)(msg_ptr + 40);
       else if ( template_id_ == 13106) security_id_ = *(int64_t *)(msg_ptr + 32);
       else if ( template_id_ == 13102) security_id_ = *(int64_t *)(msg_ptr + 24);
       else if ( template_id_ == 13103) security_id_ = *(int64_t *)(msg_ptr + 8);
       else if ( template_id_ == 13105) security_id_ = *(int64_t *)(msg_ptr + 32);
       else if ( template_id_ == 13104) security_id_ = *(int64_t *)(msg_ptr + 32);
*/    if ( template_id_== 13203) {
        security_id_ = *(int64_t *)(msg_ptr + 8);
       int seq = *(uint32_t *)(msg_ptr + 4);
       dbglogger_ << "BSEPacket " << i << " TOKEN  " << security_id_ << " NAME " << HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(security_id_, BSE_EQ_SEGMENT_MARKING) << " SEQ " << seq << "\n";
}
//    bse_tbt_data_decoder_.DecodeEvents(msg_ptr, msg_seq_no, segment_marking);

      msg_length -= body_len_;
      msg_ptr += body_len_;
      i++;
    }
  }
}
