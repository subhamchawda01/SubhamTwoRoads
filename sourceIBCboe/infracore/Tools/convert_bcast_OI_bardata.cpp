#include <arpa/inet.h>
#include <errno.h>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <time.h>
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include <arpa/inet.h>
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include <lzo/lzoconf.h>
#include <lzo/lzo1z.h>
#include "dvccode/Utils/rdtscp_timer.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/Utils/nse_daily_token_symbol_handler.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include <sstream>
#define N 4000000
#define MAX_BROADCAST_RESPONSE_BUFFER_SIZE 105536
#define MAX_BROADCAST_DATA_BUFFER 65536

#define NSE_TBT_DATA_START_OFFSET 0

#define NSE_BCAST_TRADE_RANGE_TRANS_CODE 7220
#define IN_LEN (128 * 1024L)
#define OUT_LEN (IN_LEN + IN_LEN / 16 + 64 + 3)
#define HEAP_ALLOC(var, size) lzo_align_t __LZO_MMODEL var[((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t)]
//static HEAP_ALLOC(wrkmem, LZO1Z_999_MEM_COMPRESS);

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

struct ST_TICKET_INDEX_INFO {
  int32_t Token;
  int16_t MarketType;
  int32_t FillPrice;
  int32_t FillVolume;
  int32_t OpenInterest;
  int32_t DayHiOI;
  int32_t DayLoOI;
  ST_TICKET_INDEX_INFO()
      : Token(0), MarketType(0), FillPrice(0), FillVolume(0), OpenInterest(0), DayHiOI(0), DayLoOI(INT_MAX) {}
};

#define MIN_PER_DAY 1440
long long secondUtcMid;
int MaxStart_time = 1440, MaxEnd_time = 0;
std::map<int, int> ticker_min_info_map;
int position_vector_assign = 0;
std::vector<std::vector<ST_TICKET_INDEX_INFO>> ticker_min_info(2000);
HFSAT::ClockSource& clock_source_ = HFSAT::ClockSource::GetUniqueInstance();
NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct_ = new NSE_MDS::NSETBTDataCommonStruct();
ST_TICKET_INDEX_INFO* st_ticket_index_info = new ST_TICKET_INDEX_INFO();

