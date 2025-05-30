#pragma once
#include <dirent.h>
#include <algorithm>
#include <sstream>
#include <time.h>
#include <boost/filesystem.hpp>
#include <iterator>
#include <string>

#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

#define MIN_PER_DAY 1440
namespace fs = boost::filesystem;
namespace HFSAT {
class ThrottleInfo {
 protected:
  std::vector<ORRType_t> orr_type_;  ///< b(4) type of event that happened
  int client_id_;
  int throttle_value_[MIN_PER_DAY];  ///< b(4) the size of the order that is live in market
  int OF_0_min_[MIN_PER_DAY];  ///< b(4) the size of the order that is live in market
  int OF_1_min_[MIN_PER_DAY];  ///< b(4) the size of the order that is live in market
  double order_flag_0_, order_flag_1_, order_flag_0_max_,order_flag_1_max_;
  double average_per_min_, median_;
  int size_, sum_;
  int start_min, end_min;
  int max_throttle, max_throttle_OF_0, max_throttle_OF_1;
  int cx_re, cx_re_rejc, rejc;
  int seqd, exec, cxld;
  int max_min;
  int conf, cxLD, cxre;
  std::map<int, int> server_client_order_seq;
  double exec_seqd, cxld_seqd;

 public:
  ThrottleInfo(){};
  ThrottleInfo(int clientId)
      : client_id_(clientId),
        order_flag_0_(0),
        order_flag_1_(0),
        order_flag_0_max_(0),
        order_flag_1_max_(0),
        average_per_min_(0),
        size_(0),
        sum_(0),
        start_min(-1),
        end_min(-1),
        max_throttle(0),
        max_throttle_OF_0(0),
        max_throttle_OF_1(0),
        cx_re(0),
        cx_re_rejc(0),
        rejc(0),
        seqd(0),
        exec(0),
        cxld(0),
        max_min(0),
        conf(0),
        cxLD(0),
        cxre(0) {
    for (int i = 0; i < MIN_PER_DAY; i++) {
      throttle_value_[i] = 0;
      OF_0_min_[i] = 0;
      OF_1_min_[i] = 0;
    }
  }

  void Calculate() {
    if (sum_ > 0) {
      return;
    }
    int temp_throttle[MIN_PER_DAY];
    for (int i = 0; i < MIN_PER_DAY; i++) {
      if (throttle_value_[i] != 0) {
        if (start_min == -1) {
          start_min = i;
        }
        end_min = i;
        sum_ += throttle_value_[i];
        if (throttle_value_[i] > max_throttle) {
          max_throttle = throttle_value_[i];
          max_throttle_OF_0 = OF_0_min_[i];
          max_throttle_OF_1 = OF_1_min_[i];
          max_min = i;
        }
      }
    }
    if (end_min != -1) average_per_min_ = (double)sum_ / (double)(end_min - start_min);
    memcpy((void *)temp_throttle, (void *)(throttle_value_ + start_min), (end_min - start_min) * sizeof(int));
    std::sort(temp_throttle, temp_throttle + (end_min - start_min));
    Find_median(temp_throttle);

    exec_seqd = (exec / (double)seqd) * 100;
    cxld_seqd = (cxld / (double)seqd) * 100;
  }

  void Find_median(int array[]) {
    int n = end_min - start_min;
    if (n % 2 == 0) median_ = (array[(n - 1) / 2] + array[n / 2]) / 2.0;
    // if number of elements are odd
    else
      median_ = array[n / 2];
  }

