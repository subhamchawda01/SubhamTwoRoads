#include "dvccode/Utils/combined_shm_reader_listener.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

namespace HFSAT {
class CombinedShmReader {
 public:
  static CombinedShmReader &GetUniqueInstance();
  static void ResetUniqueInstance();

  void AddListener(CombinedShmReaderListener *listener);
  void RemoveListener(CombinedShmReaderListener *listener);

  void StartReading();

 private:
  CombinedShmReader();
  ~CombinedShmReader();

  void NotifyListeners(HFSAT::MDS_MSG::GenericMDSMessage generic_mds_msg);

  static CombinedShmReader *uniqueinstance_;
  std::vector<CombinedShmReaderListener *> listeners_;

  key_t generic_mds_shm_key_;
  int generic_mds_shmid_;

  volatile HFSAT::MDS_MSG::GenericMDSMessage *generic_mds_shm_struct_;
  struct shmid_ds generic_shmid_ds_;

  // this is meant to be a local copy to track queue position, compare with volatile shared memory counter
  int index_;
};
}
