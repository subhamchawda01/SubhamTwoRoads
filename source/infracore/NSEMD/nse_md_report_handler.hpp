#include <arpa/inet.h>
#include <errno.h>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <string>
#include <time.h>
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include <arpa/inet.h>
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include <lzo/lzoconf.h>
#include <lzo/lzo1z.h>
#include "dvccode/Utils/rdtscp_timer.hpp"
#include <sstream>
#include "dvccode/Utils/allocate_cpu.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include <unistd.h> 
#include "dvccode/Utils/mds_logger.hpp"

#define NSE_MAX_FD_SUPPORTED 1024
#define MAX_LINE_SIZE 1024
#define MAX_NSE_DATA_BUFFER 65536
#define N 4000000
#define MAX_BROADCAST_RESPONSE_BUFFER_SIZE 105536
#define MAX_BROADCAST_DATA_BUFFER 65536

#define NSE_TBT_DATA_START_OFFSET 0

#define NSE_BCAST_TRADE_RANGE_TRANS_CODE 7220
#define MS_BCAST_INDICES_TRANS_CODE 7207
#define MKT_MVMT_CM_OI_IN 7130
#define BCAST_TICKER_AND_MKT_INDEX 7202

#define IN_LEN (128 * 1024L)
#define OUT_LEN (IN_LEN + IN_LEN / 16 + 64 + 3)
#define HEAP_ALLOC(var, size) lzo_align_t __LZO_MMODEL var[((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t)]

static HEAP_ALLOC(wrkmem, LZO1Z_999_MEM_COMPRESS);
static unsigned char __LZO_MMODEL out_lzo[OUT_LEN];

#define ntoh64 __builtin_bswap64
#define ntoh32 __builtin_bswap32
#define ntoh16 ntohs

const std::string currentDateTime() {
    time_t now = time(0);
      struct tm tstruct;
        char buf[80];
          tstruct = *localtime(&now);
            // Visit http://www.cplusplus.com/reference/clibrary/ctime/strftime/
           strftime(buf, sizeof(buf), "%Y%m%d", &tstruct);

           return buf;
}



namespace HFSAT{

struct OPEN_INTEREST {
  int32_t TokenNo;
  int32_t CurrentOI;

  std::string ToString(char segment_type) {
    std::ostringstream t_temp_oss;
    std::cout << "Token: " << TokenNo << "\nShortCode: "
              << HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(TokenNo,
                                                                                                            'E')
              << "\nOpen Interest: " << CurrentOI << "\n";
    return t_temp_oss.str();
  }
};

struct CM_ASSET_OI {
  char Reserved1[3];
  char Reserved2[3];
  int32_t LogTime[4];
  char MarketType[3];
  int16_t TransactionCode;
  int16_t NoOfRecords;
  char Reserved3[9];
  char TimeStamp[9];
  char Reserved[9];
  int16_t MessageLength;
};

struct MS_INDICES {
  char IndexName[21];
  int32_t IndexValue;
  int32_t HighIndexValue;
  int32_t LowIndexValue;
  int32_t OpeningIndex;
  int32_t ClosingIndex;
  int32_t PercentChange;
  int32_t YearlyHigh;
  int32_t YearlyLow;
  int32_t NoOfUpmoves;
  int32_t NoOfDownmoves;
  double Market_Capitalisation;
  char NetChangeIndicator;
  char Reserved;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "IndexName -> " << IndexName << " IndexValue -> " << IndexValue << " HighIndexValue -> "
               << HighIndexValue << " LowIndexValue -> " << LowIndexValue << " OpeningIndex -> " << OpeningIndex
               << " ClosingIndex -> " << ClosingIndex << " PercentChange -> " << PercentChange << " YearlyHigh -> "
               << YearlyHigh << " YearlyLow -> " << YearlyLow << " NoOfUpmoves -> " << NoOfUpmoves
               << " NoOfDownmoves -> " << NoOfDownmoves << " Market_Capitalisation -> " << Market_Capitalisation
               << " NetChangeIndicator -> " << NetChangeIndicator << "\n";
    return t_temp_oss.str();
  }
};
HFSAT::ClockSource &clock_source_ = HFSAT::ClockSource::GetUniqueInstance();
NSE_MDS::NSETBTDataCommonStruct *nse_tbt_data_common_struct_ = new NSE_MDS::NSETBTDataCommonStruct();
NSE_MDS::ST_TICKET_INDEX_INFO *st_ticket_index_info = new NSE_MDS::ST_TICKET_INDEX_INFO();
OPEN_INTEREST *open_interest_struct = new OPEN_INTEREST();
MS_INDICES *ms_indices_struct = new MS_INDICES();
    