  std::string ToString() {
    std::stringstream ss;
    ss << "Client_Id: " << client_id_ << "\tMean: " << average_per_min_ << "\tMedian: " << median_
       << "\tTotal: " << sum_ << "\tMaxThrottle/min: " << max_throttle 
       << "\tMax_OF_0: " << max_throttle_OF_0 << "\tMax_OF_1: " << max_throttle_OF_1 << "\tOP_0: " << order_flag_0_
       << "\tOP_1: " << order_flag_1_ << "\tCxRe: " << cx_re << "\tCxReRejc: " << cx_re_rejc
       << "\tStart Time: " << start_min / 60 << ":" << start_min % 60 << "\tEnd Time: " << end_min / 60 << ":"
       << end_min % 60 << "\tMax Min: " << max_min / 60 << ":" << max_min % 60 << "\n";
    return ss.str();
  }
  void ToFile(std::ofstream &fs) {
    fs << "Client_Id: " << client_id_ << "\tMean: " << average_per_min_ << "\tMedian: " << median_
       << "\tTotal: " << sum_ << "\tMaxThrottle/min: " << max_throttle 
       << "\tMax_OF_0: " << max_throttle_OF_0 << "\tMax_OF_1: " << max_throttle_OF_1 << "\tOP_0: " << order_flag_0_
       << "\tOF_1: " << order_flag_1_ << "\tCxRe: " << cx_re << "\tCxReRejc: " << cx_re_rejc << "\tRejc: " << rejc
       << "\tStartTime: " << start_min / 60 << ":" << start_min % 60 << "\tEndTime: " << end_min / 60 << ":"
       << end_min % 60 << "\tSeqd: " << seqd << "\tExec: " << exec << "\tCxld: " << cxld << "\tRatioExec: " << exec_seqd
       << "\tRationCxld: " << cxld_seqd << "\tMax Min: " << max_min / 60 << ":" << max_min % 60 << "\n";
  }
  void ToProductFile(std::ofstream &fs) {
    fs << "Conf: " << conf << " Cxld: " << cxLD << " CxRe: " << cxre << " Total: " << sum_ << " OF_0: " << order_flag_0_
       << " OF_1: " << order_flag_1_ << "\n";
  }

  void PushData(int size_exec, ttime_t client_request_time, int32_t order_flags, ORRType_t order_type, int server,
                int client) {
    if (size_exec == 9 || size_exec == 7) {
      // std::cout<<"before local time cal"<<client_request_time.tv_sec<<"\n";
      time_t t_ptr = client_request_time.tv_sec;
      // std::cout<<localtime(t_ptr)<<std::endl;
      struct tm *tmp = localtime(&t_ptr);
      // std::cout<<"after local time cal "<<tmp<<"\n";
      int minutes = tmp->tm_hour * 60 + tmp->tm_min;
      // std::cout<<minutes<<"minutes cal"<<std::endl;
      // order_type;
      throttle_value_[minutes]++;
      size_++;
      if (order_flags == 0) {
        order_flag_0_++;
        OF_0_min_[minutes]++;
      }
      else {
        order_flag_1_++;
        OF_1_min_[minutes]++;
      }
      if (strcmp(HFSAT::ToString(order_type), "CxlRejc") == 0)
        cx_re++;
      else if (strcmp(HFSAT::ToString(order_type), "CxReRejc") == 0)
        cx_re_rejc++;
      else if (strcmp(HFSAT::ToString(order_type), "Rejc") == 0)
        rejc++;
    }

    if (strcmp(HFSAT::ToString(order_type), "Conf") == 0)
      conf++;
    else if (strcmp(HFSAT::ToString(order_type), "Cxld") == 0)
      cxLD++;
    else if (strcmp(HFSAT::ToString(order_type), "CxRe") == 0)
      cxre++;

    if (order_flags == 1) {
      // std::cout<<HFSAT::ToString(order_type)<<std::endl;
      if (strcmp(HFSAT::ToString(order_type), "Seqd") == 0) {
        server_client_order_seq[server] = client;
        seqd++;

      } else if (strcmp(HFSAT::ToString(order_type), "Exec") == 0) {
        if (server_client_order_seq.find(server) != server_client_order_seq.end() &&
            server_client_order_seq[server] == client) {
          exec++, server_client_order_seq.erase(server);
        }

      } else if (strcmp(HFSAT::ToString(order_type), "Cxld") == 0) {
        if (server_client_order_seq.find(server) != server_client_order_seq.end() &&
            server_client_order_seq[server] == client) {
          cxld++, server_client_order_seq.erase(server);
        }
      }
    }
  }
};

template <class T>
class ThrottleLogReader {
 public:
  static bool endsWith(const std::string &mainStr, const std::string &toMatch) {
    if (mainStr.size() >= toMatch.size() &&
        mainStr.compare(mainStr.size() - toMatch.size(), toMatch.size(), toMatch) == 0)
      return true;
    else
      return false;
  }
  static void eraseSubStr(std::string &mainStr, const std::string &toErase) {
    size_t pos = mainStr.find(toErase);
    if (pos != std::string::npos) {
      mainStr.erase(pos, toErase.length());
    }
  }

