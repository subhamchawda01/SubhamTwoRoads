/**
    \file email_utils.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/

#ifndef BASE_CDEF_EMAILUTILS_H_
#define BASE_CDEF_EMAILUTILS_H_

#include <string>
#include <stdio.h>
#include <sstream>
#include <unistd.h>  //for gethostname

#define EMAIL_EXEC "/usr/lib/sendmail"
#define MAIL_SETTING "X-Mailer: htmlmail 1.0\nMime-Version: 1.0\nContent-Type: text/html; charset=US-ASCII"

namespace HFSAT {
class Email {
 protected:
  std::string sender_;
  std::string recipient_;
  std::string subject_;
  std::string attachment_;

  //    void addAttachment(std::string file_name){
  //      attachment_ = file_name;
  //    }
 public:
  std::stringstream content_stream;

  void sendMail() {
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    FILE* mail_file_ = popen((std::string(EMAIL_EXEC) + " \"" + recipient_ + "\"").c_str(), "w");
    fprintf(mail_file_, "From: %s\n", sender_.c_str());
    fprintf(mail_file_, "To: %s\n", recipient_.c_str());
    fprintf(mail_file_, "Subject: %s\n", subject_.c_str());
    fprintf(mail_file_, "%s\n\n", MAIL_SETTING);
    fprintf(mail_file_, "host : %s<br/>", hostname);
    fprintf(mail_file_, "%s", content_stream.str().c_str());
    pclose(mail_file_);
    content_stream.str("");  // clear old contents
  }

  void setSubject(std::string subject) { subject_ = subject; }

  void addSender(std::string sender) { sender_ = sender; }

  void addRecepient(std::string recipient) { recipient_ = recipient; }

  Email& toMyself(std::string _from) {
    sender_ = _from;
    recipient_ = _from;
    return *this;
  }

  Email& from(std::string _from) {
    sender_ = _from;
    return *this;
  }
  Email& to(std::string _to) {
    recipient_ = _to;
    return *this;
  }
  Email& withSubject(std::string _subject) {
    subject_ = _subject;
    return *this;
  }
};
}
#endif /*BASE_CDEF_EMAILUTILS_H_*/
