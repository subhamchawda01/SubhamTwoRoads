// =====================================================================================
//
//       Filename:  BMFDropCopyEngine.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  05/14/2014 11:57:30 AM
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

#include <ctime>
#include <map>
#include <string>
#include <fstream>

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"

#include "infracore/BasicOrderRoutingServer/defines.hpp"  // for DEF_MAX_SEC_ID
#include "dvccode/Utils/settings.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"

#include "infracore/BMFEP/BMFEPFIX/BMFEPFIXTypes.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPMessageDefs.hpp"

#include "infracore/BMFEP/BMFEPFIX/BMFEPLogon.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPLogout.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPHeartbeat.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPTestRequest.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPResendRequest.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPSeqReset.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPExecutionReport.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/lock.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/send_alert.hpp"

#include "dvccode/CDef/ors_messages.hpp"

using namespace HFSAT::ORS;

namespace HFSAT {
namespace Utils {

class BMFDropCopyEngine : public Thread {
 private:
  bool run_engine_;

  // TCP to exchange.
  TCPClientSocket tcp_client_socket_;
  /// Lock meant to make sure that the sequence numbers
  /// sent are synchronized .
  HFSAT::Lock send_seq_num_lock_;

  char socket_connect_host_[128];
  int socket_connect_port_;
  int heartbeat_interval_;

  // The messages.
  BMFEPFIX::Logon logon_;
  BMFEPFIX::Logout logout_;

  BMFEPFIX::Heartbeat heartbeat_;
  BMFEPFIX::TestRequest test_request_;

  BMFEPFIX::ExecutionReport exec_report_;

  BMFEPFIX::ResendRequest resend_request_;

  BMFEPFIX::SeqReset seq_reset_;

  // Sequencing information
  BMFEPFIX::t_FIX_SeqNum last_seq_num_;
  BMFEPFIX::t_FIX_SeqNum last_proc_seq_num_;

  // Session info.
  bool is_logged_in_;

  // Heartbeat and test request session info.
  bool is_uncomfirmed_heartbeat_;
  bool is_uncomfirmed_testrequest_;
  bool is_uncomfirmed_testrequest_2_;

  // For resending correctness. Refer doc. for 8.2.2 to understand.
  bool is_dup_resend_;

  // Maintain last sent out time, in case of a resend request.
  time_t last_send_time_;

  std::string seq_file_name_;
  std::string fixlog_file_name_;

  std::ofstream fixlog_ofs_;

  HFSAT::BulkFileWriter *fix_bulk_writer_;
  HFSAT::BulkFileWriter *fix_bulk_writer_our_msg;

  bool resend_request_processed_;
  bool use_affinity_;
  HFSAT::DebugLogger &dbglogger_;

  std::string senderCompID_;
  std::string targetCompID_;
  std::string accountID_;
  std::string password_;
  std::string appl_name_;
  std::string appl_version_;
  std::string sender_location_;
  std::string entering_trader_;
  std::string entering_firm_;

  HFSAT::DebugLogger &tradelogger_;

  bool resend_request_sent_;

 public:
  BMFDropCopyEngine(const HFSAT::ORS::Settings &_settings_, HFSAT::DebugLogger &_dbglogger_,
                    HFSAT::DebugLogger &_tradeslogger_, const std::string &_output_log_dir_)
      : run_engine_(false),
        tcp_client_socket_(),
        send_seq_num_lock_(),
        socket_connect_port_(0),
        heartbeat_interval_(30),
        logon_(true),
        logout_(true),
        heartbeat_(true),
        test_request_(true),
        exec_report_(true),
        resend_request_(true),
        seq_reset_(true),
        last_seq_num_(1),
        last_proc_seq_num_(1),
        is_logged_in_(false),
        is_uncomfirmed_heartbeat_(false),
        is_uncomfirmed_testrequest_(false),
        is_uncomfirmed_testrequest_2_(false),
        is_dup_resend_(false),
        seq_file_name_(_output_log_dir_),
        fixlog_file_name_(_output_log_dir_),
        fixlog_ofs_(),
        resend_request_processed_(false),
        use_affinity_((_settings_.has("UseAffinity") && _settings_.getValue("UseAffinity") == "Y")),
        dbglogger_(_dbglogger_),
        tradelogger_(_tradeslogger_),
        resend_request_sent_(false)

