// g++ -g -std=c++0x  sched.cc

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cstdlib>
#include <sstream>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/email_utils.hpp"

#pragma once

using namespace std;

#define PROC_NAME_TYPE_LIST_FILE "/spare/local/files/affinity/affinity_process_type_list.txt"
#define INTERFACE_AFFINITY_FILE "/spare/local/files/affinity/numa_node"

enum ProcType { kProducer, kConsumer, kLowPri, kEmpty };

struct ThreadInfo {
  int id, physicalId, coreId, cacheAlignment;
  ProcType type;
  bool consumed() { return type != kEmpty; }
};

typedef std::map<std::string, ProcType> process_type_map;

class AffinityAllocator {
  typedef int ProcId;
  static const ProcId kInvalidCore = -1;
  typedef std::vector<ThreadInfo> CoreInfo;
  typedef std::vector<CoreInfo> SocketInfo;
  typedef std::vector<SocketInfo> MachineInfo;

  std::vector<ThreadInfo> getCpuInfo() {
    vector<ThreadInfo> res;
    ThreadInfo info;
    std::vector<std::string> ss = readLines("/proc/cpuinfo");
    info.type = kEmpty;

    for (auto i = 0u; i < ss.size(); i++) {
      std::string s = ss[i];
      if (s.empty()) {
        res.push_back(info);
      } else {
        int i = s.find(":");  // seprator
        std::string value = s.substr(i + 1).c_str();

        if (s.find("processor") == 0)
          info.id = atoi(value.c_str());
        else if (s.find("physical id") == 0)
          info.physicalId = atoi(value.c_str());
        else if (s.find("core id") == 0)
          info.coreId = atoi(value.c_str());
        else if (s.find("cache_alignment") == 0)
          info.cacheAlignment = atoi(value.c_str());
      }
    }
    return res;
  }

  std::vector<std::string> readLines(const char *filename) {
    std::ifstream fin(filename);
    if (fin.bad()) std::cerr << (std::string("bad file name: ") + filename);
    std::string s;
    std::vector<std::string> ss;
    while (getline(fin, s)) ss.push_back(s);
    return ss;
  }

  int cardAffinity() {
    std::string fname = std::string(INTERFACE_AFFINITY_FILE);
    std::vector<std::string> ss = readLines(fname.c_str());
    int n = ss.size() ? atoi(ss[0].c_str()) : -1;
    return n >= 0 ? n : 0;
  }

  static MachineInfo info(std::vector<ThreadInfo> info) {
    // Create the CPU structure
    int maxSockId = -1, maxCoreId = -1;
    for (auto i = 0u; i < info.size(); i++) {
      ThreadInfo c = info[i];
      maxCoreId = max(maxCoreId, c.coreId);
      maxSockId = max(maxSockId, c.physicalId);
    }
    // we need the maxIds to calculate maxCoreCount and max SocketCount;
    // assume: all sockets have same no. of cores;
    MachineInfo mi(maxSockId + 1, SocketInfo(maxCoreId + 1, CoreInfo()));
    for (auto i = 0u; i < info.size(); i++) mi[info[i].physicalId][info[i].coreId].push_back(info[i]);
    return mi;
  }
  static bool hasConsumer(CoreInfo &r_core) {
    for (auto i = 0u; i < r_core.size(); i++) {
      if (r_core[i].type == kConsumer) return true;
    }
    return false;
  }
  static bool hasProducer(CoreInfo &r_core) {
    for (auto i = 0u; i < r_core.size(); i++) {
      if (r_core[i].type == kProducer) return true;
    }
    return false;
  }

  static bool isTotallyFree(CoreInfo &r_core) {
    for (auto i = 0u; i < r_core.size(); i++)
      if (r_core[i].consumed()) return false;
    return true;
  }

  static int emptyThread(CoreInfo &r_c) {
    // returns id of the empty hyperthread on a core
    for (auto i = 0u; i < r_c.size(); i++) {
      if (!r_c[i].consumed()) return r_c[i].id;
    }
    return kInvalidCore;
  }