//NSE_MDS::ST_TICKET_INDEX_INFO *st_ticket_index_info = new ST_TICKET_INDEX_INFO();


class NSEMDReportHandler : /*public HFSAT::Thread,*/ public HFSAT::SimpleExternalDataLiveListener{
 private:
   HFSAT::DebugLogger& dbglogger_;
   HFSAT::SimpleLiveDispatcher& simple_live_dispatcher_;
   std::string nse_MR_channel_filename;
   std::string interface;
   bool day_over_;
   HFSAT::MulticastReceiverSocket* socket_fd_to_multicast_receiver_sockets_[NSE_MAX_FD_SUPPORTED];
   char data_buffer[MAX_NSE_DATA_BUFFER];
   MDSLogger<NSE_MDS::ST_TICKET_INDEX_INFO>* data_logger_thread_;
  public:

   NSEMDReportHandler(HFSAT::DebugLogger& dbglogger, HFSAT::SimpleLiveDispatcher& simple_live_dispatcher,
                    std::string nse_MR_channel_filename, std::string interface)
   :dbglogger_(dbglogger),
    simple_live_dispatcher_(simple_live_dispatcher),
    nse_MR_channel_filename(nse_MR_channel_filename),
    interface(interface),
    day_over_(false){
       for (int32_t ctr = 0; ctr < NSE_MAX_FD_SUPPORTED; ctr++) {
          socket_fd_to_multicast_receiver_sockets_[ctr] = NULL;
       }
       CreateSockets();
       data_logger_thread_ = new MDSLogger<NSE_MDS::ST_TICKET_INDEX_INFO>("OILogs");
       if (data_logger_thread_ == nullptr) {
         std::cerr << "Cannot Create MDSLoggerThread in CombinedSourceGenericLogger. Exiting." << std::endl;
         exit(1);
       }
       //data_logger_thread_->EnableAffinity("OIMDSLoggerThread");
       std::cout<<"logger affined enabled"<<std::endl;
    }

  void CreateSockets(){
    std::ifstream nse_MR_channels_file;
     nse_MR_channels_file.open(nse_MR_channel_filename.c_str());

     if (!nse_MR_channels_file.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "Failed To Load The market report Multicast File : " << nse_MR_channel_filename
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      exit(-1);
     }
    
     char buffer[1024];

     while (nse_MR_channels_file.good()) {
       nse_MR_channels_file.getline(buffer, MAX_LINE_SIZE);
       std::string line_buffer = buffer;

       // Comments 
       if (line_buffer.find("#") != std::string::npos) continue;

       HFSAT::PerishableStringTokenizer pst(buffer, MAX_LINE_SIZE);
       std::vector<char const*> const& tokens = pst.GetTokens();
       if (tokens.size() != 3) continue;

      // Create a non-blocking socket on each stream
      // Last Argument Being True Is Very Important For Address Specific Bind
       HFSAT::MulticastReceiverSocket* new_multicast_receiver_socket = new HFSAT::MulticastReceiverSocket(
           tokens[1], atoi(tokens[2]), interface);
       std::cout << "CREATING SOCKET : " << tokens[1] << " " << tokens[2] << " Interface : "
                                  << interface << std::endl;
       DBGLOG_DUMP;
       new_multicast_receiver_socket->SetNonBlocking();
       if (new_multicast_receiver_socket->socket_file_descriptor() >= NSE_MAX_FD_SUPPORTED) {
         DBGLOG_CLASS_FUNC_LINE_FATAL
             << "SOMETHING HAS GONE WRONG IT SEEMS, DIDN'T EXPECT TO SEE A SOCKET FD WITH DESCRIPTOR : "
             << new_multicast_receiver_socket->socket_file_descriptor()
             << " MAX SUPPOERTED RANGE IS UPTO : " << NSE_MAX_FD_SUPPORTED << DBGLOG_ENDL_NOFLUSH;
         DBGLOG_DUMP;
         exit(-1);
       }

       socket_fd_to_multicast_receiver_sockets_[new_multicast_receiver_socket->socket_file_descriptor()] =
           new_multicast_receiver_socket;

       std::cout << "SOCKET FD :" << new_multicast_receiver_socket->socket_file_descriptor()
                                  << " SETUP ON : " << tokens[0] << std::endl;
       DBGLOG_DUMP;

       simple_live_dispatcher_.AddSimpleExternalDataLiveListenerSocket(
           this, new_multicast_receiver_socket->socket_file_descriptor(), true);
  
    }
    nse_MR_channels_file.close();
    DBGLOG_DUMP;
}

