
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <signal.h>
#include <getopt.h>
#include <algorithm>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/file_utils.hpp"  // To create the directory
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/ttime.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"


std::string dt = "";
std::vector < std::string > files_list_vec; 

void PrintVectorStats ( std::vector < double > vec ){

  if ( 0 == vec.size () ) return ;

  std::sort(vec.begin(), vec.end());

  double sum = 0 ;
  for ( auto & itr : vec ){
    sum += itr ; 
  }

  double avg = sum / ( vec.size() * 1.0 ) ;

  std::cout   << vec[0] << " " 
              << vec[vec.size()*0.01] << " " 
              << vec[vec.size()*0.1] << " " 
              << vec[vec.size()*0.25] << " " 
              << vec[vec.size()*0.5] << " " 
              << vec[vec.size()*0.75] << " " 
              << vec[vec.size()*0.90] << " " 
              << vec[vec.size()*0.99] << " " 
              << vec[vec.size()-1] << " " 
              << avg << " " 
              << vec.size () << std::endl ;

}


class OverallStats {

 private : 

  std::vector < double > all_files_overall_latency_stats_vec;

 public : 

  OverallStats() {
  }

  void ComputeAndDumpStats () {

    for ( auto & itr : files_list_vec ) {

      std::vector < double > overall_latency_stats_vec;
      std::string symbol;
      std::string file_name = itr ;
      HFSAT::BulkFileReader bulk_file_reader ;
      bulk_file_reader.open ( file_name.c_str() );

      if ( bulk_file_reader.is_open () ) {
        while ( true ) {

         CRYPTO_MDS::CoinBaseMktStruct coinbase_mkt_struct;

          size_t read_length = bulk_file_reader.read(reinterpret_cast<char*>(&coinbase_mkt_struct), sizeof(CRYPTO_MDS::CoinBaseMktStruct));
          if (read_length < sizeof(CRYPTO_MDS::CoinBaseMktStruct)) break ;

          std::ostringstream source_time_ ;
          source_time_ << coinbase_mkt_struct.source_time.tv_sec << "." << std::setw(6) << std::setfill('0') << coinbase_mkt_struct.source_time.tv_usec ;
          std::ostringstream local_time_ ;
          local_time_ << coinbase_mkt_struct.local_time.tv_sec << "." << std::setw(6) << std::setfill('0') << coinbase_mkt_struct.local_time.tv_usec;

          symbol = coinbase_mkt_struct.product_id ;
          double lat_diff_ = (std::stod(local_time_.str()) - std::stod(source_time_.str())) * 1000;

          overall_latency_stats_vec.push_back( lat_diff_ );
          all_files_overall_latency_stats_vec.push_back( lat_diff_ );

        }
        bulk_file_reader.close () ;

        std::cout << dt << " " << symbol << " ";
        PrintVectorStats( overall_latency_stats_vec );
      }

    }
    std::cout << dt << " overall ";
    PrintVectorStats( all_files_overall_latency_stats_vec );

  }

};

int main ( int argc, char *argv[] )
{

  if ( argc < 2 ) {
    std::cerr << "USAGE : <date> <file/s>" << std::endl ;
    std::exit(-1);
  }

  dt = argv[1];

  for ( int32_t counter = 2 ; counter < argc ; counter++ ) {
    files_list_vec.push_back(argv[counter]) ;
  }

  OverallStats overallstats ;
  overallstats.ComputeAndDumpStats ();

  return EXIT_SUCCESS;
}				// ----------  end of function main  ----------
