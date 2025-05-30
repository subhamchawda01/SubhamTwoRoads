// =====================================================================================
// 
//       Filename:  bardata_logged_message_filenamer.hpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  11/23/2022 06:30:01 AM
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

#pragma once

#include <string>
#include <sstream>

#include "dvccode/CDef/file_utils.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"

namespace HFSAT {

  class OIBarDataLoggedFileNamer {

    public :

      static std::string GetName(const char* _bardata_symbol_, int32_t yyyymmdd=0){

        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "/spare/local/"; 
         if(0 == yyyymmdd){
          t_temp_oss_ << "OI_BARDATA/";
         }else{

          int year = (yyyymmdd / 10000) % 10000;
          int mon = (yyyymmdd / 100) % 100;
	        int day = (yyyymmdd % 100);

              std::ostringstream check_temp_oss_;
              check_temp_oss_ << t_temp_oss_.str();
              check_temp_oss_ << "OI_BARDATA/SPLIT_Bardata/";
              check_temp_oss_ << year << "/" << std::setw(2) << std::setfill('0') << mon << "/" << std::setw(2) << std::setfill('0') << day << "/" ;
              check_temp_oss_ << _bardata_symbol_ ;

              if(true == HFSAT::FileUtils::ExistsAndReadable(check_temp_oss_.str())){
                t_temp_oss_ << "OI_BARDATA/SPLIT_Bardata/";
                t_temp_oss_ << year << "/" << std::setw(2) << std::setfill('0') << mon << "/" << std::setw(2) << std::setfill('0') << day << "/";
              }else{
                std::cout << "BARDATA SPLIT FILE NOT AVAILABLE : " << _bardata_symbol_ << " " << yyyymmdd << std::endl;
               // std::exit(-1);
                t_temp_oss_ << "OI_BARDATA/";
              }
        }
        
        t_temp_oss_ << _bardata_symbol_;
        return t_temp_oss_.str();

      }
  };

  class BseOiBarDataLoggedFileNamer {

    public :

      static std::string GetName(const char* _bardata_symbol_, int32_t yyyymmdd=0){

        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "/spare/local/";

        
            //default using full file                        
            if(0 == yyyymmdd){
              t_temp_oss_ << "OI_BARDATA_BSE/";
            }else{

              int year = (yyyymmdd / 10000) % 10000;
              int mon = (yyyymmdd / 100) % 100;
	      int day = (yyyymmdd % 100);

              std::ostringstream check_temp_oss_;
              check_temp_oss_ << t_temp_oss_.str();
              check_temp_oss_ << "OI_BARDATA_BSE/SPLIT_Bardata/";
              check_temp_oss_ << year << "/" << std::setw(2) << std::setfill('0') << mon << "/" << std::setw(2) << std::setfill('0') << day << "/" ;
              check_temp_oss_ << _bardata_symbol_ ;

              if(true == HFSAT::FileUtils::ExistsAndReadable(check_temp_oss_.str())){
                t_temp_oss_ << "OI_BARDATA_BSE/SPLIT_Bardata/";
                t_temp_oss_ << year << "/" << std::setw(2) << std::setfill('0') << mon << "/" << std::setw(2) << std::setfill('0') << day << "/";
              }else{
                std::cout << "BARDATA SPLIT FILE NOT AVAILABLE : " << _bardata_symbol_ << " " << yyyymmdd << std::endl;
               // std::exit(-1);
//                t_temp_oss_ << "INDEX_BARDATA/WEEKLYOPT/";
              }

//	      std::cout << HFSAT::FileUtils::ExistsAndReadable(check_temp_oss_.str()) << " " << t_temp_oss_.str() << std::endl;
          
        }

        t_temp_oss_ << _bardata_symbol_;
        return t_temp_oss_.str();

      }
  };

}
