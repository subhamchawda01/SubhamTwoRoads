// =====================================================================================
//
//       Filename:  calc_nse_span.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/04/2015 02:56:46 PM
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

#include <fstream>

#include "NSESpan/nse_span_calculator.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/file_utils.hpp"

std::string GetInstrumentFromShortcode(const std::string& shortcode){
        int first_underscore  = shortcode.find('_');
        int second_underscore = shortcode.substr(first_underscore+1).find('_');
        std::string instrument = shortcode.substr(first_underscore+1 , second_underscore);
        //std::cout<<instrument<<std::endl;
        return instrument;
}



int main(int argc, char** argv) {
  if (argc < 3) {
    std::cout << "USAGE: <exec name> <Trading date> <Shortcode Filepath>" << std::endl;
    std::cout << "Shortcode File format: \nNSE_NIFTY_PE_20160929_9100.00   -75\nNSE_SBIN_FUT_20160929	-3000"<< std::endl;
    std::cout << "Output : Span_Exposure" << std::endl;
    exit(0);
  }

  int tradindate = atoi(argv[1]);
  std::string filename = argv[2];
  std::map< std::string, std::pair<std::vector<std::string>, std::vector<int> > > underlying_to_shortcode_position_vec_pair;
  HFSAT::SecurityNameIndexer& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  if (HFSAT::FileUtils::ExistsAndReadable(filename)) {
    std::ifstream filereader(filename.c_str(), std::ifstream::in);
    if (filereader.is_open()) {
      while (filereader.good()) {
      
        int readlen = 1024;
        char buffer[readlen];
        filereader.getline(buffer, readlen);
        if (buffer[0]=='\0')
          break;
	
        HFSAT::PerishableStringTokenizer st(buffer, readlen);
        const std::vector<const char*>& tokens = st.GetTokens();
	
	if (tokens.size() > 1) {
          if (tokens[0][0] == '#') {
            continue;
          }
          std::string shortcode = std::string(tokens[0]);
         // std::cout << shortcode << std::endl;
          int pos = atoi(tokens[1]);
          std::string underlying = GetInstrumentFromShortcode(shortcode);
          (underlying_to_shortcode_position_vec_pair[underlying].first).push_back(shortcode);
	  (underlying_to_shortcode_position_vec_pair[underlying].second).push_back(pos);
          sec_name_indexer.AddString(shortcode.c_str(), shortcode);
        }
      }
    }
  }
  HFSAT::NSESpanCalculator nse_span_calc(tradindate, &sec_name_indexer);
  nse_span_calc.ReloadSpanData();
  nse_span_calc.ReloadExposureData();
  //std::cout << "Reloaded span data" << std::endl;
  float span_risk =0, exposure_margin=0;
  std::map< std::string, std::pair<std::vector<std::string>, std::vector<int> > >::iterator it; 
  for(it=underlying_to_shortcode_position_vec_pair.begin();it!=underlying_to_shortcode_position_vec_pair.end();it++){
	std::string underlying = it->first;
	std::vector<std::string> shortcode_vec = it->second.first;
	std::vector<int> position_vec  = it->second.second;
  	span_risk += nse_span_calc.GetSpanRisk(shortcode_vec, position_vec);
  	exposure_margin += nse_span_calc.GetExposureMargin(shortcode_vec, position_vec, underlying);
  }
  //std::cout << "Span margin = " << span_risk << std::endl;
  //std::cout << "Exposure margin = " << exposure_margin << std::endl;
  //std::cout << "Total margin = " << span_risk + exposure_margin << std::endl;
  std::cout << span_risk<<'_'<<exposure_margin;
  return 0;
}