  MachineInfo machInfo;
  int primary_socket_id;

  ProcId emptyThreadPrimarySocket() {
    ProcId id_ = kInvalidCore;
    for (auto i = 0u; i < machInfo[primary_socket_id].size(); i++) {
      CoreInfo core_ = machInfo[primary_socket_id][i];
      if (core_.empty()) continue;
      if (kInvalidCore != (id_ = emptyThread(core_))) return id_;
    }
    return kInvalidCore;
  }

  ProcId emptyThreadOtherSocket() {
    ProcId id_ = kInvalidCore;
    for (auto i = 0u; i < machInfo.size(); i++) {
      SocketInfo sock_ = machInfo[i];
      if (sock_.empty()) continue;
      for (unsigned j = 0; j < sock_.size(); j++) {
        CoreInfo core_ = sock_[j];
        if (core_.empty()) continue;
        if (core_[0].physicalId == primary_socket_id) break;
        if (kInvalidCore != (id_ = emptyThread(core_))) return id_;
      }
    }
    return kInvalidCore;
  }

  ProcId producerHyperthreads() {
    // assume: all producers are on primary socket
    for (auto i = 0u; i < machInfo[primary_socket_id].size(); i++) {
      CoreInfo core = machInfo[primary_socket_id][i];
      if (core.empty()) continue;
      if (hasProducer(core)) {
        int id = emptyThread(core);
        if (id != kInvalidCore) return id;
      }
    }
    return kInvalidCore;
  }
  ProcId emptyCorePrimarySocket() {
    for (auto i = 0u; i < machInfo[primary_socket_id].size(); i++) {
      CoreInfo c = machInfo[primary_socket_id][i];
      if (c.empty()) continue;
      if (isTotallyFree(c)) return c[0].id;
      // if(!hasProducer(c) && hasConsumer(c))
      //    return c[0].id;
    }
    return kInvalidCore;
  }
  ProcId emptyCoreOtherSocket() {
    int physId = primary_socket_id;  //= machInfo[primary_socket_id][0][0].physicalId;
    for (auto i = 0u; i < machInfo.size(); i++) {
      SocketInfo sock = machInfo[i];
      if (sock.empty()) continue;
      for (unsigned int j = 0; j < sock.size(); j++) {
        CoreInfo c = sock[j];
        if (c.empty()) continue;
        if (c[0].physicalId == physId) break;
        if (isTotallyFree(c)) return c[0].id;
      }
    }
    return kInvalidCore;
  }

  ProcId emptyHTPrimarySocket() {
    int physId = primary_socket_id;  // machInfo[primary_socket_id][0][0].physicalId;
    for (auto i = 0u; i < machInfo[physId].size(); i++) {
      CoreInfo c = machInfo[physId][i];
      if (c.empty()) continue;
      if (!isTotallyFree(c)) {
        int emptyThreadId = emptyThread(c);
        if (emptyThreadId != kInvalidCore) return emptyThreadId;
      }
    }
    return kInvalidCore;
  }
  ProcId emptyHTOtherSocket() {
    int physId = primary_socket_id;  // machInfo[primary_socket_id][0][0].physicalId;
    for (auto i = 0u; i < machInfo.size(); i++) {
      SocketInfo sock = machInfo[i];
      if (sock.empty()) continue;
      for (unsigned int j = 0; j < sock.size(); j++) {
        CoreInfo c = sock[j];
        if (c.empty()) continue;
        if (c[0].physicalId == physId) break;
        if (!isTotallyFree(c)) {
          int emptyThreadId = emptyThread(c);
          if (emptyThreadId != kInvalidCore) return emptyThreadId;
        }
      }
    }
    return kInvalidCore;
  }

