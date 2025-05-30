// =====================================================================================
//
//       Filename:  csm_feed_arbitrator.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/29/2014 06:29:08 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include <iostream>
#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/send_alert.hpp"

#define MAX_LAG_BETWEEN_TWO_FEEDS 250  // 250 usec
#define MAX_HISTORY_THRESHOLD 120      // If this is breached, there is some bug

#define MAX_LAG_BETWEEN_TWO_FEEDS_IN_COUNT 250

namespace HFSAT {
namespace Utils {

class CSMFeedArbitrator {
 private:
  HFSAT::DebugLogger& dbglogger_;

  std::vector<CSM_MDS::CSMCommonStruct> csm_l1_feed_history_;
  std::vector<CSM_MDS::CSMCommonStruct> csm_bookdepth_feed_history_;

  int32_t csm_l1_feed_last_processed_sequence_num_;
  int32_t csm_bookdepth_feed_last_processed_sequence_num_;

  int32_t csm_l1_feed_unique_seq_based_l1_count_;
  int32_t csm_bookdepth_feed_unique_seq_based_l1_count_;

  bool matching_point_found_;

  double last_bid_price_l1_feed_;
  int32_t last_bid_total_size_l1_feed_;
  double last_ask_price_l1_feed_;
  int32_t last_ask_total_size_l1_feed_;

  double current_bid_price_l1_feed_;
  int32_t current_bid_total_size_l1_feed_;
  double current_ask_price_l1_feed_;
  int32_t current_ask_total_size_l1_feed_;

  double last_bid_price_bookdepth_feed_;
  int32_t last_bid_total_size_bookdepth_feed_;
  double last_ask_price_bookdepth_feed_;
  int32_t last_ask_total_size_bookdepth_feed_;

  bool is_in_alert_mode_;
  bool is_alert_notification_on_;
  std::string product_;

  void NotifyFeedOutOfSyncLimitBreached(const std::string& _alert_body_) {
    return;

    // also send an alert
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    std::string alert_message = "ALERT: CSM Feed OutOfSync Limit Breached, At " + std::string(hostname);
    //        HFSAT::SendAlert::sendAlert(alert_message);

    HFSAT::Email e;

    e.setSubject("CSM -- Feed Sync Alert");
    e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
    e.addSender("ravi.parikh@tworoads.co.in");
    e.content_stream << "host_machine: " << hostname << "<br/>";
    e.content_stream << "Message : " << _alert_body_ << "<br/>";
    e.sendMail();
  }

  void CheckForFeedLeadLag() {
    if (abs((int)(csm_l1_feed_unique_seq_based_l1_count_ - csm_bookdepth_feed_unique_seq_based_l1_count_)) >
        MAX_LAG_BETWEEN_TWO_FEEDS_IN_COUNT) {
      if (!is_in_alert_mode_) {
        std::ostringstream t_temp_oss;
        t_temp_oss << " CSM Feed out of sync : " << product_ << " L1 : " << csm_l1_feed_unique_seq_based_l1_count_
                   << " L2 : " << csm_bookdepth_feed_unique_seq_based_l1_count_ << "\n";

        NotifyFeedOutOfSyncLimitBreached(t_temp_oss.str());
        is_in_alert_mode_ = true;

        DBGLOG_CLASS_FUNC_LINE << " CSM Feed out of sync : " << product_
                               << " L1 : " << csm_l1_feed_unique_seq_based_l1_count_
                               << " L2 : " << csm_bookdepth_feed_unique_seq_based_l1_count_ << "\n";
        DBGLOG_DUMP;
      }

    } else {
      if (is_in_alert_mode_) is_in_alert_mode_ = false;
    }
  }

  void CleanUpExcessiveHistory(std::string _contract_) {
    // single point of failure is matching_point_found variable
    if (matching_point_found_) {
      csm_l1_feed_history_.clear();
      csm_bookdepth_feed_history_.clear();

    } else {
      if (csm_l1_feed_history_.size() > MAX_HISTORY_THRESHOLD ||
          csm_bookdepth_feed_history_.size() > MAX_HISTORY_THRESHOLD) {
        std::cerr << "Fatal Error, History Storage Breached Threshold, L1 history : " << csm_l1_feed_history_.size()
                  << " BookDepth history : " << csm_bookdepth_feed_history_.size() << " For : " << _contract_
                  << " Last Seq : " << csm_l1_feed_last_processed_sequence_num_ << " "
                  << csm_bookdepth_feed_last_processed_sequence_num_ << "\n";

        DBGLOG_CLASS_FUNC_LINE << "Fatal Error, History Storage Breached Threshold, L1 history : "
                               << csm_l1_feed_history_.size()
                               << " BookDepth history : " << csm_bookdepth_feed_history_.size() << "\n";
        DBGLOG_DUMP;

        //            for ( unsigned int i = 0 ; i < csm_l1_feed_history_.size () ; i ++ ) {
        //
        //              std::cout << csm_l1_feed_history_ [ i ].ToString () ;
        //
        //            }
        //
        //            for ( unsigned int j = 0 ; j < csm_bookdepth_feed_history_.size () ; j++ ) {
        //
        //              std::cerr << csm_bookdepth_feed_history_ [ j ] .ToString () ;
        //
        //            }
        //
        //            exit ( -1 ) ;
      }
    }
  }

