/**
   \file BasicOrderRoutingServer/liffe_clord_id_generator.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/

#ifndef BASE_BASICORDERROUTINGSERVER_LIFFE_CLORDID_GENERATOR_H
#define BASE_BASICORDERROUTINGSERVER_LIFFE_CLORDID_GENERATOR_H

#include <fstream>
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/lock.hpp"
#include "dvccode/CDef/file_utils.hpp"

namespace HFSAT {
namespace ORS {

#define LIFFE_CLORD_ID_GEN_FILE "/spare/local/files/tmp/liffe_clord_id.txt"

class ClientOrderIdGenerator {
 private:
  /// make copy cosntructor private to disable it
  ClientOrderIdGenerator(const ClientOrderIdGenerator&);

 public:
  static ClientOrderIdGenerator& GetUniqueInstance() {
    static ClientOrderIdGenerator uniqueinstance_;
    return uniqueinstance_;
  }

  inline unsigned int GetNextSequence() { return ++sequence_; }

  // To recover from crashes, should be called in termination handler
  void persist_seq_num() {
    if (!FileUtils::exists(LIFFE_CLORD_ID_GEN_FILE)) FileUtils::MkdirEnclosing(LIFFE_CLORD_ID_GEN_FILE);

    std::ofstream f(LIFFE_CLORD_ID_GEN_FILE);
    f << sequence_;
    f.close();
    return;
  }

 protected:
  unsigned int sequence_;

  ClientOrderIdGenerator(unsigned int seed = 0) {
    // assign to seed value if a non zero argument is passed
    if (seed != 0) {
      sequence_ = seed;
      return;
    }

    // initialize to last seq_num persisted in LIFFE_CLORD_ID_GEN_FILE
    if (FileUtils::ExistsAndReadable(LIFFE_CLORD_ID_GEN_FILE) &&
        FileUtils::idleTime(LIFFE_CLORD_ID_GEN_FILE) < 8 * 3600) {
      std::ifstream f(LIFFE_CLORD_ID_GEN_FILE);
      char* buf = new char[1024];
      f.getline(buf, sizeof(buf));
      int last_seq = atoi(buf);
      sequence_ = last_seq + 1;
      delete[] buf;
      buf = NULL;
      f.close();
      return;
    }
    // initialize to 1 as default
    sequence_ = 800000;  // this value may need to be raised further
  }
};
}
}

#endif  // BASE_BASICORDERROUTINGSERVER_SEQUENCEGENERATOR_H
