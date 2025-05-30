#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/CommonTradeUtils/live_struct_sim_struct.hpp"

#include <cstdlib>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << " Usage < exec > <exchange> < live_struct_input_file_path > < sim_struct_output_file_path >\n";
    exit(-1);
  }

  HFSAT::BulkFileReader bulk_file_reader_;
  bulk_file_reader_.open(argv[2]);

  if (!bulk_file_reader_.is_open()) {
    std::cerr << " Could not open Live struct File : " << argv[2] << "\n";
    exit(-1);
  }

  HFSAT::BulkFileWriter bulk_file_writer_;
  bulk_file_writer_.Open(argv[3]);

  if (!bulk_file_writer_.is_open()) {
    std::cerr << " Could not open Sim Struct Output File : " << argv[3] << "\n";
    exit(-1);
  }

  if (!strcmp(argv[1], "ORS")) {
    HFSAT::GenericORSReplyStructLive live_struct;
    HFSAT::GenericORSReplyStruct sim_struct;
    size_t MDS_SIZE_ = sizeof(HFSAT::GenericORSReplyStructLive);
    size_t available_len_ = 0;
    while (true) {
      available_len_ = bulk_file_reader_.read(&live_struct, MDS_SIZE_);
      if (available_len_ < MDS_SIZE_) break;

      HFSAT::Utils::ConvertORSLiveStructToSimStruct(live_struct, sim_struct);

      bulk_file_writer_.Write(&sim_struct, sizeof(HFSAT::GenericORSReplyStruct));
      bulk_file_writer_.CheckToFlushBuffer();
    }

  } else if ((!strcmp(argv[1], "ICE")) || (!strcmp(argv[1], "ICE_FOD"))) {
    ICE_MDS::ICECommonStructLive live_struct;
    ICE_MDS::ICECommonStruct sim_struct;
    size_t MDS_SIZE_ = sizeof(ICE_MDS::ICECommonStructLive);
    size_t available_len_ = 0;
    while (true) {
      available_len_ = bulk_file_reader_.read(&live_struct, MDS_SIZE_);
      if (available_len_ < MDS_SIZE_) break;

      HFSAT::Utils::ConvertICELiveStructToSimStruct(live_struct, sim_struct);

      bulk_file_writer_.Write(&sim_struct, sizeof(ICE_MDS::ICECommonStruct));
      bulk_file_writer_.CheckToFlushBuffer();
    }
  } else if (!strcmp(argv[1], "AFLASH")) {
    AFLASH_MDS::AFlashCommonStructLive live_struct;
    AFLASH_MDS::AFlashCommonStruct sim_struct;
    size_t MDS_SIZE_ = sizeof(AFLASH_MDS::AFlashCommonStructLive);
    size_t available_len_ = 0;
    while (true) {
      available_len_ = bulk_file_reader_.read(&live_struct, MDS_SIZE_);
      if (available_len_ < MDS_SIZE_) break;

      HFSAT::Utils::ConvertAflashLiveStructToSimStruct(live_struct, sim_struct);
      bulk_file_writer_.Write(&sim_struct, sizeof(AFLASH_MDS::AFlashCommonStruct));
      bulk_file_writer_.CheckToFlushBuffer();
    }
  }

  bulk_file_reader_.close();
  bulk_file_writer_.Close();

  return 0;
}