  {
    // It is okay to have inefficient stuff in the constructor, hopefully run only once.
    if (!_settings_.has("SenderCompID") || !_settings_.has("TargetCompID") || !_settings_.has("AccountName") ||
        !_settings_.has("Password") || !_settings_.has("SocketConnectHost") || !_settings_.has("SocketConnectPort") ||
        !_settings_.has("HeartBtInt") || !_settings_.has("SenderLocation") || !_settings_.has("EnteringTrader") ||
        !_settings_.has("EnteringFirm") || !_settings_.has("ApplNameTag58") || !_settings_.has("ApplVersion")) {
      dbglogger_ << "Config file doesnot have either of SenderCompID,  TargetCompID, AccountName, Password, "
                    "SocketConnectHost, SocketConnectPort, HeartBtInt, SenderLocation, EnteringTrader or "
                    "EnteringFirm or ApplNameTag58 or ApplVersion\n";
      dbglogger_.CheckToFlushBuffer();
      dbglogger_.Close();
      exit(EXIT_FAILURE);
    }

    heartbeat_interval_ = atoi(_settings_.getValue("HeartBtInt").c_str()) - 1;

    senderCompID_ = _settings_.getValue("SenderCompID");
    appl_name_ = _settings_.getValue("ApplNameTag58");
    appl_version_ = _settings_.getValue("ApplVersion");
    targetCompID_ = _settings_.getValue("TargetCompID");
    accountID_ = _settings_.getValue("AccountName");
    password_ = _settings_.getValue("Password");

    sender_location_ = _settings_.getValue("SenderLocation");
    entering_trader_ = _settings_.getValue("EnteringTrader");
    entering_firm_ = _settings_.getValue("EnteringFirm");

    logon_.setFields(senderCompID_.c_str(), targetCompID_.c_str(), password_.c_str(), appl_name_.c_str(),
                     appl_version_.c_str());
    heartbeat_.setFields(senderCompID_.c_str(), targetCompID_.c_str());
    test_request_.setFields(senderCompID_.c_str(), targetCompID_.c_str());
    logout_.setFields(senderCompID_.c_str(), targetCompID_.c_str());

    resend_request_.setFields(senderCompID_.c_str(), targetCompID_.c_str());

    seq_reset_.setFields(senderCompID_.c_str(), targetCompID_.c_str());
    strcpy(socket_connect_host_, _settings_.getValue("SocketConnectHost").c_str());
    socket_connect_port_ = atoi(_settings_.getValue("SocketConnectPort").c_str());

    seq_file_name_ = seq_file_name_ + "OPTIMUMFIX.4.4." + senderCompID_ + ".seqnum.log";
    fixlog_file_name_ = fixlog_file_name_ + "OPTIMUMFIX.4.4." + senderCompID_ + ".messages.log";

    std::cout << " Seq_num_file : " << seq_file_name_ << " FIXLOG : " << fixlog_file_name_ << std::endl;
    // Restore the sequence no.s from last time.
    fixlog_ofs_.open(fixlog_file_name_.c_str(), std::ios_base::app);
    time_t start_time_ = time(NULL);
    fixlog_ofs_ << "\n\t>>> " << ctime(&start_time_) << std::endl;

    fixlog_ofs_ << "\tOptimizeAssumptions : " << (true ? "Y" : "N") << std::endl;

    FILE *p_seq_file_ = fopen(seq_file_name_.c_str(), "r");
    if (p_seq_file_) {
      fscanf(p_seq_file_, "%lu %lu", &last_seq_num_, &last_proc_seq_num_);
      fixlog_ofs_ << "Read Seqnumbers from the file : LastSeq : Last Processed : " << seq_file_name_.c_str()
                  << last_seq_num_ << "   " << last_proc_seq_num_ << std::endl;
      fclose(p_seq_file_);
    }

    std::string bk_file_name = fixlog_file_name_ + ".EXCH.BIN.BULK";
    std::string our_bk_file_name = fixlog_file_name_ + ".OUR.BIN.BULK";
    fix_bulk_writer_ = new HFSAT::BulkFileWriter(bk_file_name.c_str(), 32 * 1024,
                                                 std::ofstream::binary | std::ofstream::app | std::ofstream::ate);
    fix_bulk_writer_our_msg = new HFSAT::BulkFileWriter(
        our_bk_file_name.c_str(), 32 * 1024, std::ofstream::binary | std::ofstream::app | std::ofstream::ate);
  }

