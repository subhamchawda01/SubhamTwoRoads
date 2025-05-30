#include<dirent.h>
#include <algorithm>
#include <sstream>
#include <time.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <iterator>
#include <unordered_map>

#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp" 
#include "dvccode/CDef/mds_messages.hpp"

#define MIN_PER_DAY 1440
namespace fs = boost::filesystem;

template <class T>
class NSEBSELatencyCheck {
 public:
  static bool endsWith(const std::string &mainStr, const std::string &toMatch)
  {
    if(mainStr.size() >= toMatch.size() &&
       mainStr.compare(mainStr.size() - toMatch.size(), toMatch.size(), toMatch) == 0)
       return true;
    else
	return false;
  }
  static void eraseSubStr(std::string & mainStr, const std::string & toErase) {
    size_t pos = mainStr.find(toErase);
       if (pos != std::string::npos) {
           mainStr.erase(pos, toErase.length());
       }
  }

  static void EmailLatencyDiff(std::string alert_msg_) {
    // also send an alert
    HFSAT::Email e;
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    e.setSubject(" NSE Market Data Latency High On BSE");
    e.addRecepient("ravi.parikh@tworoads.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in");
    e.addSender("subham.chawda@tworoads-trading.co.in");
    e.content_stream << "exch: BSE" << "<br/>";
    e.content_stream << "host_machine: sdv-indb-srv11" << "<br/>";
    e.content_stream << "details : " << alert_msg_ << " <br/>";
    e.content_stream << "Current Time: " << ctime(&(current_time.tv_sec)) << " <br/>";
    e.sendMail();
  }

