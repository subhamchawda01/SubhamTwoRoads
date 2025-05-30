#include "baseinfra/FPGA/fpga_reader.hpp"

namespace HFSAT {
double powersoften[] =
    {0.0000000001, 0.000000001, 0.00000001, 0.0000001,  0.000001,   0.00001, 0.0001, 0.001,
     0.01,         0.1,         1,          10,         100,        1000,    10000,  100000,
     1000000,      10000000,    100000000,  1000000000, 10000000000};  // index 0 means pow(10, -10), i.e, offset of 10

FPGAReader::FPGAReader(const char* subject_to_subscribe_, const char* nsmsg_interface_) {
  LoadCmeFPGARef();
  /* Get a nsmsg context */
  ctx = nsmsg_init(NULL);

  /* Subscribe to a subject id */
  nsmsg_subscribe(ctx, subject_to_subscribe_, NULL);

  /* Open a nsmsg socket on nsmsg fake interface fake0 */
  sock = nsmsg_socket(ctx, nsmsg_interface_, NSMSG_NONBLOCK);
  current_index_ = 0;
  data = NULL;
  data_length = 0;
  price_exponent = -4;
}

char* FPGAReader::getSecurityName(int sec_id) {
  if (idmap_.find(sec_id) == idmap_.end()) {
    // Security not found..not sure why.
    return NULL;
  }
  return idmap_[sec_id];
}

void FPGAReader::LoadCmeFPGARef() {
  std::ifstream cme_reference_info_file_(DEF_CME_REFLOC_, std::ofstream::in);

  if (!cme_reference_info_file_.is_open()) {
    fprintf(stderr, "Cannot cme ref file: %s\n", DEF_CME_REFLOC_);
    exit(-1);
  }

  // Intermediate map
  std::tr1::unordered_map<int64_t, char*> cme_idmap_;

  char cme_ref_line_[1024];

  while (cme_reference_info_file_.good()) {
    memset(cme_ref_line_, 0, sizeof(cme_ref_line_));
    cme_reference_info_file_.getline(cme_ref_line_, 1024);

    if (strlen(cme_ref_line_) == 0 || cme_ref_line_[0] == '#')  /// comments etc
    {
      continue;
    }

    int64_t num_id = atoll(strtok(cme_ref_line_, "\n\t "));

    char* sec_name_ = (char*)calloc(12, sizeof(char));
    strncpy(sec_name_, strtok(NULL, "\n\t "), 12);
    cme_idmap_[num_id] = sec_name_;
  }

  // Now load FPGA ref

  std::ifstream fpga_instrument_ref_file_(DEF_CME_FPGA_REFLOC_, std::ofstream::in);

  if (!fpga_instrument_ref_file_.is_open()) {
    fprintf(stderr, "Cannot cme ref file: %s\n", DEF_CME_FPGA_REFLOC_);
    exit(-1);
  }

  char fpga_ref_line_[1024];

  while (fpga_instrument_ref_file_.good()) {
    memset(fpga_ref_line_, 0, sizeof(fpga_ref_line_));
    fpga_instrument_ref_file_.getline(fpga_ref_line_, 1024);

    if (strlen(fpga_ref_line_) == 0 || fpga_ref_line_[0] == '#')  /// comments etc
    {
      continue;
    }

    strtok(fpga_ref_line_, "\n\t ");  // ignore mkt_seg_id_
    int64_t num_id = atoll(strtok(NULL, "\n\t "));
    // strtok ( NULL, "\n\t " ); // ignore mkt_seg_id_
    // strtok ( NULL, "\n\t " ); // ignore
    // strtok ( NULL, "\n\t " ); // tick info - ingore here
    int32_t fpga_sec_id_ = atoi(strtok(NULL, "\n\t "));

    if (cme_idmap_.find(num_id) != cme_idmap_.end()) {
      idmap_[fpga_sec_id_] = cme_idmap_[num_id];
    } else {
      std::ostringstream ss;
      ss << "SEC_" << fpga_sec_id_;
      idmap_[fpga_sec_id_] = strdup(ss.str().c_str());
    }
  }
}

void FPGAReader::handle_error_if_any(long int rc, char* situation) {
  if (rc < 0) {
    // const char* error = nsmsg_strerror(errno);
  }
}

void FPGAReader::cleanUp() {
  nsmsg_close(sock);
  nsmsg_unsubscribe(ctx, "*");
  nsmsg_term(ctx);
}

void FPGAReader::releaseMessage() {
  nsmsg_msg_unref(&msg);
  current_index_ = 0;
  data = NULL;
  data_length = 0;
}

// Returns unsigned long value corresponding to first sz bytes starting from ptr
uint64_t FPGAReader::getBytesValue(char* ptr, int sz) {
  uint64_t fact, res = 0;
  int index = 0;
  fact = 0;
  while (index < sz) {
    res += ((uint64_t)((uint8_t)ptr[index])) << fact;
    fact += 8;
    index++;
  }
  return res;
}

// Returns signed long value corresponding to first sz bytes starting from ptr
int64_t FPGAReader::getSignedBytesValue(char* ptr, int sz) {
  int64_t fact, res = 0;
  int index = 0;
  fact = 0;
  while (index < sz) {
    res += ((uint8_t)ptr[index]) << fact;
    fact += 8;
    index++;
  }
  uint64_t sigmask = uint64_t(1) << ((8 * sz) - 1);
  uint64_t negmask = ~uint64_t(0) ^ ((uint64_t(1) << (8 * sz)) - 1);
  uint64_t posmask = (uint64_t(1) << uint64_t(8 * sz)) - 1;
  if (res & sigmask) {
    res = static_cast<int64_t>(res | negmask);
  } else {
    res = static_cast<int64_t>(res & posmask);
  }
  return res;
}

// Read new message from FPGA socket
int FPGAReader::readMessage() {
  data_length = nsmsg_recv(sock, &msg, 0);
  if (data_length > 0) {
    current_index_ = 0;
    data = (char*)nsmsg_msg_data(&msg);
  }
  return data_length;
}

// Returns security ID corresponding to the present read message
int FPGAReader::getSecurityId() {
  if (data != NULL)
    return ((int)(nsmsg_msg_subject(&msg))) & 262143;  // Taking 1st 18 bits ( & with (2^18 - 1) )
  else
    return -1;
}

// Returns pointer to the present message's data payload
char* FPGAReader::getMessageBody() {
  if (data != NULL)
    return data;
  else
    return NULL;
}

// Returns present message type
int FPGAReader::getMessageType() { return data[0]; }

// Checks if we have completely read the present message (read is in past tense)
bool FPGAReader::parsedMessageFully() { return current_index_ == data_length; }

bool FPGAReader::parseTimeMessage() {
  if (parsedMessageFully() || (data == NULL) || (getMessageType() != FPGA_TIME_MESSAGE)) return false;
  current_index_ += 1;  // skipping msg_type
  // current_time_.tv_sec = *((int32_t*)(data + current_index_));
  current_index_ += 4;
  return true;
}

// returns true on success, otherwise false
bool FPGAReader::parseCMEBookLevelUpdate(TradeType_t& _buysell_, int& level_, double& price_, int& size_,
                                         int& num_ords_, bool& intermediate_) {
  if (parsedMessageFully() || (data == NULL) || (data[0] != FPGA_BOOK_LEVEL_UPDATE)) {
    return false;
  }

  int flags;
  if (current_index_ == 0) {  // Still haven't read the 1st level update

    current_index_ += 1;  // skipping msg_type

    // current_time_.tv_usec = (*((int32_t*) (data + current_index_) ))/1000;
    current_index_ += 4;  // skipping timestamp

    flags = (int)data[current_index_];  // 8 bit flags
    current_index_ += 1;

    if (FIRST_BIT(flags)) {  // Market data seqnum present
      current_index_ += 4;
    }

    if (SECOND_BIT(flags)) {  // Instrument seqnum present
      current_index_ += 4;
    }

    price_exponent = -4;
  }
  bool implied = false;

  flags = *((uint16_t*)(data + current_index_));

  if (SECOND_BIT(flags)) {
    implied = true;
  }
  current_index_ += 2;

  // 0-1 (Ask-Bid) -> 1-0
  _buysell_ = TradeType_t(1 - FIRST_BIT(flags));

  // implied=(SECOND_BIT(flags));
  // level_=getBitsValue(flags, 2, 5) - 1;//level (starting from 0)
  level_ = ((flags & 60) >> 2) - 1;

  // int ps = getBitsValue(flags, 6, 8);// price size
  int ps = (flags & 448) >> 6;  // price size
  // int qs = getBitsValue(flags, 9, 11);// quantity size
  int qs = (flags & 3584) >> 9;  // quantity size
  // int os = getBitsValue(flags, 12, 13);// order size
  int os = (flags & 12288) >> 12;  // order size
  // int ref = getBitsValue(flags, 14, 14);// is price a reference price
  int ref = (flags & 16384) >> 14;  // is price a reference price
  // int pes = getBitsValue(flags, 15, 15);// price exponent size
  int pes = (flags & 32768) >> 15;  // price exponent size

  long long int px = 0;

  if (pes == 1) {
    price_exponent = (int)data[current_index_];
    current_index_ += 1;
  }

  if (ps != 0) {
    px = getSignedBytesValue(data + current_index_, ps);
    current_index_ += ps;
  }

  if (qs != 0) {
    size_ = getBytesValue(data + current_index_, qs);
    current_index_ += qs;
  } else
    size_ = 0;

  if (os != 0) {
    num_ords_ = getBytesValue(data + current_index_, os);
    current_index_ += os;
  } else
    num_ords_ = 0;

  if (price_exponent < -POWER_OFFSET || price_exponent > POWER_OFFSET) {
    // Not possible (some parsing error)
    std::cout << "Got invalid price exponent: " << price_exponent << "\n";
    exit(1);
  }
  if (ref == 1) {
    ref_price = px;
    price_ = (double)px * powersoften[price_exponent + POWER_OFFSET];
  } else {
    if (px == 0) {
      price_ = 0.0;
    } else {
      price_ = ((double)(ref_price + px) * powersoften[price_exponent + POWER_OFFSET]);
    }
  }

  intermediate_ = true;
  if (parsedMessageFully()) {  // last level update (marking it as non-intermediate)
    intermediate_ = false;
  }
  if (implied) {
    return false;
  }
  return true;
}

// returns true on success, otherwise false
bool FPGAReader::parseCMEExecutionSummary(double& price_, int& size_, TradeType_t& _buysell_) {
  if ((current_index_ != 0) || (data == NULL) || (data[0] != FPGA_EXECUTION_SUMMARY)) {
    return false;  // No new messages received
  }

  current_index_ += 1;  // skipping msg_type
  // current_time_.tv_usec = (*((int32_t*) (data + current_index_) ))/1000;
  current_index_ += 4;
  int flags = *((uint16_t*)(data + current_index_));
  current_index_ += 2;

  // int pe = getBitsValue(flags, 0, 3);//price exponent
  int pe = (flags & 15);  // price exponent

  // Because of the twos complement form
  if (pe > 0) {
    pe = -(15 - pe + 1);  // negate all bits
  }
  if (pe < -POWER_OFFSET || pe > POWER_OFFSET) {
    // Not possible (some parsing error)
    std::cout << "Got invalid pe: " << pe << "\n";
    exit(1);
  }
  // int ps = getBitsValue(flags, 4, 6);//price size
  int ps = (flags & 112) >> 4;  // price size

  // if ( getBitsValue(flags, 9, 9) == 1 )//Bid side aggress
  if (flags & 512)  // Bid side aggress
    _buysell_ = kTradeTypeSell;
  // else if ( getBitsValue(flags, 10, 10) == 1 )//Ask side aggress
  else if (flags & 1024)  // Ask side aggress
    _buysell_ = kTradeTypeBuy;
  else
    _buysell_ = kTradeTypeNoInfo;

  bool implied = false;
  if (flags & 2048) implied = true;

  long long int px = getSignedBytesValue(data + current_index_, ps);
  current_index_ += ps;

  size_ = *((int32_t*)(data + current_index_));
  current_index_ += 4;

  price_ = (double)px * powersoften[pe + POWER_OFFSET];
  if (implied) {
    return false;
  }
  return true;
}

timeval FPGAReader::getCurrentTime() { return current_time_; }
}
