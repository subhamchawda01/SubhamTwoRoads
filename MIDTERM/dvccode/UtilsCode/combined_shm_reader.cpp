#include "dvccode/Utils/mds_shm_interface.hpp"
#include "dvccode/Utils/combined_shm_reader.hpp"

namespace HFSAT {
CombinedShmReader *CombinedShmReader::uniqueinstance_ = nullptr;

CombinedShmReader::CombinedShmReader()
    : generic_mds_shm_key_(GENERIC_MDS_SHM_KEY), generic_mds_shmid_(-1), generic_mds_shm_struct_(NULL), index_(-1) {
  if ((generic_mds_shmid_ =
           shmget(generic_mds_shm_key_,
                  (size_t)(GENERIC_MDS_QUEUE_SIZE * (sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) + sizeof(int)),
                  IPC_CREAT | 0666)) < 0) {
    if (errno == EINVAL)
      std::cerr << "Invalid segment size specified \n";

    else if (errno == EEXIST)
      std::cerr << "Segment already exists \n";

    else if (errno == EIDRM)
      std::cerr << " Segment is marked for deletion \n";

    else if (errno == ENOENT)
      std::cerr << " Segment Doesn't Exist \n";

    else if (errno == EACCES)
      std::cerr << " Permission Denied \n";

    else if (errno == ENOMEM)
      std::cerr << " Not Enough Memory To Create Shm Segment \n";

    exit(1);
  }

  if ((generic_mds_shm_struct_ = (volatile HFSAT::MDS_MSG::GenericMDSMessage *)shmat(generic_mds_shmid_, NULL, 0)) ==
      (volatile HFSAT::MDS_MSG::GenericMDSMessage *)-1) {
    perror("shmat failed");
    exit(0);
  }

  if (shmctl(generic_mds_shmid_, IPC_STAT, &generic_shmid_ds_) == -1) {
    perror("shmctl");
    exit(1);
  }

  if (generic_shmid_ds_.shm_nattch == 1) {
    memset((void *)((HFSAT::MDS_MSG::GenericMDSMessage *)generic_mds_shm_struct_), 0,
           (GENERIC_MDS_QUEUE_SIZE * sizeof(HFSAT::MDS_MSG::GenericMDSMessage) + sizeof(int)));
  }
}

CombinedShmReader::~CombinedShmReader() {
  shmdt((void *)generic_mds_shm_struct_);

  if (shmctl(generic_mds_shmid_, IPC_STAT, &generic_shmid_ds_) == -1) {
    perror("shmctl");
    exit(1);
  }

  if (generic_shmid_ds_.shm_nattch == 0)
    shmctl(generic_mds_shmid_, IPC_RMID, 0);  // remove shm segment if no users are attached
}

CombinedShmReader &CombinedShmReader::GetUniqueInstance() {
  if (uniqueinstance_ == nullptr) {
    uniqueinstance_ = new CombinedShmReader();
  }
  return *(uniqueinstance_);
}

void CombinedShmReader::ResetUniqueInstance() {
  if (uniqueinstance_ != nullptr) {
    delete uniqueinstance_;
    uniqueinstance_ = nullptr;
  }
}

void CombinedShmReader::AddListener(CombinedShmReaderListener *listener) {
  HFSAT::VectorUtils::UniqueVectorAdd(listeners_, listener);
}

void CombinedShmReader::RemoveListener(CombinedShmReaderListener *listener) {
  HFSAT::VectorUtils::UniqueVectorRemove(listeners_, listener);
}

void CombinedShmReader::StartReading() {
  HFSAT::MDS_MSG::GenericMDSMessage generic_mds_msg;

  memset((void *)&generic_mds_msg, 0, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));

  while (true) {
    // has to be volatile, waiting on shared memory segment queue position
    volatile int queue_position_ = *((int *)(generic_mds_shm_struct_ + GENERIC_MDS_QUEUE_SIZE));

    // events are available only if the queue position at source is higher by 1, circular queue, will lag behind by 1
    // packet
    if (index_ == -1) {
      index_ = queue_position_;
    }

    if (index_ == queue_position_) {
      continue;
    }

    index_ = (index_ + 1) & (GENERIC_MDS_QUEUE_SIZE - 1);

    // memcpy is only done as safegaurd from writer writing the same segment
    memcpy(&generic_mds_msg, (HFSAT::MDS_MSG::GenericMDSMessage *)(generic_mds_shm_struct_ + index_),
           sizeof(HFSAT::MDS_MSG::GenericMDSMessage));

    NotifyListeners(generic_mds_msg);
  }
}

void CombinedShmReader::NotifyListeners(HFSAT::MDS_MSG::GenericMDSMessage generic_mds_msg) {
  for (auto listener : listeners_) {
    listener->OnShmRead(generic_mds_msg);
  }
}
}