  static void CheckLatencyDifference(std::string& current_date,std::string& bse_path, std::string& nse_path, uint16_t& time_interval, 
                                     double& latency, std::string& start_time , std::string& end_time) {
    std::string current_file_date = current_date; 
    uint16_t time_interval_min = time_interval;
    double latency_micro = latency;
    std::tr1::unordered_map<int32_t, uint64_t> shortcode_seq_time_;
    std::vector < int64_t > overall_latency_vec;
    std::string bse_file_dir = bse_path;
    std::string nse_file_dir = nse_path;
    bool initial_start_flag = true;
    int hour, minutes;
    struct tm tm_last_update_time, tm_start_time, tm_end_time;

    hour = std::stoi(start_time.substr(4,2));
    minutes = std::stoi(start_time.substr(6,2));
    tm_last_update_time.tm_hour = hour;
    tm_last_update_time.tm_min = minutes;
    tm_start_time.tm_hour = hour;
    tm_start_time.tm_min = minutes;

    hour = std::stoi(end_time.substr(4,2));
    minutes = std::stoi(end_time.substr(6,2));
    tm_end_time.tm_hour = hour;
    tm_end_time.tm_min = minutes;


    while (true) {
      time_t current_time;
      time(&current_time);
      struct tm tm_current_time = *localtime(&current_time);

      if ( tm_current_time.tm_hour > tm_end_time.tm_hour) {
          std::cout << "RUNNING AFTER END TIME EXITING" << std::endl;
          exit(0) ;
      } else if ((tm_current_time.tm_hour == tm_end_time.tm_hour) && (tm_current_time.tm_min > tm_end_time.tm_min)) {
          std::cout << "RUNNING AFTER END TIME EXITING" << std::endl;
          exit(0) ;
      }

      if ( tm_current_time.tm_hour < tm_start_time.tm_hour ) {
          int sleep_min;
          if ( tm_start_time.tm_min == tm_current_time.tm_min ) 
            sleep_min = (tm_start_time.tm_hour - tm_current_time.tm_hour) * 60;
          else if ( tm_current_time.tm_min > tm_start_time.tm_min )
            sleep_min =((tm_start_time.tm_hour - tm_current_time.tm_hour - 1) * 60) + 60 - tm_current_time.tm_min + tm_start_time.tm_min;
          else
            sleep_min =((tm_start_time.tm_hour - tm_current_time.tm_hour) * 60) + tm_start_time.tm_min - tm_current_time.tm_min;
 
          std::cout << "GOING TO SLEEP FOR SEC: " << sleep_min * 60 << std::endl;
          sleep( sleep_min * 60 );
          continue ;
      } else if ((tm_current_time.tm_hour == tm_start_time.tm_hour) && (tm_current_time.tm_min < tm_start_time.tm_min)) {
          int sleep_min = tm_start_time.tm_min - tm_current_time.tm_min;
          std::cout << "GOING TO SLEEP FOR SEC: " << sleep_min * 60 << std::endl;
          sleep( sleep_min * 60 );
          continue ;
      }

      for (const auto & entry : fs::directory_iterator(bse_file_dir)){
	if(endsWith(entry.path().string(),current_file_date+".gz") || endsWith(entry.path().string(),current_file_date)){
           std::string file_path_ = entry.path().string();
           boost::replace_all(file_path_, bse_file_dir, nse_file_dir); 
           HFSAT::BulkFileReader bulk_file_reader_bse_;
           bulk_file_reader_bse_.open(entry.path().string());
  	   //std::cout<<"file opened: " << entry.path().string() << std::endl;
	   T next_event_;
      	   if (bulk_file_reader_bse_.is_open()) {
       		while (true) {
            		size_t available_len_ = bulk_file_reader_bse_.read(&next_event_, sizeof(T));
            		if (available_len_ < sizeof(next_event_)) break;
                        time_t time = (time_t)next_event_.generic_data_.nse_data_.source_time.tv_sec ;
                        struct tm tm_curr = *localtime(&time);

                        if ( tm_curr.tm_hour < tm_start_time.tm_hour ) continue ;
                        else if ( (tm_curr.tm_hour == tm_start_time.tm_hour) && (tm_curr.tm_min < tm_start_time.tm_min) ) continue;

                        if ( tm_curr.tm_hour > tm_current_time.tm_hour ) continue;
                        else if ( (tm_curr.tm_hour == tm_current_time.tm_hour) && (tm_curr.tm_min >= tm_current_time.tm_min) ) continue;

                        if (!initial_start_flag) {
                          if ( tm_curr.tm_hour < tm_last_update_time.tm_hour ) continue ;
                          else if ( (tm_curr.tm_hour == tm_last_update_time.tm_hour) && (tm_curr.tm_min <= tm_last_update_time.tm_min) ) continue;
                        }

                        uint64_t time_stamp = (next_event_.generic_data_.nse_data_.source_time.tv_sec * 1000000) + next_event_.generic_data_.nse_data_.source_time.tv_usec;

                        shortcode_seq_time_[next_event_.generic_data_.nse_data_.msg_seq] = time_stamp;
		}
       		 bulk_file_reader_bse_.close();
		 //std::cout<<"file closed\n";
	   }
           HFSAT::BulkFileReader bulk_file_reader_nse_;
           bulk_file_reader_nse_.open(file_path_);
  	   //std::cout<<"file opened: " << file_path_ << std::endl;
      	   if (bulk_file_reader_nse_.is_open()) {
       		while (true) {
            		size_t available_len_ = bulk_file_reader_nse_.read(&next_event_, sizeof(T));
            		if (available_len_ < sizeof(next_event_)) break;
                        time_t time = (time_t)next_event_.generic_data_.nse_data_.source_time.tv_sec ;
                        struct tm tm_curr = *localtime(&time);


                        if ( tm_curr.tm_hour < tm_start_time.tm_hour ) continue ;
                        else if ( (tm_curr.tm_hour == tm_start_time.tm_hour) && (tm_curr.tm_min < tm_start_time.tm_min) ) continue;

                        if ( tm_curr.tm_hour > tm_current_time.tm_hour ) continue;
                        else if ( (tm_curr.tm_hour == tm_current_time.tm_hour) && (tm_curr.tm_min >= tm_current_time.tm_min) ) continue;

                        if (!initial_start_flag) {
                          if ( tm_curr.tm_hour < tm_last_update_time.tm_hour ) continue ;
                          else if ( (tm_curr.tm_hour == tm_last_update_time.tm_hour) && (tm_curr.tm_min <= tm_last_update_time.tm_min) )  continue;
                        }

                        uint64_t time_stamp = (next_event_.generic_data_.nse_data_.source_time.tv_sec * 1000000) + next_event_.generic_data_.nse_data_.source_time.tv_usec;
                        int64_t time_diff = shortcode_seq_time_[next_event_.generic_data_.nse_data_.msg_seq] - time_stamp;
                        if (time_diff > 0) {
                            overall_latency_vec.push_back(time_diff);
                        }

		}
       		bulk_file_reader_nse_.close();
		//std::cout<<"file closed\n";
	   }
           std::sort(overall_latency_vec.begin(), overall_latency_vec.end());
           double median_latency = overall_latency_vec[overall_latency_vec.size()*0.5];
           double latency_diff = median_latency - latency_micro;
           std::cout << "Current_Latency: " << median_latency << " Reference_Latency: " 
                     << latency_micro << " Latency_Diff: " << latency_diff << " TCount: " << overall_latency_vec.size() 
                     << " Time: " << tm_last_update_time.tm_hour << ":" << tm_last_update_time.tm_min 
                     << " - " << tm_current_time.tm_hour << ":" << tm_current_time.tm_min 
                     << " " << file_path_ << std::endl;

           if ( (latency_diff > 3) && (overall_latency_vec.size() > 0) ) {
             EmailLatencyDiff(std::string("Current_Latency: " + std::to_string(median_latency) + 
                                          " Reference_Latency: " + std::to_string(latency_micro) + " TCount: " + std::to_string(overall_latency_vec.size()) + 
                                          " Time: " + std::to_string(tm_last_update_time.tm_hour) + ":" + std::to_string(tm_last_update_time.tm_min) +
                                          " - " + std::to_string(tm_current_time.tm_hour) + ":" + std::to_string(tm_current_time.tm_min)));
           }
        }
        overall_latency_vec.clear();
        shortcode_seq_time_.clear();
      }
      tm_last_update_time = tm_current_time;
      initial_start_flag = false;
      std::cout << "GOING TO SLEEP FOR SEC: " << time_interval * 60 << std::endl;
      sleep( time_interval_min * 60 );
    }
  }
};

int main(int argc, char** argv) {
  if (argc != 8) {
    std::cout
    << " USAGE: EXEC <date> <bse_file_path> <nse_file_path> <time_interval_min> <latency_micros> <start_time[GMT_0345]> <end_time[GMT_1000]"
    << std::endl;
    exit(0);
  }

  std::string date_ = argv[1];
  std::string bse_file_path_ = argv[2];
  std::string nse_file_path_ = argv[3];
  uint16_t time_interval_min_ = atoi(argv[4]);
  double latency_micros_ = atof(argv[5]);
  std::string start_time = argv[6];
  std::string end_time = argv[7];

  NSEBSELatencyCheck<HFSAT::MDS_MSG::GenericMDSMessage>::CheckLatencyDifference(date_, bse_file_path_, nse_file_path_, time_interval_min_,
                                                                                latency_micros_, start_time, end_time); 
  
}
