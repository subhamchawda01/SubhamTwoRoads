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
#define PACKET_REQUEST_MD5SUM_SIZE 16
#define BROADCAST_PACKET_LENGTH_LENGTH sizeof(int16_t)
#define BROADCAST_PACKET_SEQUENCE_LENGTH sizeof(int32_t)
#define BROADCAST_PACKET_CHECKSUM_LENGTH PACKET_REQUEST_MD5SUM_SIZE
#define BROADCAST_PACKET_LENGTH_OFFSET 0
#define BROADCAST_PACKET_LENGTH_LENGTH sizeof(int16_t)
#define BROADCAST_PACKET_SEQUENCE_OFFSET (BROADCAST_PACKET_LENGTH_OFFSET + BROADCAST_PACKET_LENGTH_LENGTH)
#define BROADCAST_PACKET_SEQUENCE_LENGTH sizeof(int32_t)

#define BROADCAST_PACKET_RESPONSE_LENGTH \
  (BROADCAST_PACKET_LENGTH_LENGTH + BROADCAST_PACKET_SEQUENCE_LENGTH + BROADCAST_PACKET_CHECKSUM_LENGTH)
#define BROADCAST_RESPONSE_MESSAGE_HEADER_LENGTH 40
int read_offset_ = 0;
// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() {
  time_t now = time(0);
  struct tm tstruct;
  char buf[80];
  tstruct = *localtime(&now);
  // Visit http://www.cplusplus.com/reference/clibrary/ctime/strftime/
  // for more information about date/time format
  strftime(buf, sizeof(buf), "%Y%m%d", &tstruct);

  return buf;
}

struct ProcessedBroadcastHeader {
  char reserved1[3];
  char reserved2[3];
  int32_t logTime;
  char alphachar[3];
  int16_t transaction_code;
  int16_t error_code;
  int32_t bcSeqNo;
  char reserved3[2];
  char reserved4[4];
  char timestamp2[9];
  char filler2[9];
  int16_t message_length;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "HEADER_TXN_CODE -> " << transaction_code << " HEADER_LOGITME -> " << logTime << " HEADER_ALPHACHAR "
               << alphachar << " HEADER_ERROR_CODE -> " << error_code << " HEADER_TIMESTAMP2 -> " << timestamp2
               << " HEADER_LENGTH -> " << message_length << "\n";

    return t_temp_oss.str();
  }
};

struct ProcessedBPacketHeader {
  int16_t packet_length;
  int32_t packet_sequnece_number;
  int16_t packet_message_count;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "PACKET_LENGTH -> " << packet_length << " PACKET_SEQUENCE_NUMBER -> " << packet_sequnece_number
               << "\n";

    return t_temp_oss.str();
  }
};

ProcessedBPacketHeader processed_packet_header_;
inline void ProcessPakcet(char const *msg_ptr) {
  processed_packet_header_.packet_length = ntoh16(*((int16_t *)(msg_ptr + BROADCAST_PACKET_LENGTH_OFFSET)));
  processed_packet_header_.packet_sequnece_number = ntoh32(*((int32_t *)(msg_ptr + BROADCAST_PACKET_SEQUENCE_OFFSET)));
}

ProcessedBroadcastHeader processed_broadcast_header_;
inline void ProcessHeader(char const *msg_ptr) {
  memset((void *)&processed_broadcast_header_, 0, sizeof(ProcessedBroadcastHeader));
  processed_broadcast_header_.logTime = ntoh32(*((int32_t *)(msg_ptr + 4)));
  memcpy((void *)processed_broadcast_header_.alphachar, (void *)(msg_ptr + 8), 2);
  processed_broadcast_header_.transaction_code = ntoh16(*((int16_t *)(msg_ptr + 10)));
  processed_broadcast_header_.error_code = ntoh16(*((int16_t *)(msg_ptr + 12)));
  processed_broadcast_header_.bcSeqNo = ntoh32(*((int32_t *)(msg_ptr + 14)));
  memcpy((void *)processed_broadcast_header_.timestamp2, (void *)(msg_ptr + 22), 8);
  processed_broadcast_header_.message_length = ntoh16(*((int16_t *)(msg_ptr + 38)));
}

