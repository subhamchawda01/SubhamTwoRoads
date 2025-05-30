#ifndef _ORS_MESSAGE_READER_HPP_
#define _ORS_MESSAGE_READER_HPP_

#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/ORSMessages/ors_message_listener.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

namespace HFSAT {

class ORSMessageReader {
 public:
  ORSMessageReader(ORSMessagesListener* listener, const std::string filename, int security_id);
  void PlayAll();

 private:
  void DoPlayORSReply(const GenericORSReplyStruct& ors_reply);

  HFSAT::BulkFileReader bulk_file_reader_;
  ORSMessagesListener* listener_;
  int security_id_;
  std::string filename_;
};
}

#endif
