/**
  \file Tests/dvctrade/MarketAdapter/ntp_mvm_tests.cpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/

#include <fstream>
#include "dvccode/Tests/TestUtils/cpptest_utils.hpp"
#include "dvccode/Tests/Utils/bulk_file_reader_test.hpp"

namespace HFTEST {

void BulkFileReaderTest::TestNullFile(void) {
  std::string filename_ = "nofile.txt";
  bulk_file_reader_->open(filename_);
  bulk_file_reader_->close();
}

void BulkFileReaderTest::TestFileRead(void) {
  std::string filename_ = GetTestDataFullPath("GEN6_20160129.gz", "dvccode");
  bulk_file_reader_->open(filename_);

  CME_MDS::CMECommonStruct next_event_;

  int num_blocks = 0, size = -1;

  while (size != 0) {
    size = bulk_file_reader_->read(&next_event_, sizeof(CME_MDS::CMECommonStruct));
    if (size == 0) break;
    num_blocks++;
  }

  bulk_file_reader_->close();
  CPPUNIT_ASSERT_EQUAL(808, num_blocks);
}

// Like Constructor :
void BulkFileReaderTest::setUp(void) { bulk_file_reader_ = new HFSAT::BulkFileReader(); }

// Like Destructor
void BulkFileReaderTest::tearDown(void) { delete bulk_file_reader_; }

//-----------------------------------------------------------------------------
}