  ProcId randomCoreOtherSocket() {
    std::vector<int> th;
    th.clear();
    int physCount = 0, coreCount = 0;
    int physId = primary_socket_id;  // machInfo[primary_socket_id][0][0].physicalId;
    for (auto i = 0u; i < machInfo.size(); i++) {
      SocketInfo sock = machInfo[i];
      if (sock.empty()) continue;
      physCount++;
      for (unsigned int j = 0; j < sock.size(); j++) {
        CoreInfo c = sock[j];
        if (c.empty()) continue;
        if (c[0].physicalId != physId) break;
        coreCount++;
        for (unsigned int k = 0; k < c.size(); k++) {
          th.push_back(c[k].id);
        }
      }
    }
    return th[rand() % (coreCount / physCount)];
  }

 public:
  std::string ToString(ProcType proc_type_) {
    switch (proc_type_) {
      case kProducer: {
        return std::string("Producer");
      }
      case kConsumer: {
        return std::string("Consumer");
      }
      case kLowPri: {
        return std::string("Low Priority");
      }
      case kEmpty: {
        return std::string(" ");
      }
        return std::string("Process type Not specified.");
    }
    return "";
  }

  static inline process_type_map parseProcessListFile(std::vector<std::string> &process_list) {
    char line_buffer_[1024];
    process_type_map process_and_type_;
    std::string line_read_;
    std::ifstream process_type_list_file_;

    process_type_list_file_.open(PROC_NAME_TYPE_LIST_FILE);
    if (!process_type_list_file_.is_open()) {
      std::cerr << "File : " << PROC_NAME_TYPE_LIST_FILE << " does not exist \n";
      exit(-1);
    }
    process_and_type_.clear();
    while (process_type_list_file_.good()) {
      memset(line_buffer_, 0, sizeof(line_buffer_));
      process_type_list_file_.getline(line_buffer_, sizeof(line_buffer_));
      line_read_ = line_buffer_;
      if (line_read_.empty()) continue;
      if (line_read_.find("#") != std::string::npos) continue;

      HFSAT::PerishableStringTokenizer st_(line_buffer_, sizeof(line_buffer_));
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (!tokens_.empty() && tokens_.size() > 0) {
        if ((std::string(tokens_[1])).compare("P") == 0) process_and_type_[tokens_[0]] = kProducer;
        if ((std::string(tokens_[1])).compare("ORS") == 0) process_and_type_[tokens_[0]] = kProducer;
        if ((std::string(tokens_[1])).compare("C") == 0) process_and_type_[tokens_[0]] = kConsumer;
        if ((std::string(tokens_[1])).compare("N") == 0) process_and_type_[tokens_[0]] = kLowPri;

        process_list.push_back(tokens_[0]);
      }
    }
    return process_and_type_;
  }

  void init() {
    int primary_sock_id = cardAffinity();
    vector<ThreadInfo> ci = getCpuInfo();
    machInfo = info(ci);
    primary_socket_id = primary_sock_id;
  }

  std::string printAffinity() {
    std::stringstream str_;
    for (auto i = 0u; i < machInfo.size(); i++) {
      SocketInfo sock = machInfo[i];
      if (sock.empty()) continue;
      for (unsigned j = 0; j < sock.size(); j++) {
        if (!sock[j].empty()) {
          str_ << "\n\nAffined processes on socket " << sock[j][0].physicalId << std::endl;
          break;
        }
      }
      for (unsigned int j = 0; j < sock.size(); j++) {
        CoreInfo core = sock[j];
        if (core.empty()) continue;
        str_ << "\n\t\t\tAffined process on core :" << core[0].coreId << std::endl;
        for (unsigned int k = 0; k < core.size(); k++) {
          str_ << "\t\t\t\t\t\tProcessor Id : " << core[k].id << "\t Affined Proces Type : " << ToString(core[k].type)
               << std::endl;
        }
      }
    }
    return str_.str();
  }