void BardataGenerator(NSE_MDS::ST_TICKET_INDEX_INFO *buffer, size_t size){
  timeval time_;
  time_.tv_sec = buffer->source_time.tv_sec;
  time_.tv_usec = buffer->source_time.tv_usec;
  int vector_pos = (time_.tv_sec - secondUtcMid) / 60;
  if (vector_pos < 0 || vector_pos > MIN_PER_DAY) {
    std::cout << "Error with the Trading Seconds" << std::endl;
    return;
  }
  if (vector_pos > MaxEnd_time) MaxEnd_time = vector_pos;
  if (vector_pos < MaxStart_time) MaxStart_time = vector_pos;
  if (ticker_min_info_map.find(buffer->Token) == ticker_min_info_map.end()) {
      ticker_min_info_map[buffer->Token] = position_vector_assign;
      ticker_min_info[position_vector_assign].resize(MIN_PER_DAY);
      position_vector_assign++;
  }
  int index = ticker_min_info_map[buffer->Token];
  ticker_min_info[index][vector_pos].MarketType = buffer->MarketType;
  ticker_min_info[index][vector_pos].FillPrice = buffer->FillPrice;
  ticker_min_info[index][vector_pos].FillVolume = buffer->FillVolume;
  ticker_min_info[index][vector_pos].OpenInterest = buffer->OpenInterest;
  if (ticker_min_info[index][vector_pos].DayHiOI < buffer->DayHiOI) {
    ticker_min_info[index][vector_pos].DayHiOI = buffer->DayHiOI;
  }
  if (ticker_min_info[index][vector_pos].DayLoOI > buffer->DayLoOI) {
    ticker_min_info[index][vector_pos].DayLoOI = buffer->DayLoOI;
  }
}
void DecodeTradeExecutionHelper(const unsigned char* buffer, timeval& time_) {
  // 8 bytes have to be ignored
  buffer += 8;

  int16_t trans_code = ntoh16(*((int16_t*)(buffer + 10)));
  buffer += 40;
  int32_t token_val;
  int vector_pos = (time_.tv_sec - secondUtcMid) / 60;
  if (vector_pos < 0 || vector_pos > MIN_PER_DAY) {
    std::cout << "Error with the Trading Seconds" << std::endl;
    return;
  }
  if (vector_pos > MaxEnd_time) MaxEnd_time = vector_pos;

  if (vector_pos < MaxStart_time) MaxStart_time = vector_pos;
  if (trans_code == 7202) {
    //    std::cout<<"Pos: " << vector_pos <<std::endl;
    //    std::cout<<"Token: " << ntoh32(*((uint32_t*)buffer))<<std::endl;
    int32_t msg_count = ntoh16(*((int16_t*)buffer));
    buffer += 2;
    while (msg_count > 0) {
      token_val = ntoh32(*((uint32_t*)buffer));
      if (ticker_min_info_map.find(token_val) == ticker_min_info_map.end()) {
        ticker_min_info_map[token_val] = position_vector_assign;
        ticker_min_info[position_vector_assign].resize(MIN_PER_DAY);
        position_vector_assign++;
      }
      int index = ticker_min_info_map[token_val];
      ticker_min_info[index][vector_pos].MarketType = ntoh16(*((uint16_t*)(buffer + 4)));
      ticker_min_info[index][vector_pos].FillPrice = ntoh32(*((uint32_t*)(buffer + 6)));
      ticker_min_info[index][vector_pos].FillVolume = ntoh32(*((uint32_t*)(buffer + 10)));
      ticker_min_info[index][vector_pos].OpenInterest = ntoh32(*((uint32_t*)(buffer + 14)));
      if ((unsigned)ticker_min_info[index][vector_pos].DayHiOI < ntoh32(*((uint32_t*)(buffer + 18)))) {
        ticker_min_info[index][vector_pos].DayHiOI = ntoh32(*((uint32_t*)(buffer + 18)));
      }
      if ((unsigned)ticker_min_info[index][vector_pos].DayLoOI > ntoh32(*((uint32_t*)(buffer + 22)))) {
        ticker_min_info[index][vector_pos].DayLoOI = ntoh32(*((uint32_t*)(buffer + 22)));
      }
      buffer += 26;
      msg_count--;
    }
  }
}

void DecodeAndFillStruct(const unsigned char* buffer, timeval& time_) {
  // Decoder Doc: FOTS_NNF_PROTOCOL -> Broadcast -> Trade Execution Range

  int16_t num_packets = (int16_t)ntoh16(*((int16_t*)(buffer + 2)));
  buffer += 4;
  // std::cout<<"Number of packets "<<num_packets<<std::endl;
  while (num_packets > 0) {
    lzo_uint in_len;
    // lzo_uint out_len;

    in_len = (int64_t)ntoh16(*((int16_t*)(buffer)));
    // std::cout<<"in_len "<<in_len<<std::endl;
    buffer += 2;

    // Data isn't compressed
    if (in_len == 0) {
      // std::cout << "Not compressed " << std::endend_time;
      const unsigned char* tmp = out_lzo;
      DecodeTradeExecutionHelper(tmp, time_);
      buffer += in_len;
    }

    num_packets--;
  }
}