  ~BMFDropCopyEngine() {
    fix_bulk_writer_->Close();
    fix_bulk_writer_our_msg->Close();
    fixlog_ofs_.close();

    // Cleanup
    delete fix_bulk_writer_;
    delete fix_bulk_writer_our_msg;
  }

  void Connect() { tcp_client_socket_.Connect(socket_connect_host_, socket_connect_port_); }

  void sendHeartbeat() {
    if (!is_logged_in_) {
      return;
    }
    if (is_uncomfirmed_testrequest_2_) {
      //      // This is what happened:
      //      // (1) We timed out 30 seconds, so we sent a heartbeat.
      //      // (2) We timed out again without receiving confirmation for that heartbeat, so we sent a test request.
      //      // (3) We timed out again without receiving confirmation for the test request sent out, we sent the 2nd
      //      test request.
      //      // (3) We timed out again without receiving confirmation for the 2nd test request sent out. <- The
      //      connection is dead.

      fixlog_ofs_ << "\tConnection to BMFEP was terminated. 2 Un-acked test request.\n" << std::endl;

      char hostname[128];
      hostname[127] = '\0';
      gethostname(hostname, 127);
      std::string alert_message = "ALERT: BMFEP Test request fail at " + std::string(hostname) + "\n";
      HFSAT::SendAlert::sendAlert(alert_message);
    }
    if (is_uncomfirmed_heartbeat_ && !is_uncomfirmed_testrequest_) {
      // We sent out a heartbeat msg 30 seconds ago, that never got confirmed,
      // time to send out a Test Request to check connectivity exists.

      fixlog_ofs_ << "\tUnconf. HBT: Sending TEST req. to Exchange" << std::endl;
      /*Not sending test requests immediately since our HBT interval < exchange's
        And they don't reply to such test requests*/
      // sendTestRequest ();
      last_send_time_ = time(NULL);
      is_uncomfirmed_testrequest_ = true;

    } else if (is_uncomfirmed_heartbeat_ && is_uncomfirmed_testrequest_) {
      fixlog_ofs_ << "\tUnconf. HBT : UNConf TESTREQ: Sending 2nd TESTREQ" << std::endl;
      sendTestRequest();
      is_uncomfirmed_testrequest_2_ = true;

    } else {
      heartbeat_.setUTCTime(last_send_time_ = time(NULL));

      send_seq_num_lock_.LockMutex();

      heartbeat_.setSeqNum(last_seq_num_++);
      heartbeat_.setCheckSum();

      fixlog_ofs_ << "<< " << heartbeat_.msg_char_buf_ << std::endl;

      // + 3 to skip over "10=", +3 to skip over "XXX" <- checksum and +1 to skip over ^A at the end.
      // Heartbeat msgs have varying lengths, the computation below has to be performed each time.
      unsigned int write_len_ = (int)(heartbeat_.Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH + BMFEP_FIX_Tag_10_Width_ +
                                      DELIMITER_SOH_WIDTH_ - heartbeat_.Header_Tag_8_);

      if (tcp_client_socket_.WriteN(write_len_, (void *)heartbeat_.msg_char_buf_) < (int)write_len_) {
        fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
      }

      send_seq_num_lock_.UnlockMutex();

      // This is to make sure that the heartbeat that we sent out, gets back a heartbeat as reply.
      is_uncomfirmed_heartbeat_ = true;
    }
    return;
  }

  void Login() {
    if (is_logged_in_) return;

    logon_.setUTCTime(last_send_time_ = time(NULL));
    send_seq_num_lock_.LockMutex();
    logon_.setSeqNum(last_seq_num_++);
    logon_.setCheckSum();

    // + 3 to skip over "10=", +3 to skip over "XXX" <- checksum and +1 to skip over ^A at the end.
    unsigned int write_len_ = (unsigned int)(logon_.Trailer_Tag_10_ + 3 + 3 + 1 - logon_.Header_Tag_8_);

    if (tcp_client_socket_.WriteN(write_len_, (void *)logon_.msg_char_buf_) < (int)write_len_) {
      fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
    }

    send_seq_num_lock_.UnlockMutex();

    fixlog_ofs_ << "<< " << logon_.msg_char_buf_ << std::endl;

    fixlog_ofs_ << " LOGON : " << last_seq_num_ << " : " << last_proc_seq_num_ << std::endl;
  }