  inline bool GenerateArbitrationStatsFromThisDelta(CSM_MDS::msgType& _feed_type_,
                                                    CSM_MDS::CSMCommonStruct& _csm_best_level_data_) {
    bool same_seq_msg = false;

    if (CSM_MDS::CSM_TOB == _feed_type_) {
      if (csm_l1_feed_last_processed_sequence_num_ < _csm_best_level_data_.message_seq_no_) {
        csm_l1_feed_unique_seq_based_l1_count_++;

      } else if (csm_l1_feed_last_processed_sequence_num_ == _csm_best_level_data_.message_seq_no_) {
        if (csm_l1_feed_unique_seq_based_l1_count_ >= csm_bookdepth_feed_unique_seq_based_l1_count_)
          same_seq_msg = true;
      }

      csm_l1_feed_last_processed_sequence_num_ = _csm_best_level_data_.message_seq_no_;

    } else {
      if (csm_bookdepth_feed_last_processed_sequence_num_ < _csm_best_level_data_.message_seq_no_) {
        csm_bookdepth_feed_unique_seq_based_l1_count_++;

      } else if (csm_bookdepth_feed_last_processed_sequence_num_ == _csm_best_level_data_.message_seq_no_) {
        if (csm_bookdepth_feed_last_processed_sequence_num_ > csm_l1_feed_unique_seq_based_l1_count_)
          same_seq_msg = true;
      }

      csm_bookdepth_feed_last_processed_sequence_num_ = _csm_best_level_data_.message_seq_no_;
    }

    if (is_alert_notification_on_) CheckForFeedLeadLag();

    return same_seq_msg;
  }

  bool AreTheTwoFeedL1StructsSame(CSM_MDS::CSMCommonStruct& _csm_l1_, CSM_MDS::CSMCommonStruct& _csm_book_l1_) {
    if (abs((_csm_l1_.time_.tv_sec * 1000000 + _csm_l1_.time_.tv_usec) -
            (_csm_book_l1_.time_.tv_sec * 1000000 + _csm_book_l1_.time_.tv_usec)) > MAX_LAG_BETWEEN_TWO_FEEDS)
      return false;

    if (_csm_l1_.data_.csm_dels_.price_ != _csm_book_l1_.data_.csm_dels_.price_) return false;
    if (_csm_l1_.data_.csm_dels_.size_[0] != _csm_book_l1_.data_.csm_dels_.size_[0]) return false;
    if (_csm_l1_.data_.csm_dels_.type_ != _csm_book_l1_.data_.csm_dels_.type_) return false;

    return true;
  }

  bool CanMatchingPointBeFoundUsingThisDelta(CSM_MDS::msgType& _feed_type_,
                                             CSM_MDS::CSMCommonStruct& _csm_best_level_data_) {
    // only called if the matching point is not already set
    matching_point_found_ = false;

    if (CSM_MDS::CSM_TOB == _feed_type_) {
      for (int book_depth_history_counter = csm_bookdepth_feed_history_.size() - 1; book_depth_history_counter >= 0;
           book_depth_history_counter--) {
        if (AreTheTwoFeedL1StructsSame(_csm_best_level_data_,
                                       csm_bookdepth_feed_history_[book_depth_history_counter])) {
          csm_l1_feed_last_processed_sequence_num_ = _csm_best_level_data_.message_seq_no_;
          csm_bookdepth_feed_last_processed_sequence_num_ =
              csm_bookdepth_feed_history_[book_depth_history_counter].message_seq_no_;
          csm_l1_feed_unique_seq_based_l1_count_ = 1;
          csm_bookdepth_feed_unique_seq_based_l1_count_ = 1;
          matching_point_found_ = true;
          break;
        }
      }

    } else {
      for (int l1_history_counter = csm_l1_feed_history_.size() - 1; l1_history_counter >= 0; l1_history_counter--) {
        if (AreTheTwoFeedL1StructsSame(csm_l1_feed_history_[l1_history_counter], _csm_best_level_data_)) {
          csm_l1_feed_last_processed_sequence_num_ = csm_l1_feed_history_[l1_history_counter].message_seq_no_;
          csm_bookdepth_feed_last_processed_sequence_num_ = _csm_best_level_data_.message_seq_no_;
          csm_l1_feed_unique_seq_based_l1_count_ = 1;
          csm_bookdepth_feed_unique_seq_based_l1_count_ = 1;
          matching_point_found_ = true;
          break;
        }
      }
    }

    return matching_point_found_;
  }

