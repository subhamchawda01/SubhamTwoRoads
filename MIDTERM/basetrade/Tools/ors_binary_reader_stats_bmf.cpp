// =====================================================================================
// 
//       Filename:  ors_binary_reader_stats.cpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  04/02/2018 08:49:44 AM
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

#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <signal.h>
#include <getopt.h>
#include <map>
#include <algorithm>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/file_utils.hpp"  // To create the directory
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/ors_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "baseinfra/LoggedSources/ors_message_filenamer.hpp"


std::string server = "" ;
int32_t base_saci = 0  ;
int32_t freq = 0 ;
bool are_we_appending = false ;
std::string dt = "";

std::vector < std::string > files_list_vec; 

bool SortBasedOnCT ( HFSAT::GenericORSReplyStructLiveProShm i, HFSAT::GenericORSReplyStructLiveProShm j){
  return ( ( i.ors_end_cycles_delta_ + i.csw_start_cycles_ ) < ( j.ors_end_cycles_delta_ + j.csw_start_cycles_ )) ;
}

void PrintAndDumpVectorStats ( std::vector < double > vec, std::string filename, uint32_t timevalue = 0 ){

  if ( 0 == vec.size () ) return ;

  std::ofstream statsstream ;


  if ( false == are_we_appending ) {
    statsstream.open ( filename.c_str(), std::ios::out ) ;
    are_we_appending = true ;
  }else{
    statsstream.open ( filename.c_str(), std::ios::app ) ;
  }

  std::sort(vec.begin(), vec.end());

  double sum = 0 ;
  for ( auto & itr : vec ){
    sum += itr ; 
  }

  double avg = sum / ( vec.size() * 1.0 ) ;

  if ( timevalue != 0 ){
    statsstream << timevalue << " " ;
  }

  statsstream << vec[0] << " " 
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

  statsstream.flush();
  statsstream.close ();

}


class FileStats {

 private : 

  std::vector < double > overall_latency_stats_vec;
  std::vector < double > rolling_15_min_stats_vec;
  std::vector < double > same_ct_orders_st_diff_vec;

  std::vector < double > overall_seq_stats_vec;
  std::vector < double > overall_cxlseq_stats_vec;
  std::vector < double > overall_modseq_stats_vec;

 public : 

  FileStats () {
  }