  static std::string SplitFilename (const std::string& str){
    std::size_t found = str.find_last_of("/\\");
    return str.substr(found+1);
  }

  static void ReadMDSStructsFiles(std::string date_of_file, std::string dir_path, std::string exchange) {
    bool flag_exist = false;
    std::string output_file_dir_product =
        "/home/dvcinfra/important/ThrottleProject/product_throttle_generated/" + date_of_file;
    std::string output_file_dir_overall = "/home/dvcinfra/important/ThrottleProject/throttle_generated/" + date_of_file;
    std::cout<<"date_of_file " <<  date_of_file <<" dir_path " << dir_path <<std::endl;
    mkdir(output_file_dir_product.c_str(), 0777);
    if ( exchange == "BSE" )
      HFSAT::SecurityDefinitions::GetUniqueInstance(std::stoi(date_of_file)).LoadBSESecurityDefinitions();
    else
      HFSAT::SecurityDefinitions::GetUniqueInstance(std::stoi(date_of_file)).LoadNSESecurityDefinitions();
    std::map<int, HFSAT::ThrottleInfo *> client_id_throttle_map_;

    // std::vector<std::string> dir_paths={"/run/media/dvcinfra/BACKUP/ORSBCAST_MULTISHM_BKP"};
    std::vector<std::string> dir_paths = {dir_path};
    // std::string dir_paths[]={"/run/media/root/BACKUP/ORSBCAST_MULTISHM_Q19"
    // ,"/run/media/root/BACKUP/ORSBCAST_MULTISHM_S7"};
    for (unsigned int dir_path = 0; dir_path < dir_paths.size(); dir_path++) {
      std::cout << "dir " << dir_paths[dir_path] << std::endl;
      for (const auto &entry : fs::directory_iterator(dir_paths[dir_path])) {
        if (endsWith(entry.path().string(), date_of_file + ".gz")) {
          std::cout << "Path file: " << entry.path().string() << std::endl;
          std::map<int, HFSAT::ThrottleInfo *> client_id_shortcode_throttle_map_;
          std::string dump_file_name = entry.path().string();
          eraseSubStr(dump_file_name, "_" + date_of_file + ".gz");
          eraseSubStr(dump_file_name, dir_paths[dir_path] + "/");
          std::string shortcode = "";
          std::string file_name = SplitFilename(dump_file_name);
          // std::cout<<"DUmp FILE NAME "<<  file_name << " " << HFSAT::BSESecurityDefinitions::GetShortCodeFromExchangeSymbol(file_name)<< std::endl;
          if ( exchange == "BSE" ) {
              shortcode = file_name;
          }
          else {
            if (file_name.find("BSE_") == std::string::npos)
              shortcode = HFSAT::BSESecurityDefinitions::GetShortCodeFromExchangeSymbol(file_name);
            else if (file_name.find("NSE_") == std::string::npos)
              shortcode = HFSAT::NSESecurityDefinitions::GetShortCodeFromExchangeSymbol(file_name);
            else
              shortcode = file_name;
          }

          std::cout << "shortcode: " << shortcode << std::endl;
          HFSAT::BulkFileReader bulk_file_reader_;
          bulk_file_reader_.open(entry.path().string());
          std::cout << "file opened:\n";
          T next_event_;
          if (bulk_file_reader_.is_open()) {
            while (true) {
              size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(T));
              if (available_len_ < sizeof(next_event_)) break;
              int client_id = next_event_.server_assigned_client_id_ >> 16;
              if (client_id_throttle_map_.find(client_id) == client_id_throttle_map_.end()) {
                client_id_throttle_map_[client_id] = new HFSAT::ThrottleInfo(client_id);
              }
              if (client_id_shortcode_throttle_map_.find(client_id) == client_id_shortcode_throttle_map_.end()) {
                client_id_shortcode_throttle_map_[client_id] = new HFSAT::ThrottleInfo(client_id);
              }

              // if(client_id==340&&(next_event_.size_executed_==9||next_event_.size_executed_==7) ) std::cout<<"Path
              // file: "<<entry.path().string()<<std::endl;
              flag_exist = true;

              client_id_throttle_map_[client_id]->PushData(next_event_.size_executed_, next_event_.client_request_time_,
                                                           next_event_.order_flags_, next_event_.orr_type_,
                                                           next_event_.server_assigned_order_sequence_,
                                                           next_event_.client_assigned_order_sequence_);
              client_id_shortcode_throttle_map_[client_id]->PushData(
                  next_event_.size_executed_, next_event_.client_request_time_, next_event_.order_flags_,
                  next_event_.orr_type_, next_event_.server_assigned_order_sequence_,
                  next_event_.client_assigned_order_sequence_);
            }
            std::cout << "while executed\n";
            bulk_file_reader_.close();
            std::cout << "file closed\n";
          }
          if (flag_exist == true) {
            for (auto it = client_id_shortcode_throttle_map_.begin(); it != client_id_shortcode_throttle_map_.end();
                 it++) {
              std::ofstream fs;
              std::string output_file = std::to_string(it->first) + "_" + shortcode + "_" + date_of_file;
              fs.open(output_file_dir_product + "/" + output_file);
              it->second->Calculate();
              it->second->ToProductFile(fs);
              fs.close();
              std::cout << "product dump to file : " << output_file_dir_product << std::endl;
            }
          }
        }
      }
    }
    if (flag_exist == true) {
      std::ofstream fs;
      fs.open(output_file_dir_overall);
      std::cout << "overall dump to file : " << output_file_dir_overall << std::endl;
      for (auto it = client_id_throttle_map_.begin(); it != client_id_throttle_map_.end(); it++) {
        it->second->Calculate();
        it->second->ToFile(fs);
      }
      fs.close();
    }

