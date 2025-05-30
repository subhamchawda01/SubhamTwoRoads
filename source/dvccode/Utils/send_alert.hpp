/**
    \file send_alert.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#ifndef _BASE_UTILS_SEND_ALERT_HPP_
#define _BASE_UTILS_SEND_ALERT_HPP_

#define ALERT_SERVER_PORT 7980

#include <string.h>
#include "dvccode/Utils/tcp_client_socket.hpp"
#include <iostream>
#include <fstream>

namespace HFSAT {

class SendAlert {
  // based on javascript encodeURIComponent()
  static std::string urlencode(const std::string& c) {
    std::string escaped = "";
    int max = c.length();
    for (int i = 0; i < max; i++) {
      if ((48 <= c[i] && c[i] <= 57) ||   // 0-9
          (65 <= c[i] && c[i] <= 90) ||   // abc...xyz
          (97 <= c[i] && c[i] <= 122) ||  // ABC...XYZ
          (c[i] == '~' || c[i] == '!' || c[i] == '*' || c[i] == '(' || c[i] == ')' || c[i] == '\'')) {
        escaped.append(&c[i], 1);
      } else {
        escaped.append("%");
        escaped.append(char2hex(c[i]));  // converts char 255 to std::string "ff"
      }
    }
    return escaped;
  }

  static std::string char2hex(char dec) {
    static char map[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    std::string r;
    r.append(map + (unsigned)dec / 16, 1);
    r.append(map + (unsigned)dec % 16, 1);
    return r;
  }

 public:
  static bool sendAlert(const std::string& message) {
    static char* alert_url = NULL;
    if (alert_url == NULL) {
      std::ifstream in;
      in.open("/spare/local/files/alert_server_url");
      if (!in.is_open() && !in.good()) {
        std::cerr << "alert streamer url file not found at "
                  << "/spare/local/files/alert_server_url"
                  << "\n";
        return false;
      }
      alert_url = (char*)calloc(64, 1);
      in.read(alert_url, 64);
      int len = strlen(alert_url);
      while (--len && len >= 0) {
        if (alert_url[len] >= '0' && alert_url[len] <= '9') break;
        alert_url[len] = '\0';
      }
      in.close();
    }

    if (alert_url == NULL || alert_url[0] == '\0') return false;  // uninitialized url

    std::string encoded_message = "GET /?msg=" + urlencode(message);
    // std::cout <<encoded_message << "\n";
    HFSAT::TCPClientSocket sock = HFSAT::TCPClientSocket(true);
    sock.Connect(alert_url, ALERT_SERVER_PORT);
    sock.WriteN(encoded_message.length(), (void*)encoded_message.c_str());
    char* response = new char[1000];
    sock.ReadN(1000, response);
    // std::cout << "response:::\n" << response << "\n-----\n";
    bool isSuccess = (strstr(response, "HTTP/1.0 200 OK") != NULL);

    delete[] response;
    response = NULL;
    sock.Close();

    return isSuccess;
  }
};
}
#endif /* _BASE_UTILS_SEND_ALERT_HPP_ */
