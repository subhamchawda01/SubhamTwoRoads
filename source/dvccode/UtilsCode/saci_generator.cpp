#include <fstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/Utils/saci_generator.hpp"

namespace HFSAT {

SaciIncrementGenerator* SaciIncrementGenerator::unique_instance_ = nullptr;

SaciIncrementGenerator& SaciIncrementGenerator::GetUniqueInstance() {
  if (unique_instance_ == nullptr) {
    unique_instance_ = new SaciIncrementGenerator();
  }
  return *(unique_instance_);
}

void SaciIncrementGenerator::ResetUniqueInstance() {
  if (unique_instance_ != nullptr) {
    delete unique_instance_;
    unique_instance_ = nullptr;
  }
}

// Reads last dumped saci generator from the file if present else starts from 1
SaciIncrementGenerator::SaciIncrementGenerator() {
  saci_increment_value = 1;
  std::string saci_gen_file(SACI_GEN_FILE);
  if (FileUtils::ExistsAndReadable(saci_gen_file) && FileUtils::idleTime(saci_gen_file) < 8 * 3600) {
    std::ifstream f(saci_gen_file);
    char* buf = new char[1024];
    f.getline(buf, sizeof(buf));
    saci_increment_value = atoi(buf);
    delete[] buf;
    buf = nullptr;
    f.close();
  }
}

// returns the saci generator value: used by shared_mem_reader
int SaciIncrementGenerator::GetNextSaciIncrementValue() { return saci_increment_value++; }

// To recover from crashes, should be called in ors termination handler
void SaciIncrementGenerator::PersistSaciIncrementGenerator() {
  std::string saci_gen_file(SACI_GEN_FILE);
  if (!FileUtils::exists(saci_gen_file)) FileUtils::MkdirEnclosing(saci_gen_file);

  std::ofstream f(saci_gen_file);
  f << saci_increment_value;
  f.close();
  return;
}
}