  ProcId SiblingHyperThread(int processor_id) {
    for (auto i = 0u; i < machInfo.size(); i++) {
      SocketInfo sock = machInfo[i];
      if (sock.empty()) continue;
      for (unsigned int j = 0; j < sock.size(); j++) {
        CoreInfo c = sock[j];
        if (c.empty()) continue;
        for (unsigned int k = 0; k < c.size(); k++) {
          if (c[k].id == processor_id) {
            for (unsigned int l = 0; l < c.size(); l++) {
              if (c[l].id != processor_id) {
                return c[l].id;
              }
            }
          }
        }
      }
    }
    return kInvalidCore;
  }

  ProcId allocate(ProcType t) {
    switch (t) {
      case kProducer: {
        ProcId id = emptyCorePrimarySocket();
        if (kInvalidCore == id) {
          if (kInvalidCore != (id = emptyCoreOtherSocket())) return id;
          if (kInvalidCore != (id = emptyHTPrimarySocket())) return id;
          if (kInvalidCore != (id = emptyHTOtherSocket())) return id;
        }
        return id;
      }
      case kConsumer: {
        ProcId id;
        if (kInvalidCore != (id = producerHyperthreads())) return id;
        if (kInvalidCore != (id = emptyCorePrimarySocket())) return id;
        if (kInvalidCore != (id = emptyCoreOtherSocket())) return id;
        if (kInvalidCore != (id = emptyHTPrimarySocket())) return id;
        if (kInvalidCore != (id = emptyHTOtherSocket())) return id;
        break;
      }
      case kLowPri: {
        ProcId id;
        if (kInvalidCore != (id = emptyCoreOtherSocket())) return id;
        if (kInvalidCore != (id = emptyHTOtherSocket())) return id;
        return kInvalidCore;
      }
      case kEmpty: {
        return kInvalidCore;
      }
    }
    return kInvalidCore;
  }

  std::string updateMachInfo(ProcId id, ProcType type) {
    std::stringstream st;
    st << "Updating Processor id : " << id << " ProcType : " << type << std::endl;
    unsigned int i = 0, j = 0, k = 0;
    ;
    for (i = 0; i < machInfo.size(); i++) {
      if (machInfo[i].empty()) continue;
      for (j = 0; j < machInfo[i].size(); j++) {
        if (machInfo[i][j].empty()) continue;
        for (k = 0; k < machInfo[i][j].size(); k++) {
          if (machInfo[i][j][k].id == id) machInfo[i][j][k].type = type;
        }
      }
    }
    return st.str();
  }

  // Function to exclude core 0 from affining (since it is being used by other system processes: VMA/onload)
  void ExcludeCore0() {
    unsigned int thread_ctr = 0, core_zero = 0;
    for (unsigned int socket_ctr = 0; socket_ctr < machInfo.size(); socket_ctr++) {
      if (machInfo[socket_ctr].size() <= 0) {
        // No cores on the socket
        return;
      }
      for (thread_ctr = 0; thread_ctr < machInfo[socket_ctr][core_zero].size(); thread_ctr++) {
        updateMachInfo(machInfo[socket_ctr][core_zero][thread_ctr].id, kProducer);
      }
    }
  }

  void AlertMail(int pid, ProcType process_type_) {
    std::cerr << "Error: Unable to allocate core for process. Could not affine the process with PID " << pid
              << " to any core. Process Type: " << ToString(process_type_) << std::endl;
    return;  // Nobody looks at these mails, no point of sending. The clients have the ability to know if affinity has
             // failed, they can exit if its critical for them.
    char host_name_[256];
    std::string to_;
    gethostname(host_name_, 256);
    char *addr = getenv("ERRMAIL");
    if (addr != NULL) {
      to_ = std::string(addr);
    } else {
      to_ = std::string("nseall@tworoads.co.in");
    }
    HFSAT::Email email_;
    email_.addSender("nseall@tworoads.co.in");
    email_.addRecepient(to_);
    email_.setSubject("Unable to allocate core for process");
    email_.content_stream << "Could not affine the process with PID " << pid << " to any core on server" << host_name_
                          << ".\n Process Type: " << ToString(process_type_);
    email_.sendMail();
  }
};