  void ComputeAndDumpStats ( std::string file_name ) {

    std::string symbol = "";
    uint32_t first_time_point = 0 ;
    uint64_t last_ct_value = 0 ;
    uint64_t last_st_value = 0 ;
    uint32_t last_ct_sec = 0 ;

    std::string overall_latency_outfile = server + "_overall_latency.txt" ; 
    std::string timely_latency_outfile = server + "_timely_latency.txt" ;
    std::string samect_latency_outfile = server + "_samect_latency.txt" ;
    std::string seq_latency_outfile = server + "_seq_latency.txt" ;
    std::string cxlseq_latency_outfile = server + "_cxlseq_latency.txt" ;
    std::string modifyseq_latency_outfile = server + "_modifyseq_latency.txt" ;

    HFSAT::BulkFileReader bulk_file_reader ;
    bulk_file_reader.open ( file_name.c_str() );

    if ( bulk_file_reader.is_open () ) {
      while ( true ) {

       HFSAT::GenericORSReplyStructLiveProShm reply_struct;

        size_t read_length = bulk_file_reader.read(reinterpret_cast<char*>(&reply_struct), sizeof(HFSAT::GenericORSReplyStructLiveProShm));
        if (read_length < sizeof(HFSAT::GenericORSReplyStructLiveProShm)) break ;

        if ( reply_struct.orr_type_ != HFSAT::kORRType_Conf && 
             reply_struct.orr_type_ != HFSAT::kORRType_Cxld && 
             reply_struct.orr_type_ != HFSAT::kORRType_CxRe ) continue ;

        if ( 0 == reply_struct.csw_start_cycles_ ) continue ;

        int32_t this_saci_base = reply_struct.server_assigned_client_id_ >> 16 ;

        if ( this_saci_base != base_saci ) continue ;

        time_t time = (time_t)reply_struct.client_request_time_.tv_sec ;
        struct tm tm_curr = *localtime(&time);

        if ( tm_curr.tm_hour >= 21 ){
          if ( tm_curr.tm_min > 0 || tm_curr.tm_sec > 0 ) continue ;
        }

        if ( tm_curr.tm_hour <= 10 ) {
          if ( tm_curr.tm_min < 55 ) continue ;
        }

        if ( symbol == "" ) {
          symbol = reply_struct.symbol_ ;
          overall_latency_outfile = server + "_" + symbol + "_" + dt + "_overall_latency.txt" ;
          timely_latency_outfile = server + "_" + symbol + "_" + dt + "_timely_latency.txt" ;
          samect_latency_outfile = server + "_" + symbol +  "_" + dt + "_samect_latency.txt" ;
          seq_latency_outfile = server + "_" + symbol + "_" + dt + "_seq_latency.txt" ;
          cxlseq_latency_outfile = server + "_" + symbol + "_" + dt + "_cxlseq_latency.txt" ;
          modifyseq_latency_outfile = server + "_" + symbol + "_" + dt + "_modfiyseq_latency.txt" ;
        }

        last_ct_sec = reply_struct.client_request_time_.tv_sec ;

        if( 0 == first_time_point ){
          first_time_point = reply_struct.client_request_time_.tv_sec ;
        }

//        st_count = reply_struct.time_set_by_server_.val ; 
//        ct_count = reply_struct.exch_assigned_sequence_ ;

        double latency = ( reply_struct.ors_end_cycles_delta_ ) / ( freq * 1.0 ) ;

        overall_latency_stats_vec.push_back( latency ) ;
        rolling_15_min_stats_vec.push_back( latency ) ;

        if ( reply_struct.orr_type_ == HFSAT::kORRType_Conf ) {
          overall_seq_stats_vec.push_back( latency ) ;
        }else if ( reply_struct.orr_type_ == HFSAT::kORRType_Cxld ){
          overall_cxlseq_stats_vec.push_back( latency ) ;
        }else if ( reply_struct.orr_type_ == HFSAT::kORRType_CxRe ) {
          overall_modseq_stats_vec.push_back ( latency );
        }

        if ( ((uint32_t)reply_struct.client_request_time_.tv_sec - first_time_point) > 900 ){ 
          first_time_point = reply_struct.client_request_time_.tv_sec ;
          PrintAndDumpVectorStats( rolling_15_min_stats_vec, timely_latency_outfile, first_time_point );
          rolling_15_min_stats_vec.clear ();
        }

        if ( 0 == last_ct_value ) {
          last_ct_value = ( reply_struct.client_request_time_.tv_sec * 1000000 ) + ( reply_struct.client_request_time_.tv_usec ) ;
        }else {
          uint64_t current_ct_value = ( reply_struct.client_request_time_.tv_sec * 1000000 ) + ( reply_struct.client_request_time_.tv_usec ) ;
          if ( current_ct_value == last_ct_value ) {
            double st_diff = ( reply_struct.ors_end_cycles_delta_ - last_st_value ) / ( freq * 1.0 ) ;
            same_ct_orders_st_diff_vec.push_back( st_diff ) ;
          }
          last_ct_value = current_ct_value ;
        }

        last_st_value = reply_struct.ors_end_cycles_delta_;

      }

      bulk_file_reader.close () ;

      PrintAndDumpVectorStats( overall_latency_stats_vec, overall_latency_outfile );
      PrintAndDumpVectorStats( same_ct_orders_st_diff_vec, samect_latency_outfile );
      PrintAndDumpVectorStats ( overall_seq_stats_vec, seq_latency_outfile );
      PrintAndDumpVectorStats ( overall_cxlseq_stats_vec, cxlseq_latency_outfile );
      PrintAndDumpVectorStats ( overall_modseq_stats_vec, modifyseq_latency_outfile );

      if ( rolling_15_min_stats_vec.size () > 0 ) {
        PrintAndDumpVectorStats( rolling_15_min_stats_vec, timely_latency_outfile, last_ct_sec );
      }
    }

  }

};

class OverallStats {

 private : 


  std::vector < HFSAT::GenericORSReplyStructLiveProShm > alldata_vec;

  std::vector < double > overall_latency_stats_vec;
  std::vector < double > rolling_15_min_stats_vec;
  std::vector < double > same_ct_orders_st_diff_vec;

  std::vector < double > overall_seq_stats_vec;
  std::vector < double > overall_cxlseq_stats_vec;
  std::vector < double > overall_modseq_stats_vec;
  std::vector< double > cs_stats_vec;
  std::vector< double > shm1_stats_vec;
  std::vector< double > strat_stats_vec;
  std::vector< double > shm2_stats_vec;
  std::vector< double > ors_stats_vec;

