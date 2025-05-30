#include "baseinfra/EventDispatcher/minute_bar_events_dataloader_and_dispatcher.hpp"
#include <getopt.h>

void print_usage(const char* prg_name) {
  printf(" This is the Back Adjustment Executable \n");
  printf(
      " Usage:%s --start_date <start_YYYYMMDD> --end_date <end_YYYYMMDD> --index_file <index_file_path> --output_file <backadjusted_outfile> --ticker <ticker> --header <0/1> \n",
      prg_name);
  printf(" --start_date  start_date of test run \n");
  printf(" --end_date end_date of test run\n");
  printf(" --index_file Complete path of the index file, can be found in hftrap/Defines/\n" );
  printf(" --output_file File name which will have adjusted minute bardata with adjustment factor\n" );
  printf(" --ticker Ticker of the underlying\n");
  printf(" --header 0/1 depending on whether you want col names to be printed in the outfile\n");
}

static struct option data_options[] = {{"help", no_argument, 0, 'h'}, 
                                       {"start_date", required_argument, 0, 's'},
                                       {"end_date", required_argument, 0, 'e'},
                                       {"index_file", required_argument, 0, 'i'},
                                       {"output_file", required_argument, 0, 'o'},
                                       {"ticker", required_argument, 0, 't'},
                                       {"header", required_argument, 0, 'x'},
                                       {0, 0, 0, 0}};

int main( int argc, char *argv[] )
{
  int c; //getopt argument
  int hflag = 0;
  std::string ticker_ = "";
  std::string index_file_ = "";
  std::string output_file_ = "";
  int start_date_ = 0;
  int end_date_ = 0;
  bool include_header_ = false;
  
  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "", data_options, &option_index);
    if (c == -1) break;
    switch (c) {
      case 'h':
        hflag = 1;
        break;
     
      case 's':
        start_date_ = atoi(optarg);
        break;
   
      case 'e':
        end_date_ = atoi(optarg);
        break;

      case 'i':
        index_file_ = optarg;
        break;

      case 'o':
        output_file_ = optarg;
        break;
    
      case 't':
        ticker_ = optarg;
        break;

      case 'x':
        include_header_ = ( atoi(optarg) == 1 ) ? true : false;
    
      case '?':
        if (optopt == 't' || optopt == 's' || optopt == 'e' || optopt == 'i' || optopt == 'o' ) {
          fprintf(stderr, "Option %c requires an argument .. will exit \n", optopt);
          exit(-1);
        }
        break;

      default:
        fprintf(stderr, "Weird option specified .. no handling yet \n");
        break;
    }
  }

  if (hflag) {
    print_usage(argv[0]);
    exit(-1);
  }

  if( start_date_ == 0 || end_date_ == 0 || ticker_.empty() || index_file_.empty() || output_file_.empty() )
  {
    print_usage(argv[0]);
    exit(-1);
  }
  std::cout << "#Loading data. Please be patient..." << std::endl;
  hftrap::eventdispatcher::MinuteBarDataLoaderAndDispatcher & mbar_dispatcher = hftrap::eventdispatcher::MinuteBarDataLoaderAndDispatcher::GetUniqueInstance () ;
  time_t start = time(0);
   mbar_dispatcher.AddData( ticker_.c_str(), 'F', start_date_, end_date_, "0,1" );
  time_t end  = time(0);
  std::cout << "#Loaded data. Time taken : "<< end - start << " seconds" << std::endl;
  
  mbar_dispatcher.BackAdjust( index_file_, output_file_, include_header_ ); 
}