  void Logout() {
    if (!is_logged_in_) {
      return;
    }

    logout_.setUTCTime(last_send_time_ = time(NULL));

    send_seq_num_lock_.LockMutex();

    logout_.setSeqNum(last_seq_num_++);
    logout_.setCheckSum();
    // + 3 to skip over "10=", +3 to skip over "XXX" <- checksum and +1 to skip over ^A at the end.
    unsigned int write_len_ = (unsigned int)(logout_.Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH +
                                             BMFEP_FIX_Tag_10_Width_ + DELIMITER_SOH_WIDTH_ - logout_.Header_Tag_8_);

    if (tcp_client_socket_.WriteN(write_len_, (void *)logout_.msg_char_buf_) < (int)write_len_) {
      fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
    }

    send_seq_num_lock_.UnlockMutex();

    fixlog_ofs_ << "<< " << logout_.msg_char_buf_ << std::endl;
  }

  // Will be accessed from HBT manager
  time_t &lastMsgSentAtThisTime() { return last_send_time_; }
  inline bool is_engine_running() { return run_engine_; }

  void Disconnect() { Logout(); }

  void sendTestRequest() {
    if (!is_logged_in_) {
      return;
    }
    test_request_.setUTCTime(last_send_time_ = time(NULL));
    send_seq_num_lock_.LockMutex();

    test_request_.setSeqNum(last_seq_num_++);
    test_request_.setCheckSum();

    unsigned int write_len_ = test_request_.getWriteLen();

    if (tcp_client_socket_.WriteN(write_len_, (void *)test_request_.msg_char_buf_) < (int)write_len_) {
      fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
    }

    send_seq_num_lock_.UnlockMutex();

    fixlog_ofs_ << "<< " << test_request_.msg_char_buf_ << std::endl;
  }

  void onExecReport(HFSAT::ORS::BMFEPFIX::ExecutionReport &exec_report_) {
    HFSAT::ORS::BMFEPFIX::t_FIX_SeqNum server_assigned_order_sequence_ = exec_report_.getExecTag11();
    HFSAT::ORS::BMFEPFIX::t_FIX_OrdStatus ordstatus_ = exec_report_.getExecTag39();
    HFSAT::ORS::BMFEPFIX::t_FIX_ExecType exectype_ = exec_report_.getExecTag150();

    fixlog_ofs_ << "35=9 | 39=" << ordstatus_ << " | 150=" << exectype_ << " | 11=" << server_assigned_order_sequence_
                << std::endl;

    switch (ordstatus_) {
      case '0': {
        // Order confirmation case
        // Tag 150 must be 0
        if (exectype_ == '0') {
        } else if (exectype_ == '5') {
        }

      } break;

      case '4':
        // case 'C': Ther
        {  // cancel confirmation
          //            if ( exectype_ == '4' || exectype_ == 'C')  // Canceled or expired
          //              p_engine_listener_->OnOrderCxl (server_assigned_order_sequence_, exec_report_.getExecTag55 ()
          //              );
        }
        break;

      case '5': {
      } break;

      case '1':
      // partial trade notification
      case '2':
        // full trade notification
        {
          tradelogger_ << exec_report_.getExecTag55() << '\001'
                       << (int32_t)(exec_report_.getExecTag54() == 1 ? kTradeTypeBuy : kTradeTypeSell) << '\001'
                       << exec_report_.getExecTag32() << '\001' << exec_report_.getExecTag31() << '\001'
                       << exec_report_.getExecTag11() << '\n';

          tradelogger_.DumpCurrentBuffer();

          struct timeval exec_time;
          gettimeofday(&exec_time, NULL);

          dbglogger_ << exec_report_.getExecTag55() << '\001'
                     << (int32_t)(exec_report_.getExecTag54() == 1 ? kTradeTypeBuy : kTradeTypeSell) << '\001'
                     << exec_report_.getExecTag32() << '\001' << exec_report_.getExecTag31() << '\001'
                     << exec_report_.getExecTag11() << '\001'
                     << (uint64_t)(exec_time.tv_sec * 1000000 + exec_time.tv_usec) << "\n";

          dbglogger_.DumpCurrentBuffer();
        }
        break;

      case '8': {
        // Doesnot matter what Tag 150 is (probably 8) we reject just on basis of 39=8
        //            p_engine_listener_->OnReject (server_assigned_order_sequence_, kExchOrderReject,
        //            exec_report_.getExecTag55 ());

      } break;

      default: {
        dbglogger_ << typeid(*this).name() << ':' << __func__ << ' ' << " Unexpected ordstatus_ = " << ordstatus_
                   << "\n";
        dbglogger_.DumpCurrentBuffer();
      }
    }
  }

