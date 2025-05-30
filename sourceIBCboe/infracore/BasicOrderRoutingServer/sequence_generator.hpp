/**
    \file BasicOrderRoutingServer/sequence_generator.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */

#ifndef BASE_BASICORDERROUTINGSERVER_SEQUENCEGENERATOR_H
#define BASE_BASICORDERROUTINGSERVER_SEQUENCEGENERATOR_H

#include <fstream>
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/lock.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include <dirent.h>
#include <string.h>
#include <vector>
#include <cstdlib>
#include <ctime>

namespace HFSAT {
namespace ORS {

#define SEQ_GEN_FILE "/spare/local/files/tmp/seq_gen.txt"  /// spare/local/files/tmp/seq_gen.txt"
#define SEQ_GEN_FILE_RTS "/spare/local/files/tmp/seq_gen_rts.txt"
#define SEQ_GEN_FILE_MICEX "/spare/local/files/tmp/seq_gen_micex.txt"
#define SEQ_GEN_FILE_ASX "/spare/local/files/tmp/seq_gen_asx.txt"

class SequenceGenerator {
 private:
  /// make copy cosntructor private to disable it
  SequenceGenerator(const SequenceGenerator&);
  unsigned int MAX_ORDER_ID;  // Upper cap on exchange ClOrdId (for exchanges like RTS)
  bool cap_on_sequence_;      // Do we reset sequence after it has reached the value determined by MAX_ORDER_ID

 public:
  static SequenceGenerator& GetUniqueInstance(std::string exch_ = "NONE") {
    static SequenceGenerator* uniqueinstance_ = new SequenceGenerator(0, exch_);
    return *uniqueinstance_;
  }

  /// Called by ClientThread in connection to creating a new order
  /// needs to be thread safe to have uniqueness in the field server_assigned_order_sequence_
  /// Currently implemented using Lock
  /// TODO: see alternative implementation using boost::interprocess::detail::atomic_inc32
  /// the asm volatile part could also be useful in other places
  inline unsigned int GetNextSequence() {
    if (cap_on_sequence_ && (sequence_ >= MAX_ORDER_ID)) {
      sequence_ = 0;
    }
    return ++sequence_;
  }

  inline unsigned int GetNextSequence(int jump_seq) {
    if (cap_on_sequence_ && (sequence_ >= MAX_ORDER_ID)) {
      sequence_ = 0;
    }
    sequence_ += jump_seq;
    return sequence_;
  }

  inline void RevertToPreviousSequence() {
    if (cap_on_sequence_ && (sequence_ == 0)) {
      sequence_ = 0;
    } else {
      --sequence_;
    }
  }

  // To recover from crashes, should be called in termination handler
  void persist_seq_num() {
    if (!FileUtils::exists(seq_gen_file_)) FileUtils::MkdirEnclosing(seq_gen_file_);

    std::ofstream f(seq_gen_file_);
    f << sequence_;
    f.close();
    return;
  }

 protected:
  unsigned int sequence_;
  std::string seq_gen_file_;

  SequenceGenerator(unsigned int seed = 0, std::string exch_ = "NONE")
      : MAX_ORDER_ID(100000000), cap_on_sequence_(false), seq_gen_file_(SEQ_GEN_FILE) {
    // initialize to 1 as default
    sequence_ = 1;

    // assign to seed value if a non zero argument is passed
    if (exch_ == "RTS") {
      seq_gen_file_ = SEQ_GEN_FILE_RTS;
      cap_on_sequence_ = true;
      MAX_ORDER_ID = 90000000;
    }
    if (exch_ == "MICEX") seq_gen_file_ = SEQ_GEN_FILE_MICEX;
    if (exch_ == "ASX") seq_gen_file_ = SEQ_GEN_FILE_ASX;
    std::cout << "Seq File Name = " << seq_gen_file_ << std::endl;
    if (seed != 0) {
      sequence_ = seed;
      return;
    }

    // initialize to last seq_num persisted in seq_gen_file_
    if (FileUtils::ExistsAndReadable(seq_gen_file_) &&
        (exch_ == "ASX" || (FileUtils::idleTime(seq_gen_file_) < 8 * 3600))) {
      std::ifstream f(seq_gen_file_);
      char* buf = new char[1024];
      f.getline(buf, sizeof(buf));
      int last_seq = atoi(buf);
      sequence_ = last_seq + 1;
      delete[] buf;
      buf = NULL;
      f.close();
      return;
    } else {
      int new_saos = GetSequenceFromCrash();
      if (new_saos != 0) {
        new_saos += 1000;
        std::cout << "Setting saos to : " << new_saos << std::endl;
        sequence_ = new_saos;
      }
    }
  }

  bool IsUpDirecory(const char* directory) {
    if (strcmp(directory, "..") == 0 || strcmp(directory, ".") == 0)
      return true;
    else
      return false;
  }

  // search for a file with name 'fileName' recursively in directory 'path'
  void FindFile(const std::string& fileName, const std::string& path, std::vector<std::string>& trade_files) {
    dirent* entry;
    DIR* dir = opendir(path.c_str());

    if (dir == NULL) return;

    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_type == DT_REG) {
        if (fileName.compare(entry->d_name) == 0) {
          std::string resultPath = path + "/" + entry->d_name;
          trade_files.push_back(resultPath);
        }
      }
    }

    rewinddir(dir);

    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_type == DT_DIR) {
        if (!IsUpDirecory(entry->d_name)) {
          std::string nextDirectoryPath = path + "/" + entry->d_name;
          FindFile(fileName, nextDirectoryPath, trade_files);
        }
      }
    }

    closedir(dir);
  }

  // in case of a crash in ors, we try to get highest saos from ors trade files.
  int GetSequenceFromCrash() {
    std::string path;
    std::vector<std::string> trade_files;
    int max_saos = 0;
    time_t t = time(0);  // get time now
    struct tm* now = localtime(&t);
    std::string filename = "trades." + std::to_string(now->tm_year + 1900);
    if (now->tm_mon + 1 < 10) filename += "0";
    filename += std::to_string(now->tm_mon + 1);
    if (now->tm_mday < 10) filename += "0";
    filename += std::to_string(now->tm_mday);

    FindFile(filename, "/spare/local/ORSlogs", trade_files);
    char delimiter = 1;
    for (unsigned int i = 0; i < trade_files.size(); i++) {
      std::ifstream infile(trade_files[i].c_str());
      std::string line;
      while (std::getline(infile, line)) {
        line = line.substr(line.find(delimiter) + 1);  // skipping exchange symbol
        line = line.substr(line.find(delimiter) + 1);  // skipping side
        line = line.substr(line.find(delimiter) + 1);  // skipping size
        line = line.substr(line.find(delimiter) + 1);  // skipping price
        std::string saos_string = line;
        if (line.find(delimiter) != std::string::npos) saos_string = line.substr(0, line.find(delimiter));

        int saos = atoi(saos_string.c_str());
        if (saos > max_saos) max_saos = saos;
      }
      infile.close();
      std::cout << "Reading saos from file: " << trade_files[i] << std::endl;
    }
    return max_saos;
  }
};
}
}

#endif  // BASE_BASICORDERROUTINGSERVER_SEQUENCEGENERATOR_H
