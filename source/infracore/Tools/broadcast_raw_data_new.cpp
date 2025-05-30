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
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include <arpa/inet.h>
#include <lzo/lzoconf.h>
#include <lzo/lzo1z.h>
#include <sstream>
#define N 4000000
#define MAX_BROADCAST_RESPONSE_BUFFER_SIZE 105536
#define MAX_BROADCAST_DATA_BUFFER 65536
#define NSE_TBT_DATA_START_OFFSET 0

#define NSE_BCAST_TRADE_RANGE_TRANS_CODE 7220
#define IN_LEN (128 * 1024L)
#define OUT_LEN (IN_LEN + IN_LEN / 16 + 64 + 3)
#define HEAP_ALLOC(var, size) lzo_align_t __LZO_MMODEL var[((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t)]
static HEAP_ALLOC(wrkmem, LZO1Z_999_MEM_COMPRESS);


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

#define BROADCAST_PACKET_RESPONSE_LENGTH (BROADCAST_PACKET_LENGTH_LENGTH + BROADCAST_PACKET_SEQUENCE_LENGTH + BROADCAST_PACKET_CHECKSUM_LENGTH)
#define BROADCAST_RESPONSE_MESSAGE_HEADER_LENGTH 40

char nse_msg_buffer_[MAX_BROADCAST_RESPONSE_BUFFER_SIZE];
int read_offset_ = 0;

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
               << alphachar << " HEADER_ERROR_CODE -> " << error_code
               << " HEADER_TIMESTAMP2 -> " << timestamp2 << " HEADER_LENGTH -> " << message_length << "\n";

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
  memcpy((void *)processed_broadcast_header_.alphachar, (void *)(msg_ptr + 8),2);
  processed_broadcast_header_.transaction_code = ntoh16(*((int16_t *)(msg_ptr + 10)));
  processed_broadcast_header_.error_code = ntoh16(*((int16_t *)(msg_ptr + 12)));
  processed_broadcast_header_.bcSeqNo = ntoh32(*((int32_t *)(msg_ptr + 14)));
  memcpy((void *)processed_broadcast_header_.timestamp2,(void *)(msg_ptr + 22),8);
  processed_broadcast_header_.message_length = ntoh16(*((int16_t *)(msg_ptr + 38)));

}



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

uint32_t ProcessExchangeResponse(char *nse_msg_buffer, const uint32_t &msg_length) {
   uint32_t length_to_be_processed = msg_length;
   const char *msg_ptr = nse_msg_buffer;
   std::cout<<"ProcessExchangeResponseENTER "<<length_to_be_processed<<" BCASt "<<BROADCAST_PACKET_RESPONSE_LENGTH<<std::endl;
   while (length_to_be_processed > 0) {
	   std::cout<<"ProcessExchangeResponseWHILE "<<length_to_be_processed <<std::endl;
    if (length_to_be_processed < BROADCAST_PACKET_RESPONSE_LENGTH) {
	    std::cout << "INCOMPLETE HEADER, Requires : " << BROADCAST_PACKET_RESPONSE_LENGTH
                                   << " Messsage Length Left : " << length_to_be_processed
                                   << " Initial Length Given : " << msg_length << std::endl;

      memmove((void *)nse_msg_buffer, (void *)msg_ptr, length_to_be_processed);

      return length_to_be_processed;
    }
    std::cout<<"ProcessPakcet END"<<std::endl;
    ProcessPakcet(msg_ptr);
    std::cout<<"ProcessPakcet END PCLENGTH"<<processed_packet_header_.packet_length <<" LENGTHTOPROCESS "<<length_to_be_processed <<std::endl;
    if (length_to_be_processed < ((uint32_t)processed_packet_header_.packet_length)) {
      memmove((void *)nse_msg_buffer, (void *)msg_ptr, length_to_be_processed);
      std::cout<<"ProcessPakcet RETURN "<<length_to_be_processed<<std::endl;
      return length_to_be_processed;
    }

    msg_ptr += BROADCAST_PACKET_RESPONSE_LENGTH;
    std::cout<<"BROADCAST_PACKET_RESPONSE_LENGTH " <<BROADCAST_PACKET_RESPONSE_LENGTH<<std::endl;
    ProcessHeader(msg_ptr);
    std::cout<<"BROADCAST_PACKET_RESPONSE_LENGTH END"<<std::endl;
    msg_ptr += BROADCAST_RESPONSE_MESSAGE_HEADER_LENGTH;
    std::cout<<"BROADCAST_RESPONSE_MESSAGE_HEADER_LENGTH "<<BROADCAST_RESPONSE_MESSAGE_HEADER_LENGTH<<std::endl;
    std::cout<<"TC CODE: "<<processed_broadcast_header_.transaction_code<<std::endl;
    switch (processed_broadcast_header_.transaction_code) {
      	case 7200: {
			  //Market By Order /MBP
			 std::cout<<"Market By order"<<std::endl;		
      	} break;

      	case 7201: {
		     //Mkt Watch
			 std::cout<<"Market Watch"<<std::endl;
      	} break;

      	case 7202: {
			 //ticker
			 std::cout<<"Ticker"<<std::endl;
      	} break;
        case 7208: {
			   //only MBP
			  std::cout<<"MBP"<<std::endl;
      	} break;
      	case 7220: {
			 //TradeExecution Range
			 std::cout<<"Trade Range"<<std::endl;
      	} break;
      	case 6501: {
		std::cout<<"BCAST_JRNL_VCT_MSG"<<std::endl;
	} break;
	case 7206: {
                std::cout<<"BCAST_System Info"<<std::endl;
      	} break;
	case 7305: 
		   { 
			   std::cout << "BCAST_SECURITY_MSTR_CHG (7305)" << std::endl;
    	}break;
	case 7340: {
			   std::cout<<" BCAST_SEC_MSTR_CHNG_PERIODIC"<<std::endl;
		   }break;
	case 7306: {std::cout<<"BCAST_PART_MSTR_CHG (7306)";}break;
      default:{
		      std::cout<<"Default "<<processed_broadcast_header_.transaction_code<<std::endl;
	      }
	      break;
    }
      msg_ptr +=
        (processed_packet_header_.packet_length - BROADCAST_PACKET_RESPONSE_LENGTH - BROADCAST_RESPONSE_MESSAGE_HEADER_LENGTH);
    length_to_be_processed -= (processed_packet_header_.packet_length);
   }

   return 0;
}

