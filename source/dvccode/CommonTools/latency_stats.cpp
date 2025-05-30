// =====================================================================================
// 
//       Filename:  latency_stats.cpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  Tuesday 08 September 2020 07:03:27  UTC
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

#include <cstdlib>
#include <iostream>
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

//std::vector<int32_t> 
//std::vector<double> 
//
//uint32_t LoadLines (HFSAT::BulkFileReader & reader, int32_t & start_seq, int32_t & last_seq, std::vector<uint64_t> & oldcy, std::vector<double> & oldt, int32_t & last_seen) {
//
//  char buffer[1024];
//  int count=0; 
//  int ret_val=0;
//  int32_t last = 0;
//
//  for( int cnt=0; cnt<10000; cnt++){
//    oldcy[cnt] = 0;
//    oldt[cnt] = 0;
//  }
//  
//  while( true ) {
//
//    ret_val = reader.GetLine(buffer, 1024);
//    if ( ret_val <= 0 ) break;
//
//    HFSAT::PerishableStringTokenizer st(buffer, 1024);
//    const std::vector<const char *> &tokens = st.GetTokens();
//
//    int32_t packetseq = std::stoi(tokens[0], nullptr, sizeof(tokens[0]);
//    uint64_t cycles = strtoul(tokens[1], NULL, 0);
//    double time = std::stod(tokens[3], sizeof(tokens[3]));
//
//    oldcy[(packetseq-start_seq)] = cycles;
//    oldt[(packetseq-start_seq)] = time;
//
//    last_seq = packetseq;
//    if( last > last_seq )break;
//
//  }
//
//  last_seen = last ;
//
//  return ret_val;
//}
//
//void ComputeLatencyCycles( std::vector<uint64_t> & cycles1, std::vector<uint64_t> & cycles2 ){
//
// for( int count=0; count<10000; count++ ){
// 
// }
//
//}
//
//void ComputeLatencyTime( std::vector<double> & time1, std::vector<double> & time2 ){
//
//}
//

int main ( int argc, char *argv[] )
{

  HFSAT::BulkFileReader old_reader;
  HFSAT::BulkFileReader fpga_reader;

  old_reader.open(argv[1]);
  fpga_reader.open(argv[2]);

  std::map<int32_t, uint64_t> old_cycles;
  std::map<int32_t, double> old_time;

  std::map<int32_t, uint64_t> fpga_cycles;
  std::map<int32_t, double> fpga_time;


//  int32_t start_seq = std::stoi(std::string(argv[3])); 
//  int32_t old_last = 0 ;
//  int32_t fpga_last = 0 ;

  char buffer[1024];

  while ( true ){

    int ret_val = old_reader.GetLine(buffer, 1024);
    if ( ret_val <= 0 ) break;

    HFSAT::PerishableStringTokenizer st(buffer, 1024);
    const std::vector<const char *> &tokens = st.GetTokens();
    if( tokens.size() != 3 ) continue;

    std::string::size_type sz;

    int32_t packetseq = std::stoi(tokens[0], nullptr, sizeof(tokens[0]));
    uint64_t cycles = strtoul(tokens[1], NULL, 0);
    double time = std::stod(std::string(tokens[2]), &sz);

    old_cycles[packetseq] = cycles;
    old_time[packetseq] = time;

  }

  std::cout << "Sizeof : " << old_cycles.size() << " " << old_time.size() <<std::endl ;

//  while( LoadLines( old_reader, start_seq, start_seq+9999, old_cycles, old_time, old_last) && LoadLines (fpga_reader, start_seq, start_seq+9999, fpga_cycles, fpga_time, fpga_last) ){
//
//    ComputeLatencyCycles( old_cycles, fpga_cycles);
//    ComputeLatencyTime( old_time, fpga_time );
//
//    start_seq = old_last<fpga_last ? old_last : fpga_last;
//
//  }

  return EXIT_SUCCESS;
}				// ----------  end of function main  ----------