  // The bunch of strstr calls can be eliminated by simply extracting the value
  // of the 35= tag value and running a simple switch case on it.
  void processExchMsg(char *msg_buf_) {
    fixlog_ofs_ << ">> " << msg_buf_ << std::endl;

    char tag_35_value_ = *(strstr(msg_buf_, "35=") + 3);
    int index_ = 0;
    char *reply_msg_buf_ = msg_buf_;

    // Definitely process the logon message
    if (tag_35_value_ == 'A') {
      is_logged_in_ = true;
      is_uncomfirmed_heartbeat_ = false;
      std::cout << "Is Logged in\n";
    }
    // Get the sequence no. on the received msg.
    // This is sent out as the lastProcessed Seq num on outgoing messag

    BMFEPFIX::t_FIX_SeqNum recv_last_proc_seq_num_ = 0;
    for (msg_buf_ = strstr(msg_buf_, "34=") + 3; *msg_buf_ != BMFEP_Delimiter_SOH_; ++msg_buf_) {
      recv_last_proc_seq_num_ = recv_last_proc_seq_num_ * 10 + (*msg_buf_ - '0');
    }

    // Definitely Reply to the TEST REQUEST of the EXCHANGE
    // else we will die
    if (tag_35_value_ == '1') {
      char tag_value_buf_[25];
      for (index_ = 0, msg_buf_ = strstr(msg_buf_, "112=") + 4; *msg_buf_ != BMFEP_Delimiter_SOH_; ++msg_buf_) {
        tag_value_buf_[index_++] = *msg_buf_;
      }
      tag_value_buf_[index_] = '\0';

      heartbeat_.SetTag112(tag_value_buf_);

      heartbeat_.setUTCTime(last_send_time_ = time(NULL));
      send_seq_num_lock_.LockMutex();

      heartbeat_.setSeqNum(last_seq_num_++);
      heartbeat_.setCheckSum();

      fixlog_ofs_ << "<< " << heartbeat_.msg_char_buf_ << std::endl;

      unsigned int write_len_ = (int)(heartbeat_.Trailer_Tag_10_ + 3 + 3 + 1 - heartbeat_.Header_Tag_8_);

      if (tcp_client_socket_.WriteN(write_len_, (void *)heartbeat_.msg_char_buf_) < (int)write_len_) {
        fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
      }

      send_seq_num_lock_.UnlockMutex();

      heartbeat_.ClearTag112();
      fixlog_ofs_ << "\tFirst resend request to be sent " << std::endl;
      // For BMF FIX, the logon handshake is different,
      // We are already logged in when we receive a 35=A message.

      is_logged_in_ = true;  // Redundant, but harmless.
      // onLogon (); // Redundant, eliminated to avoid side-effects if any.
    }

    if (recv_last_proc_seq_num_ > last_proc_seq_num_ + 1 && tag_35_value_ != '5') {
      if (resend_request_sent_) {
        return;
      }
      // Seqnum greater than expected, send resend request.

      fixlog_ofs_ << "\t Higher seq. no received Expected : Recv :" << recv_last_proc_seq_num_ << " Expec "
                  << last_proc_seq_num_ + 1 << " : Sending Resend Req.  " << std::endl;
      resend_request_.setUTCTime(last_send_time_ = time(NULL));

      // BMFEPFIX::t_FIX_SeqNum seq_num_to_be_resent_ = last_proc_seq_num_ + 1;
      // char seq_num_to_be_resent_buff [9];
      // for ( index_ = 0; seq_num_to_be_resent_ > 0;  seq_num_to_be_resent_ /= 10 )
      //   seq_num_to_be_resent_buff[index_++] = ((seq_num_to_be_resent_ % 10 ) + '0');
      // seq_num_to_be_resent_buff [ index_ ] = '\0';

      // resend_request_.SetTag7_16 ( seq_num_to_be_resent_buff );

      send_seq_num_lock_.LockMutex();

      resend_request_.setSeqNum(last_seq_num_++);
      resend_request_.setBeginSeqNo(last_proc_seq_num_ + 1);

      resend_request_.setCheckSum();

      fixlog_ofs_ << "<< " << resend_request_.msg_char_buf_ << std::endl;

      unsigned int write_len_ = (int)(resend_request_.Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH +
                                      BMFEP_FIX_Tag_10_Width_ + DELIMITER_SOH_WIDTH_ - resend_request_.Header_Tag_8_);

      if (tcp_client_socket_.WriteN(write_len_, (void *)resend_request_.msg_char_buf_) < (int)write_len_) {
        fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
      }

      send_seq_num_lock_.UnlockMutex();

      // The first resend request we send out should have tag 43 = N.
      if (!is_dup_resend_) {
        resend_request_.ClearDupField();

        // Hence forth all resend requests should have tag 43 = Y.
        is_dup_resend_ = true;
      }

      resend_request_sent_ = true;

      return;
    } else if (recv_last_proc_seq_num_ < last_proc_seq_num_ + 1 && tag_35_value_ != '5' && tag_35_value_ != '4') {
      // Assumption: Exchange is always correct in its seq. num
      // Something gone wrong on our end, probably in the sequence file
      // Reset our last msg seen from exchange

      // TODO: Scenario where exchange sends duplicate reset msg for our
      // duplicate resend req. SCENARIO ENCOUNTERED: increase our seq no,
      // decrease exchange seq. no.
      last_proc_seq_num_ = recv_last_proc_seq_num_;
      return;
    }

    resend_request_sent_ = false;

    // In sequence msg, update the last processed sequence no.
    ++last_proc_seq_num_;

    msg_buf_ = reply_msg_buf_;
    switch (tag_35_value_) {
      case '0': {
        // Last sent out heartbeat and/or test request is confirmed. We live on!
        is_uncomfirmed_heartbeat_ = false;
        is_uncomfirmed_testrequest_ = false;
        is_uncomfirmed_testrequest_2_ = false;
        //      last_proc_seq_num_ -= 5;
      } break;
      case 'A': {
        // Already handled above.
        return;
      } break;

      case '5': {
        is_logged_in_ = false;

        // >> 8=FIX.4.4|9=56|35=5|34=91|49=BRK06|52=20110721-17:48:56.300|56=CODVC01|10=042|
        last_proc_seq_num_ = 0;
        for (char *p_last_proc_seq_num_ = strstr(msg_buf_, "34=") + 3; *p_last_proc_seq_num_ != BMFEP_Delimiter_SOH_;
             ++p_last_proc_seq_num_) {
          last_proc_seq_num_ = last_proc_seq_num_ * 10 + (*p_last_proc_seq_num_ - '0');
        }
        // Logout confirmed or forced.
        FILE *p_seq_file_ = fopen(seq_file_name_.c_str(), "w");
        if (p_seq_file_) {
          fprintf(p_seq_file_, "%lu %lu\n", last_seq_num_, last_proc_seq_num_);
          fixlog_ofs_ << " Logged Out received: Write " << last_seq_num_ << " : " << last_proc_seq_num_ << std::endl;
          fclose(p_seq_file_);
        }

        run_engine_ = false;

      } break;

      case '2': {
        if (resend_request_processed_) {
          break;
        }
        resend_request_processed_ = true;
        // Resend Request.
        // Send a sequence reset msg.
        // This is the exchange specified behavior for Administrative msgs, but we always use it.
        // This eliminates the need to maintain sent out msgs.
        // Get the sequence no. specified in tag 7.
        unsigned long new_seq_num_ = last_seq_num_;
        for (index_ = last_seq_num_ = 0, msg_buf_ = strstr(msg_buf_, "\0017=") + 3; *msg_buf_ != BMFEP_Delimiter_SOH_;
             ++msg_buf_) {
          last_seq_num_ = last_seq_num_ * 10 + (*msg_buf_ - '0');
        }

        fixlog_ofs_ << " \tTag 7: " << last_seq_num_ << " Last Processe num :" << last_proc_seq_num_ << std::endl;
        new_seq_num_ = (last_seq_num_ > new_seq_num_) ? last_seq_num_ : new_seq_num_;
        new_seq_num_ = (last_proc_seq_num_ > new_seq_num_) ? last_proc_seq_num_ : new_seq_num_;

        ++new_seq_num_;
        fixlog_ofs_ << "New Seq. no: " << new_seq_num_ << std::endl;

        send_seq_num_lock_.LockMutex();

        seq_reset_.setSeqNum(last_seq_num_);
        seq_reset_.setNewSeqNum(new_seq_num_);
        seq_reset_.setUTCTime(time(NULL));
        seq_reset_.setCheckSum();
        fixlog_ofs_ << "<< " << seq_reset_.msg_char_buf_ << std::endl;

        unsigned int write_len_ = (int)(seq_reset_.Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH +
                                        BMFEP_FIX_Tag_10_Width_ + DELIMITER_SOH_WIDTH_ - seq_reset_.Header_Tag_8_);

        if (tcp_client_socket_.WriteN(write_len_, (void *)seq_reset_.msg_char_buf_) < (int)write_len_) {
          fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
        }

        last_seq_num_ = new_seq_num_;
        //      last_proc_seq_num_ = new_seq_num_ - 1;

        send_seq_num_lock_.UnlockMutex();
        fixlog_ofs_ << " \t Resend & Seq, Reset LPSN : " << last_proc_seq_num_ << std::endl;
        // Also send out a heart beat next time with the updated seq. nos..
        is_uncomfirmed_testrequest_2_ = is_uncomfirmed_testrequest_ = is_uncomfirmed_heartbeat_ = false;
      } break;

      case '8': {
        is_uncomfirmed_heartbeat_ = false;
        is_uncomfirmed_testrequest_ = false;
        is_uncomfirmed_testrequest_2_ = false;

        // Execution report.
        exec_report_.generateReport(msg_buf_);

        HFSAT::ORS::BMFEPFIX::t_FIX_OrdStatus ordstatus_ = exec_report_.getExecTag39();

        // don't broadcast rejects, these aren't filled up completely
        if (ordstatus_ == '8') {
          onExecReport(exec_report_);
          break;
        }

        onExecReport(exec_report_);
        break;

      } break;

      case '9': {
        // Cancel Rejection report.
        exec_report_.generateReport(msg_buf_);

      } break;

      case '3': {
      } break;

      case 'j': {
      } break;

      case '4': {
        // Gap fill sequence reset msg, just fast forward the sequence no.s

        // Get the sequence no. specified in tag 36.
        // Set the last processed sequence no. as this value - 1.
        char last_seq_buf_[20];
        for (index_ = 0, msg_buf_ = strstr(msg_buf_, "36=") + TWO_DIGIT_AND_EQUAL_WIDTH;
             *msg_buf_ != BMFEP_Delimiter_SOH_; ++msg_buf_) {
          last_seq_buf_[index_++] = *msg_buf_;
        }
        last_seq_buf_[index_] = '\0';
        last_proc_seq_num_ = atol(last_seq_buf_) - 1;  // TODO_OPT
        fixlog_ofs_ << " \tFill Gap from Exchange: : LProcess_Seq_num :" << last_proc_seq_num_ << std::endl;
        is_uncomfirmed_testrequest_2_ = false;
        is_uncomfirmed_testrequest_ = false;
        is_uncomfirmed_heartbeat_ = false;
      } break;
    }

    if (tag_35_value_ != '2') {  // Something other than a resend request was just processed.
      resend_request_processed_ = false;
    }

    // FIXBASED ENGINE is not obliged to send HBT to our HBT,
    // so any next message should be treated as the
    // response to the last sent HBT. This looks the correct semantic
    // @Verified from reliable sources at the CME_EXCHANGE
    is_uncomfirmed_heartbeat_ = false;

    fixlog_ofs_ << " \t Processed Seq No: Expecting " << last_proc_seq_num_ + 1 << std::endl;
  }

