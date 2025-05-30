#include "bmf_fpga_util.hpp"

namespace BMFFPGAUtil {

bool ReadChannelConfig(std::ifstream& stream_config, SiliconUmdf::EventAPI::sumdfConfiguration_t& sumdf_config) {
  char str[500];
  char temp[4];

  char channel_ip[90][16];
  unsigned port[90];
  unsigned stream_id = 0;
  std::string channel_id[90];
  SiliconUmdf::EventAPI::StreamType stream_type[90];
  SiliconUmdf::EventAPI::MarketSegment market_segment[90];
  SiliconUmdf::EventAPI::ExchangePlatform exchange_type[90];

  if (!stream_config.good()) {
    printf("Failed to open streams.cfg\n");
    exit(0);
  }

  // Read streams.cfg and configure streams
  printf("\nConfiguring streams:\n");

  while (stream_config.good()) {
    stream_config.get(temp[0]);
    while (temp[0] == ' ' || temp[0] == '\n') {
      stream_config.get(temp[0]);
      if (!stream_config.good()) break;
    }

    if (!stream_config.good()) break;

    if (temp[0] == '#') {
      stream_config.getline(str, 255);
    } else if (temp[0] == '>') {
      stream_config.getline(str, 255);
      break;
    } else {
      stream_config.unget();

      // Read stream_id
      stream_config >> stream_id;

      if (!stream_config.good()) break;

      // Read Channel IDs
      stream_config >> channel_id[stream_id];

      stream_config.get();

      // Read Ip
      stream_config.getline(channel_ip[stream_id], 16, ' ');

      // Read Port
      stream_config >> port[stream_id];

      // Read stream type
      unsigned stream_type_code;
      stream_config >> stream_type_code;

      // Read Market select 0=PUMA_1_6 1=PUMA_2_0
      unsigned exchange_platform_code;
      stream_config >> exchange_platform_code;

      char market_segment_char;
      stream_config >> market_segment_char;

      printf("StreamID %d [%s]->   IP = %s   Port = %d   ", stream_id, channel_id[stream_id].c_str(),
             channel_ip[stream_id], port[stream_id]);

      switch (stream_type_code) {
        case 0:
          printf("INSTRUMENTAL   ");
          stream_type[stream_id] = SiliconUmdf::EventAPI::INSTRUMENTAL;
          break;
        case 1:
          printf("INCREMENTAL   ");
          stream_type[stream_id] = SiliconUmdf::EventAPI::INCREMENTAL;
          break;
        case 2:
          printf("SNAPSHOT   ");
          stream_type[stream_id] = SiliconUmdf::EventAPI::SNAPSHOT;
          break;
        default:
          return false;
          break;
      }

      (void)stream_type;

      switch (exchange_platform_code) {
        case 0:
          printf("BVMF_PUMA_1_6   ");
          exchange_type[stream_id / 3] = SiliconUmdf::EventAPI::BVMF_PUMA_1_6;
          break;
        case 1:
          printf("BVMF_PUMA_2_0   ");
          exchange_type[stream_id / 3] = SiliconUmdf::EventAPI::BVMF_PUMA_2_0;
          break;
        default:
          return false;
          break;
      }

      switch (market_segment_char) {
        case 'D':
          printf("BMF\n");
          market_segment[stream_id / 3] = SiliconUmdf::EventAPI::BMF;
          break;
        case 'E':
          printf("BOVESPA\n");
          market_segment[stream_id / 3] = SiliconUmdf::EventAPI::BOVESPA;
          break;
        default:
          return false;
          break;
      }

      sumdf_config.channelArray[stream_id / 3].exchangePlatform = exchange_type[stream_id / 3];
      sumdf_config.channelArray[stream_id / 3].marketSegment = market_segment[stream_id / 3];
      switch (stream_id % 3) {
        case 0:  // instrumental
          strncpy(sumdf_config.channelArray[stream_id / 3].instrumentalStream.ip, channel_ip[stream_id], 16);
          sumdf_config.channelArray[stream_id / 3].instrumentalStream.port = port[stream_id];
          break;
        case 1:  // incremental
          strncpy(sumdf_config.channelArray[stream_id / 3].exchangeChannelId, channel_id[stream_id].c_str(), 4);
          strncpy(sumdf_config.channelArray[stream_id / 3].incrementalStream.ip, channel_ip[stream_id], 16);
          sumdf_config.channelArray[stream_id / 3].incrementalStream.port = port[stream_id];
          break;
        case 2:  // snapshot
          sumdf_config.channelArray[stream_id / 3].snapshotStream.ip[0] = channel_ip[stream_id][0];
          strncpy(sumdf_config.channelArray[stream_id / 3].snapshotStream.ip, channel_ip[stream_id], 16);
          sumdf_config.channelArray[stream_id / 3].snapshotStream.port = port[stream_id];
          break;
        default:
          return false;
          break;
      }
    }
  }

  Uint64 instruments[90];

  if (stream_config.good()) {
    printf("\nINSTRUMENT FILTERING CONFIG\n");
  }
  while (stream_config.good()) {
    stream_config.get(temp[0]);
    while (temp[0] == ' ' || temp[0] == '\n') {
      stream_config.get(temp[0]);
      if (!stream_config.good()) break;
    }

    if (!stream_config.good()) break;

    if (temp[0] == '#' || temp[0] == '>') {
      stream_config.getline(str, 255);
      continue;
    }

    stream_config.unget();

    stream_config.getline(str, 500);
    std::istringstream ss(str);

    ss >> stream_id;

    int i;
    for (i = 0; ss.good(); i++) {
      ss >> (instruments[i]);
    }

#ifdef INSTRUMENT_FILTER_ENABLED
    printf("Stream_id= %d NumInstrumentsFilter= %d\n", stream_id, i);
// SiliconUmdf::EventAPI::enableInstrumentsFilter(handler, stream_id, instruments, i);
#endif
  }
  return true;
}
}