    //	for (auto it=client_id_throttle_map_.begin(); it!=client_id_throttle_map_.end(); it++){
    //     		it->second->Calculate();
    //     		std:: cout<<it->second->ToString();
    //   		}
  }

  /*
    static void ReadMDSStructsFilesPNL(std::string date_of_file) {
      bool flag_exist=false;
      HFSAT::SecurityDefinitions::GetUniqueInstance(std::stoi(date_of_file)).LoadBSESecurityDefinitions();
      HFSAT::SecurityDefinitions::GetUniqueInstance(std::stoi(date_of_file)).LoadNSESecurityDefinitions();

     //std::vector<std::string> dir_paths={"/run/media/dvcinfra/BACKUP/ORSBCAST_MULTISHM_BKP"};
      std::vector<std::string>
  dir_paths={"/run/media/dvcinfra/BACKUP/ORSBCAST_MULTISHM_Q19","/run/media/dvcinfra/BACKUP/ORSBCAST_MULTISHM_S7"};
     //std::string dir_paths[]={"/run/media/root/BACKUP/ORSBCAST_MULTISHM_Q19"
  ,"/run/media/root/BACKUP/ORSBCAST_MULTISHM_S7"}; for ( unsigned int dir_path=0;dir_path<dir_paths.size();dir_path++){
           std::cout<<"dir "<<dir_paths[dir_path]<<std::endl;
        for (const auto & entry : fs::directory_iterator(dir_paths[dir_path])){
          if(endsWith(entry.path().string(),date_of_file+".gz")){
           std::cout<<"Path file: "<<entry.path().string()<<std::endl;
           std::map<int,HFSAT::ThrottleInfo*> client_id_throttle_map_;
           std::string dump_file_name = entry.path().string();
           eraseSubStr(dump_file_name,"_"+date_of_file+".gz");
           eraseSubStr(dump_file_name,dir_paths[dir_path]+"/");
           std::string shortcode = "";
           if ( dump_file_name.find("BSE_") == std::string::npos)
             shortcode = HFSAT::BSESecurityDefinitions::GetShortCodeFromExchangeSymbol(dump_file_name);
           else
             shortcode = dump_file_name;
           std::cout << "shortcode: " << shortcode << std::endl;
           HFSAT::BulkFileReader bulk_file_reader_;
           bulk_file_reader_.open(entry.path().string());
           std::cout<<"file opened:\n";
           T next_event_;
           if (bulk_file_reader_.is_open()) {
                  while (true) {
                          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(T));
                          if (available_len_ < sizeof(next_event_)) break;
                          int client_id=next_event_.server_assigned_client_id_>>16;
                          if(client_id_throttle_map_.find(client_id)==client_id_throttle_map_.end()){
                                  client_id_throttle_map_[client_id]=new HFSAT::ThrottleInfo(client_id);
                          }

                          //if(client_id==340&&(next_event_.size_executed_==9||next_event_.size_executed_==7) )
  std::cout<<"Path file: "<<entry.path().string()<<std::endl; flag_exist=true;

                                  client_id_throttle_map_[client_id]->PushData(next_event_.size_executed_,next_event_.client_request_time_,next_event_.order_flags_,next_event_.orr_type_,next_event_.server_assigned_order_sequence_,next_event_.client_assigned_order_sequence_);

                  }
                   std::cout<<"while executed\n";
                   bulk_file_reader_.close();
                   std::cout<<"file closed\n";
           }
           if(flag_exist==true){
             for (auto it=client_id_throttle_map_.begin(); it!=client_id_throttle_map_.end(); it++){
                  std::ofstream fs;
                  std::string output_file = std::to_string(it->first)+"_"+shortcode+"_"+date_of_file;
                  fs.open(output_file);
                  it->second->Calculate();
                  it->second->ToProductFile(fs);
                  fs.close();
                  std::cout <<"data dump to file : " << output_file << std::endl;
             }
           }
          }
        }
      }

  //	for (auto it=client_id_throttle_map_.begin(); it!=client_id_throttle_map_.end(); it++){
  //     		it->second->Calculate();
  //     		std:: cout<<it->second->ToString();
  //   		}
    }
  */
};

