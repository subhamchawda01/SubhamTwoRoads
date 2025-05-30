/**
   \file LoggedFileDecoder.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#pragma once
#include "infracore/lwfixfast/auto_ntp_template.hpp"
#include "infracore/lwfixfast/auto_bmfpuma_template.hpp"
#include "infracore/lwfixfast/auto_ntp_ord_template.hpp"
#include "infracore/lwfixfast/auto_bmf_template.hpp"
#include "infracore/lwfixfast/auto_puma_template.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/defines.hpp"
#include <map>
#include <stdio.h>
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include <vector>
#include <iostream>
#include <iomanip>

class FastStreamDecoder {
  std::map<int32_t, FastDecoder*> decoderMap;
  std::vector<FastDecoder*> used;
  uint32_t reset_template_id;

  // bytes_to_skip is the number of bytes we need to skip in every message
  // Needed right now only for MOEX (RTS)
  // MOEX add 4 bytes in each msg (before pmap) which denotes msgseqnum (for arbitration)
  int bytes_to_skip;

  FastStreamDecoder(int _skip_bytes_ = 0) : used(), bytes_to_skip(_skip_bytes_) {}  // private constructor
  //  FastStreamDecoder(const FastStreamDecoder& ); //disable copy

  void printHexString(char* c, int len) {
    for (int i = 0; i < len; ++i) {
      uint8_t ch = c[i];
      fprintf(stderr, "%02x ", ch);
    }
    fprintf(stderr, "\n");
  }

 public:
  static FastStreamDecoder GetNtpDecoder() {
    FastStreamDecoder d;
    NTP_TEMPLATE_DECODER::DecoderMap::initilize(d.decoderMap);
    d.reset_template_id = -1;
    return d;
  }

  // Remove the memory allocated
  static void RemoveNtpDecoder(FastStreamDecoder& _fast_stream_decoder_) {
    NTP_TEMPLATE_DECODER::DecoderMap::cleanUpMem(_fast_stream_decoder_.decoderMap);
  }

  static FastStreamDecoder GetBMFPumaDecoder() {
    FastStreamDecoder d;
    BMFPUMA_TEMPLATE_DECODER::DecoderMap::initilize(d.decoderMap);
    d.reset_template_id = -1;
    return d;
  }

  // Remove the memory allocated
  static void RemoveBMFPumaDecoder(FastStreamDecoder& _fast_stream_decoder_) {
    BMFPUMA_TEMPLATE_DECODER::DecoderMap::cleanUpMem(_fast_stream_decoder_.decoderMap);
  }

  static FastStreamDecoder GetNtpOrdDecoder() {
    FastStreamDecoder d;
    NTP_ORD_TEMPLATE_DECODER::DecoderMap::initilize(d.decoderMap);
    d.reset_template_id = -1;
    return d;
  }

  static FastStreamDecoder GetBmfDecoder() {
    FastStreamDecoder d;
    BMF_TEMPLATE_DECODER::DecoderMap::initilize(d.decoderMap);
    d.reset_template_id = -1;
    return d;
  }

  static FastStreamDecoder GetPumaDecoder() {
    FastStreamDecoder d;
    PUMA_TEMPLATE_DECODER::DecoderMap::initilize(d.decoderMap);
    d.reset_template_id = -1;
    return d;
  }

  void decode(char* bytes, uint32_t len) {
    FFUtils::ByteStreamReader input(bytes, len);
    // printHexString(input.getCurrCharPointer(), input.getNumBytesToRead());

    CopyField<uint32_t> templateId = CopyField<uint32_t>(true, false, 0);

    //    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
    while (input.canRead()) {
      // HFSAT::CpucycleProfiler::GetUniqueInstance().Start(2);
      input.skipNBytes(bytes_to_skip);  // Skip extra bytes (to start reading with pmap)
      FFUtils::PMap pmap0 = input.extractPmap();
      templateId.decode(input, pmap0);
      //      printf("Decoding templateID: %3d\n", (int)templateId.previousValue.getValue());

      if (templateId.previousValue.getValue() == reset_template_id) {
        for (uint32_t i = 0; i < used.size(); ++i) used[i]->reset();
        used.clear();
      } else if (decoderMap.find(templateId.previousValue.getValue()) != decoderMap.end()) {
        decoderMap[templateId.previousValue.getValue()]->decode(input, pmap0);
        used.push_back(decoderMap[templateId.previousValue.getValue()]);
      } else {
        timeval current_time_;
        gettimeofday(&current_time_, NULL);
        std::cerr << current_time_.tv_sec << "." << std::setw(6) << std::setfill('0') << current_time_.tv_usec << " ";
        std::cerr << typeid(*this).name() << ':' << __func__;
        std::cerr << " invalid template id. " << templateId.previousValue.getValue() << "\tpossible errors, "
                  << "\n\t\ti) bug in decoding. "
                  << "\n\t\tii) incorrect template file. "
                  << "\n\t\tiii) incorrect bytes of data as argument\n";
        printHexString(input.getCurrCharPointer() - input.getCurrPointer(),
                       input.getCurrPointer() + input.getNumBytesToRead());
        std::cerr << "curPosition " << input.getCurrPointer() << "\n";
        std::cerr << "totalLen " << input.getCurrPointer() + input.getNumBytesToRead() << "\n";
        std::cerr << "Note: bytes prior to current position might have been modified in the decoding process\n";
        std::cerr << "reseting the packet instead of exiting." << std::endl;

        for (uint32_t i = 0; i < used.size(); ++i) used[i]->reset();
        used.clear();
      }
      // HFSAT::CpucycleProfiler::GetUniqueInstance().End(2);
    }

    for (unsigned int i = 0; i < used.size(); ++i) used[i]->reset();

    used.clear();
    //    HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
  }
};
