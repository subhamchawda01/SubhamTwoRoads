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

  typedef enum { kInstrumentTypeCash = 0, kInstrumentTypeFUT, kInstrumentTypeOPT, kInstrumentTypeSpot } InstrumentType_t;

  class BarDataLoggedFileNamer {

    public :

      static InstrumentType_t GetInstrumentTypeFromBarDataSymbol(const char* _shortcode_){

        InstrumentType_t t_temp = kInstrumentTypeCash;
        NSEInstrumentType_t t_nse_inst = NSESecurityDefinitions::GetInstrumentTypeFromShortCode(_shortcode_);

        if(NSE_IDXFUT == t_nse_inst || NSE_STKFUT == t_nse_inst){
          t_temp = kInstrumentTypeFUT;
        }else if(NSE_IDXOPT == t_nse_inst || NSE_STKOPT == t_nse_inst){
          t_temp = kInstrumentTypeOPT;
        }else if(NSE_IDXSPT == t_nse_inst){
          t_temp = kInstrumentTypeSpot;		
	}

        return t_temp;
      }

      static std::string GetName(InstrumentType_t _instrument_type_, const char* _bardata_symbol_, int32_t yyyymmdd=0){

        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "/spare/local/"; 

        switch(_instrument_type_){
          case kInstrumentTypeCash:{
            t_temp_oss_ << "CASH_BarData/";
          }break;
          case kInstrumentTypeFUT:{
            t_temp_oss_ << "BarData/";
          }break;
	  case kInstrumentTypeSpot:{
            t_temp_oss_ << "BarData_SPOT/";
          }break;					   
          case kInstrumentTypeOPT:{
            //default using full file                        
            if(0 == yyyymmdd){
              t_temp_oss_ << "INDEX_BARDATA/WEEKLYOPT/";
            }else{

              int year = (yyyymmdd / 10000) % 10000;
              int mon = (yyyymmdd / 100) % 100;
	      int day = (yyyymmdd % 100);

              std::ostringstream check_temp_oss_;
              check_temp_oss_ << t_temp_oss_.str();
              check_temp_oss_ << "INDEX_BARDATA/SPLIT_WEEKLYOPT/";
              check_temp_oss_ << year << "/" << std::setw(2) << std::setfill('0') << mon << "/" << std::setw(2) << std::setfill('0') << day << "/" ;
              check_temp_oss_ << _bardata_symbol_ ;

              if(true == HFSAT::FileUtils::ExistsAndReadable(check_temp_oss_.str())){
                t_temp_oss_ << "INDEX_BARDATA/SPLIT_WEEKLYOPT/";
                t_temp_oss_ << year << "/" << std::setw(2) << std::setfill('0') << mon << "/" << std::setw(2) << std::setfill('0') << day << "/";
              }else{
                std::cout << "BARDATA SPLIT FILE NOT AVAILABLE : " << _bardata_symbol_ << " " << yyyymmdd << std::endl;
                std::exit(-1);
//                t_temp_oss_ << "INDEX_BARDATA/WEEKLYOPT/";
              }

//	      std::cout << HFSAT::FileUtils::ExistsAndReadable(check_temp_oss_.str()) << " " << t_temp_oss_.str() << std::endl;
            }
          }break;
          default:break;
        }

        t_temp_oss_ << _bardata_symbol_;
        return t_temp_oss_.str();

      }
  };

  class BseBarDataLoggedFileNamer {

    public :

      static InstrumentType_t GetInstrumentTypeFromBarDataSymbol(const char* _shortcode_){

        InstrumentType_t t_temp = kInstrumentTypeCash;
        BSEInstrumentType_t t_bse_inst = BSESecurityDefinitions::GetInstrumentTypeFromShortCode(_shortcode_);

        if(BSE_IF == t_bse_inst || BSE_SF == t_bse_inst){
          t_temp = kInstrumentTypeFUT;
        }else if(BSE_IO == t_bse_inst || BSE_SO == t_bse_inst){
          t_temp = kInstrumentTypeOPT;               
        }else if(BSE_IDXSPT == t_bse_inst){
          t_temp = kInstrumentTypeSpot;               
        }

        return t_temp;
      }

      static std::string GetName(InstrumentType_t _instrument_type_, const char* _bardata_symbol_, int32_t yyyymmdd=0){

        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "/spare/local/";

        switch(_instrument_type_){
          case kInstrumentTypeCash:{
            t_temp_oss_ << "CASH_BarData/";
          }break;
          case kInstrumentTypeFUT:{
            t_temp_oss_ << "BarData/";
          }break;
          case kInstrumentTypeSpot:{
            t_temp_oss_ << "BarData_SPOT/";
          }break;
          case kInstrumentTypeOPT:{
            //default using full file                        
            if(0 == yyyymmdd){
              t_temp_oss_ << "INDEX_BARDATA_BSE/WEEKLYOPT/";
            }else{

              int year = (yyyymmdd / 10000) % 10000;
              int mon = (yyyymmdd / 100) % 100;
	      int day = (yyyymmdd % 100);

              std::ostringstream check_temp_oss_;
              check_temp_oss_ << t_temp_oss_.str();
              check_temp_oss_ << "INDEX_BARDATA_BSE/SPLIT_WEEKLYOPT/";
              check_temp_oss_ << year << "/" << std::setw(2) << std::setfill('0') << mon << "/" << std::setw(2) << std::setfill('0') << day << "/" ;
              check_temp_oss_ << _bardata_symbol_ ;

              if(true == HFSAT::FileUtils::ExistsAndReadable(check_temp_oss_.str())){
                t_temp_oss_ << "INDEX_BARDATA_BSE/SPLIT_WEEKLYOPT/";
                t_temp_oss_ << year << "/" << std::setw(2) << std::setfill('0') << mon << "/" << std::setw(2) << std::setfill('0') << day << "/";
              }else{
                std::cout << "BARDATA SPLIT FILE NOT AVAILABLE : " << _bardata_symbol_ << " " << yyyymmdd << std::endl;
                std::exit(-1);
//                t_temp_oss_ << "INDEX_BARDATA/WEEKLYOPT/";
              }

//	      std::cout << HFSAT::FileUtils::ExistsAndReadable(check_temp_oss_.str()) << " " << t_temp_oss_.str() << std::endl;
            }
          }break;
          default:break;
        }

        t_temp_oss_ << _bardata_symbol_;
        return t_temp_oss_.str();

      }
  };

}
