// =====================================================================================
//
//       Filename:  combined_mds_messages_chix_l1_processor.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/30/2014 01:06:07 PM
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

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/af_msg_parser.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"

namespace HFSAT {

class AflashListener {
 public:
  virtual ~AflashListener() {}

  virtual void onAflashMsgNew(int uid_, timeval time_, char symbol_[4], uint8_t type_, uint8_t version_,
                              uint8_t nfields_, AFLASH_MDS::AFlashDatum fields[AFLASH_MDS::MAX_FIELDS],
                              uint16_t category_id_) = 0;
};

class CombinedMDSMessagesAflashProcessor {
 protected:
  DebugLogger& dbglogger_;  ///< error logger
  // std::vector <uint16_t> valid_categories_ ;
  std::vector<AflashListener*> aflash_listeners_;
  ExternalTimeListener* p_time_keeper_;

  inline void NotifyAflashListeners(AFLASH_MDS::AFlashCommonStructLive* next_event_) {
    //    dbglogger_ << "Notifying AflashListeners, size:" << aflash_listeners_.size() << "\n";
    for (std::vector<AflashListener*>::iterator it = aflash_listeners_.begin(); it != aflash_listeners_.end(); it++) {
      AflashListener* this_listener_ = *it;
      this_listener_->onAflashMsgNew(next_event_->uid_, next_event_->time_, next_event_->symbol_, next_event_->type_,
                                     next_event_->version_, next_event_->nfields_, next_event_->fields,
                                     next_event_->category_id_);
    }
  }

  CombinedMDSMessagesAflashProcessor(DebugLogger& _dbglogger_) : dbglogger_(_dbglogger_), p_time_keeper_(NULL) {
    /*
    AF_MSGSPECS::AF_MsgParser& af_msgparser_ = AF_MSGSPECS::AF_MsgParser::GetUniqueInstance( &dbglogger_ );
    std::string catlists_fname_ = "/spare/local/tradeinfo/Alphaflash/allowed_category_ids";

    if ( ! FileUtils::ExistsAndReadable ( catlists_fname_ ) ) {
      std::string t_err_ = std::string("Allowed Category Ids List file missing: ") + catlists_fname_;
      ExitVerbose ( kExitErrorCodeGeneral, t_err_.c_str() );
    }

    std::ifstream catlists_infile_;
    catlists_infile_.open ( catlists_fname_.c_str() );

    if ( catlists_infile_.is_open() ) {
      const int kL1AvgBufferLen = 4096;
      char readline_buffer_ [ kL1AvgBufferLen ];

      while ( catlists_infile_.good() ) {
        bzero ( readline_buffer_, kL1AvgBufferLen );
        catlists_infile_.getline ( readline_buffer_, kL1AvgBufferLen );
        PerishableStringTokenizer st_ ( readline_buffer_, kL1AvgBufferLen );
        const std::vector < const char * > & tokens_ = st_.GetTokens();
        if ( tokens_.size ( ) > 0u ) {
          valid_categories_.push_back( atoi(tokens_[0]) );
        }
      }
    }
    */
  }

  CombinedMDSMessagesAflashProcessor(CombinedMDSMessagesAflashProcessor const& copy);
  CombinedMDSMessagesAflashProcessor& operator=(CombinedMDSMessagesAflashProcessor const& copy);

  ~CombinedMDSMessagesAflashProcessor() {}

 public:
  static CombinedMDSMessagesAflashProcessor* GetUniqueInstance(DebugLogger& _dbglogger_) {
    static CombinedMDSMessagesAflashProcessor p_uniqueinstance_(_dbglogger_);
    return &p_uniqueinstance_;
  }

  inline void AddAflashListener(AflashListener* p_new_listener_) {
    aflash_listeners_.push_back(p_new_listener_);
    dbglogger_ << "New listener added to CombinedMDSMessagesAflashProcessor\n";
  }

  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline void ProcessAflashEvent(AFLASH_MDS::AFlashCommonStructLive* next_event_) {
    if (next_event_ == NULL) {
      std::cerr << "Error: The AFlashCommonStructLive received is NULL \n";
      dbglogger_ << "Error: The AFlashCommonStructLive received is NULL \n";
      exit(1);
    }

    //    dbglogger_ << "Alphaflash Message Received in CombinedMDSMessagesAflashProcessor\n";
    //    dbglogger_ << next_event_->ToString() << "\n";
    //    dbglogger_.DumpCurrentBuffer () ;

    /*
    if ( std::find( valid_categories_.begin(), valid_categories_.end(), (int) next_event_->category_id_ ) ==
    valid_categories_.end() ) {
      dbglogger_ << "This message does not lie in the valid category messages\n";
      return;
    }*/
    p_time_keeper_->OnTimeReceived(next_event_->time_);

    AF_MSGSPECS::AF_MsgParser& af_msgparser_ = AF_MSGSPECS::AF_MsgParser::GetUniqueInstance(&dbglogger_);
    AF_MSGSPECS::Category* catg_ = af_msgparser_.getCategoryforId(next_event_->category_id_);
    if (!catg_) {
      dbglogger_ << "Error: Category does NOT match the specifications\n";
      return;
    }
    AF_MSGSPECS::Message* msg_ = af_msgparser_.getMsgFromCatg(catg_, (short int)next_event_->type_);
    if (!msg_) {
      dbglogger_ << "Error: Message does NOT match the specifications\n";
      return;
    }

    if (next_event_->type_ == AF_MSGSPECS::kEstimate) {
      dbglogger_ << "Estimate Message.. Ignoring it\n";
    }

    NotifyAflashListeners(next_event_);
  }
};
}
