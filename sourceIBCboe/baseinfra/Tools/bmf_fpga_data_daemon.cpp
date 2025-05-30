#include <getopt.h>
#include "bmf_fpga_data_daemon.hpp"
#include "baseinfra/FPGA/BMF_FPGA/bmf_fpga_util.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/defines.hpp"
#include "baseinfra/FPGA/BMF_FPGA/bmf_fpga_decoder.hpp"
#include "dvccode/CDef/refdata_locator.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/Utils/allocate_cpu.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

#define FPGA_INSTRUMENT_FILTER_FILE "/spare/local/files/BMF/fpga-filter.txt"

bool exitFlag = false;
BMF_FPGA::BMFFpgaDecoder bmf_fpga_decoder;
std::vector<std::vector<uint64_t>> security_ids_to_filter;

void signalHandler(int signum) {
  std::cout << " \n";
  std::cout << "Interruption signal (" << signum << ") received!\n";
  // bmf_fpga_decoder.PrintReferenceData();
  std::string summary = HFSAT::CpucycleProfiler::GetUniqueInstance().GetCpucycleSummaryString();

  std::cout << "CPU Cycle Profiler Stats: \n" << summary << std::endl;

  exitFlag = true;
  signal(signum, SIG_DFL);
  kill(getpid(), signum);
  exit(0);
}

static struct option data_options[] = {{"help", no_argument, 0, 'h'},
                                       {"config", required_argument, 0, 'a'},
                                       {"mode", required_argument, 0, 'b'},
                                       {0, 0, 0, 0}};

HFSAT::FastMdConsumerMode_t ExtractMode(std::string& mode) {
  if (mode == "Logger") {
    return HFSAT::kLogger;
  } else if (mode == "Writer") {
    return HFSAT::kProShm;
  } else if (mode == "Hybrid") {
    return HFSAT::kRaw;
  }

  return HFSAT::kModeMax;
}

void LoadRefDataToFilter() {
  std::ifstream dfile;
  std::string filter_file = FPGA_INSTRUMENT_FILTER_FILE;

  dfile.open(filter_file.c_str(), std::ofstream::in);

  if (!dfile.is_open()) {
    fprintf(stderr, "Cannot open file %s for reading data to filter\n", filter_file.c_str());
    exit(-1);
  }

  security_ids_to_filter.resize(30, std::vector<uint64_t>());

  char line[1024];
  while (!dfile.eof()) {
    memset(line, 0, sizeof(line));
    dfile.getline(line, sizeof(line));

    // Skip empty lines and comments
    if (strlen(line) == 0 || line[0] == '#') {
      continue;
    }

    uint64_t security_id = atoll(strtok(line, "\n\t "));
    char* strname = (char*)calloc(20, sizeof(char));
    strncpy(strname, strtok(NULL, "\n\t "), 20);
    int channel_num = atoi(strtok(NULL, "\n\t "));

    if (strlen(strname) > 11) {
      continue;
    }

    std::cout << "Filter: Channel: " << channel_num - 1 << " SecID: " << security_id << " Secname: " << strname
              << std::endl;
    security_ids_to_filter[channel_num - 1].push_back(security_id);

    if (security_ids_to_filter[channel_num - 1].size() > 32) {
      std::cerr << "Exiting ..as filtering more than 32 products per channel isn't allowed in FPGA. Please change the "
                   "filter file : "
                << filter_file << std::endl;
      exit(1);
    }
  }

  dfile.close();
}

void ParseCommandLineArgs(int argc, char** argv, std::string& config_file, std::string& mode, bool& help_flag) {
  while (1) {
    int option_index = 0;
    int c = getopt_long(argc, argv, "", data_options, &option_index);
    if (c == -1) break;
    switch (c) {
      case 'h':
        help_flag = true;
        break;
      case 'a':
        config_file = optarg;
        break;

      case 'b':
        mode = optarg;
        break;

      case '?':
        if (optopt == 'a' || optopt == 'b') {
          fprintf(stderr, "Option %c requires an argument .. will exit \n", optopt);
          exit(-1);
        }
        break;

      default:
        fprintf(stderr, "Option not supported \n");
        break;
    }
  }
}

void PrintUsage(const char* prg_name) {
  printf(" This is the BMF Fpga Data Daemon exec \n");
  printf(" Usage:%s --config <config_file> --mode <Logger/Writer/Hybrid>\n", prg_name);
}