  /*void thread_main() {
    while (!day_over_) {
      sleep(1);
    }
  }*/
  void DayOver() { day_over_ = true; }
  void DecodeTradeExecutionHelper(const unsigned char *buffer, char segment_type) {
  // 8 bytes have to be ignored
  buffer += 8;

  int16_t trans_code = ntoh16(*((int16_t *)(buffer + 10)));
  int16_t NoOfRecords = ntoh16(*((int16_t *)(buffer + 12)));
  struct timeval source_time;
  // memcpy((void *)timestamp,(void *)(buffer + 22),8);
  // bcast header
  buffer += 40;
  source_time = clock_source_.GetTimeOfDay();
  if (trans_code == NSE_BCAST_TRADE_RANGE_TRANS_CODE) {
    int32_t msg_count = ntoh32(*((int32_t *)buffer));
    buffer += 4;
    while (msg_count > 0) {
      nse_tbt_data_common_struct_->token = ntoh32(*((uint32_t *)buffer));
      nse_tbt_data_common_struct_->data.nse_trade_range.high_exec_band = ntoh32(*((uint32_t *)(buffer + 4)));
      nse_tbt_data_common_struct_->data.nse_trade_range.low_exec_band = ntoh32(*((uint32_t *)(buffer + 8)));
             std::cout << "Token: " << nse_tbt_data_common_struct_->token << "\nShortCode: "
                      << HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(
                             nse_tbt_data_common_struct_->token, segment_type)
                      << " High: " << nse_tbt_data_common_struct_->data.nse_trade_range.high_exec_band
                      << " Low: " << nse_tbt_data_common_struct_->data.nse_trade_range.low_exec_band << std::endl; 
      buffer += 12;
      msg_count--;
    }
  } else if (trans_code == BCAST_TICKER_AND_MKT_INDEX) {
    int32_t msg_count = ntoh16(*((int16_t *)buffer));
    buffer += 2;
    while (msg_count > 0) {
      st_ticket_index_info->source_time.tv_sec = source_time.tv_sec;
      st_ticket_index_info->source_time.tv_usec = source_time.tv_usec;
      st_ticket_index_info->Token = ntoh32(*((uint32_t *)buffer));
      st_ticket_index_info->MarketType = ntoh16(*((uint16_t *)(buffer + 4)));
      st_ticket_index_info->FillPrice = ntoh32(*((uint32_t *)(buffer + 6)));
      st_ticket_index_info->FillVolume = ntoh32(*((uint32_t *)(buffer + 10)));
      st_ticket_index_info->OpenInterest = ntoh32(*((uint32_t *)(buffer + 14)));
      st_ticket_index_info->DayHiOI = ntoh32(*((uint32_t *)(buffer + 18)));
      st_ticket_index_info->DayLoOI = ntoh32(*((uint32_t *)(buffer + 22)));
      data_logger_thread_->log(*st_ticket_index_info);
      buffer += 26;
      msg_count--;
    }
  } else if (trans_code == MKT_MVMT_CM_OI_IN) {
    int msg_count = NoOfRecords;
    while (msg_count > 0) {
      open_interest_struct->TokenNo = ntoh32(*((uint32_t *)buffer));
      open_interest_struct->CurrentOI = ntoh32(*((uint32_t *)(buffer + 4)));
            std::cout << open_interest_struct->ToString(segment_type);
      buffer += 8;
      msg_count--;
    }
  } else if (trans_code == MS_BCAST_INDICES_TRANS_CODE) {
    int16_t msg_count = ntoh16(*((int16_t *)(buffer + 0)));  // 40 is already incremented
    buffer += 2;
    while (msg_count > 0) {
      memcpy((void *)ms_indices_struct->IndexName, (void *)(buffer), 21);
      // strcpy(ms_indices_struct->IndexName,reinterpret_cast<const char*>(buffer));
      buffer += 1;  // byte align
      ms_indices_struct->IndexValue = ntoh32(*((uint32_t *)(buffer + 21)));
      ms_indices_struct->HighIndexValue = ntoh32(*((uint32_t *)(buffer + 25)));
      ms_indices_struct->LowIndexValue = ntoh32(*((uint32_t *)(buffer + 29)));
      ms_indices_struct->OpeningIndex = ntoh32(*((uint32_t *)(buffer + 33)));
      ms_indices_struct->ClosingIndex = ntoh32(*((uint32_t *)(buffer + 37)));
      ms_indices_struct->PercentChange = ntoh32(*((uint32_t *)(buffer + 41)));
      ms_indices_struct->YearlyHigh = ntoh32(*((uint32_t *)(buffer + 45)));
      ms_indices_struct->YearlyLow = ntoh32(*((uint32_t *)(buffer + 49)));
      ms_indices_struct->NoOfUpmoves = ntoh32(*((uint32_t *)(buffer + 53)));
      ms_indices_struct->NoOfDownmoves = ntoh32(*((uint32_t *)(buffer + 57)));
      ms_indices_struct->Market_Capitalisation = *((double *)(buffer + 61));
      ms_indices_struct->NetChangeIndicator = *((char *)(buffer + 69));
      std::cout << ms_indices_struct->ToString();
      buffer += 71;
      msg_count--;
    }
  } else if (trans_code == 7211) {
//    std::cout << "BCAST_SPD_MBP_DELTA (7211) " << std::endl;
  } else if (trans_code == 7208) {
//    std::cout << "BCAST_ONLY_MBP (7208) " << std::endl;
  } else if (trans_code == 6541) {
//    std::cout << "BC_CIRCUIT_CHECK (6541) " << std::endl;
  } else {
    std::cout << "UnKnown Trans_code: " << trans_code << std::endl;
  }
//  std::cout << std::endl;
  }

