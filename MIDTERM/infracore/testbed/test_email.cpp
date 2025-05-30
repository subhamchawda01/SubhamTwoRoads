/**
    \file test_unsorted_vector.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include "dvccode/CDef/email_utils.hpp"

int main(int argc, char *argv[]) {
  HFSAT::Email e;

  e.setSubject("Test email");
  e.addRecepient(argv[1]);  // change this to use it
  e.addSender("nseall@tworoads.co.in");
  e.content_stream << "line1<br/>";
  e.content_stream << "line2<br/>";
  e.content_stream << "line3<br/>";
  e.content_stream << "line4<br/>";
  e.sendMail();
}
// command to compile
// g++ -o test_email test_email.cpp -I../../infracore_install/  -L../../infracore_install/lib/
