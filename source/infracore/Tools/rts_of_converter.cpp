#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/rts_mds_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

#include <cstdlib>
#include <string>
#include <vector>

// Forward Declaration
namespace HFSAT {
namespace RTS_OF_CONVERTER {
void ConvertOrderfeed(std::string trading_date, std::string shortcode, std::string infile_order_feed,
                      std::string outfile_orderfeed_feed);
}
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << " Usage < exec > < orderfeed-old file > < orderfeed-new file > <date>\n";
    exit(-1);
  }
  // Shortcode isn't required now. But would be required in future. So Adding a dummy for it.
  char dummy[3];
  dummy[2] = '\0';
  HFSAT::RTS_OF_CONVERTER::ConvertOrderfeed("20170903", dummy, argv[1], argv[2]);

  return 0;
}

namespace HFSAT {
namespace RTS_OF_CONVERTER {

class RTSOFConverter {
 public:
  RTSOFConverter(std::string new_orderfeed_file_name) : order_id_to_details_(2), is_cached_(false) {
    bulk_file_writer_.Open(new_orderfeed_file_name);

    if (!bulk_file_writer_.is_open()) {
      std::cerr << " Could not open PriceFeed Output File : " << new_orderfeed_file_name << "\n";
      exit(-1);
    }
  }

  void Process(RTS_MDS::RTSOFCommonStruct event) {
    RTS_MDS::RTSOFCommonStructv2 new_event = Convert(event);

    if (!(new_event.side - '0' >= 0 && new_event.side - '0' <= 1)) {
      std::cout << "Invalid Side for SeqNum: " << new_event.seq_num << " OrderId: " << new_event.order_id << std::endl;
    }
    switch (event.msg_type) {
      case RTS_MDS::RTSOFMsgType::kRTSAdd: {
        OrderDetails details;
        details.order_id = new_event.order_id;
        details.price = new_event.price;
        details.side = new_event.side;
        details.size = new_event.size;
        if (new_event.side - '0' >= 0 && new_event.side - '0' <= 1) {
          order_id_to_details_[new_event.side - '0'][new_event.order_id] = details;
        }

        break;
      }
      case RTS_MDS::RTSOFMsgType::kRTSDelete: {
        if (new_event.side - '0' >= 0 && new_event.side - '0' <= 1) {
          if (order_id_to_details_[new_event.side - '0'].find(new_event.order_id) !=
              order_id_to_details_[new_event.side - '0'].end()) {
            order_id_to_details_[new_event.side - '0'].erase(new_event.order_id);
          }
        }
        break;
      }
      case RTS_MDS::RTSOFMsgType::kRTSExec: {
        if (new_event.is_full_exec) {
          if (new_event.side - '0' >= 0 && new_event.side - '0' <= 1) {
            if (order_id_to_details_[new_event.side - '0'].find(new_event.order_id) !=
                order_id_to_details_[new_event.side - '0'].end()) {
              order_id_to_details_[new_event.side - '0'].erase(new_event.order_id);
            }
          }
        } else {
          if (new_event.side - '0' >= 0 && new_event.side - '0' <= 1) {
            if (order_id_to_details_[new_event.side - '0'].find(new_event.order_id) !=
                order_id_to_details_[new_event.side - '0'].end()) {
              order_id_to_details_[new_event.side - '0'][new_event.order_id].size -= new_event.size;
              if (order_id_to_details_[new_event.side - '0'][new_event.order_id].size <= 0) {
                order_id_to_details_[new_event.side - '0'].erase(new_event.order_id);
              }
            }
          }
        }
        break;
      }
      case RTS_MDS::RTSOFMsgType::kRTSDeleteAll: {
        order_id_to_details_[0].clear();
        order_id_to_details_[1].clear();
        break;
      }
      default: { std::cout << "msg_type:" << static_cast<int>(event.msg_type) << " not found." << std::endl; }
    }
    DumpToFile(new_event);
  }
  void End() { FlushToFile(); }

 private:
  void DumpToFile(RTS_MDS::RTSOFCommonStructv2& event) {
    if (is_cached_) {
      cached_event_.is_intermediate = false;  //((GetTimeDiff(cached_event_.time_, event.time_) < 10) ? true : false);
      bulk_file_writer_.Write(&cached_event_, sizeof(RTS_MDS::RTSOFCommonStructv2));
      bulk_file_writer_.CheckToFlushBuffer();
    }
    cached_event_ = event;
    is_cached_ = true;
  }

  void FlushToFile() {
    if (is_cached_) {
      cached_event_.is_intermediate = false;
      bulk_file_writer_.Write(&cached_event_, sizeof(RTS_MDS::RTSOFCommonStructv2));
      bulk_file_writer_.DumpCurrentBuffer();
    }
    is_cached_ = false;
  }