 public:
  CSMFeedArbitrator(HFSAT::DebugLogger& _dbglogger_)
      : dbglogger_(_dbglogger_),
        csm_l1_feed_history_(),
        csm_bookdepth_feed_history_(),
        csm_l1_feed_last_processed_sequence_num_(0),
        csm_bookdepth_feed_last_processed_sequence_num_(0),
        csm_l1_feed_unique_seq_based_l1_count_(0),
        csm_bookdepth_feed_unique_seq_based_l1_count_(0),
        matching_point_found_(false),
        last_bid_price_l1_feed_(0.00),
        last_bid_total_size_l1_feed_(0),
        last_ask_price_l1_feed_(0.00),
        last_ask_total_size_l1_feed_(0),
        current_bid_price_l1_feed_(0.00),
        current_bid_total_size_l1_feed_(0),
        current_ask_price_l1_feed_(0.00),
        current_ask_total_size_l1_feed_(0),
        last_bid_price_bookdepth_feed_(0.00),
        last_bid_total_size_bookdepth_feed_(0),
        last_ask_price_bookdepth_feed_(0.00),
        last_ask_total_size_bookdepth_feed_(0),
        is_in_alert_mode_(false),
        is_alert_notification_on_(true)

  {}

  inline void TurnOffAlertNotificationForSpread() { is_alert_notification_on_ = false; }