// Class in which changes has to be made

template <class T>
class IndLogReader {
 public:
  static bool endsWith(const std::string &mainStr, const std::string &toMatch) {
    if (mainStr.size() >= toMatch.size() &&
        mainStr.compare(mainStr.size() - toMatch.size(), toMatch.size(), toMatch) == 0)
      return true;
    else
      return false;
  }
  static void eraseSubStr(std::string &mainStr, const std::string &toErase) {
    size_t pos = mainStr.find(toErase);
    if (pos != std::string::npos) {
      mainStr.erase(pos, toErase.length());
    }
  }
  static void ReadMDSStructsFile(std::string file_path, std::string date_of_file, std::string server_name,
                                 HFSAT::BulkFileReader &bulk_file_reader_, std::string& start_time , std::string& end_time) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(std::stoi(date_of_file)).LoadBSESecurityDefinitions();
    HFSAT::SecurityDefinitions::GetUniqueInstance(std::stoi(date_of_file)).LoadNSESecurityDefinitions();

    int hour, minutes;
    struct tm tm_start_time, tm_end_time;
    hour = std::stoi(start_time.substr(4,2));
    minutes = std::stoi(start_time.substr(6,2));
    tm_start_time.tm_hour = hour;
    tm_start_time.tm_min = minutes;