int main(int argc, char** argv) {
  // Assigning signal handler to quit application by typing Ctrl-4
  signal(SIGQUIT, signalHandler);
  signal(SIGINT, signalHandler);
  HFSAT::AllocateCPUUtils::GetUniqueInstance().AllocateCPUOrExit("bmf_fpga_data_daemon");
  HFSAT::CpucycleProfiler::SetUniqueInstance(10);
  HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(1, "MBO -> MBP conversion time ");

  std::string config_file = "", mode = "";
  bool help_flag = false;

  ParseCommandLineArgs(argc, argv, config_file, mode, help_flag);

  if (config_file == "" || mode == "" || help_flag) {
    PrintUsage(argv[0]);
    exit(0);
  }

  bmf_fpga_decoder.Initialize(ExtractMode(mode));

  LoadRefDataToFilter();

  // Creating the SiliconUMDF device handler
  SiliconUmdf::EventAPI::DeviceHandler handler;

  // Configuration struct
  SiliconUmdf::EventAPI::sumdfConfiguration_t sumdf_config;

  // Filling sumdf_config.channelArray using channel configuration file
  ifstream stream_config;
  stream_config.open(config_file.c_str(), ifstream::in);

  if (!BMFFPGAUtil::ReadChannelConfig(stream_config, sumdf_config)) {
    std::cout << "Failed to parse config file : " << config_file << std::endl;
    exit(1);
  }

  // Setting SiliconUMDF path and logFile
  strcpy(sumdf_config.siliconUMDF_path, "/usr/siliconUmdf");
  strcpy(sumdf_config.siliconUMDF_logFile, "SiliconUMDF");
  sumdf_config.siliconUMDF_logNameFormat = SiliconUmdf::EventAPI::APPEND_DATE_AND_TIME;

  // waitEvents timeout (30s timeout)
  sumdf_config.returnEventsConfiguration.waitEventTimeout = 1;
  // Return events configuration
  sumdf_config.returnEventsConfiguration.returnEvents = SiliconUmdf::EventAPI::RETURN_BOOK_AND_MDENTRIES;

  sumdf_config.generateMbpBookFromMbo = true;

  //~ // TCP Recovery configuration (BMF PUMA 2.0)
  sumdf_config.tcpRecoveryBMF.enabled = false;
  strcpy(sumdf_config.tcpRecoveryBMF.senderCompID, "MBOCHIP01");
  strcpy(sumdf_config.tcpRecoveryBMF.targetCompID, "TCPRCV01C");
  strcpy(sumdf_config.tcpRecoveryBMF.ip, "10.10.0.10");
  sumdf_config.tcpRecoveryBMF.port = 10000;

  // TCP Recovery configuration (BOVESPA PUMA 2.0)
  sumdf_config.tcpRecoveryBOVESPA.enabled = false;
  strcpy(sumdf_config.tcpRecoveryBOVESPA.senderCompID, "MBOCHIP01");
  strcpy(sumdf_config.tcpRecoveryBOVESPA.targetCompID, "TCPRCVEQT01C");
  strcpy(sumdf_config.tcpRecoveryBOVESPA.ip, "10.10.0.10");
  sumdf_config.tcpRecoveryBOVESPA.port = 10000;

  // Opening device
  if (SiliconUmdf::EventAPI::open(handler, sumdf_config) != true) {
    printf("Failed to open SiliconUMDF. Check SiliconUMDF.log.\n");
    exit(1);
  }

  // waitEvent variables
  SiliconUmdf::EventAPI::Events_t events;
  SiliconUmdf::EventAPI::WaitEventsReturn waitEventsReturn;

  // MDEntries filter
  SiliconUmdf::EventAPI::MDEntriesAllowed_t mdEntriesAllowed;
  mdEntriesAllowed.bid = true;
  mdEntriesAllowed.offer = true;
  mdEntriesAllowed.trade = true;

  // Selecting channels to enable
  bool channelEnabled[30];
  for (int i = 0; i < 30; i++) {
    channelEnabled[i] = false;

    if (security_ids_to_filter[i].size() > 0 || (ExtractMode(mode) == HFSAT::kLogger)) {
      channelEnabled[i] = true;
      std::cout << "Listening to channel : " << i << std::endl;
    }
  }

  // Subscribe all event types
  SiliconUmdf::EventAPI::subscribeAll(handler, SiliconUmdf::EventAPI::S_QUOTES);
  SiliconUmdf::EventAPI::subscribeAll(handler, SiliconUmdf::EventAPI::S_INSTRUMENT_STATUS);
  SiliconUmdf::EventAPI::subscribeAll(handler, SiliconUmdf::EventAPI::S_INSTRUMENT_UPDATES);
  SiliconUmdf::EventAPI::subscribeAll(handler, SiliconUmdf::EventAPI::S_NEWS);

  for (int channel_num = 0; channel_num < 10; channel_num++) {
    if (channelEnabled[channel_num]) {
      // Joining channel with instrument filter

      // Instruments filter (maximum 32 instruments per channel)
      Uint64 securityIds[32];

      int num_securities_to_filter = security_ids_to_filter[channel_num].size();

      for (int secid_idx = 0; secid_idx < num_securities_to_filter; secid_idx++) {
        securityIds[secid_idx] = security_ids_to_filter[channel_num][secid_idx];
      }

      bool should_filter = (ExtractMode(mode) == HFSAT::kLogger) ? false : true;

      SiliconUmdf::EventAPI::JoinChannelReturn ret;
      ret = SiliconUmdf::EventAPI::joinChannel(handler, channel_num, should_filter, num_securities_to_filter,
                                               securityIds, mdEntriesAllowed);

      switch (ret) {
        case SiliconUmdf::EventAPI::JOIN_SUCCEEDED:
          // Requesting instrument list
          SiliconUmdf::EventAPI::requestInstrumentList(handler, channel_num);
          break;

        case SiliconUmdf::EventAPI::JOIN_FAILED:
          printf("joinChannel %d failed. Check the SiliconUMDF log file", channel_num);
          break;

        default:
          break;
      }
    }
  }

  stringstream ss;

  // ProcesssMDSFiletoSHM();

  while (!exitFlag) {
    waitEventsReturn = SiliconUmdf::EventAPI::waitEvents(handler, events);

    switch (waitEventsReturn) {
      case SiliconUmdf::EventAPI::SUCCEEDED:

        switch (events.eventsReturned) {
          case SiliconUmdf::EventAPI::INSTRUMENT_LIST:
            bmf_fpga_decoder.ProcessReferenceData(events);
            break;

          case SiliconUmdf::EventAPI::BOOK_AND_MDENTRIES_BVMF_PUMA_2_0:
            bmf_fpga_decoder.ProcessBook(events);
            bmf_fpga_decoder.ProcessMDEntries(events);
            break;

          case SiliconUmdf::EventAPI::EMPTY_ALL_BOOKS:
            std::cerr << "Empty all books message received" << std::endl;
            break;

          case SiliconUmdf::EventAPI::INSTRUMENT_STATUS:
            bmf_fpga_decoder.ProcessInstrumentStatus(events);
            break;

          case SiliconUmdf::EventAPI::GROUP_STATUS:
            bmf_fpga_decoder.ProcessGroupStatus(events);
            break;

          case SiliconUmdf::EventAPI::ERROR_EVENT: {
            struct timeval tv;
            gettimeofday(&tv, NULL);

            ss.str("");
            ss << "---------------" << endl;
            ss << " ERROR EVENT   " << endl;
            ss << "---------------" << endl;
            ss << "   Description= " << events.error.description << endl;
            ss << "   ChannelID= " << events.sumdfChannelId << endl;
            ss << "   Time: " << tv.tv_sec << "." << tv.tv_usec << std::endl;
            std::cout << ss.str();

            unsigned int channel_id = events.sumdfChannelId;
            // In case of error the application MUST leave the channel. The other channels are not affected.
            SiliconUmdf::EventAPI::leaveChannel(handler, channel_id);

            // Instruments filter (maximum 32 instruments per channel)
            Uint64 securityIds[32];

            int num_securities_to_filter = security_ids_to_filter[channel_id].size();

            for (int secid_idx = 0; secid_idx < num_securities_to_filter; secid_idx++) {
              securityIds[secid_idx] = security_ids_to_filter[channel_id][secid_idx];
            }

            bool should_filter = (ExtractMode(mode) == HFSAT::kLogger) ? false : true;

            // Join channel again to continue receiving the market data
            SiliconUmdf::EventAPI::JoinChannelReturn ret;
            ret = SiliconUmdf::EventAPI::joinChannel(handler, channel_id, should_filter, num_securities_to_filter,
                                                     securityIds, mdEntriesAllowed);

            switch (ret) {
              case SiliconUmdf::EventAPI::JOIN_SUCCEEDED:
                SiliconUmdf::EventAPI::requestInstrumentList(handler, channel_id);
                break;
              default:
                printf("joinChannel %d failed. Check the SiliconUMDF log file", channel_id);
                break;
            }

          } break;

          default:
            break;
        }
        break;

      case SiliconUmdf::EventAPI::FAILED:
        ss.str("");
        ss << "-------------------" << endl;
        ss << " waitEvent FAILED  " << endl;
        ss << "-------------------" << endl;
        cout << ss.str();
        exitFlag = true;
        break;
      case SiliconUmdf::EventAPI::TIMED_OUT:
        break;

      default:
        break;

    }  // switch waitEventsReturn
  }    // while true

  SiliconUmdf::EventAPI::close(handler);
}