  void run_logger_thread(){
      data_logger_thread_->run();
  }

  void DecodeTradeExecutionRange(const unsigned char *buffer, char const &segment) {
  // Decoder Doc: FOTS_NNF_PROTOCOL -> Broadcast -> Trade Execution Range
  nse_tbt_data_common_struct_->segment_type = segment;
  nse_tbt_data_common_struct_->msg_type = NSE_MDS::MsgType::kNSETradeExecutionRange;
  // Packed and sent further to processor
  gettimeofday(&nse_tbt_data_common_struct_->source_time, NULL);
  int16_t num_packets = (int16_t)ntoh16(*((int16_t *)(buffer + 2)));
  buffer += 4;
  while (num_packets > 0) {
    lzo_uint in_len;
    lzo_uint out_len;
    in_len = (int64_t)ntoh16(*((int16_t *)(buffer)));
    buffer += 2;

   // std::cout<<"numpackets: " << num_packets<<std::endl;
    // Data isn't compressed
    if (in_len == 0) {
      DecodeTradeExecutionHelper(buffer, segment);
      int16_t msg_len = (int16_t)ntoh16(*((int16_t *)(buffer + 46)));
      buffer += (msg_len + 8);
    } else {
      lzo1z_decompress(buffer, in_len, out_lzo, &out_len, wrkmem);
      const unsigned char *tmp = out_lzo;
      DecodeTradeExecutionHelper(tmp, segment);
      buffer += in_len;
    }
    num_packets--;
   }
   }
   void ProcessAllEvents(int32_t socket_fd) {
    char* msg_ptr = data_buffer;
    char seg = 'F';
    DecodeTradeExecutionRange((const unsigned char*) msg_ptr, seg);
   }
};
}
