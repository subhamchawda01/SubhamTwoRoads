/**
 \file EobiD/eobi_decoder.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 162, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551

*/

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include <time.h>
#include <unordered_map>

#define AFLASH_MESSAGE_SIZE 16
#define MAX_UDP_MSG_LEN 65536

void DecodeAndFillStruct(uint8_t* msg_buf, AFLASH_MDS::AFlashCommonStruct* this_packet_) {
  // TODO: Parse message, and fill this_packet_ struct here
  this_packet_->category_id_ = 0;

  // 2 bytes: msg_length
  uint16_t msg_length_ = ntoh16(*(reinterpret_cast<uint16_t*>(&msg_buf[0])));

  /* Currently NOT checking if the checksum matches */
  // uint32_t crc32 = ntohl(*(reinterpret_cast<uint32_t *> (&msg_buf[msg_length_ - AF_MSGSPECS::CRC_SIZE])) );

  // next 4 bytes: unqiue_id
  int32_t txmit_id = (int32_t)ntoh32(*(reinterpret_cast<int32_t*>(&msg_buf[2])));  // signed long int

  this_packet_->uid_ = txmit_id;
  this_packet_->type_ = msg_buf[6];     // msg_type
  this_packet_->version_ = msg_buf[7];  // msg_version
  this_packet_->category_id_ =
      ntoh16(*(reinterpret_cast<uint16_t*>(&msg_buf[8])));  // category corresponding to the event

  AF_MSGSPECS::AF_MsgParser& af_msgparser_ = AF_MSGSPECS::AF_MsgParser::GetUniqueInstance(NULL);
  AF_MSGSPECS::Category* catg_ = af_msgparser_.getCategoryforId(this_packet_->category_id_);
  if (catg_ == NULL) {
    this_packet_->category_id_ = 0;
    return;
  }

  AF_MSGSPECS::Message* msg_ = af_msgparser_.getMsgFromCatg(catg_, this_packet_->type_);
  if (msg_ == NULL) {
    this_packet_->category_id_ = 0;
    return;
  }

  int field_buffer_offset_ = AF_MSGSPECS::HEADER_SIZE;

  /*  Loads up the the fields of the msg upto total of AFLASH_MDS::MAX_FIELDS
   *  although the msg can have many fields, it will retain the first <AFLASH_MDS::MAX_FIELDS> fields */
  size_t fno = 0;
  for (; fno < AFLASH_MDS::MAX_FIELDS && field_buffer_offset_ < (msg_length_ - AF_MSGSPECS::CRC_SIZE); fno++) {
    short field_type_ = msg_buf[field_buffer_offset_];
    short field_id_ = msg_buf[field_buffer_offset_ + 1];

    AF_MSGSPECS::Field* t_field_ = af_msgparser_.getFieldFromMsg(msg_, field_id_, field_type_);

    if (t_field_ == NULL) {
      this_packet_->category_id_ = 0;
      return;
    }

    this_packet_->fields[fno].field_id_ = field_id_;
    int value_offset = field_buffer_offset_ + 2;

    switch (t_field_->field_type_) {
      case AF_MSGSPECS::kShort_value_enumeration:
      case AF_MSGSPECS::kShort: {
        this_packet_->fields[fno].data_.vInt =
            (long int)(int16_t)ntoh16(*(reinterpret_cast<uint16_t*>(&msg_buf[value_offset])));
        field_buffer_offset_ += AF_MSGSPECS::SHORT_INDICATOR_SIZE;
      } break;

      case AF_MSGSPECS::kInt: {
        this_packet_->fields[fno].data_.vInt =
            (long int)(int32_t)ntoh32(*(reinterpret_cast<uint32_t*>(&msg_buf[value_offset])));
        field_buffer_offset_ += AF_MSGSPECS::INT_INDICATOR_SIZE;
      } break;

      case AF_MSGSPECS::kLong: {
        this_packet_->fields[fno].data_.vInt = (long int)ntoh64(*(reinterpret_cast<uint64_t*>(&msg_buf[value_offset])));
        field_buffer_offset_ += AF_MSGSPECS::LONG_INDICATOR_SIZE;
      } break;

      case AF_MSGSPECS::kFloat: {
        this_packet_->fields[fno].data_.vFloat = (double)af_msgparser_.swapFloat((uint8_t*)&msg_buf[value_offset]);
        field_buffer_offset_ += AF_MSGSPECS::FLOAT_INDICATOR_SIZE;
      } break;

      case AF_MSGSPECS::kDouble: {
        this_packet_->fields[fno].data_.vFloat = af_msgparser_.swapDouble((uint8_t*)&msg_buf[value_offset]);
        field_buffer_offset_ += AF_MSGSPECS::DOUBLE_INDICATOR_SIZE;
      } break;

      case AF_MSGSPECS::kBoolean: {
        this_packet_->fields[fno].data_.vBool = (bool)msg_buf[value_offset];
        field_buffer_offset_ += AF_MSGSPECS::BOOL_INDICATOR_SIZE;
      } break;

      default:
        this_packet_->category_id_ = 0;
        return;
    }
  }
  this_packet_->nfields_ = fno;
}

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "Usage: <exec> <input-file> <dataloggingdir> <date>" << std::endl;
    exit(1);
  }

  char* raw_file_ = argv[1];
  std::string data_dir_ = argv[2];
  std::string date_ = argv[3];
  std::string mdsname_ = data_dir_ + "/AFL_" + date_;
  std::string logname_ = data_dir_ + "/log_AFL_" + date_;

  HFSAT::BulkFileReader reader;
  reader.open(raw_file_);

  if (!reader.is_open()) {
    std::cerr << " Can't open file, exiting \n";
    exit(1);
  }

  HFSAT::BulkFileWriter bulk_file_writer_;
  bulk_file_writer_.Open(mdsname_);

  uint8_t* msg_buf = (uint8_t*)calloc(MAX_UDP_MSG_LEN, 1);

  AFLASH_MDS::AFlashCommonStruct* this_packet_ = new AFLASH_MDS::AFlashCommonStruct();
  this_packet_->uid_ = 0;
  std::unordered_map<int, bool> uid_seen_;

  while (true) {
    uint32_t msg_len;
    int read_size;

    read_size = reader.read(&(msg_len), sizeof(uint32_t));
    if (read_size < (int)sizeof(uint32_t)) break;

    if (msg_len <= AFLASH_MESSAGE_SIZE) {
      continue;
    }

    read_size = reader.read(&(this_packet_->time_), sizeof(this_packet_->time_));
    if (read_size < (int)sizeof(this_packet_->time_)) break;

    read_size = reader.read(msg_buf, msg_len);

    int uid = ntoh32(*((int32_t*)(msg_buf + 2)));
    if (uid_seen_.find(uid) != uid_seen_.end()) {
      continue;
    }
    uid_seen_[uid] = true;

    DecodeAndFillStruct(msg_buf, this_packet_);

    bulk_file_writer_.Write(this_packet_, sizeof(AFLASH_MDS::AFlashCommonStruct));
    bulk_file_writer_.CheckToFlushBuffer();
  }
  bulk_file_writer_.Close();

  reader.close();
  free(msg_buf);

  return 0;
}