  inline bool ShouldProcessThisL1FromThisFeed(
      CSM_MDS::msgType& _feed_type_, CSM_MDS::CSMCommonStruct& _csm_best_level_data_,
      std::vector<CSM_MDS::CSMCommonStruct>& _pending_intermediate_to_be_processed_) {
    product_ = _csm_best_level_data_.contract_;

    // Needed this as we didn't subscribe to l1 for strategies till May6,2014
    //        if ( std::string ( _csm_best_level_data_.contract_ ).find ( "_Time" ) != std::string::npos ) return true ;

    if (CSM_MDS::CSM_DELTA != _feed_type_ && CSM_MDS::CSM_TOB != _feed_type_) {
      DBGLOG_CLASS_FUNC_LINE << "Unexpected Feed Type : " << _feed_type_
                             << " Data : " << _csm_best_level_data_.ToString() << "\n";
      DBGLOG_DUMP;
      return false;
    }

    if (1 != _csm_best_level_data_.data_.csm_dels_.level_) {
      DBGLOG_CLASS_FUNC_LINE << "Unexpected Level : " << _csm_best_level_data_.data_.csm_dels_.level_
                             << " Data : " << _csm_best_level_data_.ToString() << "\n";
      DBGLOG_DUMP;
      return false;
    }

    if (matching_point_found_) {
      bool duplicate_packets_ = false;

      if (CSM_MDS::CSM_TOB == _feed_type_) {
        // BID
        if ('0' == _csm_best_level_data_.data_.csm_dels_.type_) {
          current_bid_price_l1_feed_ = _csm_best_level_data_.data_.csm_dels_.price_;

          if (0 != _csm_best_level_data_.data_.csm_dels_.size_[0]) {
            current_bid_total_size_l1_feed_ = _csm_best_level_data_.data_.csm_dels_.size_[0];
          }

        } else {  // ASK

          current_ask_price_l1_feed_ = _csm_best_level_data_.data_.csm_dels_.price_;

          if (0 != _csm_best_level_data_.data_.csm_dels_.size_[0]) {
            current_ask_total_size_l1_feed_ = _csm_best_level_data_.data_.csm_dels_.size_[0];
          }
        }

        if (!_csm_best_level_data_.data_.csm_dels_.intermediate_) {
          if (0 != last_bid_price_l1_feed_) {
            duplicate_packets_ = ((last_bid_price_l1_feed_ == current_bid_price_l1_feed_) &&
                                  (last_bid_total_size_l1_feed_ == current_bid_total_size_l1_feed_) &&
                                  (last_ask_price_l1_feed_ == current_ask_price_l1_feed_) &&
                                  (last_ask_total_size_l1_feed_ == current_ask_total_size_l1_feed_));
          }

          last_bid_price_l1_feed_ = current_bid_price_l1_feed_;
          last_ask_price_l1_feed_ = current_ask_price_l1_feed_;
          last_bid_total_size_l1_feed_ = current_bid_total_size_l1_feed_;
          last_ask_total_size_l1_feed_ = current_ask_total_size_l1_feed_;
        }

      } else if (CSM_MDS::CSM_DELTA == _feed_type_) {
        if (0 != last_bid_price_bookdepth_feed_) {
          if ('0' == _csm_best_level_data_.data_.csm_dels_.type_) {
            duplicate_packets_ =
                (last_bid_price_bookdepth_feed_ == _csm_best_level_data_.data_.csm_dels_.price_ &&
                 last_bid_total_size_bookdepth_feed_ == _csm_best_level_data_.data_.csm_dels_.size_[0]);

          } else {
            duplicate_packets_ =
                (last_ask_price_bookdepth_feed_ == _csm_best_level_data_.data_.csm_dels_.price_ &&
                 last_ask_total_size_bookdepth_feed_ == _csm_best_level_data_.data_.csm_dels_.size_[0]);
          }
        }

        if ('0' == _csm_best_level_data_.data_.csm_dels_.type_) {
          last_bid_price_bookdepth_feed_ = _csm_best_level_data_.data_.csm_dels_.price_;
          last_bid_total_size_bookdepth_feed_ = _csm_best_level_data_.data_.csm_dels_.size_[0];

        } else {
          last_ask_price_bookdepth_feed_ = _csm_best_level_data_.data_.csm_dels_.price_;
          last_ask_total_size_bookdepth_feed_ = _csm_best_level_data_.data_.csm_dels_.size_[0];
        }
      }

      if (duplicate_packets_) {
        std::cerr << "DUPLICATE PACKET FOUND AT SEQUEN : " << _csm_best_level_data_.message_seq_no_ << "\n";
        return false;
      }

      if (_csm_best_level_data_.data_.csm_dels_.intermediate_ && CSM_MDS::CSM_TOB == _feed_type_) {
        _pending_intermediate_to_be_processed_.push_back(_csm_best_level_data_);
        return false;
      }

      //          std::cerr << " Before Going For generation : " << _csm_best_level_data_.message_seq_no_ << " Counters
      //          : " << csm_l1_feed_unique_seq_based_l1_count_ << " " << csm_bookdepth_feed_unique_seq_based_l1_count_
      //          << "\n" ;

      if (!GenerateArbitrationStatsFromThisDelta(_feed_type_, _csm_best_level_data_)) {
        //            if ( csm_l1_feed_unique_seq_based_l1_count_ == csm_bookdepth_feed_unique_seq_based_l1_count_ ) {
        //
        //              std::cerr << " Feed Match Point : " << csm_l1_feed_unique_seq_based_l1_count_ << " " <<
        //              csm_bookdepth_feed_unique_seq_based_l1_count_ << " THis Message Sequence : " <<
        //              _csm_best_level_data_.message_seq_no_<< " Last Processed Seq L1 : " <<
        //              csm_l1_feed_last_processed_sequence_num_ << " BookDepth : " <<
        //              csm_bookdepth_feed_last_processed_sequence_num_ << "\n" ;
        //
        //            }

        if (CSM_MDS::CSM_TOB == _feed_type_) {
          return csm_l1_feed_unique_seq_based_l1_count_ >= csm_bookdepth_feed_unique_seq_based_l1_count_;

        } else {
          return csm_bookdepth_feed_unique_seq_based_l1_count_ > csm_l1_feed_unique_seq_based_l1_count_;
        }

      } else {
        // This is for duplicate sequence message
        return true;
      }

    } else {
      bool matching_point_using_this_l1_update =
          CanMatchingPointBeFoundUsingThisDelta(_feed_type_, _csm_best_level_data_);

      if (!matching_point_using_this_l1_update) {
        if (CSM_MDS::CSM_TOB == _feed_type_) {
          // store history
          csm_l1_feed_history_.push_back(_csm_best_level_data_);

        } else {
          csm_bookdepth_feed_history_.push_back(_csm_best_level_data_);
        }

        CleanUpExcessiveHistory(_csm_best_level_data_.contract_);

        return CSM_MDS::CSM_DELTA == _feed_type_;
      }
    }

    // default
    return false;
  }
};
}
}