struct ST_TICKET_INDEX_INFO {
  int32_t Token;
  int16_t MarketType;
  int32_t FillPrice;
  int32_t FillVolume;
  int32_t OpenInterest;
  int32_t DayHiOI;
  int32_t DayLoOI;

  std::string ToString(char segment_type) {
    std::ostringstream t_temp_oss;
    t_temp_oss << "Token: " << Token << "\nShortCode: "
               << HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(
                      Token, segment_type)
               << "\nMarkeType: " << MarketType << "\nFillPrice: " << FillPrice << "\nFillVOlume: " << FillVolume
               << "\nOpen Interest: " << OpenInterest << "\nDayHigh Open interest: " << DayHiOI
               << "\nDayLow Open interest: " << DayLoOI << "\n";
    return t_temp_oss.str();
  }
};

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
ST_TICKET_INDEX_INFO *st_ticket_index_info = new ST_TICKET_INDEX_INFO();
OPEN_INTEREST *open_interest_struct = new OPEN_INTEREST();
MS_INDICES *ms_indices_struct = new MS_INDICES();

void DecodeTradeExecutionHelper(const unsigned char *buffer, char segment_type) {
  // 8 bytes have to be ignored
  buffer += 8;

  int16_t trans_code = ntoh16(*((int16_t *)(buffer + 10)));
  int32_t LogTime = ntoh32(*((int32_t *)(buffer + 4)));
  int16_t msg_length = ntoh16(*((int16_t *)(buffer + 38)));
  int16_t NoOfRecords = ntoh16(*((int16_t *)(buffer + 12)));
  struct timeval source_time;
  // memcpy((void *)timestamp,(void *)(buffer + 22),8);
  // bcast header
  buffer += 40;
  source_time = clock_source_.GetTimeOfDay();
  if (trans_code == NSE_BCAST_TRADE_RANGE_TRANS_CODE) {
    int32_t msg_count = ntoh32(*((int32_t *)buffer));
    buffer += 4;
    std::cout << "Trade Execution Ranges (7220) " << std::endl;
    while (msg_count > 0) {
      nse_tbt_data_common_struct_->token = ntoh32(*((uint32_t *)buffer));
      nse_tbt_data_common_struct_->data.nse_trade_range.high_exec_band = ntoh32(*((uint32_t *)(buffer + 4)));
      nse_tbt_data_common_struct_->data.nse_trade_range.low_exec_band = ntoh32(*((uint32_t *)(buffer + 8)));
      /*       std::cout << "Token: " << nse_tbt_data_common_struct_->token << "\nShortCode: "
                      << HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(
                             nse_tbt_data_common_struct_->token, segment_type)
                      << " High: " << nse_tbt_data_common_struct_->data.nse_trade_range.high_exec_band
                      << " Low: " << nse_tbt_data_common_struct_->data.nse_trade_range.low_exec_band << std::endl; */
      buffer += 12;
      msg_count--;
    }
  } else if (trans_code == BCAST_TICKER_AND_MKT_INDEX) {
    int32_t msg_count = ntoh16(*((int16_t *)buffer));
    buffer += 2;
    std::cout << "BCAST_TICKER_AND_MKT_INDEX (7202) MSG: " << msg_count << " TimeStamp: " << source_time.tv_sec << "."
              << std::setw(6) << std::setfill('0') << source_time.tv_usec << " LEN: " << msg_length
              << " TIME: " << LogTime << std::endl;
    while (msg_count > 0) {
      st_ticket_index_info->Token = ntoh32(*((uint32_t *)buffer));
      st_ticket_index_info->MarketType = ntoh16(*((uint16_t *)(buffer + 4)));
      st_ticket_index_info->FillPrice = ntoh32(*((uint32_t *)(buffer + 6)));
      st_ticket_index_info->FillVolume = ntoh32(*((uint32_t *)(buffer + 10)));
      st_ticket_index_info->OpenInterest = ntoh32(*((uint32_t *)(buffer + 14)));
      st_ticket_index_info->DayHiOI = ntoh32(*((uint32_t *)(buffer + 18)));
      st_ticket_index_info->DayLoOI = ntoh32(*((uint32_t *)(buffer + 22)));
      //      std::cout << st_ticket_index_info->ToString(segment_type);
      buffer += 26;
      msg_count--;
    }
  } else if (trans_code == MKT_MVMT_CM_OI_IN) {
    int msg_count = NoOfRecords;
    std::cout << "MKT_MVMT_CM_OI_IN (7130) Code:"
              << " MSG: " << msg_count << " TimeStamp: " << source_time.tv_sec << "." << std::setw(6)
              << std::setfill('0') << source_time.tv_usec << std::endl;
    while (msg_count > 0) {
      open_interest_struct->TokenNo = ntoh32(*((uint32_t *)buffer));
      open_interest_struct->CurrentOI = ntoh32(*((uint32_t *)(buffer + 4)));
      //      std::cout << open_interest_struct->ToString(segment_type);
      buffer += 8;
      msg_count--;
    }
  } else if (trans_code == MS_BCAST_INDICES_TRANS_CODE) {
    int16_t msg_count = ntoh16(*((int16_t *)(buffer + 0)));  // 40 is already incremented
    buffer += 2;

    std::cout << "MS_BCAST_INDICES_TRANS_CODE (7207) Code:"
              << " MSG: " << msg_count << " TimeStamp: " << source_time.tv_sec << "." << std::setw(6)
              << std::setfill('0') << source_time.tv_usec << " LEN: " << msg_length << " TIME: " << LogTime
              << std::endl;
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
    std::cout << "BCAST_SPD_MBP_DELTA (7211) " << std::endl;
  } else if (trans_code == 7208) {
    std::cout << "BCAST_ONLY_MBP (7208) " << std::endl;
  } else if (trans_code == 6541) {
    std::cout << "BC_CIRCUIT_CHECK (6541) " << std::endl;
  } else {
    std::cout << "UnKnown Trans_code: " << trans_code << std::endl;
  }
  std::cout << std::endl;
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

int main(int argc, char *argv[]) {
  if (argc < 5) {
    printf("USAGE: %s <ip> <port> <interface> <OF of>\nSize of TimeVal: %ld\t Int: %ld\n", argv[0], sizeof(timeval),
           sizeof(uint32_t));
    exit(0);
  }

  HFSAT::MulticastReceiverSocket *sock = new HFSAT::MulticastReceiverSocket(argv[1], atoi(argv[2]), argv[3]);
  sock->setBufferSize(N);
  HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());
  char fname[200];
  char seg = 'F';
  sprintf(fname, "/spare/local/MDSlogs/RawData/Bcast/%s_%s.raw", argv[4], currentDateTime().c_str());
  std::cerr << "File name: " << fname << std::endl;
  HFSAT::BulkFileWriter bfw(fname, N);
  if (!bfw.is_open()) {
    std::cerr << "Cannot open file: " << argv[4] << std::endl;
  }
  char *msg_buf = new char[N];
  int32_t msg_len = 1;
  timeval time_;

  while (msg_len > 0) {
    msg_len = sock->ReadN(MAX_BROADCAST_DATA_BUFFER, (void *)msg_buf);
    if (msg_len == -1) {
      std::cout << "Error occured" << std::endl;
      continue;
    }
    char *msg_ptr = msg_buf;
    DecodeTradeExecutionRange((const unsigned char *)msg_ptr, seg);
    gettimeofday(&time_, NULL);
    bfw.Write(&msg_len, sizeof(int));
    bfw.Write(&time_, sizeof(time_));
    bfw.Write((void *)msg_buf, msg_len);
    bfw.DumpCurrentBuffer();
  }
  bfw.DumpCurrentBuffer();
  bfw.Close();
  return 0;
}