int onInputAvailable(char *buffer, int32_t length) {
    int32_t read_length = length;
    memcpy((char *)nse_msg_buffer_ + read_offset_, buffer, length);
    if (read_length > 0) {
      std::cout<<"ProcessExchangeResponse CALLED LENGTH"<<read_length<<std::endl;
      read_offset_ = ProcessExchangeResponse(nse_msg_buffer_, read_offset_ + read_length);
      std::cout<<"ProcessExchangeResponse END OFFSET "<<read_offset_<<std::endl;
      return 0;  // Must return at this point for tcp direct
    } else if (read_length < 0) {
   	std::cout<<"Error IN INput Available"<<std::endl;
	return 0;
    }
    return 0;
}


int main(int argc, char* argv[]) {
  if (argc < 4) {
    printf("USAGE: %s <ip> <port> <interface>\nSize of TimeVal: %ld\t Int: %ld\n", argv[0], sizeof(timeval),
           sizeof(uint32_t));
    exit(0);
  }

  HFSAT::MulticastReceiverSocket* sock = new HFSAT::MulticastReceiverSocket(argv[1], atoi(argv[2]), argv[3]);
  sock->setBufferSize(N);
  // sock -> SetNonBlocking();

  char fname[200];
  sprintf(fname, "/spare/local/MDSlogs/RawData/Bcast/%s_%s_%s_%s.raw", argv[1], argv[2], argv[3], currentDateTime().c_str());
  std::cerr << "File name: " << fname << std::endl;
  HFSAT::BulkFileWriter bfw(fname, N);
  if (!bfw.is_open()) {
    std::cerr << "Cannot open file: " << argv[4] << std::endl;
  }
  char* msg_buf = new char[N];
  // char msg_len_string[10];
  uint32_t msg_len = 1;
  timeval time_;

  while (msg_len > 0) {
    msg_len = sock->ReadN(MAX_BROADCAST_DATA_BUFFER, (void*)msg_buf);
    if ( msg_len == -1 ){
	    std::cout<<"Error occured"<<std::endl;
	    continue;
    }
    std::cout<<"onInputAvailable CALLED"<<std::endl;
    onInputAvailable(msg_buf, msg_len);
    std::cout<<"onInputAvailable END"<<std::endl;

    gettimeofday(&time_, NULL);
    bfw.Write(&msg_len, sizeof(int));
    bfw.Write(&time_, sizeof(time_));
    bfw.Write((void*)msg_buf, msg_len);
    bfw.DumpCurrentBuffer();
  }
  bfw.DumpCurrentBuffer();
  bfw.Close();
  return 0;
}
