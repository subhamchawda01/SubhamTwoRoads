/**
    \file LoggedFileDecoder.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "infracore/lwfixfast/auto_ntp_template.hpp"
#include "infracore/lwfixfast/auto_bmf_template.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/defines.hpp"
#include <map>
#include <stdio.h>
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include <vector>

void printHexString(char* c, int len) {
  for (int i = 0; i < len; ++i) {
    uint8_t ch = c[i];
    printf("%02x ", ch);
  }
  printf("\n");
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: " << argv[0] << " <file-path> <exch>\n";
    return 1;
  }
  std::string exch = argv[2];

  HFSAT::BulkFileReader r;
  r.open(argv[1]);

  int len;
  char bytes[2048];

  std::map<int, FastDecoder*> decoderMap;
  uint32_t reset_tmp_id = -1;
  uint32_t skip_byte_len = 0;
  FFUtils::ByteArr header;
  HFSAT::CpucycleProfiler::SetUniqueInstance(3);
  HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(1, "single template");
  HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(2, "single packet  ");

  while (true) {
    int len_read = r.read(&len, 4u);
    if (len_read < 4) break;
    if (exch == "RTS") len += skip_byte_len;  // first 16-byte is time
    len_read = r.read(bytes, len);
    if (len_read < len) break;

    //      printHexString(bytes, len);
    std::vector<FastDecoder*> used;
    FFUtils::ByteStreamReader* input = new FFUtils::ByteStreamReader(bytes, len);
    CopyField<uint32_t> templateId = CopyField<uint32_t>(true, false, 0);
    input->skipNBytes(skip_byte_len);  // read header

    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(2);
    while (input->canRead()) {
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
      FFUtils::PMap pmap0 = input->extractPmap();
      templateId.decode(*input, pmap0);

      // std::cerr << "decoding template " << templateId.previousValue.getValue() << "\n";
      if (templateId.previousValue.getValue() == reset_tmp_id) {
        for (unsigned int i = 0; i < used.size(); ++i) used[i]->reset();
        used.clear();
      } else {
        decoderMap[templateId.previousValue.getValue()]->decode(*input, pmap0);
        used.push_back(decoderMap[templateId.previousValue.getValue()]);
      }

      HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
    }

    for (unsigned int i = 0; i < used.size(); ++i) used[i]->reset();
    used.clear();

    HFSAT::CpucycleProfiler::GetUniqueInstance().End(2);
  }

  std::cerr << HFSAT::CpucycleProfiler::GetUniqueInstance().GetCpucycleSummaryString();

  return 0;
}
