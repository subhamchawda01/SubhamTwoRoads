namespace HFSAT {
class CombinedShmReaderListener {
 public:
  virtual void OnShmRead(HFSAT::MDS_MSG::GenericMDSMessage generic_mds_msg) = 0;
  virtual ~CombinedShmReaderListener() {}
};
}
