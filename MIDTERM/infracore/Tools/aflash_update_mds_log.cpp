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

#define AFLASH_MESSAGE_SIZE 16
#define MAX_UDP_MSG_LEN 65536

void UpdateAflashStruct(AFLASH_MDS::AFlashCommonStruct* this_packet_) {
  AF_MSGSPECS::AF_MsgParser& af_msgparser_ = AF_MSGSPECS::AF_MsgParser::GetUniqueInstance();
  AF_MSGSPECS::Category* catg_ = af_msgparser_.getCategoryforId(this_packet_->category_id_);
  if (!catg_) {
    return;
  }
  AF_MSGSPECS::Message* msg_ = af_msgparser_.getMsgFromCatg(catg_, (short int)this_packet_->type_);
  if (!msg_) {
    return;
  }

  for (unsigned fno = 0; fno < this_packet_->nfields_; fno++) {
    AF_MSGSPECS::Field* t_field_ = af_msgparser_.getFieldFromMsg(msg_, this_packet_->fields[fno].field_id_);
    if (!t_field_) {
      continue;
    }

    if (t_field_->field_type_ == AF_MSGSPECS::kShort) {
      this_packet_->fields[fno].data_.vInt = (long int)(int16_t)((uint16_t)this_packet_->fields[fno].data_.vInt);
    } else if (t_field_->field_type_ == AF_MSGSPECS::kInt) {
      this_packet_->fields[fno].data_.vInt = (long int)(int32_t)((uint32_t)this_packet_->fields[fno].data_.vInt);
    }
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: <exec> <input-mdslog> <output-mdslog>" << std::endl;
    exit(1);
  }

  char* in_mdslog_ = argv[1];
  std::string out_mdslog_ = argv[2];

  HFSAT::BulkFileReader reader;
  reader.open(in_mdslog_);

  if (!reader.is_open()) {
    std::cerr << " Can't open file, exiting \n";
    exit(1);
  }

  HFSAT::BulkFileWriter bulk_file_writer_;
  bulk_file_writer_.Open(out_mdslog_);

  AFLASH_MDS::AFlashCommonStruct* this_packet_ = new AFLASH_MDS::AFlashCommonStruct();
  size_t af_size_ = sizeof(AFLASH_MDS::AFlashCommonStruct);

  while (true) {
    size_t msg_len_ = reader.read(this_packet_, af_size_);
    if (msg_len_ < af_size_) break;

    UpdateAflashStruct(this_packet_);

    bulk_file_writer_.Write(this_packet_, sizeof(AFLASH_MDS::AFlashCommonStruct));
    bulk_file_writer_.CheckToFlushBuffer();
  }
  bulk_file_writer_.Close();

  reader.close();

  return 0;
}
