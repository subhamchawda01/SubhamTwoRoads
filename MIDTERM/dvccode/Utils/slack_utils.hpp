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
#define AWSISSUES "aws-issues"
#define PRODISSUES "prod-issues"
#define EVENTUPDATES "events"
#define TESTCHANNEL "test"
#define PRODUCTIONUPDATES "productionupdates"
#define NOTIFICATIONMONITOR "notification_monitor"
#define AUDITCHANNEL "dvc-audits"
#define RISKMANGER "risk-manager"
#define AWSZABBIX "aws-zabbix"
#define RESULTISSUES "result_issues"
#define STRATHIGHLIGHTS "strat_highlights"
#define DATAINFRA "datainfra"
#define NSEMED "nsemed"
#define WEEKLYSG "weeklysg"
#define MONTHLYSG "monthlysg"
#define RV_STRATEGY "banknifty-rv"
#define MOMENTUM "momentum"
#define MIDTERMDATA "midterm-data-gen"
#define DISPERSION "midterm-dispersion"
#define EARNINGS_STRAT "midterm-earnings"
#define TAG_PNL "tag_based_pnl"
#define EOD_TRADE_OPS "eod_trade_ops"
#define NSE_PROD_ISSUES "nse-prod-issues"
#define SIM_INFRA "sim-infra"
#define EOD_PRIVATE "eod_private"
#define BASETRADE "basetrade"
#define SIMREALDIFF "sim_real_diff"

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
  SlackManager(std::string slack_channel = AWSISSUES) {
    std::string channel_url = "";
    channel_set = false;
    devnull = NULL;

    /*Picking corresponding slack URL for the channel slack_channel here
     * Add more slack channel-URL pairs here (else-if conditions).
     * Can have a config file, but the channels are going to be limited and constant,
     * so handling a separate config file will be extra hassle*/
    if (slack_channel == AWSISSUES) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == PRODISSUES) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == EVENTUPDATES) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == TESTCHANNEL) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == PRODUCTIONUPDATES) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == NOTIFICATIONMONITOR) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == AUDITCHANNEL) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == RISKMANGER) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == AWSZABBIX) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == RESULTISSUES) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == STRATHIGHLIGHTS) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (NSEMED == slack_channel) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (WEEKLYSG == slack_channel) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == DATAINFRA) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == MONTHLYSG) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == RV_STRATEGY) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == MOMENTUM) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == MIDTERMDATA) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == EARNINGS_STRAT) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == DISPERSION) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == TAG_PNL) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == EOD_TRADE_OPS) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == NSE_PROD_ISSUES) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == SIM_INFRA) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == ALERTS) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == EOD_PRIVATE) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == BASETRADE) {
      channel_url = "https://hooks.slack.com/services/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";
      channel_set = true;
    } else if (slack_channel == SIMREALDIFF) {
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
