// =====================================================================================
//
//       Filename:  aws_queues_metadata.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  03/01/2014 05:34:41 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include "basetrade/AWS/aws_defines.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

namespace HFSAT {
namespace AWS {

class AWSQueuesMetadataHandler {
 private:
  std::map<std::string, HFSAT::AWS::AWSQueue*> aws_queues_metadata_;

 public:
  void LoadAWSQueuesMetaData() {
    HFSAT::BulkFileReader* bulk_file_reader = new HFSAT::BulkFileReader();
    bulk_file_reader->open(AWS_QUEUES_METADATA_BIN_FILE);

    if (!bulk_file_reader->is_open()) {
      HFSAT::AWS::ReportAWSError(HFSAT::AWS::FILE_ISSUE,
                                 std::string("COULD NOT LOAD QUEUES METADATA FILE : ") + AWS_QUEUES_METADATA_BIN_FILE);
      aws_queues_metadata_.clear();
      return;
    }

    while (true) {
      HFSAT::AWS::AWSQueue* aws_queue = new HFSAT::AWS::AWSQueue();
      memset((void*)(aws_queue), 0, sizeof(HFSAT::AWS::AWSQueue));

      size_t read_length = bulk_file_reader->read(aws_queue, sizeof(HFSAT::AWS::AWSQueue));
      if (read_length < sizeof(HFSAT::AWS::AWSQueue)) break;

      aws_queues_metadata_[aws_queue->uniq_queue_name_] = aws_queue;
    }

    if (bulk_file_reader) {
      bulk_file_reader->close();
      delete bulk_file_reader;
      bulk_file_reader = NULL;
    }
  }

  std::map<std::string, HFSAT::AWS::AWSQueue*>& GetQueuesMetaData() { return aws_queues_metadata_; }

  void UpdateSystem() {
    HFSAT::BulkFileWriter* bulk_file_writer = NULL;
    bulk_file_writer->Open(AWS_QUEUES_METADATA_BIN_FILE, std::ios::out);

    if (!bulk_file_writer->is_open()) {
      ReportAWSError(FILE_ISSUE, std::string("FAILED TO OPEN QUEUES METADATA FILE FOR WRITING : ") +
                                     std::string(AWS_QUEUES_METADATA_BIN_FILE));
      exit(1);
    }

    std::map<std::string, HFSAT::AWS::AWSQueue*>::iterator aws_queues_itr;

    for (aws_queues_itr = aws_queues_metadata_.begin(); aws_queues_itr != aws_queues_metadata_.end();
         aws_queues_itr++) {
      bulk_file_writer->Write((void*)(aws_queues_itr->second), sizeof(HFSAT::AWS::AWSQueue));
      bulk_file_writer->CheckToFlushBuffer();
    }

    if (bulk_file_writer) {
      bulk_file_writer->Close();
      delete bulk_file_writer;
      bulk_file_writer = NULL;
    }
  }
};
}
}