 public : 

  OverallStats() {
  }

  void ComputeAndDumpStats () {

    uint32_t first_time_point = 0 ;
    uint64_t last_ct_value = 0 ;
    uint64_t last_st_value = 0 ;
    uint32_t last_ct_sec = 0 ;
    HFSAT::GenericORSReplyStructLiveProShm prev_struct ;

    std::string overall_latency_outfile = server + "_" + dt + "_overall_latency.txt" ; 
    std::string timely_latency_outfile = server + "_" + dt + "_timely_latency.txt" ;
    std::string samect_latency_outfile = server + "_" + dt +"_samect_latency.txt" ;
    std::string seq_latency_outfile = server + "_" + dt + "_seq_latency.txt" ;
    std::string cxlseq_latency_outfile = server + "_" + dt + "_cxlseq_latency.txt" ;
    std::string modifyseq_latency_outfile = server + "_" + dt + "_modifyseq_latency.txt" ;
    std::string cs_latency_outfile = server + "_" + dt + "_cs_latency.txt" ;
    std::string shm1_latency_outfile = server + "_" + dt + "_shm1_latency.txt" ;
    std::string strat_latency_outfile = server + "_" + dt + "_strat_latency.txt" ;
    std::string shm2_latency_outfile = server + "_" + dt + "_shm2_latency.txt" ;
    std::string ors_latency_outfile = server + "_" + dt + "_ors_latency.txt" ;

    for ( auto & itr : files_list_vec ) {

      std::string file_name = itr ;
      HFSAT::BulkFileReader bulk_file_reader ;
      bulk_file_reader.open ( file_name.c_str() );

      if ( bulk_file_reader.is_open () ) {

        while ( true ) {

         HFSAT::GenericORSReplyStructLiveProShm reply_struct;

          size_t read_length = bulk_file_reader.read(reinterpret_cast<char*>(&reply_struct), sizeof(HFSAT::GenericORSReplyStructLiveProShm));
          if (read_length < sizeof(HFSAT::GenericORSReplyStructLiveProShm)) break ;

          if ( reply_struct.orr_type_ != HFSAT::kORRType_Conf && 
               reply_struct.orr_type_ != HFSAT::kORRType_Cxld && 
               reply_struct.orr_type_ != HFSAT::kORRType_CxRe ) continue ;

          if ( 0 == reply_struct.csw_start_cycles_ ) continue ;

          int32_t this_saci_base = reply_struct.server_assigned_client_id_ >> 16 ;

          if ( this_saci_base != base_saci ) continue ;

          time_t time = (time_t)reply_struct.client_request_time_.tv_sec ;
          struct tm tm_curr = *localtime(&time);

          if ( tm_curr.tm_hour >= 21 ){
            if ( tm_curr.tm_min > 0 || tm_curr.tm_sec > 0 ) {
              std::cout << " Ignore :" << reply_struct.ToString () ;
              continue ;
            }
          }

          if ( tm_curr.tm_hour <= 10 ) {
            if ( tm_curr.tm_min < 55 ) {
              std::cout << " Ignore :" << reply_struct.ToString () ;
              continue ;
            }
          }

          alldata_vec.push_back( reply_struct ) ;

        }

        bulk_file_reader.close () ;

      }

    }

    std::sort( alldata_vec.begin(), alldata_vec.end(), SortBasedOnCT );

    for( auto & reply_struct : alldata_vec ) {

      last_ct_sec = reply_struct.client_request_time_.tv_sec ;

      if( 0 == first_time_point ){
        first_time_point = reply_struct.client_request_time_.tv_sec ;
      }

//      st_count = reply_struct.time_set_by_server_.val ; 
//      ct_count = reply_struct.exch_assigned_sequence_ ;

      double latency = ( reply_struct.ors_end_cycles_delta_ ) / ( freq * 1.0 ) ;
      
      overall_latency_stats_vec.push_back( latency ) ;
      rolling_15_min_stats_vec.push_back( latency ) ;
      cs_stats_vec.push_back( ((double)( reply_struct.csw_end_cycles_delta_)) / ( freq * 1.0 ) );
      shm1_stats_vec.push_back( ((double)(reply_struct.query_start_cycles_delta_ - reply_struct.csw_end_cycles_delta_)) / ( freq * 1.0 ) );
      strat_stats_vec.push_back( ((double)(reply_struct.query_end_cycles_delta_ - reply_struct.query_start_cycles_delta_)) / ( freq * 1.0 ) );
      shm2_stats_vec.push_back( ((double)(reply_struct.ors_start_cycles_delta_ - reply_struct.query_end_cycles_delta_)) / ( freq * 1.0 ) );
      ors_stats_vec.push_back( ((double)(reply_struct.ors_end_cycles_delta_ - reply_struct.ors_start_cycles_delta_)) / ( freq * 1.0 ) );
      if ( reply_struct.orr_type_ == HFSAT::kORRType_Conf ) {
        overall_seq_stats_vec.push_back( latency ) ;
      }else if ( reply_struct.orr_type_ == HFSAT::kORRType_Cxld ){
        overall_cxlseq_stats_vec.push_back( latency ) ;
      }else if ( reply_struct.orr_type_ == HFSAT::kORRType_CxRe ) {
        overall_modseq_stats_vec.push_back ( latency );
      }

      if ( ((uint32_t)reply_struct.client_request_time_.tv_sec - first_time_point) > 900 ){
        first_time_point = reply_struct.client_request_time_.tv_sec ;
        PrintAndDumpVectorStats( rolling_15_min_stats_vec, timely_latency_outfile, first_time_point );
        rolling_15_min_stats_vec.clear ();
      }

      if ( 0 == last_ct_value ) {
        last_ct_value = ( reply_struct.client_request_time_.tv_sec * 1000000 ) + ( reply_struct.client_request_time_.tv_usec ) ;
      }else {
        uint64_t current_ct_value = ( reply_struct.client_request_time_.tv_sec * 1000000 ) + ( reply_struct.client_request_time_.tv_usec ) ;
        if ( current_ct_value == last_ct_value ) {
          double st_diff = ( reply_struct.ors_end_cycles_delta_ - last_st_value ) / ( freq * 1.0 ) ;

          if( st_diff > 100 ){

            std::cout << "1---> " << prev_struct.ToString () ;
            std::cout << "2---> " << reply_struct.ToString () ;

            std::cout << st_diff << " " << reply_struct.ors_end_cycles_delta_ << " " << last_st_value << " " << last_ct_value << " " << current_ct_value << std::endl ;
          }

          same_ct_orders_st_diff_vec.push_back( st_diff ) ;
        }
        last_ct_value = current_ct_value ;
      }

      last_st_value = reply_struct.ors_end_cycles_delta_;
      prev_struct = reply_struct ;

    }

    PrintAndDumpVectorStats( overall_latency_stats_vec, overall_latency_outfile );
    PrintAndDumpVectorStats( same_ct_orders_st_diff_vec, samect_latency_outfile );
    PrintAndDumpVectorStats ( overall_seq_stats_vec, seq_latency_outfile );
    PrintAndDumpVectorStats ( overall_cxlseq_stats_vec, cxlseq_latency_outfile );
    PrintAndDumpVectorStats ( overall_modseq_stats_vec, modifyseq_latency_outfile );
    PrintAndDumpVectorStats ( cs_stats_vec, cs_latency_outfile ); 
    PrintAndDumpVectorStats ( shm1_stats_vec, shm1_latency_outfile );
    PrintAndDumpVectorStats ( strat_stats_vec, strat_latency_outfile );
    PrintAndDumpVectorStats ( shm2_stats_vec, shm2_latency_outfile );
    PrintAndDumpVectorStats ( ors_stats_vec, ors_latency_outfile );
    
    if ( rolling_15_min_stats_vec.size () > 0 ) {
      PrintAndDumpVectorStats( rolling_15_min_stats_vec, timely_latency_outfile, last_ct_sec );
    }

  }

};

int main ( int argc, char *argv[] )
{

  if ( argc < 6 ) {
    std::cerr << "USAGE : <server> <saci-base> <srv-freq> <date> <file/s>" << std::endl ;
    std::exit(-1);
  }

  server = argv[1] ; 
  base_saci = atoi(argv[2]);
  freq = atoi(argv[3]);
  dt = argv[4];

  for ( int32_t counter = 5 ; counter < argc ; counter++ ) {
    files_list_vec.push_back(argv[counter]) ;
    FileStats stats;
    stats.ComputeAndDumpStats(argv[counter]) ;
  }

  OverallStats overallstats ;
  overallstats.ComputeAndDumpStats ();

  return EXIT_SUCCESS;
}				// ----------  end of function main  ----------