void DumpBarData(char* fname, int date_, char seg) {
  HFSAT::BulkFileWriter bulk_file_writer_;
  bulk_file_writer_.Open(fname);
  long long start_time = 3 * 60 + 45;
  long long end_time = 10 * 60;
  HFSAT::SecurityDefinitions::GetUniqueInstance(date_).LoadNSESecurityDefinitions();
  HFSAT::Utils::NSEDailyTokenSymbolHandler& nse_daily_token_symbol_handler_ =
      HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(date_);
  for (auto const& token : ticker_min_info_map) {
    std::string internal_symbol = nse_daily_token_symbol_handler_.GetInternalSymbolFromToken(token.first, seg);
    if (std::string("INVALID") == internal_symbol) continue;
    std::string exchange_symbol = HFSAT::NSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol);
    if (std::string("INVALID") == exchange_symbol) continue;
    std::string shortcode = HFSAT::NSESecurityDefinitions::GetShortCodeFromExchangeSymbol(exchange_symbol);
    int bhavcopy_oi =  HFSAT::NSESecurityDefinitions::GetOpenInterestFromExchangeSymbol(exchange_symbol);
    if (std::string("INVALID") == shortcode) continue;
    long timestamp_ = HFSAT::DateTime::GetTimeUTC(date_, 344);
    int exp_ = HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(shortcode);
    int strike_ = HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCodeGeneric(shortcode);
    int prev_oi = 0, prev_dayhigh = 0, prev_day_low = 0, netOIchange;
    long long total_open_interest = 0, no_of_open_interest = 0;
    for (int index = start_time; index < end_time; index++) {
      timestamp_ += 60;
      if (index < MaxStart_time) continue;
      if (ticker_min_info[token.second][index].OpenInterest == 0) {
        if (prev_oi == 0) continue;
        ticker_min_info[token.second][index].OpenInterest = prev_oi;
        ticker_min_info[token.second][index].DayLoOI = prev_day_low;
        ticker_min_info[token.second][index].DayHiOI = prev_dayhigh;
      }
      netOIchange = ticker_min_info[token.second][index].OpenInterest - prev_oi;
      no_of_open_interest++;
      total_open_interest += ticker_min_info[token.second][index].OpenInterest;
      char buf[1024] = {0};
      sprintf(buf, "%10ld %12d %-25s %12d %12d %16lld %12d %12d %12d %12d %12d", timestamp_, token.first,
              shortcode.c_str(), ticker_min_info[token.second][index].OpenInterest, netOIchange,
              (total_open_interest / no_of_open_interest), ticker_min_info[token.second][index].DayHiOI,
              ticker_min_info[token.second][index].DayLoOI, bhavcopy_oi, strike_, exp_);
      bulk_file_writer_ << buf << '\n';
      bulk_file_writer_.CheckToFlushBuffer();
      prev_oi = ticker_min_info[token.second][index].OpenInterest;
      prev_day_low = ticker_min_info[token.second][index].DayLoOI;
      prev_dayhigh = ticker_min_info[token.second][index].DayHiOI;
    }
  }
  bulk_file_writer_.Close();
}

int main(int argc, char* argv[]) {
  if (argc < 5) {
    printf("USAGE: %s <input-file> <dataloggingdir> <date> <OI of>\nSize of TimeVal: %ld\t Int: %ld\n", argv[0],
           sizeof(timeval), sizeof(uint32_t));
    exit(0);
  }
  char* raw_file_ = argv[1];
  int date_ = atoi(argv[3]);
  char seg = 'F';
  std::cout << "Time: " << HFSAT::DateTime::GetTimeMidnightUTC(date_) << std::endl;
  std::cout << "Time: " << HFSAT::DateTime::GetTimeUTC(date_, 345) << std::endl;
  secondUtcMid = HFSAT::DateTime::GetTimeMidnightUTC(date_);
  HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(date_);
  char fname[200];
  sprintf(fname, "%s/%s_OI_%s", argv[2], argv[4], std::to_string(date_).c_str());
  std::cout << "File name: " << fname << std::endl;
  HFSAT::BulkFileReader reader;
  reader.open(raw_file_);
  if (!reader.is_open()) {
    std::cerr << " Can't open file, exiting \n";
    exit(1);
  }
  // char* msg_buf = new char[N];
  // uint32_t msg_len = 1;
  // timeval time_;
  int read_size;
  NSE_MDS::ST_TICKET_INDEX_INFO next_event_;
  if (reader.is_open()) {
    // while (true) {
    //   read_size = reader.read(&(msg_len), sizeof(uint32_t));
    //   if (read_size < (int)sizeof(uint32_t)) break;
    //   if (msg_len <= (int)sizeof(ST_TICKET_INDEX_INFO)) {
    //     continue;
    //   }
    //   read_size = reader.read(&(time_), sizeof(time_));
    //   if (read_size < (int)sizeof(time_)) break;

    //   read_size = reader.read(msg_buf, msg_len);
    //   DecodeAndFillStruct((const unsigned char*)msg_buf, time_);
    // }

    while(true){
      read_size = reader.read(&(next_event_), sizeof(next_event_));
      if (read_size < (int)sizeof(next_event_)) break;
      BardataGenerator(&next_event_, sizeof(next_event_));
    }

  }
  DumpBarData(fname, date_, seg);
  reader.close();
  return 0;
}