  RTS_MDS::RTSOFCommonStructv2 Convert(RTS_MDS::RTSOFCommonStruct event) {
    RTS_MDS::RTSOFCommonStructv2 new_event;

    switch (event.msg_type) {
      case RTS_MDS::RTSOFMsgType::kRTSAdd: {
        new_event.time_ = event.time_;
        new_event.msg_type = event.msg_type;
        new_event.seq_num = event.seq_num;
        new_event.md_flags = event.md_flags;
        memcpy(new_event.contract, event.contract, RTS_MDS_CONTRACT_TEXT_SIZE);
        new_event.packet_num = 0;
        new_event.order_id = event.data.add.order_id;
        new_event.side = event.data.add.side;
        new_event.size = event.data.add.size;
        new_event.price = event.data.add.price;
        new_event.is_full_exec = false;
        new_event.is_intermediate = false;

        break;
      }
      case RTS_MDS::RTSOFMsgType::kRTSDelete: {
        new_event.time_ = event.time_;
        new_event.msg_type = event.msg_type;
        new_event.seq_num = event.seq_num;
        new_event.md_flags = event.md_flags;
        memcpy(new_event.contract, event.contract, RTS_MDS_CONTRACT_TEXT_SIZE);
        new_event.packet_num = 0;
        new_event.order_id = event.data.del.order_id;
        new_event.side = event.data.del.side;
        new_event.size = 0;
        new_event.price = 0;

        if (new_event.side - '0' >= 0 && new_event.side - '0' <= 1) {
          if (order_id_to_details_[new_event.side - '0'].find(new_event.order_id) !=
              order_id_to_details_[new_event.side - '0'].end()) {
            new_event.size = order_id_to_details_[new_event.side - '0'][new_event.order_id].size;
            new_event.price = order_id_to_details_[new_event.side - '0'][new_event.order_id].price;
          } else {
            std::cout << "Delete Order Details not present for order id : " << new_event.order_id << std::endl;
          }
        }

        new_event.is_full_exec = false;
        new_event.is_intermediate = false;

        break;
      }
      case RTS_MDS::RTSOFMsgType::kRTSExec: {
        new_event.time_ = event.time_;
        new_event.msg_type = event.msg_type;
        new_event.seq_num = event.seq_num;
        new_event.md_flags = event.md_flags;
        memcpy(new_event.contract, event.contract, RTS_MDS_CONTRACT_TEXT_SIZE);
        new_event.packet_num = 0;
        new_event.order_id = event.data.exec.order_id;
        new_event.side = event.data.exec.side;
        new_event.size = event.data.exec.size_exec;
        new_event.price = event.data.exec.price;
        new_event.is_full_exec = false;
        new_event.is_intermediate = false;

        if (new_event.side - '0' >= 0 && new_event.side - '0' <= 1) {
          if (order_id_to_details_[new_event.side - '0'].find(new_event.order_id) !=
              order_id_to_details_[new_event.side - '0'].end()) {
            if (order_id_to_details_[new_event.side - '0'][new_event.order_id].size <= new_event.size) {
              new_event.is_full_exec = true;
            }
          }
        }

        break;
      }
      case RTS_MDS::RTSOFMsgType::kRTSDeleteAll: {
        new_event.time_ = event.time_;
        new_event.msg_type = event.msg_type;
        new_event.seq_num = event.seq_num;
        new_event.md_flags = event.md_flags;
        memcpy(new_event.contract, event.contract, RTS_MDS_CONTRACT_TEXT_SIZE);
        new_event.packet_num = 0;
        new_event.order_id = 0;
        new_event.side = 0;
        new_event.size = 0;
        new_event.price = 0;
        new_event.is_full_exec = false;
        new_event.is_intermediate = false;
        break;
      }
      default: { std::cout << "msg_type:" << static_cast<int>(event.msg_type) << " not found." << std::endl; }
    }

    return new_event;
  }

  int GetTimeDiff(timeval t1, timeval t2) {
    int sec_diff = (t2.tv_sec - t1.tv_sec);
    int usec_diff = (t2.tv_usec - t1.tv_usec);
    if (usec_diff < 0) {
      usec_diff += 1000000;
      sec_diff--;
    }
    if (sec_diff < 0) {
      sec_diff = 0;
    }
    return (1000000 * sec_diff + usec_diff);
  }

  struct OrderDetails {
    uint64_t order_id;
    double price;
    uint32_t size;
    char side;
  };

  std::vector<std::map<uint64_t, OrderDetails> > order_id_to_details_;
  BulkFileWriter bulk_file_writer_;
  RTS_MDS::RTSOFCommonStructv2 cached_event_;
  bool is_cached_;
};

void ConvertOrderfeed(std::string trading_date, std::string shortcode, std::string infile_order_feed,
                      std::string outfile_orderfeed_feed) {
  HFSAT::BulkFileReader bulk_file_reader_;
  bulk_file_reader_.open(infile_order_feed);

  if (!bulk_file_reader_.is_open()) {
    std::cerr << " Could not open Orderfeed File : " << infile_order_feed << "\n";
    exit(-1);
  }

  RTSOFConverter converter(outfile_orderfeed_feed);

  size_t MDS_SIZE_ = sizeof(RTS_MDS::RTSOFCommonStruct);
  RTS_MDS::RTSOFCommonStruct event, next_event;
  size_t available_len_ = bulk_file_reader_.read(&event, MDS_SIZE_);

  while (true) {
    if (available_len_ < MDS_SIZE_) break;
    available_len_ = bulk_file_reader_.read(&next_event, MDS_SIZE_);

    converter.Process(event);

    event = next_event;
  }

  converter.End();

  bulk_file_reader_.close();
}
}
}
