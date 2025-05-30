#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/af_msg_parser.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include <time.h>

namespace AF_MSGSPECS {
void msgParseForCat(char *msgdata, uint16_t category_id_to_seek_, AF_MSGSPECS::AF_MsgParser &af_msgparser_,
                    std::map<uint32_t, bool> &txid_map_) {
  std::stringstream parsed_;
  std::string delim_ = " | ";

  uint16_t msg_length_ = ntoh16(*(reinterpret_cast<uint16_t *>(&msgdata[0])));
  int32_t txmit_id = (int32_t)ntoh32(*(reinterpret_cast<uint32_t *>(&msgdata[2])));  // signed long int
  short type_ = msgdata[6];

  if (txid_map_.find(txmit_id) != txid_map_.end()) {
    return;
  }

  uint16_t category_id_ = ntoh16(*(reinterpret_cast<uint16_t *>(&msgdata[8])));
  if (category_id_ != category_id_to_seek_) {
    return;
  }

  Category *catg_ = af_msgparser_.getCategoryforId(category_id_);
  if (catg_ == NULL) {
    return;
  }

  Message *msg_ = af_msgparser_.getMsgFromCatg(catg_, type_);
  if (msg_ == NULL) {
    return;
  }

  txid_map_[txmit_id] = true;

  parsed_ << category_id_ << delim_ << catg_->category_name_ << delim_ << catg_->description_ << delim_
          << catg_->country_ << delim_ << msg_->msg_type_ << "\n";

  int field_buffer_offset_ = HEADER_SIZE;
  do {
    short field_type_ = msgdata[field_buffer_offset_];
    short field_id_ = msgdata[field_buffer_offset_ + 1];

    Field *t_field_ = af_msgparser_.getFieldFromMsg(msg_, field_id_, field_type_);
    if (t_field_ == NULL) {
      break;
    }

    Datum *t_datum_ = af_msgparser_.getDatumFromField(catg_, t_field_->datum_id_);
    if (t_datum_ == NULL) {
      break;
    }

    std::string field_val_ = af_msgparser_.getFieldTypeValue(t_field_, msgdata, field_buffer_offset_, msg_length_);

    parsed_ << "\t" << t_field_->datum_id_ << delim_ << t_datum_->desc_ << delim_ << t_datum_->type_ << delim_
            << field_val_ << "\n";
  } while (field_buffer_offset_ < (msg_length_ - CRC_SIZE));

  std::cout << parsed_.str() << "\n";
}
}

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "Usage: <exec> <input-file> <category_id>" << std::endl;
    exit(-1);
  }

  AF_MSGSPECS::AF_MsgParser &af_msgparser_ = AF_MSGSPECS::AF_MsgParser::GetUniqueInstance(NULL);

  HFSAT::BulkFileReader *reader = new HFSAT::BulkFileReader();
  reader->open(argv[1]);

  if (!reader->is_open()) {
    std::cerr << " Can't open file, exiting \n";
    exit(-1);
  }

  uint16_t category_id_ = atoi(argv[2]);
  std::map<uint32_t, bool> txid_map_;

  while (true) {
    uint32_t msg_len;
    timeval time_;
    int read_size;

    read_size = reader->read(&(msg_len), sizeof(uint32_t));
    if (read_size < (int)sizeof(uint32_t)) break;

    read_size = reader->read(&(time_), sizeof(time_));
    if (read_size < (int)sizeof(time_)) break;

    char buffer[65536];

    read_size = reader->read(buffer, msg_len);
    if (read_size < (int)msg_len) {
      std::cerr << " read_size of Message is less than the msg_length";
      break;
    }

    AF_MSGSPECS::msgParseForCat(buffer, category_id_, af_msgparser_, txid_map_);
  }

  if (reader) {
    reader->close();
  }

  return 0;
}
