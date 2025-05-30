/**
  \file dvccode/Tests/dvccode/Utils/bulk_file_writer_test.cpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/

#include "dvccode/Tests/Utils/bulk_file_writer_test.hpp"

namespace HFTEST {
using namespace HFSAT;

/**
 *
 */
void BulkFileWriterTest::setUp() { bulk_file_writer_ = new HFSAT::BulkFileWriter(); }

/**
 * Tests a file is being created with bulkfilewriter
 */
void BulkFileWriterTest::TestFileWrite(void) {
  std::string filename_ = GetTestDataFullPath("newfile_test.txt", "dvccode");

  int size = 0;
  // If the file already exists then remove it
  if (FileUtils::ExistsWithSize(filename_, size)) {
    remove(filename_.c_str());
  }

  bulk_file_writer_->Open(filename_.c_str());

  // Write random data to the file
  int count = 0;
  while (count < 100) {
    char c = count;
    (*bulk_file_writer_) << c << " ";
    count++;
  }
  bulk_file_writer_->Close();

  // Currently just checking if the file was created successfully

  bool file_created = FileUtils::ExistsWithSize(filename_, size);

  // delete the file if created
  if (file_created) {
    remove(filename_.c_str());
  }

  CPPUNIT_ASSERT(file_created);
}

/**
 * Tests if a file is created as well as is opened in append mode
 */
void BulkFileWriterTest::TestFileAppend(void) {
  std::string filename_ = GetTestDataFullPath("newfile_append_test.txt", "dvccode");

  int size = 0;
  // If the file already exists then remove it
  if (FileUtils::ExistsWithSize(filename_, size)) {
    remove(filename_.c_str());
  }

  bulk_file_writer_->Open(filename_.c_str());

  // Write random data to the file
  auto count = 0u;
  while (count < 100u) {
    char c = count;
    (*bulk_file_writer_) << c << " ";
    count++;
  }
  bulk_file_writer_->Close();

  // Currently just checking if the file was created successfully

  auto first_size = boost::filesystem::file_size(filename_.c_str());

  if (first_size > 0) {
    bulk_file_writer_->Open(filename_.c_str(), std::ios::app);

    // write random data to the file again
    auto count = 0u;
    while (count < 100u) {
      char c = count;
      (*bulk_file_writer_) << c << " ";
      count++;
    }

    bulk_file_writer_->Close();

    auto second_size = boost::filesystem::file_size(filename_.c_str());
    CPPUNIT_ASSERT(second_size == 2 * first_size);

  } else {
    CPPUNIT_ASSERT(0);  // This was earlier CPPUNIT_ASSERT_FAIL, but that does not exist it seems
  }
}

void BulkFileWriterTest::tearDown() { delete bulk_file_writer_; }
}
