/**
    \file slack_utils.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

 */

#ifndef BASE_CDEF_SLACKUTILS_H_
#define BASE_CDEF_SLACKUTILS_H_

#include <stdio.h>
#include <curl/curl.h>
#include <string>
#include <sstream>
#include <cstring>
#include <iostream>
#include <algorithm>

#define ALERTS "alerts"
#define TESTCHANNEL "testing"
#define NSEMED "nsemed"
#define DISPERSION "midterm-dispersion"
#define EARNINGS_STRAT "midterm-earnings"
#define MIDTERM_MARGIN "midterm-margin"
#define RISKMANGER "risk-manager"
#define TAG_PNL "tag_based_pnl"
#define BASETRADE "basetrade"
#define MIDTERM_PROC_TRACKING "midterm-proc-tracking"
#define MIDTERM_ORDER_REJECTS "midterm-order-rejects"
#define MIDTERM_PNL_ALERTS "midterm-pnl-alerts"
#define MIDTERM_POS_MISMATCH "midterm-pos-mismatch"
#define MIDTERM_SHOCKRISK "midterm-shockrisk"
#define NSEHFT_REJECTS "nsehft-rejects"
#define NSE_OEBU_ALERTS "nse_oebu_alerts"
#define NSE_INFO "nseinfo"
#define NSE_DATACOPY "datacopy-nse"
#define BSE_REJECTS "bsehft-rejects"
#define NSE_PRODISSUES "production-issues" 
#define MAIL_SERVICE "mail-service"
namespace HFSAT {

// Class for sending notifications to a specific slack channel
class SlackManager {
 protected:
  CURL *curl;
  FILE *devnull;  // pointer to redirect the stderr curl output to /dev/null
  char json_payload[5000];
  bool channel_set;

 public:
  // Argument: name of slack channel
  SlackManager(std::string slack_channel = TESTCHANNEL) {
    std::string channel_url = "";
    channel_set = false;
    devnull = NULL;

    /*Picking corresponding slack URL for the channel slack_channel here
     * Add more slack channel-URL pairs here (else-if conditions).
     * Can have a config file, but the channels are going to be limited and constant,
     * so handling a separate config file will be extra hassle*/
    if (slack_channel == TESTCHANNEL) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (NSEMED == slack_channel) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == MIDTERM_MARGIN) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (MIDTERM_PROC_TRACKING == slack_channel) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (MIDTERM_ORDER_REJECTS == slack_channel) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (MIDTERM_PNL_ALERTS == slack_channel) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (MIDTERM_POS_MISMATCH == slack_channel) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (MIDTERM_SHOCKRISK == slack_channel) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (NSEHFT_REJECTS == slack_channel) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (NSE_OEBU_ALERTS == slack_channel) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (NSE_INFO == slack_channel) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (NSE_PRODISSUES == slack_channel) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (NSE_DATACOPY == slack_channel) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (BSE_REJECTS == slack_channel) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if ( MAIL_SERVICE == slack_channel) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else {
      std::cerr << "No corresponding slack URL found for " << slack_channel << "\n";
    }

    /*  In windows, this will init the winsock stuff */
    curl_global_init(CURL_GLOBAL_ALL);

    /*  get a curl handle */
    curl = curl_easy_init();

    if (curl && channel_set) {
      // curl will write some stuff to stderr, redirect it to /dev/null
      // Comment the below two lines if you want to debug why a request is not working
      devnull = fopen("/dev/null", "w+");
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, devnull);

      /*  First set the URL that is about to receive our POST. This URL can
       *  just as well be a https:// URL if that is what should receive the
       *  data. */
      curl_easy_setopt(curl, CURLOPT_URL, channel_url.c_str());
    }
  }

  ~SlackManager() {
    /*  always cleanup */
    curl_easy_cleanup(curl);
    if (devnull != NULL) {
      fclose(devnull);
      devnull = NULL;
    }
  }

  // This function sends the string message to the slack channel
  // The string must follow the format described here: https://api.slack.com/docs/formatting
  void sendNotification(std::string message) {
    if (!channel_set) {
      return;
    }

    std::ostringstream temp_oss;
    temp_oss << "payload={\"text\": \"" << message << "\"}";
    strcpy(json_payload, temp_oss.str().c_str());
    std::replace(json_payload, json_payload + strlen(json_payload), '&',
                 'n');  // Replace & as its not valid character in Slack API

    /*  Now specify the POST data */
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
    /*  Perform the request, res will get the return code */
    CURLcode res = curl_easy_perform(curl);
    /*  Check for errors */
    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
  }
};
}
#endif /*BASE_CDEF_EMAILUTILS_H_*/