    hour = std::stoi(end_time.substr(4,2));
    minutes = std::stoi(end_time.substr(6,2));
    tm_end_time.tm_hour = hour;
    tm_end_time.tm_min = minutes;

    std::string dir_path = file_path;

    struct stat buffer;
    if ((stat(dir_path.c_str(), &buffer) == 0)) {
      ;
      // std::cout << "Entered Directory Exists Working" << std::endl;
    } else {
      std::cout << "Entered Directory doesn't Exists Exiting" << std::endl;
      exit(1);
    }

    // std::cout<<"Dir path "<< dir_path <<std::endl;

    // Mapping Servername to Modified SACI Id

    // File location of the Server_name:Modified_SACI Id file

    std::string server_nick_name = "/home/pengine/prod/live_configs/server_mapping.txt";

    std::unordered_map<std::string, std::string> ref;

    // Reading file data line by line and mapping the server name into server code
    std::ifstream file(server_nick_name);

    if (!file.is_open()) {
      std::cout << "Config File for mapping server doesn't Exists " << server_nick_name << std::endl;
      exit(1);
    }

    std::string str;
    while (std::getline(file, str)) {
      std::stringstream ss(str);
      std::string word;

      ss >> word;
      std::string short_name = word;

      ss >> word;
      std::string server_code = word;

      ref[short_name] = server_code;
    }

    std::string server_number = ref[server_name];
    // std::cout<<"SERVER_NAME: " << server_number <<std::endl;
    if (server_number == "") {
      std::cout << "SERVER " << server_name << " DOESN't Exist in Conf" << server_nick_name << std::endl;
      return;
    }
    // std::cout << server_number << std::endl;

    // std::cout << "Map generated with Server id :" << server_number << std::endl;

    // Reading the files in the logs directory file_path(parameter)

    for (const auto &entry : fs::directory_iterator(dir_path)) {
      //  	Checking for the desired file with desired date in the dir_path

      if (endsWith(entry.path().string(), date_of_file + ".gz")) {
        // Printing the file name of the zipped file to be opened

        //std::cout << "Path file: " << entry.path().string() << std::endl;

        HFSAT::BulkFileReader bulk_file_reader_;
        bulk_file_reader_.open(entry.path().string());
        // std::cout<<"Selected File Opened:\n";
        T next_event_;
        if (bulk_file_reader_.is_open()) {
          while (true) {
            size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(T));
            if (available_len_ < sizeof(next_event_)) break;
            int client_id = next_event_.server_assigned_client_id_ >> 16;

            //	std::cout << client_id << std::endl;
            // If Client id is the desired client id

            // std:: cout << "Client Id " << client_id << std::endl;

            //	std:: cout << "Server Number " << server_number  << std::endl;

            if (client_id == stoi(server_number)) {
              time_t time = (time_t)next_event_.client_request_time_.tv_sec ;
              struct tm tm_curr = *localtime(&time);

              if ( tm_curr.tm_hour < tm_start_time.tm_hour ) continue ;
              else if ( (tm_curr.tm_hour == tm_start_time.tm_hour) && (tm_curr.tm_min < tm_start_time.tm_min) ) continue;

              if ( tm_curr.tm_hour > tm_end_time.tm_hour ) continue;
              else if ( (tm_curr.tm_hour == tm_end_time.tm_hour) && (tm_curr.tm_min > tm_end_time.tm_min) ) continue;

              std::cout << next_event_.ToString();
            }
          }
          bulk_file_reader_.close();
        }
      }
    }
  }

  // End of class IndLogReader
};

// End of namespace
}  // namespace HFSAT
