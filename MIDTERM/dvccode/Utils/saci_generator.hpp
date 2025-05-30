#ifndef _SACI_GEN_HPP_
#define _SACI_GEN_HPP_

namespace HFSAT {

#define SACI_GEN_FILE "/spare/local/files/tmp/saci_gen.txt"

class SaciIncrementGenerator {
 public:
  static SaciIncrementGenerator& GetUniqueInstance();
  static void ResetUniqueInstance();
  int GetNextSaciIncrementValue();
  void PersistSaciIncrementGenerator();

 private:
  SaciIncrementGenerator();
  static SaciIncrementGenerator* unique_instance_;
  int saci_increment_value;
};
}

#endif