  // Handle multiple msgs. Using a naive implementation now, should be optimized.
  void processMsgs(char *msg_char_buf_, int read_size_) {
    char *p_end_msg_char_ = msg_char_buf_ + read_size_;

    for (char *temp_msg_char_buf_ = strstr(msg_char_buf_, "\00110=");
         temp_msg_char_buf_ && msg_char_buf_ < p_end_msg_char_; temp_msg_char_buf_ = strstr(msg_char_buf_, "\00110=")) {
      temp_msg_char_buf_[8] = '\0';
      if (strlen(msg_char_buf_) > 40) {
        processExchMsg(msg_char_buf_);
      }
      msg_char_buf_ = temp_msg_char_buf_ + 8;
      *msg_char_buf_ = '8';
    }
  }

  void thread_main() {
    if (use_affinity_) {
      setName("BMFDropCopyEngine");
      AllocateCPUOrExit();
    }

#define MAX_BMFEPFIX_MSG_SIZE 16384

    char msg_char_buf_[MAX_BMFEPFIX_MSG_SIZE];
    run_engine_ = true;
    int read_offset_ = 0;
    char delim[3] = {'E', 'O', 'M'};
    char beg[3] = {'B', 'E', 'G'};

    while (run_engine_) {
      char *read_location_ = msg_char_buf_ + read_offset_;
      int read_size_ = tcp_client_socket_.ReadN(
          MAX_BMFEPFIX_MSG_SIZE - read_offset_ -
              1,  //-1 since we are relying on setting the '\0' delimiter after the received message
          (void *)read_location_);

      if (read_size_ > 0) {
        fix_bulk_writer_->Write(beg, 3);
        fix_bulk_writer_->Write(read_location_, read_size_);
        fix_bulk_writer_->Write(delim, 3);
        fix_bulk_writer_->CheckToFlushBuffer();

        read_offset_ += read_size_;
        msg_char_buf_[read_offset_] = '\0';

        if (!strncmp(msg_char_buf_ + read_offset_ - 8, "\00110=", 4)) {
          processMsgs(msg_char_buf_, read_offset_);
          read_offset_ = 0;
        } else {  // An incomplete message was read off the socket.
          dbglogger_ << "bmfep_log : Incomplete message read_size_=" << read_size_ << " read_offset_=" << read_offset_
                     << " @ " << HFSAT::GetTimeOfDay() << "\n";

          // Check whether it has a valid completely formed msg within it.
          // Starting from the end , and moving towards the front ,
          // find a valid checksum tag 10=XXX
          char t_incomplete_msg_[MAX_BMFEPFIX_MSG_SIZE];
          memset(t_incomplete_msg_, 0, sizeof(t_incomplete_msg_));

          // Eg. msg is
          // "8=FIX.4.49=30635=834=17266049=OE01552=20120419-17:14:46.29056=XLIN00681=357866=011=08439114=017=7256:673306537=724553491738=139=040=244=6335048=480054554=155=WINM1259=060=20120419-17:14:46.30375=20120419150=0151=1198=72117109123453=3448=COLO0447=D452=54448=DVC447=D452=36448=8447=D452=710=1048=FIX.4.49=30635=834=172"
          // We want to process
          // ""8=FIX.4.49=30635=834=17266049=OE01552=20120419-17:14:46.29056=XLIN00681=357866=011=08439114=017=7256:673306537=724553491738=139=040=244=6335048=480054554=155=WINM1259=060=20120419-17:14:46.30375=20120419150=0151=1198=72117109123453=3448=COLO0447=D452=54448=DVC447=D452=36448=8447=D452=710=104"
          // and get the remaining incomplete message "8=FIX.4.49=30635=834=172" in msg_char_buf_ with
          // read_offset_ = 27 , in position for reading the remaining message.

          int start_index_ = read_offset_ - 8;
          for (; start_index_ >= 0; --start_index_) {
            if (!strncmp(msg_char_buf_ + start_index_, "\00110=", 4)) {
              start_index_ += 8;  // Go to the beginning of the next msg.

              // Copy the remaining part of the incomplete msg to the temporary buffer.
              strncpy(t_incomplete_msg_, msg_char_buf_ + start_index_, read_offset_ - start_index_);

              // Null terminate the complete msg before sending it to processMsgs.
              msg_char_buf_[start_index_] = '\0';
              processMsgs(msg_char_buf_, start_index_);

              // Having processed the complete message , copy the incomplete portion
              // of the next message back to msg_char_buf , the remainder of
              // this message will be read in the next call to ReadN.
              strcpy(msg_char_buf_, t_incomplete_msg_);
              read_offset_ = strlen(t_incomplete_msg_);

              break;
            }
          }
        }
      } else {
        run_engine_ = false;
      }
    }

#undef MAX_BMFEPFIX_MSG_SIZE
  }
};
}
}
