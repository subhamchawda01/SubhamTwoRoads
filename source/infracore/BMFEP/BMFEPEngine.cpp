// =====================================================================================
//
//       Filename:  BMFEPEngine.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/18/2015 07:02:56 AM
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

#include <typeinfo>
#include <ctime>

#include "infracore/BMFEP/BMFEPEngine.hpp"
#include "dvccode/Utils/send_alert.hpp"
#include "dvccode/Utils/misc.hpp"
#include "infracore/BasicOrderRoutingServer/sequence_generator.hpp"
#include "infracore/BasicOrderRoutingServer/multisession_engine.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

namespace HFSAT {
namespace ORS {

#define MAX_BMFEPFIX_MSG_SIZE 8192

BMFEPEngine::BMFEPEngine(Settings settings, HFSAT::DebugLogger &logger, std::string output_log_dir_, int _id,
                         AsyncWriter *pWriter, AsyncWriter *pReader)
    : BaseEngine(settings, logger),
      make_optimize_assumptions_(
          (settings.has("OptimizeAssumptions") && settings.getValue("OptimizeAssumptions") == "Y")),
      id_(_id),
      run_engine_(false),
      tcp_client_socket_(true, pWriter, pReader, output_log_dir_ + "/audit." +
                                                     HFSAT::DateTime::GetCurrentIsoDateLocalAsString() + "." +
                                                     settings.getValue("SenderCompID")),
      socket_connect_port_(0),
      heartbeat_interval_(30),

      container_(make_optimize_assumptions_),

      last_seq_num_(1),
      last_proc_seq_num_(1),

      is_logged_in_(false),

      is_uncomfirmed_heartbeat_(false),
      is_uncomfirmed_testrequest_(false),
      is_uncomfirmed_testrequest_2_(false),

      last_send_time_(0),

      seq_file_name_(output_log_dir_),
      fixlog_file_name_(output_log_dir_),
      fixlog_ofs_(),
      security_id_list_(),

      resend_request_processed_(false),

      use_dropcopy_mcast_packets_((settings.has("UseDropCopy") && settings.getValue("UseDropCopy") == "Y")),
      dropcopy_multicast_receiver_socket_(settings.getValue("DC_MCASTIP"),
                                          atoi(settings.getValue("DC_MCASTPORT").c_str()), "eth5"),
      on_exec_saos_processed_(),
      on_conf_saos_processed_(),
      on_cxl_saos_processed_(),
      on_mod_saos_processed_(),
      on_rej_saos_processed_(),
      on_cxlrej_saos_processed_(),
      on_tradebust_saos_processed_(),
      on_pexec_saos_processed_(),
      msg_char_buf_(nullptr),
      read_offset_(0),
      this_exch_source_(settings.getValue("Exchange")),
      current_processing_ptr_(NULL),
      current_tag_11_sequence_number_ptr_(NULL),
      last_processed_exchange_reply_sequence_number_(-1),
      current_server_assigned_sequnece_in_processing_(-1),
      current_exchange_sequence_number_(0),
      current_execution_price_(0.0),
      current_execution_price_factor_(0),
      current_executed_qty_(0),
      current_remaining_qty_(0),
      started_processing_exec_notifs_(false),
      is_resend_request_sent_in_playback_mode_(false),
      is_live_order_cancelled_in_playback_mode_(false),
      last_heartbeat_seq_num_in_playback_mode_(0),
      playback_mgr_(),
      is_exchange_stp_enabled_(false),
      MAX_SEQUENCE_NUMBERS_WE_EXPECT_DURING_ONE_RUN(32768),
      sequence_to_metadata_vec_(),
      starting_sequence_offset_(0),
      fast_px_convertor_vec_(DEF_MAX_SEC_ID, NULL),
      processe_queue_req_(),
      is_test_request_cached_(false),
      is_resend_request_cached_(false),
      is_seq_reset_cached_(false) {
  // It is okay to have inefficient stuff in the constructor, hopefully run only once.
  if (!settings.has("SenderCompID") || !settings.has("TargetCompID") || !settings.has("AccountName") ||
      !settings.has("Password") || !settings.has("SocketConnectHost") || !settings.has("SocketConnectPort") ||
      !settings.has("HeartBtInt") || !settings.has("SenderLocation") || !settings.has("EnteringTrader") ||
      !settings.has("EnteringFirm")) {
    dbglogger_ << "Config file doesnot have either of SenderCompID,  TargetCompID, AccountName, Password, "
                  "SocketConnectHost, SocketConnectPort, HeartBtInt, SenderLocation, EnteringTrader or EnteringFirm\n";
    dbglogger_.CheckToFlushBuffer();
    dbglogger_.Close();
    exit(EXIT_FAILURE);
  }

  if (std::string("BMFEP") != this_exch_source_ && std::string("BMFEQ") != this_exch_source_) {
    dbglogger_ << "THIS ENGINE ONLY SUPPORTS BMFEP / BMFEQ MESSAGE SPECS \n";
    dbglogger_.DumpCurrentBuffer();

    exit(-1);
  }

  is_exchange_stp_enabled_ = ((settings.has("EnableExchangeSTP")) && ("Y" == settings.getValue("EnableExchangeSTP")));

  DBGLOG_CLASS_FUNC_LINE_INFO << "EXCHANGE STP ENABLED ? " << is_exchange_stp_enabled_ << DBGLOG_ENDL_DUMP;

  // -1 because sometimes it times out with 30 s, and 35=1 msgs are expensive.
  heartbeat_interval_ = atoi(settings.getValue("HeartBtInt").c_str()) - 1;

  senderCompID_ = settings.getValue("SenderCompID");
  targetCompID_ = settings.getValue("TargetCompID");
  accountID_ = settings.getValue("AccountName");
  password_ = settings.getValue("Password");

  if (!settings.has("ApplNameTag58") || !settings.has("ApplVersion")) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "COULDN'T FIND TAG 58 FOR LOGIN : " << DBGLOG_ENDL_DUMP_AND_EXIT;
  }

  appl_name_ = settings.getValue("ApplNameTag58");
  appl_version_ = settings.getValue("ApplVersion");

  sender_location_ = settings.getValue("SenderLocation");
  entering_trader_ = settings.getValue("EnteringTrader");
  entering_firm_ = settings.getValue("EnteringFirm");

  container_.logon.setFields(senderCompID_.c_str(), targetCompID_.c_str(), password_.c_str(), appl_name_.c_str(),
                             appl_version_.c_str());
  container_.heartbeat.setFields(senderCompID_.c_str(), targetCompID_.c_str());
  container_.test_request_heartbeat_.setFields(senderCompID_.c_str(), targetCompID_.c_str());
  container_.test_request.setFields(senderCompID_.c_str(), targetCompID_.c_str());
  container_.logout.setFields(senderCompID_.c_str(), targetCompID_.c_str());
  container_.resend_request.setFields(senderCompID_.c_str(), targetCompID_.c_str());
  container_.seq_reset.setFields(senderCompID_.c_str(), targetCompID_.c_str());
  strcpy(socket_connect_host_, settings.getValue("SocketConnectHost").c_str());
  socket_connect_port_ = atoi(settings.getValue("SocketConnectPort").c_str());

  std::ostringstream t_temp_oss;
  t_temp_oss << "/ExchangeMessageLogging_" << senderCompID_ << "_" << HFSAT::DateTime::GetCurrentIsoDateLocal()
             << ".log";
  fixlog_file_name_ = fixlog_file_name_ + t_temp_oss.str();

  stringstream ss;
  ss << seq_file_name_ << "OPTIMUMFIX.4.4.seqnum." << senderCompID_ << ".log";
  seq_file_name_ = ss.str();
  std::cout << " Seq_num_file : " << seq_file_name_ << " FIXLOG : " << fixlog_file_name_ << std::endl;
  // Restore the sequence no.s from last time.
  fixlog_ofs_.open(fixlog_file_name_, std::ios_base::app);
  time_t start_time_ = time(NULL);
  fixlog_ofs_ << "\n\t>>> " << ctime(&start_time_) << std::endl;
  fixlog_ofs_ << "\tOptimizeAssumptions : " << (make_optimize_assumptions_ ? "Y" : "N") << std::endl;

  if (!Misc::ReadSequenceNo(seq_file_name_, &last_seq_num_, &last_proc_seq_num_)) {
    fixlog_ofs_ << "Read Seqnumbers from the file : LastSeq : Last Processed : " << seq_file_name_ << last_seq_num_
                << "   " << last_proc_seq_num_ << std::endl;
  }

  //--------------Security-indexeer---------------
  // Open up the FIXFAST reference file for BMFEP and read the group code for the symbol.
  // (TAG 55): Internal_ref_id e.g ( DOLN15, WINQ11, etc)
  // (TAG 48): BMFEPBR+ numeric_id of instrument in TAG 55

  std::string ref_filename = DEF_NTP_REFLOC_;

  if (std::string("BMFEQ") == this_exch_source_) {
    ref_filename = DEF_PUMA_REFLOC_;
  }

  std::ifstream ref_file_;
  ref_file_.open(ref_filename.c_str(), std::ifstream::in);

  if (!ref_file_.is_open()) {
    std::cerr << "Could not open " << ref_filename << std::endl;
    exit(-1);
  }

  char line[1024];
  while (!ref_file_.eof()) {
    bzero(line, 1024);
    ref_file_.getline(line, 1024);
    if (strlen(line) == 0 || strstr(line, "#") != NULL) continue;
    HFSAT::PerishableStringTokenizer st_(line, 1024);
    const std::vector<const char *> &tokens_ = st_.GetTokens();
    if (tokens_.size() < 3) {
      std::cerr << "Malformatted line in " << ref_filename << std::endl;
      exit(-1);
    }
    // Symbol is internal_ref_id
    // Sec id is BMFEPBR + numeric_id
    symbol_to_sec_id_map_[std::string(tokens_[1])] = "BMFBR" + std::string(tokens_[0]);
  }
  ref_file_.close();

  //-------------------End SimpleSecurtityIndexing------------------------
  // The reason is to make cancelorders  here so
  // that bmf_ep_client_thread can access them as well to place order and
  // account thread to cancelallpending orders on logout, etc-----------

  std::string bk_file_name = fixlog_file_name_ + ".EXCH.BIN.BULK";
  std::string our_bk_file_name = fixlog_file_name_ + ".OUR.BIN.BULK";
  fix_bulk_writer_ = new HFSAT::BulkFileWriter(bk_file_name.c_str(), 32 * 1024,
                                               std::ofstream::binary | std::ofstream::app | std::ofstream::ate);
  fix_bulk_writer_our_msg = new HFSAT::BulkFileWriter(our_bk_file_name.c_str(), 32 * 1024,
                                                      std::ofstream::binary | std::ofstream::app | std::ofstream::ate);
  msg_char_buf_ = (char *)calloc(MAX_BMFEPFIX_MSG_SIZE, 1);
  if (!msg_char_buf_) {
    std::cerr << "failed to allocate msg buffer of size:" << MAX_BMFEPFIX_MSG_SIZE << std::endl;
    exit(-2);
  }
  run_engine_ = true;
  // todo: passing 'this' from the constructor is a plain wrong;
  // create a setter/Init method for supplying the p_engine
  // Initialize the pool of sequence and it's metadata, Don't move this block you can see I depend on the starting
  // sequence offset for the rest of accesses to this vector
  for (uint32_t memory_allocation_counter = 0;
       memory_allocation_counter < MAX_SEQUENCE_NUMBERS_WE_EXPECT_DURING_ONE_RUN; memory_allocation_counter++) {
    sequence_to_metadata_vec_.push_back(new SequenceMetaData());
  }

  // SHould be the last line of the constructor
  starting_sequence_offset_ = last_seq_num_;

  HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance().AddSSSIListener(this);

  processe_queue_req_.orq_request_type_ = ORQ_PROCESS_QUEUE;
}

BMFEPEngine::~BMFEPEngine() {
  fix_bulk_writer_->Close();
  fix_bulk_writer_our_msg->Close();
  fixlog_ofs_.close();

  free(msg_char_buf_);

  // Cleanup
  delete fix_bulk_writer_;
  delete fix_bulk_writer_our_msg;

  // CleanUp
  for (uint32_t memory_deallocation_counter = 0; memory_deallocation_counter < sequence_to_metadata_vec_.size();
       memory_deallocation_counter++) {
    if (NULL != sequence_to_metadata_vec_[memory_deallocation_counter]) {
      delete (sequence_to_metadata_vec_[memory_deallocation_counter]);
      sequence_to_metadata_vec_[memory_deallocation_counter] = NULL;
    }
  }

  // Deallocate fast_px_convertor_vec_
  for (int secid = 0; secid < DEF_MAX_SEC_ID; secid++) {
    if (fast_px_convertor_vec_[secid] != NULL) {
      delete fast_px_convertor_vec_[secid];
      fast_px_convertor_vec_[secid] = NULL;
    }
  }
}

inline void BMFEPEngine::UpdateSequenceMetadataVector(const BMFEPFIX::t_FIX_SeqNum &seqnum_, const int &order_saos_,
                                                      const bool &is_send_or_cxl_, const bool &flush_) {
  // Vector Storage Exhausted
  if (seqnum_ - starting_sequence_offset_ >= MAX_SEQUENCE_NUMBERS_WE_EXPECT_DURING_ONE_RUN) {
    for (uint32_t memory_allocation_counter = 0;
         memory_allocation_counter < MAX_SEQUENCE_NUMBERS_WE_EXPECT_DURING_ONE_RUN; memory_allocation_counter++) {
      sequence_to_metadata_vec_.push_back(new SequenceMetaData());
    }

    // Double up the range
    MAX_SEQUENCE_NUMBERS_WE_EXPECT_DURING_ONE_RUN = MAX_SEQUENCE_NUMBERS_WE_EXPECT_DURING_ONE_RUN * 2;
  }

  (sequence_to_metadata_vec_[seqnum_ - starting_sequence_offset_])->server_assigned_sequence = order_saos_;
  (sequence_to_metadata_vec_[seqnum_ - starting_sequence_offset_])->is_send_otherwise_cxl_ =
      is_send_or_cxl_;  // TODO NO SUPPORT FOR MODIFY YET, SIMPLY TREAT AS SEND
  (sequence_to_metadata_vec_[seqnum_ - starting_sequence_offset_])->flush_trigger_ =
      flush_;  // indicate data has been flushed
}

void BMFEPEngine::sendLogon() {
  if (is_logged_in_) return;

  container_.logon.setUTCTime(last_send_time_ = time(NULL));
  container_.logon.setSeqNum(last_seq_num_++);
  container_.logon.setCheckSum();

  // + 3 to skip over "10=", +3 to skip over "XXX" <- checksum and +1 to skip over  at the end.
  unsigned int write_len_ =
      (unsigned int)(container_.logon.Trailer_Tag_10_ + 3 + 3 + 1 - container_.logon.Header_Tag_8_);

  if (tcp_client_socket_.WriteN(write_len_, (void *)container_.logon.msg_char_buf_) < (int)write_len_) {
    fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
  }

  fixlog_ofs_ << "<< " << container_.logon.msg_char_buf_ << std::endl;

  fixlog_ofs_ << " LOGON : " << last_seq_num_ << " : " << last_proc_seq_num_ << std::endl;

  IsOnloadMsgWarmSuppoted(tcp_client_socket_.socket_file_descriptor());
  DBGLOG_CLASS_FUNC_LINE_INFO << "Onload Msg Warm Supported?: " << is_msg_warm_supported_ << DBGLOG_ENDL_FLUSH;
}

void BMFEPEngine::ResendRequest(BMFEPFIX::t_FIX_SeqNum begin_seq_num) {
  if (!is_logged_in_) {
    dbglogger_ << "MSES: Sending resend request without loggin in: " << id_ << "\n";
    return;
  }

  container_.resend_request.setBeginSeqNo(begin_seq_num);
  container_.resend_request.setSeqNum(last_seq_num_++);
  timeval current_time;
  gettimeofday(&current_time, NULL);
  container_.resend_request.setUTCTime(last_send_time_ = (time_t)current_time.tv_sec);

  if (!is_dup_resend_) {
    container_.resend_request.ClearDupField();

    // Hence forth all resend requests should have tag 43 = Y.
    is_dup_resend_ = true;
  }

  container_.resend_request.setCheckSum();

  // + 3 to skip over "10=", +3 to skip over "XXX" <- checksum and +1 to skip over  at the end.
  unsigned int write_len_ =
      (unsigned int)(container_.resend_request.Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH + BMFEP_FIX_Tag_10_Width_ +
                     DELIMITER_SOH_WIDTH_ - container_.resend_request.Header_Tag_8_);

  if (tcp_client_socket_.WriteN(write_len_, (void *)container_.resend_request.msg_char_buf_) < (int)write_len_) {
    fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
  }

  fixlog_ofs_ << "<< " << container_.resend_request.msg_char_buf_ << std::endl;

  fixlog_ofs_ << " RESENDREQ : " << last_seq_num_ << " : " << last_proc_seq_num_ << std::endl;
}


void BMFEPEngine::sendLogout() {
  if (!is_logged_in_) {
    return;
  }

  container_.logout.setUTCTime(last_send_time_ = time(NULL));

  container_.logout.setSeqNum(last_seq_num_++);
  container_.logout.setCheckSum();
  // + 3 to skip over "10=", +3 to skip over "XXX" <- checksum and +1 to skip over  at the end.
  unsigned int write_len_ =
      (unsigned int)(container_.logout.Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH + BMFEP_FIX_Tag_10_Width_ +
                     DELIMITER_SOH_WIDTH_ - container_.logout.Header_Tag_8_);

  if (tcp_client_socket_.WriteN(write_len_, (void *)container_.logout.msg_char_buf_) < (int)write_len_) {
    fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
  }

  fixlog_ofs_ << "<< " << container_.logout.msg_char_buf_ << std::endl;
}

void BMFEPEngine::CommitFixSequences() {
  // Logout confirmed or forced.
  FILE *p_seq_file_ = fopen(seq_file_name_.c_str(), "w");
  if (p_seq_file_) {
    fprintf(p_seq_file_, "%lu %lu\n", last_seq_num_, last_proc_seq_num_);
    fixlog_ofs_ << " Logged Out received: Write " << last_seq_num_ << " : " << last_proc_seq_num_ << std::endl;
    fclose(p_seq_file_);
  }
}

// // Some methods inherited from BaseEngine.
// ///order management calls
void BMFEPEngine::SendOrder(const Order *rp_order_, BMFEPFIX::NewOrder *p_target_new_order_) {
  if (true == is_logged_in_) {
    time_t last_send_time_ = (time_t)rp_order_->ors_timestamp_.tv_sec;
    // Batch Processing Of Dynamic Fields
    uint32_t write_len_ =
        p_target_new_order_->SetDynamicSendOrderFieldsUsingOrderStruct(rp_order_, last_send_time_, last_seq_num_);

    if (tcp_client_socket_.WriteN(write_len_, (void *)p_target_new_order_->msg_char_buf_) < (int)write_len_) {
      fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
    }

    UpdateSequenceMetadataVector(last_seq_num_++, rp_order_->server_assigned_order_sequence_, true, true);

#if NOT_USING_ZERO_LOGGIN_ORS
    fixlog_ofs_ << "<< " << p_target_new_order_->msg_char_buf_ << std::endl;
#endif

  } else {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "BMFEP SENDING ORDER WITHOUT BEING LOGGED IN : " << id_ << DBGLOG_ENDL_FLUSH;
  }
}
// // Must be optimized.
void BMFEPEngine::CancelOrder(const Order *rp_order_, BMFEPFIX::CancelOrder *p_target_cancel_order_) {
  if (is_logged_in_) {
    time_t last_send_time_ = (time_t)rp_order_->ors_timestamp_.tv_sec;
    uint32_t write_len_ =
        p_target_cancel_order_->SetDynamicCancelOrderFieldsUsingOrderStruct(rp_order_, last_send_time_, last_seq_num_);

#if NOT_USING_ZERO_LOGGIN_ORS
    fixlog_ofs_ << "Sending the cancel order to exchange " << std::endl;
#endif

    if (tcp_client_socket_.WriteN(write_len_, (void *)p_target_cancel_order_->msg_char_buf_) < (int)write_len_) {
      fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
    }

    UpdateSequenceMetadataVector(last_seq_num_++, rp_order_->server_assigned_order_sequence_, false, true);

#if NOT_USING_ZERO_LOGGIN_ORS
    fixlog_ofs_ << "<< " << p_target_cancel_order_->msg_char_buf_ << std::endl;
#endif

  } else {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "BMFEP TRYING TO CANCEL AN ORDER WITHOUT BEING LOGGED IN : " << id_
                                 << DBGLOG_ENDL_FLUSH;
  }
}

// // Must be optimized.
// // Have not TESTED this guy much
void BMFEPEngine::ModifyOrder(const Order *rp_order_, BMFEPFIX::CancelReplace *p_target_cancel_replace_,
                              Order *orig_order) {
  if (is_logged_in_) {
    time_t last_send_time_ = (time_t)rp_order_->ors_timestamp_.tv_sec;
    uint32_t write_len_ = p_target_cancel_replace_->SetDynamicCancelReplaceOrderFieldsUsingOrderStruct(
        rp_order_, last_send_time_, last_seq_num_);

#if NOT_USING_ZERO_LOGGIN_ORS
    fixlog_ofs_ << "<< " << p_target_cancel_replace_->msg_char_buf_ << std::endl;
#endif

    if (tcp_client_socket_.WriteN(write_len_, (void *)p_target_cancel_replace_->msg_char_buf_) < (int)write_len_) {
      fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
    }

    UpdateSequenceMetadataVector(last_seq_num_++, rp_order_->server_assigned_order_sequence_, true, true);

  } else {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "BMFEP TRYING TO CANCEL AN ORDER WITHOUT BEING LOGGED IN : " << id_
                                 << DBGLOG_ENDL_FLUSH;
  }
}

// There are only 3 types of messages that can be added to queue ( or cached ). Those are TestRequest, ResednRequest and
// SequenceReset
void BMFEPEngine::ProcessMessageQueue() {
  if (is_test_request_cached_) {
    container_.test_request_heartbeat_.setUTCTime(last_send_time_ = time(NULL));

    container_.test_request_heartbeat_.setSeqNum(last_seq_num_++);
    container_.test_request_heartbeat_.setCheckSum();

    fixlog_ofs_ << "<< " << container_.test_request_heartbeat_.msg_char_buf_ << std::endl;

    unsigned int write_len_ = (int)(container_.test_request_heartbeat_.Trailer_Tag_10_ + 3 + 3 + 1 -
                                    container_.test_request_heartbeat_.Header_Tag_8_);

    if (tcp_client_socket_.WriteN(write_len_, (void *)container_.test_request_heartbeat_.msg_char_buf_) <
        (int)write_len_) {
      fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
    }

    container_.test_request_heartbeat_.ClearTag112();

    fixlog_ofs_ << "\tFirst resend request to be sent " << std::endl;
    // For BMF FIX, the logon handshake is different,
    // We are already logged in when we receive a 35=A message.

    is_logged_in_ = true;  // Redundant, but harmless.
    is_test_request_cached_ = false;
  }

  if (is_resend_request_cached_) {
    container_.resend_request.setUTCTime(last_send_time_ = time(NULL));
    container_.resend_request.setSeqNum(last_seq_num_++);
    container_.resend_request.setCheckSum();

    fixlog_ofs_ << "<< " << container_.resend_request.msg_char_buf_ << std::endl;

    unsigned int write_len_ =
        (int)(container_.resend_request.Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH + BMFEP_FIX_Tag_10_Width_ +
              DELIMITER_SOH_WIDTH_ - container_.resend_request.Header_Tag_8_);

    if (tcp_client_socket_.WriteN(write_len_, (void *)container_.resend_request.msg_char_buf_) < (int)write_len_) {
      fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
    }

    container_.resend_request.SetDupField();

    is_resend_request_cached_ = false;
  }

  if (is_seq_reset_cached_) {
    fixlog_ofs_ << " \tTag 7: " << tag_7_resend_req_ << " Last Processe num :" << last_proc_seq_num_ << std::endl;
    BMFEPFIX::t_FIX_SeqNum new_seq_num_ = (last_seq_num_ > tag_7_resend_req_) ? last_seq_num_ : tag_7_resend_req_;

    ++new_seq_num_;
    fixlog_ofs_ << "New Seq. no: " << new_seq_num_ << std::endl;

    container_.seq_reset.setSeqNum(tag_7_resend_req_);
    container_.seq_reset.setNewSeqNum(new_seq_num_);
    container_.seq_reset.setUTCTime(time(NULL));
    container_.seq_reset.setCheckSum();

    fixlog_ofs_ << "<< " << container_.seq_reset.msg_char_buf_ << std::endl;

    unsigned int write_len_ =
        (int)(container_.seq_reset.Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH + BMFEP_FIX_Tag_10_Width_ +
              DELIMITER_SOH_WIDTH_ - container_.seq_reset.Header_Tag_8_);

    if (tcp_client_socket_.WriteN(write_len_, (void *)container_.seq_reset.msg_char_buf_) < (int)write_len_) {
      fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
    }

    last_seq_num_ = new_seq_num_;
    //        last_proc_seq_num_ = new_seq_num_ - 1;

    fixlog_ofs_ << " \t Resend & Seq, Reset LPSN : " << last_proc_seq_num_ << std::endl;
    // Also send out a heart beat next time with the updated seq. nos..
    is_uncomfirmed_testrequest_2_ = is_uncomfirmed_testrequest_ = is_uncomfirmed_heartbeat_ = false;

    is_seq_reset_cached_ = false;
  }
}

void BMFEPEngine::sendTestRequest() {
  if (!is_logged_in_) {
    return;
  }
  container_.test_request.setUTCTime(last_send_time_ = time(NULL));
  container_.test_request.setSeqNum(last_seq_num_++);
  container_.test_request.setCheckSum();

  unsigned int write_len_ = container_.test_request.getWriteLen();

  if (tcp_client_socket_.WriteN(write_len_, (void *)container_.test_request.msg_char_buf_) < (int)write_len_) {
    fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
  }

  fixlog_ofs_ << "<< " << container_.test_request.msg_char_buf_ << std::endl;
}

void BMFEPEngine::ProcessFakeSend(HFSAT::ORS::Order *ord, ORQType_t type) {
  if (!is_logged_in_) {
    return;
  }

  BMFEPFIX::NewOrder *new_ord = container_.new_order_map[ord->security_id_];

  if (new_ord == NULL) return;

  new_ord->setSeqNum(0);

  int write_len = new_ord->getWriteLen();

  if (is_msg_warm_supported_) {
    send(tcp_client_socket_.socket_file_descriptor(), &new_ord->msg_char_buf_, write_len, ONLOAD_MSG_WARM);
  }
}

void BMFEPEngine::CheckToSendHeartbeat() {
  if (!is_logged_in_) {
    return;
  }

  time_t cptime = time(NULL);
  int diff = cptime - lastMsgSentAtThisTime();
  if (diff >= heartbeat_interval_ - 1) {
    sendHeartbeat();
  }
}

void BMFEPEngine::sendHeartbeat() {
  if (!is_logged_in_) {
    return;
  }
  if (is_uncomfirmed_testrequest_2_) {
    // 	// This is what happened:
    // 	// (1) We timed out 30 seconds, so we sent a heartbeat.
    // 	// (2) We timed out again without receiving confirmation for that heartbeat, so we sent a test request.
    // 	// (3) We timed out again without receiving confirmation for the test request sent out, we sent the 2nd test
    // request.
    // 	// (3) We timed out again without receiving confirmation for the 2nd test request sent out. <- The connection is
    // dead.

    fixlog_ofs_ << "\tConnection to BMFEP was terminated. 2 Un-acked test request.\n" << std::endl;

    sendLogout();
    is_logged_in_ = false;
    onLogout();  // Treat it as if we were logged out.

    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    std::string alert_message = "ALERT: BMFEP Test request fail at " + std::string(hostname) + "\n";
    HFSAT::SendAlert::sendAlert(alert_message);
  }
  {
    container_.heartbeat.setUTCTime(last_send_time_ = time(NULL));

    container_.heartbeat.setSeqNum(last_seq_num_++);
    container_.heartbeat.setCheckSum();

#if NOT_USING_ZERO_LOGGIN_ORS

    fixlog_ofs_ << "<< " << container_.heartbeat.msg_char_buf_ << std::endl;

#endif

    // + 3 to skip over "10=", +3 to skip over "XXX" <- checksum and +1 to skip over  at the end.
    // Heartbeat msgs have varying lengths, the computation below has to be performed each time.
    unsigned int write_len_ =
        (int)(container_.heartbeat.Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH + BMFEP_FIX_Tag_10_Width_ +
              DELIMITER_SOH_WIDTH_ - container_.heartbeat.Header_Tag_8_);

    if (tcp_client_socket_.WriteN(write_len_, (void *)container_.heartbeat.msg_char_buf_) < (int)write_len_) {
      fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
    }

    // This is to make sure that the heartbeat that we sent out, gets back a heartbeat as reply.
    is_uncomfirmed_heartbeat_ = true;
  }
  return;
}

// The bunch of strstr calls can be eliminated by simply extracting the value
// of the 35= tag value and running a simple switch case on it.
void BMFEPEngine::processExchMsg(char *msg_buf_) {
#if NOT_USING_ZERO_LOGGIN_ORS

  fixlog_ofs_ << ">> " << msg_buf_ << std::endl;

#endif
  auto tag35 = strstr(msg_buf_, "35=");
  if (!tag35) {
    dbglogger_ << "MSESS: msg with tag. len = " << strlen(msg_buf_) << " data = \n>> " << msg_buf_ << "\n";
    return;
  }
  char tag_35_value_ = *(tag35 + 3);
  int index_ = 0;
  char *reply_msg_buf_ = msg_buf_;

  // Definitely process the logon message
  if (tag_35_value_ == 'A') {
    is_logged_in_ = true;
    onLogon();
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

    container_.heartbeat.SetTag112(tag_value_buf_);

    container_.heartbeat.setUTCTime(last_send_time_ = time(NULL));

    container_.heartbeat.setSeqNum(last_seq_num_);  // donot incerement seq. on response to test request
    container_.heartbeat.setCheckSum();

    fixlog_ofs_ << "<< " << container_.heartbeat.msg_char_buf_ << std::endl;

    unsigned int write_len_ =
        (int)(container_.heartbeat.Trailer_Tag_10_ + 3 + 3 + 1 - container_.heartbeat.Header_Tag_8_);

    if (tcp_client_socket_.WriteN(write_len_, (void *)container_.heartbeat.msg_char_buf_) < (int)write_len_) {
      fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
    }

    container_.heartbeat.ClearTag112();
    fixlog_ofs_ << "\tFirst resend request to be sent " << std::endl;
    // For BMF FIX, the logon handshake is different,
    // We are already logged in when we receive a 35=A message.

    is_logged_in_ = true;  // Redundant, but harmless.
    // onLogon (); // Redundant, eliminated to avoid side-effects if any.
  }

  if (recv_last_proc_seq_num_ > last_proc_seq_num_ + 1 && tag_35_value_ != '5') {
    // Seqnum greater than expected, send resend request.

    fixlog_ofs_ << "\t Higher seq. no received Expected : Recv :" << recv_last_proc_seq_num_ << " Expec "
                << last_proc_seq_num_ + 1 << " : Sending Resend Req.  " << std::endl;

    ResendRequest(last_proc_seq_num_ + 1);

    return;
  } else if (recv_last_proc_seq_num_ < last_proc_seq_num_ + 1 && tag_35_value_ != '5') {
    // Assumption: Exchange is always correct in its seq. num
    // Something gone wrong on our end, probably in the sequence file
    // Reset our last msg seen from exchange

    // TODO: Scenario where exchange sends duplicate reset msg for our
    // duplicate resend req. SCENARIO ENCOUNTERED: increase our seq no,
    // decrease exchange seq. no.
    last_proc_seq_num_ = recv_last_proc_seq_num_;
    return;
  }

  // In sequence msg, update the last processed sequence no.
  ++last_proc_seq_num_;

  // if (is_dup_resend_) {
  // 	is_dup_resend_ = false;
  // 	container_.resend_request.SetDupField ();
  // }

  msg_buf_ = reply_msg_buf_;
  switch (tag_35_value_) {
    case '0': {
      // Last sent out heartbeat and/or test request is confirmed. We live on!
      is_uncomfirmed_heartbeat_ = false;
      is_uncomfirmed_testrequest_ = false;
      is_uncomfirmed_testrequest_2_ = false;
      //        last_proc_seq_num_ -= 5;
    } break;
    case 'A': {
      // Already handled above.
      return;
    } break;

    case '5': {
      is_logged_in_ = false;

      // dump the last sequence number known
      fixlog_ofs_ << ">> " << msg_buf_ << std::endl;

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

      onLogout();
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

      container_.seq_reset.setSeqNum(last_seq_num_);
      container_.seq_reset.setNewSeqNum(new_seq_num_);
      container_.seq_reset.setUTCTime(time(NULL));
      container_.seq_reset.setCheckSum();

      fixlog_ofs_ << "<< " << container_.seq_reset.msg_char_buf_ << std::endl;

      unsigned int write_len_ =
          (int)(container_.seq_reset.Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH + BMFEP_FIX_Tag_10_Width_ +
                DELIMITER_SOH_WIDTH_ - container_.seq_reset.Header_Tag_8_);

      if (tcp_client_socket_.WriteN(write_len_, (void *)container_.seq_reset.msg_char_buf_) < (int)write_len_) {
        fixlog_ofs_ << "Couldnot send complete msg to BMFEP" << std::endl;
      }

      last_seq_num_ = new_seq_num_;
      //        last_proc_seq_num_ = new_seq_num_ - 1;

      fixlog_ofs_ << " \t Resend & Seq, Reset LPSN : " << last_proc_seq_num_ << std::endl;
      // Also send out a heart beat next time with the updated seq. nos..
      is_uncomfirmed_testrequest_2_ = is_uncomfirmed_testrequest_ = is_uncomfirmed_heartbeat_ = false;
    } break;

    case '8': {
      // Execution report.
      container_.exec_report.generateReport(msg_buf_);

      HFSAT::ORS::BMFEPFIX::t_FIX_OrdStatus ordstatus_ = container_.exec_report.getExecTag39();

      if (ordstatus_ == '8') {  // for Rejects , TODO , this is an overhead for all the normal replies

        fixlog_ofs_ << ">> " << msg_buf_ << std::endl;
      }

      onExecReport(container_.exec_report);
    } break;

    case '9': {
      // Cancel Rejection report.
      container_.exec_report.generateReport(msg_buf_);
      onCxlReject(container_.exec_report);
    } break;

    case '3': {
      // Tag 45 corresponds to the msg_seq_num which triggered this rejection.
      char reject_seq_buf_[20];
      for (index_ = 0, msg_buf_ = strstr(msg_buf_, "45=") + TWO_DIGIT_AND_EQUAL_WIDTH;
           *msg_buf_ != BMFEP_Delimiter_SOH_; ++msg_buf_) {
        reject_seq_buf_[index_++] = *msg_buf_;
      }
      reject_seq_buf_[index_] = '\0';
      BMFEPFIX::t_FIX_SeqNum reject_seq_num_ = atol(reject_seq_buf_);

      onSessionLevelReject(reject_seq_num_);
    } break;

    case 'j': {
      // Tag 45 corresponds to the msg_seq_num which triggered this rejection.
      char reject_seq_buf_[20];
      for (index_ = 0, msg_buf_ = strstr(msg_buf_, "45=") + TWO_DIGIT_AND_EQUAL_WIDTH;
           *msg_buf_ != BMFEP_Delimiter_SOH_; ++msg_buf_) {
        reject_seq_buf_[index_++] = *msg_buf_;
      }
      reject_seq_buf_[index_] = '\0';
      BMFEPFIX::t_FIX_SeqNum reject_seq_num_ = atol(reject_seq_buf_);

      onBusinessLevelReject(reject_seq_num_);
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

#if NOT_USING_ZERO_LOGGIN_ORS
  fixlog_ofs_ << " \t Processed Seq No: Expecting " << last_proc_seq_num_ + 1 << std::endl;
#endif
}

/// technical session control calls
void BMFEPEngine::Connect() {
  tcp_client_socket_.Connect(socket_connect_host_, socket_connect_port_);
  p_engine_listener_->OnConnect(true);
}


void BMFEPEngine::SendSpreadOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_) {
}

void BMFEPEngine::SendThreeLegOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_, HFSAT::ORS::Order *order3_) {
}

void BMFEPEngine::SendTwoLegOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_) {
}


void BMFEPEngine::DisConnect() {
  // This should normally not do anything, but in case the user has not
  // explicitly logged out, this will log us out.
  sendLogout();
}

void BMFEPEngine::Login() { sendLogon(); }

void BMFEPEngine::Logout() { sendLogout(); }

void BMFEPEngine::onLogon() { p_engine_listener_->OnLogin(true); }

void BMFEPEngine::onLogout() { p_engine_listener_->OnLogout(); }

// From Thread class inherited from BaseEngine.
void BMFEPEngine::thread_main() {}
std::vector<int> BMFEPEngine::init() {
  vector<int> ss;
  ss.push_back(tcp_client_socket_.socket_file_descriptor());
  if (use_dropcopy_mcast_packets_) ss.push_back(dropcopy_multicast_receiver_socket_.socket_file_descriptor());
  return ss;
}

inline bool BMFEPEngine::ProcessExchangeSequenceAndNotifyFurtherProcessing(const uint64_t &_received_seq_,
                                                                           const uint64_t &_last_processed_seq_num_,
                                                                           const bool &_is_logout_) {
  //      fixlog_ofs_ << "BEGIN : " << _last_processed_seq_num_ << "\n" ;
  //      fixlog_ofs_.flush () ;

  if (_received_seq_ > _last_processed_seq_num_ + 1 && (!_is_logout_)) {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "OPTIMIZED PROCESSING WENT WRONG, POTENTIALLY MISSED TO PROCESS SOMETHING"
                                 << DBGLOG_ENDL_FLUSH;
    dbglogger_.DumpCurrentBuffer();

    // also send an alert
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    HFSAT::Email e;

    e.setSubject(
        "POTENTIAL ERROR : INCONSISTENCY DETECTED ON OPTIMIZED FIX PROCESSING, MISSED TO PROCESS SOMETHING, NOT A "
        "FATAL ERROR, CONTINUING....");
    e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
    e.addSender("ravi.parikh@tworoads.co.in");
    e.content_stream << "host_machine: " << hostname << "<br/>";
    e.content_stream << "REFER TO THE LOGS FOR FURTHER INFORMATION"
                     << "<br/>";
    e.sendMail();

    fixlog_ofs_ << "\t NEW PROCESSING Higher seq. no received Expected : Recv :" << _received_seq_ << " Expec "
                << last_proc_seq_num_ + 1 << " : Sending Resend Req.  " << std::endl;

    container_.resend_request.setBeginSeqNo(last_proc_seq_num_ + 1);

    is_resend_request_cached_ = true;
    last_proc_seq_num_ = _received_seq_;  // TODO Take Care OF this hack
    ORSControllerThread::GetUniqueInstance().AddRequest(processe_queue_req_);
    return false;

  } else if (_received_seq_ < _last_processed_seq_num_ + 1 && (!_is_logout_)) {
    if (is_playback_mode_) {
      return true;
    } else {
      last_proc_seq_num_ = _received_seq_;
      return false;
    }
  }

  // Increase the last processed sequence by 1
  last_proc_seq_num_++;

  //      fixlog_ofs_ << "END: " << _last_processed_seq_num_ << "\n" ;
  //      fixlog_ofs_.flush () ;

  return true;
}

inline void BMFEPEngine::ProcessLogonResponse(char *_start_of_tag_35_ptr_, char *_single_message_end_ptr_,
                                              bool &_optimized_processing_failed_) {
  is_logged_in_ = true;
  if (!is_playback_mode_) {
    onLogon();
  }
  char *this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                              SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

  // NOW We are at tag 34 value
  last_processed_exchange_reply_sequence_number_ = 0;

  for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
    last_processed_exchange_reply_sequence_number_ =
        last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
  }

  fixlog_ofs_ << " LR : " << last_proc_seq_num_ << " " << _start_of_tag_35_ptr_ << "\n";
  fixlog_ofs_.flush();

  if (false == ProcessExchangeSequenceAndNotifyFurtherProcessing(last_processed_exchange_reply_sequence_number_,
                                                                 last_proc_seq_num_, false)) {
    return;
  }
}

inline void BMFEPEngine::ProcessHeartbeat(char *_start_of_tag_35_ptr_, char *_single_message_end_ptr_,
                                          bool &_optimized_processing_failed_) {
  char *this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                              SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

  // NOW We are at tag 34 value
  last_processed_exchange_reply_sequence_number_ = 0;

  for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
    last_processed_exchange_reply_sequence_number_ =
        last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
  }

  if (is_playback_mode_) {
    ProcessHeartbeatForPlayback(last_processed_exchange_reply_sequence_number_);
  }

  //      fixlog_ofs_ << " HBT : " << last_proc_seq_num_ << "\n" ;
  //      fixlog_ofs_.flush () ;

  if (false == ProcessExchangeSequenceAndNotifyFurtherProcessing(last_processed_exchange_reply_sequence_number_,
                                                                 last_proc_seq_num_, false)) {
    return;
  }

  is_uncomfirmed_heartbeat_ = is_uncomfirmed_testrequest_ = is_uncomfirmed_testrequest_2_ = false;
}

inline void BMFEPEngine::ProcessResendRequest(char *_start_of_tag_35_ptr_, char *_single_message_end_ptr_,
                                              bool &_optimized_processing_failed_) {
  char *this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                              SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

  // NOW We are at tag 34 value
  last_processed_exchange_reply_sequence_number_ = 0;

  for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
    last_processed_exchange_reply_sequence_number_ =
        last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
  }

  fixlog_ofs_ << " RESEND : " << last_proc_seq_num_ << "\n";
  fixlog_ofs_.flush();

  if (false == ProcessExchangeSequenceAndNotifyFurtherProcessing(last_processed_exchange_reply_sequence_number_,
                                                                 last_proc_seq_num_, false)) {
    return;
  }

  this_processing_ptr += SKIP_OVER_COMBINED_TAGS_49_52_56;

  if (resend_request_processed_) {
    return;
  }

  resend_request_processed_ = true;
  // Resend Request.
  // Send a sequence reset msg.
  // This is the exchange specified behavior for Administrative msgs, but we always use it.
  // This eliminates the need to maintain sent out msgs.
  // Get the sequence no. specified in tag 7.
  for (tag_7_resend_req_ = 0, this_processing_ptr = strstr(_start_of_tag_35_ptr_, "\0017=") + 3;
       *this_processing_ptr != BMFEP_Delimiter_SOH_ && this_processing_ptr < _single_message_end_ptr_;
       ++this_processing_ptr) {
    tag_7_resend_req_ = tag_7_resend_req_ * 10 + (*this_processing_ptr - '0');
  }

  is_seq_reset_cached_ = true;
  ORSControllerThread::GetUniqueInstance().AddRequest(processe_queue_req_);
}

inline void BMFEPEngine::ProcessReject(char *_start_of_tag_35_ptr_, char *_single_message_end_ptr_,
                                       bool &_optimized_processing_failed_) {
  char *this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                              SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

  // NOW We are at tag 34 value
  last_processed_exchange_reply_sequence_number_ = 0;

  for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
    last_processed_exchange_reply_sequence_number_ =
        last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
  }

  if (false == ProcessExchangeSequenceAndNotifyFurtherProcessing(last_processed_exchange_reply_sequence_number_,
                                                                 last_proc_seq_num_, false)) {
    return;
  }
}

inline void BMFEPEngine::ProcessOrderCancelReject(char *_start_of_tag_35_ptr_, char *_single_message_end_ptr_,
                                                  bool &_optimized_processing_failed_) {
  char *this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                              SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

  // NOW We are at tag 34 value
  last_processed_exchange_reply_sequence_number_ = 0;

  for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
    last_processed_exchange_reply_sequence_number_ =
        last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
  }

  if (false == ProcessExchangeSequenceAndNotifyFurtherProcessing(last_processed_exchange_reply_sequence_number_,
                                                                 last_proc_seq_num_, false)) {
    return;
  }

  while (TAG_11_INT_VALUE != UINT32CASTVALUE(this_processing_ptr) && this_processing_ptr < _single_message_end_ptr_)
    this_processing_ptr++;

  if (this_processing_ptr >= _single_message_end_ptr_) {
    // Notify That The Optimized Procesing Has Failed, The caller can now switch back to older processing
    _optimized_processing_failed_ = true;

    return;
  }
  current_tag_11_sequence_number_ptr_ = this_processing_ptr;
  // Should Reach Here only if we have matched tag 11 [ (char)1 11= ]
  this_processing_ptr += (LENGTH_OF_DELIM + SKIP_OVER_TAG_11_WIHTOUT_VALUE_LENGTH);
  current_server_assigned_sequnece_in_processing_ = 0;

  for (; DELIM != *this_processing_ptr && this_processing_ptr < _single_message_end_ptr_; this_processing_ptr++) {
    current_server_assigned_sequnece_in_processing_ =
        current_server_assigned_sequnece_in_processing_ * 10 + (*this_processing_ptr - '0');
  }

  this_processing_ptr += LENGTH_OF_DELIM;

  while (TAG_434_INT_VALUE != UINT32CASTVALUE(this_processing_ptr) && this_processing_ptr < _single_message_end_ptr_)
    this_processing_ptr++;

  this_processing_ptr += 4;
  char cancel_or_modify_rej = *this_processing_ptr;

  DBGLOG_CLASS_FUNC_LINE << " Cxl/Modify Reject : " << cancel_or_modify_rej
                         << " SAOS: " << current_server_assigned_sequnece_in_processing_ << DBGLOG_ENDL_FLUSH;

  if (cancel_or_modify_rej == '1') {
    p_engine_listener_->OnCxlReject(current_server_assigned_sequnece_in_processing_, kExchCancelReject);
  } else if (cancel_or_modify_rej == '2') {
    p_engine_listener_->OnOrderModReject(current_server_assigned_sequnece_in_processing_, kExchCancelReplaceReject);
  } else {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "Unknown Cancel/Modify Reject " << cancel_or_modify_rej
                                 << " SAOS: " << current_server_assigned_sequnece_in_processing_ << DBGLOG_ENDL_FLUSH;
  }
}

inline void BMFEPEngine::ProcessBusinessMessageReject(char *_start_of_tag_35_ptr_, char *_single_message_end_ptr_,
                                                      bool &_optimized_processing_failed_) {
  char *this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                              SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

  // NOW We are at tag 34 value
  last_processed_exchange_reply_sequence_number_ = 0;

  for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
    last_processed_exchange_reply_sequence_number_ =
        last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
  }

  if (false == ProcessExchangeSequenceAndNotifyFurtherProcessing(last_processed_exchange_reply_sequence_number_,
                                                                 last_proc_seq_num_, false)) {
    return;
  }

  // Don't care about optimizing this, should rarely occur
  char reject_seq_buf_[20];
  int32_t index_ = 0;

  for (index_ = 0, this_processing_ptr = strstr(_start_of_tag_35_ptr_, "45=") + TWO_DIGIT_AND_EQUAL_WIDTH;
       *this_processing_ptr != BMFEP_Delimiter_SOH_ && this_processing_ptr < _single_message_end_ptr_;
       ++this_processing_ptr) {
    reject_seq_buf_[index_++] = *this_processing_ptr;
  }

  reject_seq_buf_[index_] = '\0';
  BMFEPFIX::t_FIX_SeqNum reject_seq_num_ = atol(reject_seq_buf_);

  if ((reject_seq_num_ - starting_sequence_offset_) >= MAX_SEQUENCE_NUMBERS_WE_EXPECT_DURING_ONE_RUN) {
    SendEmailNotification(
        "RECEIVED AN EXCHANGE REPLY REJECT FOR WHICH NO METADATA RECORD EXISTS AND HENCE WILL NOT BE PASSED TO "
        "CLIENTS");
    return;
  }

  if (false == sequence_to_metadata_vec_[reject_seq_num_ - starting_sequence_offset_]->flush_trigger_) {
    SendEmailNotification(
        "THREADS LAGGING NOTICED, DATA NOT FLUSHED TO MEMORY AND CAUSING READ WAIT, NOT PASSING REJECT TO CLIENTS");
    return;
  }

  // Valid Data Found
  if (true == sequence_to_metadata_vec_[reject_seq_num_ - starting_sequence_offset_]->is_send_otherwise_cxl_) {
    p_engine_listener_->OnReject(
        sequence_to_metadata_vec_[reject_seq_num_ - starting_sequence_offset_]->server_assigned_sequence);

  } else {
    p_engine_listener_->OnCxlReject(
        sequence_to_metadata_vec_[reject_seq_num_ - starting_sequence_offset_]->server_assigned_sequence,
        kCxlRejectReasonUnknownOrder);
  }
}

inline void BMFEPEngine::ProcessLogout(char *_start_of_tag_35_ptr_, char *_single_message_end_ptr_,
                                       bool &_optimized_processing_failed_) {
  char *this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                              SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

  // NOW We are at tag 34 value
  last_processed_exchange_reply_sequence_number_ = 0;

  for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
    last_processed_exchange_reply_sequence_number_ =
        last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
  }

  if (false == ProcessExchangeSequenceAndNotifyFurtherProcessing(last_processed_exchange_reply_sequence_number_,
                                                                 last_proc_seq_num_, true)) {
    return;
  }

  is_logged_in_ = false;

  // dump the last sequence number known
  fixlog_ofs_ << ">> " << _start_of_tag_35_ptr_ << std::endl;

  FILE *p_seq_file_ = fopen(seq_file_name_.c_str(), "w");
  if (p_seq_file_) {
    fprintf(p_seq_file_, "%lu %d\n", last_seq_num_, last_processed_exchange_reply_sequence_number_);
    fixlog_ofs_ << " Logged Out received: Write " << last_seq_num_ << " : "
                << last_processed_exchange_reply_sequence_number_ << std::endl;
    fclose(p_seq_file_);
  }

  onLogout();
}

inline void BMFEPEngine::ProcessTestRequest(char *_start_of_tag_35_ptr_, char *_single_message_end_ptr_,
                                            bool &_optimized_processing_failed_) {
  char *this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                              SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

  // NOW We are at tag 34 value
  last_processed_exchange_reply_sequence_number_ = 0;

  for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
    last_processed_exchange_reply_sequence_number_ =
        last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
  }

  this_processing_ptr += SKIP_OVER_SAFE_COMBINED_TAGS_49_52_56;

  while (TAG_112_INT_VALUE != UINT32CASTVALUE(this_processing_ptr) && this_processing_ptr < _single_message_end_ptr_)
    this_processing_ptr++;

  if (this_processing_ptr >= _single_message_end_ptr_) {
    // Notify That The Optimized Procesing Has Failed, The caller can now switch back to older processing
    _optimized_processing_failed_ = true;

    return;
  }

  this_processing_ptr += SKIP_OVER_TAG_112_WIHTOUT_VALUE_LENGTH;

  char place_holder_buffer[32];
  uint32_t index = 0;

  for (index = 0; DELIM != *this_processing_ptr; this_processing_ptr++) {
    place_holder_buffer[index++] = *this_processing_ptr;
  }

  place_holder_buffer[index] = '\0';

  container_.test_request_heartbeat_.SetTag112(place_holder_buffer);

  is_test_request_cached_ = true;
  ORSControllerThread::GetUniqueInstance().AddRequest(processe_queue_req_);
  if (false == ProcessExchangeSequenceAndNotifyFurtherProcessing(last_processed_exchange_reply_sequence_number_,
                                                                 last_proc_seq_num_, false)) {
    return;
  }
}

void BMFEPEngine::CancelLiveOrdersInPlaybackMode() {
  if (is_playback_mode_) {
    const std::map<int, std::vector<BMFPlaybackStruct>> &all_live_orders = playback_mgr_.GetAllLiveOrders();

    dbglogger_ << "Live Orders As determined by Playback : \n";
    if (all_live_orders.empty()) {
      dbglogger_ << "None\n";
    }
    for (auto iter = all_live_orders.begin(); iter != all_live_orders.end(); iter++) {
      dbglogger_ << "Security : "
                 << SimpleSecuritySymbolIndexer::GetUniqueInstance().GetSecuritySymbolFromId(iter->first)
                 << " (Session " << id_ << ")\n";
      if (iter->second.empty()) {
        dbglogger_ << "None\n";
      }
      for (auto order : iter->second) {
        dbglogger_ << "\nClOrdId : " << order.origClOrdID << "\nSize : " << order.size
                   << "\nSide : " << ((order.side == 1) ? "B" : ((order.side == 2) ? "S" : "U")) << "\n";
      }
    }

    int throttle = 20;
    if (m_settings.has("ORSThrottleLimit")) {
      throttle = atoi(m_settings.getValue("ORSThrottleLimit").c_str());
    }

    if (throttle <= 0) {
      dbglogger_ << " Throttle value : " << throttle << " <= 0 using default = 20 \n";
      dbglogger_.DumpCurrentBuffer();
      throttle = 20;
    }
    long sleep_time = (1000 / (0.8 * throttle)) * 1000;
    for (auto iter = all_live_orders.begin(); iter != all_live_orders.end(); iter++) {
      for (auto live_order : iter->second) {
        Order order;
        order.size_remaining_ = live_order.size;
        order.server_assigned_order_sequence_ = live_order.origClOrdID;
        order.buysell_ = ((live_order.side == 1) ? (kTradeTypeBuy)
                                                 : ((live_order.side == 2) ? (kTradeTypeSell) : (kTradeTypeNoInfo)));
        order.security_id_ = iter->first;
        gettimeofday(&order.ors_timestamp_, NULL);
        CancelOrder(&order);
        HFSAT::usleep(sleep_time);
      }
    }
    is_live_order_cancelled_in_playback_mode_ = true;

    const std::map<int, int> &all_live_positions = playback_mgr_.GetAllLivePositions();
    dbglogger_ << "Live Positions On Playback : \n";
    if (all_live_positions.empty()) {
      dbglogger_ << "None\n";
    }
    for (auto iter = all_live_positions.begin(); iter != all_live_positions.end(); iter++) {
      dbglogger_ << SimpleSecuritySymbolIndexer::GetUniqueInstance().GetSecuritySymbolFromId(iter->first) << " "
                 << iter->second << " (Session " << id_ << ")\n";
    }
    dbglogger_.DumpCurrentBuffer();
  }
}

void BMFEPEngine::EndPlaybackMode() {
  if (is_playback_mode_) {
    playback_mgr_.ResetAllInstances();
    is_playback_mode_ = false;
    dbglogger_ << "Ending Playback Mode\n";
    dbglogger_.DumpCurrentBuffer();

    // playback mode is finished. Lets resume normal opertation.
    onLogon();
  }
}

void BMFEPEngine::ProcessHeartbeatForPlayback(const int heartbeat_seq_num) {
  if (!last_heartbeat_seq_num_in_playback_mode_) {
    last_heartbeat_seq_num_in_playback_mode_ = heartbeat_seq_num;
  } else {
    if (heartbeat_seq_num == (last_heartbeat_seq_num_in_playback_mode_ + 1)) {
      if (!is_resend_request_sent_in_playback_mode_) {
        BeginPlaybackMode();
      } else if (!is_live_order_cancelled_in_playback_mode_) {
        CancelLiveOrdersInPlaybackMode();
      } else {
        EndPlaybackMode();
      }
    }
    last_heartbeat_seq_num_in_playback_mode_ = heartbeat_seq_num;
  }
}

inline void BMFEPEngine::ProcessSequenceReset(char *_start_of_tag_35_ptr_, char *_single_message_end_ptr_,
                                              bool &_optimized_processing_failed_) {
  char *this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                              SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

  // NOW We are at tag 34 value
  last_processed_exchange_reply_sequence_number_ = 0;

  for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
    last_processed_exchange_reply_sequence_number_ =
        last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
  }

  //      fixlog_ofs_ << " RESET : " << last_proc_seq_num_ << "\n" ;
  //      fixlog_ofs_.flush () ;

  if (false == ProcessExchangeSequenceAndNotifyFurtherProcessing(last_processed_exchange_reply_sequence_number_,
                                                                 last_proc_seq_num_, false)) {
    return;
  }

  // Exchange Has Notified About SequenceReset and Gapfill

  char last_seq_buf_[32];
  memset(last_seq_buf_, 0, 32);

  int index_ = 0;
  for (index_ = 0, this_processing_ptr = strstr(_start_of_tag_35_ptr_, "36=") + TWO_DIGIT_AND_EQUAL_WIDTH;
       this_processing_ptr < _single_message_end_ptr_ && *this_processing_ptr != BMFEP_Delimiter_SOH_;
       this_processing_ptr++) {
    last_seq_buf_[index_++] = *this_processing_ptr;
  }

  last_seq_buf_[index_] = '\0';
  last_proc_seq_num_ = atol(last_seq_buf_) - 1;  // TODO_OPT
  fixlog_ofs_ << " \tFill Gap from Exchange: : LProcess_Seq_num :" << last_proc_seq_num_ << std::endl;
  is_uncomfirmed_testrequest_2_ = false;
  is_uncomfirmed_testrequest_ = false;
  is_uncomfirmed_heartbeat_ = false;
}

inline void BMFEPEngine::ProcessOrderConfirmation(char *_start_of_tag_39_ptr_, char *_single_message_end_ptr_,
                                                  bool &_optimized_processing_failed_) {
  char *this_processing_ptr = _start_of_tag_39_ptr_ + SKIP_OVER_SAFE_COMBINED_TAGS_39_40_44_48_54_55_59_60_75_150_151;

  while (TAG_198_INT_VALUE != UINT32CASTVALUE(this_processing_ptr) && this_processing_ptr < _single_message_end_ptr_)
    this_processing_ptr++;

  this_processing_ptr += SKIP_OVER_TAG_198_WITHOUT_VALUE_LENGTH;
  current_exchange_sequence_number_ = 0;

  for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
    current_exchange_sequence_number_ = current_exchange_sequence_number_ * 10 + (*this_processing_ptr - '0');
  }

  p_engine_listener_->OnOrderConfBMF(current_server_assigned_sequnece_in_processing_,
                                     current_exchange_sequence_number_);
}

inline void BMFEPEngine::ProcessPartialAndCompleteFill(char *_start_of_tag_11_ptr_, char *_single_message_end_ptr_,
                                                       bool &_optimized_processing_failed_) {
  char *this_processing_ptr = _start_of_tag_11_ptr_ + SKIP_OVER_COMBINED_TAGS_14_17;

  while (TAG_31_INT_VALUE != UINT32CASTVALUE(this_processing_ptr) && this_processing_ptr < _single_message_end_ptr_)
    this_processing_ptr++;

  this_processing_ptr += (LENGTH_OF_DELIM + SKIP_OVER_TAG_31_WITHOUT_VALUE_LENGTH);

  current_execution_price_ = 0;

  for (; (DELIM != *this_processing_ptr) && ((char)'.' != *this_processing_ptr); this_processing_ptr++) {
    current_execution_price_ = current_execution_price_ * 10 + (*this_processing_ptr - '0');
  }

  if ('.' == *this_processing_ptr) {
    this_processing_ptr++;
    current_execution_price_factor_ = 0;

    for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
      current_execution_price_ = current_execution_price_ * 10 + (*this_processing_ptr - '0');
      current_execution_price_factor_++;
    }

    if (current_execution_price_factor_ < 0 || current_execution_price_factor_ > MAX_DECIMAL_PLACES_SUPPORTED) {
      _optimized_processing_failed_ = true;
      return;
    }

    current_execution_price_ *= floating_price_decimal_lookup_[current_execution_price_factor_];
  }

  this_processing_ptr += (LENGTH_OF_DELIM + SKIP_OVER_TAG_32_WITHOUT_VALUE_LENGTH);
  current_executed_qty_ = 0;

  for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
    current_executed_qty_ = current_executed_qty_ * 10 + (*this_processing_ptr - '0');
  }

  this_processing_ptr += SKIP_OVER_COMBINED_TAGS_37_38_39_40_44_48_54_55_59_60_75_150;

  while (TAG_151_INT_VALUE != UINT32CASTVALUE(this_processing_ptr)) this_processing_ptr++;

  this_processing_ptr += SKIP_OVER_TAG_151_WITHOUT_VALUE_LENGTH;
  current_remaining_qty_ = 0;

  for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
    current_remaining_qty_ = current_remaining_qty_ * 10 + (*this_processing_ptr - '0');
  }

  p_engine_listener_->OnOrderExecBMF(current_server_assigned_sequnece_in_processing_, current_execution_price_,
                                     current_executed_qty_, current_remaining_qty_);
}

inline void BMFEPEngine::ProcessOrderCancellation(char *_start_of_tag_39_ptr_, char *_single_message_end_ptr_,
                                                  bool &_optimized_processing_failed_) {
  p_engine_listener_->OnOrderCxlBMF(current_server_assigned_sequnece_in_processing_);
}

// How is this message being decoded ?
//- From the initial buffer we identified this being a single cancel replacement message
//- We have already reached to the message tag 39= as we identified it as cancel replace
//- From this point, skip over the minimum safe offset to the first tag of interest, while skipping over we have only
// used bare minimum tag values and a minimum of 1 char as value
// - Assumptions: Exchange never changes the order of the FIX message tags in replies intraday, so either we fail at the
// first message of the day or we should not
// Sample Message:
// Sent: 8=FIX.4.4|9=000251|35=G|34=000001420|49=XALP0004|52=20150518-00:00:00.000|56=OELINE001|
//       41=000031|11=000031|55=INDM15|54=1|38=000000030|40=2|44=0057000.0000000|78=1|79=1|661=99|
//       453=3|448=COLO0|447=D|452=54|448=DVC|447=D|452=36|448=98|447=D|452=7|60=20150518-00:00:00.000|10=178|
// Received: 8=FIX.4.4|9=302|35=8|34=113|49=OELINE001|52=20150518-13:47:46.966|56=XALP0004|1=98104|6=0|11=000031|
//           14=0|17=70338:33|37=7027924799|38=5|39=4|40=2|41=000031|44=57000|54=1|55=INDM15|59=0|
//           60=20150518-13:47:46.990|75=20150518|150=4|151=0|198=7031815011|453=3|448=COLO0|
//           447=D|452=54|448=DVC|447=D|452=36|448=98|447=D|452=7|10=186|
inline void BMFEPEngine::ProcessCancelReplacement(char *_start_of_tag_39_ptr_, char *_single_message_end_ptr_,
                                                  bool &_optimized_processing_failed_) {
  char *this_processing_ptr = _start_of_tag_39_ptr_;

  this_processing_ptr += SKIP_OVER_SAFE_COMBINED_TAGS_39_40_41;

  while (TAG_44_INT_VALUE != UINT32CASTVALUE(this_processing_ptr) && this_processing_ptr < _single_message_end_ptr_)
    this_processing_ptr++;

  if (this_processing_ptr == _single_message_end_ptr_) {
    _optimized_processing_failed_ = true;
    return;
  }

  this_processing_ptr += (LENGTH_OF_DELIM + SKIP_OVER_TAG_44_WITHOUT_VALUE_LENGTH);

  double order_replacement_price_ = 0;
  uint32_t order_replacement_price_factor_ = 0;

  for (; (DELIM != *this_processing_ptr) && ((char)'.' != *this_processing_ptr); this_processing_ptr++) {
    order_replacement_price_ = order_replacement_price_ * 10 + (*this_processing_ptr - '0');
  }

  if ('.' == *this_processing_ptr) {
    this_processing_ptr++;
    order_replacement_price_factor_ = 0;

    for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
      order_replacement_price_ = order_replacement_price_ * 10 + (*this_processing_ptr - '0');
      order_replacement_price_factor_++;
    }

    if (order_replacement_price_factor_ < 0 || order_replacement_price_factor_ > MAX_DECIMAL_PLACES_SUPPORTED) {
      _optimized_processing_failed_ = true;
      return;
    }

    order_replacement_price_ *= floating_price_decimal_lookup_[order_replacement_price_factor_];
  }

  this_processing_ptr += (LENGTH_OF_DELIM + SKIP_OVER_COMBINED_TAGS_54_55_59_60_75_150);

  while (TAG_151_INT_VALUE != UINT32CASTVALUE(this_processing_ptr) && this_processing_ptr < _single_message_end_ptr_)
    this_processing_ptr++;

  if (this_processing_ptr == _single_message_end_ptr_) {
    _optimized_processing_failed_ = true;
    return;
  }

  this_processing_ptr += (SKIP_OVER_TAG_151_WITHOUT_VALUE_LENGTH);

  current_remaining_qty_ = 0;

  for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
    current_remaining_qty_ = current_remaining_qty_ * 10 + (*this_processing_ptr - '0');
  }

  this_processing_ptr += (LENGTH_OF_DELIM + SKIP_OVER_TAG_198_WITHOUT_VALUE_LENGTH);
  current_exchange_sequence_number_ = 0;

  for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
    current_exchange_sequence_number_ = current_exchange_sequence_number_ * 10 + (*this_processing_ptr - '0');
  }

  p_engine_listener_->OnOrderCancelReplacedBMF(current_server_assigned_sequnece_in_processing_,
                                               current_exchange_sequence_number_, order_replacement_price_,
                                               current_remaining_qty_, fast_px_convertor_vec_);
}

inline void BMFEPEngine::ProcessExchangeReject(char *_start_of_tag_35_ptr_, char *_single_message_end_ptr_,
                                               bool &_optimized_processing_failed_) {
  fixlog_ofs_ << "REJECT : " << _start_of_tag_35_ptr_ << "\n";
  fixlog_ofs_.flush();

  p_engine_listener_->OnReject(current_server_assigned_sequnece_in_processing_);
}

inline void BMFEPEngine::ProcessExecutionReport(char *_start_of_tag_35_ptr_, char *_single_message_end_ptr_,
                                                bool &_optimized_processing_failed_) {
  // Skipping Over
  // -> TAG 35 ( 35=8 )
  // -> TAG 34 ( 34= )
  char *this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                              SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

  // NOW We are at tag 34 value
  last_processed_exchange_reply_sequence_number_ = 0;

  for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
    last_processed_exchange_reply_sequence_number_ =
        last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
  }

  if (false == ProcessExchangeSequenceAndNotifyFurtherProcessing(last_processed_exchange_reply_sequence_number_,
                                                                 last_proc_seq_num_, false)) {
    return;
  }

  this_processing_ptr += SKIP_OVER_SAFE_COMBINED_TAGS_49_52_56_1_6;

  while (TAG_11_INT_VALUE != UINT32CASTVALUE(this_processing_ptr) && this_processing_ptr < _single_message_end_ptr_)
    this_processing_ptr++;

  if (this_processing_ptr >= _single_message_end_ptr_) {
    // Notify That The Optimized Procesing Has Failed, The caller can now switch back to older processing
    _optimized_processing_failed_ = true;

    return;
  }
  current_tag_11_sequence_number_ptr_ = this_processing_ptr;
  // Should Reach Here only if we have matched tag 11 [ (char)1 11= ]
  this_processing_ptr += (LENGTH_OF_DELIM + SKIP_OVER_TAG_11_WIHTOUT_VALUE_LENGTH);
  current_server_assigned_sequnece_in_processing_ = 0;

  for (; DELIM != *this_processing_ptr && this_processing_ptr < _single_message_end_ptr_; this_processing_ptr++) {
    current_server_assigned_sequnece_in_processing_ =
        current_server_assigned_sequnece_in_processing_ * 10 + (*this_processing_ptr - '0');
  }

  int size = 0, exec_size = 0;
  if (is_playback_mode_ && is_resend_request_sent_in_playback_mode_) {
    char *this_processing_ptr_temp = this_processing_ptr;
    while ((TAG_38_INT_VALUE != UINT32CASTVALUE(this_processing_ptr_temp)) &&
           (TAG_32_INT_VALUE != UINT32CASTVALUE(this_processing_ptr_temp)) &&
           this_processing_ptr_temp < _single_message_end_ptr_)
      this_processing_ptr_temp++;

    if (this_processing_ptr_temp >= _single_message_end_ptr_) {
      // Notify That The Optimized Procesing Has Failed, The caller can now switch back to older processing
      std::cerr << typeid(*this).name() << ':' << __func__ << " "
                << "Playback for this message failed :\n"
                << _start_of_tag_35_ptr_ << "\n";
      return;
    }
    if (TAG_32_INT_VALUE == UINT32CASTVALUE(this_processing_ptr_temp)) {
      this_processing_ptr_temp += (LENGTH_OF_DELIM + SKIP_OVER_TAG_32_WIHTOUT_VALUE_LENGTH);
      for (; DELIM != *this_processing_ptr_temp && this_processing_ptr_temp < _single_message_end_ptr_;
           this_processing_ptr_temp++) {
        exec_size = exec_size * 10 + (*this_processing_ptr_temp - '0');
      }

      while ((TAG_38_INT_VALUE != UINT32CASTVALUE(this_processing_ptr_temp)) &&
             this_processing_ptr_temp < _single_message_end_ptr_)
        this_processing_ptr_temp++;

      if (this_processing_ptr_temp >= _single_message_end_ptr_) {
        // Notify That The Optimized Procesing Has Failed, The caller can now switch back to older processing
        std::cerr << typeid(*this).name() << ':' << __func__ << " "
                  << "Playback for this message failed :\n"
                  << _start_of_tag_35_ptr_ << "\n";
        return;
      }
    }
    this_processing_ptr_temp += (LENGTH_OF_DELIM + SKIP_OVER_TAG_38_WIHTOUT_VALUE_LENGTH);
    for (; DELIM != *this_processing_ptr_temp && this_processing_ptr_temp < _single_message_end_ptr_;
         this_processing_ptr_temp++) {
      size = size * 10 + (*this_processing_ptr_temp - '0');
    }
  }

  this_processing_ptr += DELIM;
  this_processing_ptr += SKIP_OVER_SAFE_COMBINED_TAGS_14_17_37_38;

  while (TAG_39_INT_VALUE != UINT32CASTVALUE(this_processing_ptr) && this_processing_ptr < _single_message_end_ptr_)
    this_processing_ptr++;

  if (this_processing_ptr >= _single_message_end_ptr_) {
    // Notify That The Optimized Procesing Has Failed, The caller can now switch back to older processing
    _optimized_processing_failed_ = true;

    return;
  }

  int side = 0;
  std::string symbol;
  int remaining_size = 0;
  if (is_playback_mode_ && is_resend_request_sent_in_playback_mode_) {
    char *this_processing_ptr_temp = this_processing_ptr;
    while (TAG_54_INT_VALUE != UINT32CASTVALUE(this_processing_ptr_temp) &&
           this_processing_ptr_temp < _single_message_end_ptr_)
      this_processing_ptr_temp++;

    if (this_processing_ptr_temp >= _single_message_end_ptr_) {
      // Notify That The Optimized Procesing Has Failed, The caller can now switch back to older processing
      std::cerr << typeid(*this).name() << ':' << __func__ << " "
                << "Playback for this message failed :\n"
                << _start_of_tag_35_ptr_ << "\n";
      return;
    }
    this_processing_ptr_temp += (LENGTH_OF_DELIM + SKIP_OVER_TAG_54_WIHTOUT_VALUE_LENGTH);
    for (; DELIM != *this_processing_ptr_temp && this_processing_ptr_temp < _single_message_end_ptr_;
         this_processing_ptr_temp++) {
      side = side * 10 + (*this_processing_ptr_temp - '0');
    }

    while (TAG_55_INT_VALUE != UINT32CASTVALUE(this_processing_ptr_temp) &&
           this_processing_ptr_temp < _single_message_end_ptr_)
      this_processing_ptr_temp++;
    if (this_processing_ptr_temp >= _single_message_end_ptr_) {
      // Notify That The Optimized Procesing Has Failed, The caller can now switch back to older processing
      std::cerr << typeid(*this).name() << ':' << __func__ << " "
                << "Playback for this message failed :\n"
                << _start_of_tag_35_ptr_ << "\n";
      return;
    }
    this_processing_ptr_temp += (LENGTH_OF_DELIM + SKIP_OVER_TAG_55_WIHTOUT_VALUE_LENGTH);
    int len = 0;
    char *this_processing_ptr_temp_fwd = this_processing_ptr_temp;
    for (; DELIM != *this_processing_ptr_temp_fwd && this_processing_ptr_temp_fwd < _single_message_end_ptr_;
         this_processing_ptr_temp_fwd++) {
      len++;
    }
    symbol = std::string(this_processing_ptr_temp, len);
    while (TAG_151_INT_VALUE != UINT32CASTVALUE(this_processing_ptr_temp) &&
           this_processing_ptr_temp < _single_message_end_ptr_)
      this_processing_ptr_temp++;
    this_processing_ptr_temp += (SKIP_OVER_TAG_151_WIHTOUT_VALUE_LENGTH);
    if (this_processing_ptr_temp < _single_message_end_ptr_) {
      for (; DELIM != *this_processing_ptr_temp && this_processing_ptr_temp < _single_message_end_ptr_;
           this_processing_ptr_temp++) {
        remaining_size = remaining_size * 10 + (*this_processing_ptr_temp - '0');
      }
    }
  }

  this_processing_ptr += DELIM;
  BMFPlaybackStruct dataplayback;
  int playback_security_id = 0;
  if (is_playback_mode_ && is_resend_request_sent_in_playback_mode_) {
    dataplayback.side = side;
    dataplayback.size = size;
    dataplayback.origClOrdID = current_server_assigned_sequnece_in_processing_;

    SimpleSecuritySymbolIndexer &indexer = SimpleSecuritySymbolIndexer::GetUniqueInstance();

    if (playback_exchange_symbols_.find(symbol) == playback_exchange_symbols_.end()) {
      playback_exchange_symbols_.insert(symbol);
      indexer.AddString((*playback_exchange_symbols_.find(symbol)).c_str());
    }

    playback_security_id = indexer.GetIdFromSecname((*playback_exchange_symbols_.find(symbol)).c_str());
    if (playback_security_id == -1) {
      std::cerr << "Product " << (*playback_exchange_symbols_.find(symbol))
                << " not present in SimpleSecuritySymbolIndexer. Security Id returned is -1. Ignoring this message.\n";
      return;
    }
  }
  switch (UINT32CASTVALUE(this_processing_ptr)) {
    case TAG_39_NEW_ORDER_INT_VALUE: {
      if (is_playback_mode_) {
        if (is_resend_request_sent_in_playback_mode_) {
          playback_mgr_.OrderNew(playback_security_id, dataplayback);
        }

      } else {
        ProcessOrderConfirmation(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);
      }

    } break;

    case TAG_39_PARTIAL_FILL_INT_VALUE:
    case TAG_39_FILL_INT_VALUE: {
      if (is_playback_mode_) {
        if (is_resend_request_sent_in_playback_mode_) {
          dataplayback.size = exec_size;
          playback_mgr_.OrderExec(playback_security_id, dataplayback);
        }

      } else {
        ProcessPartialAndCompleteFill(current_tag_11_sequence_number_ptr_, _single_message_end_ptr_,
                                      _optimized_processing_failed_);
      }

    } break;

    case TAG_39_CANCEL_INT_VALUE:
    case TAG_39_EXPIRE_INT_VALUE: {
      if (is_playback_mode_) {
        if (is_resend_request_sent_in_playback_mode_) {
          playback_mgr_.OrderCancel(playback_security_id, dataplayback);
        }

      } else {
        ProcessOrderCancellation(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);
      }

    } break;

    case TAG_39_REPLACED_INT_VALUE: {
      if (is_playback_mode_) {
        if (is_resend_request_sent_in_playback_mode_) {
          dataplayback.size = remaining_size;
          playback_mgr_.OrderCancelReplace(playback_security_id, dataplayback);
        }

      } else {
        ProcessCancelReplacement(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);
      }

    } break;

    case TAG_39_REJECTED_INT_VALUE: {
      if (!is_playback_mode_) {
        ProcessExchangeReject(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);
      }

    } break;

    default: {
      // Notify That The Optimized Procesing Has Failed, The caller can now switch back to older processing
      _optimized_processing_failed_ = true;

    } break;
  }
}

inline void BMFEPEngine::ProcessSingleMessage(char *_single_message_start_ptr_, char *_single_message_end_ptr_,
                                              bool &_optimized_processing_failed_,
                                              bool &_processed_session_level_message_) {
  // Skipping Over
  // -> TAG 8 ( 8=FIX.4.4 )
  // -> TAG 9 ( 9= )
  char *this_processing_ptr = _single_message_start_ptr_ + SKIP_OVER_TAG_8_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                              SKIP_OVER_TAG_9_WITHOUT_VALUE_LENGTH;

  while (DELIM != *this_processing_ptr && this_processing_ptr < _single_message_end_ptr_) this_processing_ptr++;

  if (this_processing_ptr >= _single_message_end_ptr_) {
    // Notify That The Optimized Procesing Has Failed, The caller can now switch back to older processing
    _optimized_processing_failed_ = true;

    return;
  }

  this_processing_ptr += LENGTH_OF_DELIM;
  switch (UINT32CASTVALUE(this_processing_ptr)) {
    case TAG_35_LOGON_RESPONSE_INT_VALUE: {
      _processed_session_level_message_ = true;
      ProcessLogonResponse(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

    } break;

    case TAG_35_HEARTBEAT_INT_VALUE: {
      _processed_session_level_message_ = true;
      ProcessHeartbeat(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

    } break;

    case TAG_35_TEST_REQUEST_INT_VALUE: {
      _processed_session_level_message_ = true;
      ProcessTestRequest(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

    } break;

    case TAG_35_RESEND_REQUEST_INT_VALUE: {
      _processed_session_level_message_ = true;
      ProcessResendRequest(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

    } break;

    case TAG_35_REJECT_INT_VALUE: {
      _processed_session_level_message_ = true;
      ProcessReject(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

    } break;

    case TAG_35_SEQUENCE_REST_INT_VALUE: {
      _processed_session_level_message_ = true;
      ProcessSequenceReset(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

    } break;

    case TAG_35_ORDER_CANCEL_REJECT_INT_VALUE: {
      ProcessOrderCancelReject(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

    } break;

    case TAG_35_BUSINESS_MESSAGE_REJECT_INT_VALUE: {
      ProcessBusinessMessageReject(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

    } break;

    case TAG_35_LOGOUT_INT_VALUE: {
      _processed_session_level_message_ = true;
      ProcessLogout(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

    } break;

    case TAG_35_EXECUTIONREPORT_INT_VALUE: {
      if (!started_processing_exec_notifs_) started_processing_exec_notifs_ = true;

      ProcessExecutionReport(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

    } break;
    default: { _optimized_processing_failed_ = true; } break;
  }

  if (TAG_35_RESEND_REQUEST_INT_VALUE != UINT32CASTVALUE(this_processing_ptr)) {
    resend_request_processed_ = false;
  }

  is_uncomfirmed_heartbeat_ = false;
}

inline void BMFEPEngine::ProcessExchangeReply(char *_exchange_reply_, char *_end_of_complete_message_ptr_,
                                              const int32_t &_msg_length_, bool &_optimized_processing_failed_) {
  // By Default assume that we are processing exec reports only in the first if condition
  bool processed_session_level_message = false;

  if (started_processing_exec_notifs_ &&
      (MINIMUM_APPROXIMATE_LENGTH_OF_THE_TWO_OR_MORE_EXECUTION_REPORT > _msg_length_)) {
    ProcessSingleMessage(_exchange_reply_, _end_of_complete_message_ptr_, _optimized_processing_failed_,
                         processed_session_level_message);

    // Looks like exchange sent us session level message followed by some execution report or multiple session level
    // message,
    // Needs special handling now that we have processed 1 message of that, let's find out other message
    if ((true == processed_session_level_message) &&
        (_msg_length_ > MAXIMUM_APPROXIMATE_LENGTH_OF_ONE_SESSION_LEVEL_MESSAGE)) {
      // This one we already have processed
      char *this_processing_ptr = strstr(_exchange_reply_, "\00110=") + LENGTH_OF_TRAILER_TAG;

      // Start with next message, don't care how many messageas are left to be processed, all will be covered
      for (char *next_processing_ptr = strstr(this_processing_ptr, "\00110=");
           next_processing_ptr != NULL && this_processing_ptr < _end_of_complete_message_ptr_;
           this_processing_ptr = next_processing_ptr + LENGTH_OF_TRAILER_TAG,
                next_processing_ptr = strstr(this_processing_ptr, "\00110=")) {
        ProcessSingleMessage(this_processing_ptr, next_processing_ptr + LENGTH_OF_TRAILER_TAG,
                             _optimized_processing_failed_, processed_session_level_message);
      }
    }

  } else {
    for (char *this_processing_ptr = _exchange_reply_ + 0,
              *next_processing_ptr = strstr(this_processing_ptr, "\00110=");
         next_processing_ptr != NULL && this_processing_ptr < _end_of_complete_message_ptr_;
         this_processing_ptr = next_processing_ptr + LENGTH_OF_TRAILER_TAG,
              next_processing_ptr = strstr(this_processing_ptr, "\00110=")) {
      ProcessSingleMessage(this_processing_ptr, next_processing_ptr + LENGTH_OF_TRAILER_TAG,
                           _optimized_processing_failed_, processed_session_level_message);
    }
  }
}

inline int32_t BMFEPEngine::DecodeBMFMessage(char *_msg_char_buf_, const int32_t &_msg_length_,
                                             bool &_optimized_processing_failed_) {
  // Set The Processing Pointer
  current_processing_ptr_ = _msg_char_buf_ + 0;

  // Pass The String As A Complete Char Arrary With Null Pad To Operate Over it, We are not modifying input here, we are
  // only marking end of the char array
  _msg_char_buf_[_msg_length_] = '\0';

  // First Check Whether We Have The Complete Message Or the Partial One, If the Last 8 chars are corrosponding to
  // trailer tag then we have the complete message,
  // However a complete message can still have n number of messages
  if (TRAILER_TAG_INT_VALUE == UINT32CASTVALUE(current_processing_ptr_ + _msg_length_ -
                                               LENGTH_OF_TRAILER_TAG)) {  // This means a complete message

    // Pass A complete message for processing, mark the start and end pointer for it to identify where to start and stop
    // processing
    ProcessExchangeReply(_msg_char_buf_, _msg_char_buf_ + _msg_length_, _msg_length_, _optimized_processing_failed_);

    // No Partial Leftover part
    return 0;
  }

  fixlog_ofs_ << "PROCESSING A PARTIAL MESSAGE, TOTAL LENGTH : " << _msg_length_ << "\n";
  fixlog_ofs_.flush();

  // Now We can have a complete message and some additional partial message part, or just a partial message
  // Check Whether we have any complete message to process in the entire buffer, starting from back end would be
  // optimized

  // Starting From 1 less than the point which is already tested above
  current_processing_ptr_ = _msg_char_buf_ + _msg_length_ - LENGTH_OF_TRAILER_TAG - 1;
  int32_t marking_end_point_of_complete_message = _msg_length_ - LENGTH_OF_TRAILER_TAG - 1;

  // Until We hit the start from tail
  while (marking_end_point_of_complete_message >= 0) {
    // Check For Trailer tag uint value against each iteration
    if (TRAILER_TAG_INT_VALUE == UINT32CASTVALUE(current_processing_ptr_)) {
      // Got the complete reply here
      ProcessExchangeReply(_msg_char_buf_, current_processing_ptr_ + LENGTH_OF_TRAILER_TAG, _msg_length_,
                           _optimized_processing_failed_);

      // Copy The left over part and return the offset for the next read to begin
      memmove(_msg_char_buf_, current_processing_ptr_ + LENGTH_OF_TRAILER_TAG,
              _msg_length_ - (marking_end_point_of_complete_message + LENGTH_OF_TRAILER_TAG));

      return (_msg_length_ - (marking_end_point_of_complete_message + LENGTH_OF_TRAILER_TAG));
    }

    current_processing_ptr_--;
    marking_end_point_of_complete_message--;
  }

  // Don't have any complete message in the buffer, simply return the length of the current read
  return _msg_length_;
}

int BMFEPEngine::onInputAvailable(int sock, char *buffer, int32_t length) {
  if (!run_engine_) {
    dbglogger_ << "MSESS: onInputAvailable while engine was not running. id=" << id_ << "\n";
    return kSessionNotRunning;
  }
  while (true) {
#if NOT_USING_ZERO_LOGGIN_ORS
    char delim[3] = {'E', 'O', 'M'};
    char beg[3] = {'B', 'E', 'G'};
#endif
    char *read_location_ = msg_char_buf_ + read_offset_;
    if (sock == tcp_client_socket_.socket_file_descriptor()) {
      int read_size_ = tcp_client_socket_.ReadN(
          MAX_BMFEPFIX_MSG_SIZE - read_offset_ - 1,
          (void *)read_location_);  //-1 since we are relying on setting the '\0' delimiter after the received message
      if (read_size_ == 0) {
        run_engine_ = false;
        return is_logged_in_ ? kSessionTerminated : kSessionClosed;
      }

      if (read_size_ > 0) {
#if NOT_USING_ZERO_LOGGIN_ORS
        fix_bulk_writer_->Write(beg, 3);
        fix_bulk_writer_->Write(read_location_, read_size_);
        fix_bulk_writer_->Write(delim, 3);
        fix_bulk_writer_->CheckToFlushBuffer();
#endif

        read_offset_ += read_size_;
        msg_char_buf_[read_offset_] = '\0';

#if NOT_USING_ZERO_LOGGIN_ORS
        // fixlog_ofs_ << " ORIGINAL : " << read_offset_ << " CURRENT : " << read_size_ << " MSG : " << msg_char_buf_<<
        // "\n";
        fixlog_ofs_ << ">> " << msg_char_buf_ << "\n";
        fixlog_ofs_.flush();
#endif

        bool optimized_processing_failed = false;

        read_offset_ = DecodeBMFMessage(msg_char_buf_, read_offset_, optimized_processing_failed);

        if (true == optimized_processing_failed || read_offset_ < 0) {
          DBGLOG_CLASS_FUNC_LINE_ERROR << "OPTIMIZED PROCESSING WENT WRONG, READ OFFSET RETURNED : " << read_offset_
                                       << "\n";
          dbglogger_.DumpCurrentBuffer();

          // also send an alert
          char hostname[128];
          hostname[127] = '\0';
          gethostname(hostname, 127);
          HFSAT::Email e;

          e.setSubject(
              "FATAL ERROR : BMFEP OPTIMIZED REPLY PROCESSING FAILED, CANCELLING EXISTING ORDERS AND SHUTTING DOWN...");
          e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
          e.addSender("ravi.parikh@tworoads.co.in");
          e.content_stream << "host_machine: " << hostname << "<br/>";
          e.content_stream << "REFER TO THE LOGS FOR FURTHER INFORMATION"
                           << "<br/>";

          e.sendMail();

          // Stop Allowing Further Orders
          p_engine_listener_->CancelAllPendingOrders();
          sendLogout();
          is_logged_in_ = false;
          onLogout();

          exit(-1);
        }

        return 0;

        //
        //            if( !strncmp ( msg_char_buf_ + read_offset_ - 8 , "\00110=", 4 )) {
        //                processMsgs ( msg_char_buf_, read_offset_ );
        //                read_offset_ = 0; //reset the offset; all bytes have been read and processed and nothing is
        //                left from the previous packets
        //                return 0;
        //            }else{
        //
        //                dbglogger_ << "bmfep_log : Incomplete message read_size_=" << read_size_ << " read_offset_="
        //                << read_offset_ << " @ " << HFSAT::GetTimeOfDay ( ) << "\n";
        //                // Check whether it has a valid completely formed msg within it.
        //                // Starting from the end , and moving towards the front ,
        //                // find a valid checksum tag ^A10=XXX^A
        //                char t_incomplete_msg_ [ MAX_BMFEPFIX_MSG_SIZE ];
        //                memset ( t_incomplete_msg_ , 0 , sizeof ( t_incomplete_msg_ ) );
        //
        //                // Eg. msg is
        //                "8=FIX.4.4^A9=306^A35=8^A34=172660^A49=OE015^A52=20120419-17:14:46.290^A56=XLIN0068^A1=35786^A6=0^A11=084391^A14=0^A17=7256:6733065^A37=7245534917^A38=1^A39=0^A40=2^A44=63350^A48=4800545^A54=1^A55=WINM12^A59=0^A60=20120419-17:14:46.303^A75=20120419^A150=0^A151=1^A198=72117109123^A453=3^A448=COLO0^A447=D^A452=54^A448=DVC^A447=D^A452=36^A448=8^A447=D^A452=7^A10=104^A8=FIX.4.4^A9=306^A35=8^A34=172"
        //                // We want to process
        //                ""8=FIX.4.4^A9=306^A35=8^A34=172660^A49=OE015^A52=20120419-17:14:46.290^A56=XLIN0068^A1=35786^A6=0^A11=084391^A14=0^A17=7256:6733065^A37=7245534917^A38=1^A39=0^A40=2^A44=63350^A48=4800545^A54=1^A55=WINM12^A59=0^A60=20120419-17:14:46.303^A75=20120419^A150=0^A151=1^A198=72117109123^A453=3^A448=COLO0^A447=D^A452=54^A448=DVC^A447=D^A452=36^A448=8^A447=D^A452=7^A10=104^A"
        //                // and get the remaining incomplete message "8=FIX.4.4^A9=306^A35=8^A34=172" in msg_char_buf_
        //                with
        //                // read_offset_ = 27 , in position for reading the remaining message.
        //                int start_index_ = read_offset_ - 8;
        //                for ( ; start_index_ >= 0 ; -- start_index_ )
        //                  {
        //                    if ( ! strncmp ( msg_char_buf_ + start_index_ , "\00110=" , 4 ) )
        //                      {
        //                        start_index_ += 8; // Go to the beginning of the next msg.
        //
        //                        // Copy the remaining part of the incomplete msg to the temporary buffer.
        //                        strncpy ( t_incomplete_msg_ , msg_char_buf_ + start_index_ , read_offset_ -
        //                        start_index_ );
        //
        //                        // Null terminate the complete msg before sending it to processMsgs.
        //                        msg_char_buf_ [ start_index_ ] = '\0';
        //                        processMsgs ( msg_char_buf_ , start_index_ );
        //
        //                        // Having processed the complete message , copy the incomplete portion
        //                        // of the next message back to msg_char_buf , the remainder of
        //                        // this message will be read in the next call to ReadN.
        //                        strcpy ( msg_char_buf_ , t_incomplete_msg_ );
        //                        read_offset_ = strlen ( t_incomplete_msg_ );
        //
        //                        break;
        //                      }
        //                  }
        //            }
        //

      } else {
        dbglogger_ << " Read returned : " << read_size_ << " Error no: " << strerror(errno)
                   << "in BMFEPEngine Thread Main \n";
        dbglogger_.DumpCurrentBuffer();
        run_engine_ = false;

        // Persist saos
        HFSAT::ORS::SequenceGenerator::GetUniqueInstance().persist_seq_num();

        fix_bulk_writer_->Close();
        fix_bulk_writer_our_msg->Close();
        fixlog_ofs_.close();

        // Cleanup
        delete fix_bulk_writer_;
        delete fix_bulk_writer_our_msg;

        return kSessionTerminated;
      }
    }

    if (sock ==
        dropcopy_multicast_receiver_socket_.socket_file_descriptor()) {  // drop copy will broadcast only exec reports

      unsigned int exe_report_read_bytes_ = dropcopy_multicast_receiver_socket_.ReadN(
          sizeof(HFSAT::ORS::BMFEPFIX::ExecutionReport), (void *)(&container_.exec_report));

      if (exe_report_read_bytes_ < sizeof(HFSAT::ORS::BMFEPFIX::ExecutionReport)) {
        dbglogger_ << " Read : " << exe_report_read_bytes_
                   << " bytes, expected : " << sizeof(HFSAT::ORS::BMFEPFIX::ExecutionReport) << "\n";
        dbglogger_.DumpCurrentBuffer();
        continue;  // fetch next packet
      }

      onExecReport(container_.exec_report);
      continue;
    }
  }
}

#undef MAX_BMFEPFIX_MSG_SIZE

// Handle multiple msgs. Using a naive implementation now, should be optimized.
void BMFEPEngine::processMsgs(char *msg_char_buf_, int read_size_) {
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

// These two rejects are what I call special purpose rejects, mainly because I don't expect them to happen,
// and because we will still treat them as simple reject msgs.
void BMFEPEngine::onSessionLevelReject(HFSAT::ORS::BMFEPFIX::t_FIX_SeqNum server_seqnum_) {
  onBusinessLevelReject(server_seqnum_);
}

void BMFEPEngine::onBusinessLevelReject(HFSAT::ORS::BMFEPFIX::t_FIX_SeqNum server_seqnum_) {
  if ((server_seqnum_ - starting_sequence_offset_) >= MAX_SEQUENCE_NUMBERS_WE_EXPECT_DURING_ONE_RUN) {
    SendEmailNotification(
        "RECEIVED AN EXCHANGE REPLY REJECT FOR WHICH NO METADATA RECORD EXISTS AND HENCE WILL NOT BE PASSED TO "
        "CLIENTS");
    return;
  }

  if (false == sequence_to_metadata_vec_[server_seqnum_ - starting_sequence_offset_]->flush_trigger_) {
    SendEmailNotification(
        "THREADS LAGGING NOTICED, DATA NOT FLUSHED TO MEMORY AND CAUSING READ WAIT, NOT PASSING REJECT TO CLIENTS");
    return;
  }

  // Valid Data Found
  if (true == sequence_to_metadata_vec_[server_seqnum_ - starting_sequence_offset_]->is_send_otherwise_cxl_) {
    p_engine_listener_->OnReject(
        sequence_to_metadata_vec_[server_seqnum_ - starting_sequence_offset_]->server_assigned_sequence);

  } else {
    p_engine_listener_->OnCxlReject(
        sequence_to_metadata_vec_[server_seqnum_ - starting_sequence_offset_]->server_assigned_sequence,
        kCxlRejectReasonUnknownOrder);
  }
}

void BMFEPEngine::onCxlReject(HFSAT::ORS::BMFEPFIX::ExecutionReport &cxl_reject_report_) {
  HFSAT::ORS::BMFEPFIX::t_FIX_SeqNum server_assigned_order_sequence_ = cxl_reject_report_.getExecTag11();

  /*
      //first check if the profile is correct then check if the saos is already processed
      std::string symbol_profile_ = cxl_reject_report_.getExecTag55 () ;

      std::string symbol_ = "" ;
      std::string profile_ = "" ;

      if( symbol_profile_.find( "~" ) != std::string::npos ){

      symbol_ =  symbol_profile_.substr( 0, symbol_profile_.find( "~" ) ) ;
      profile_ = symbol_profile_.substr( symbol_profile_.find( "~" ) + 1 ) ;

      dbglogger_ << " DropCopy Report Received with SAOS : " << server_assigned_order_sequence_ << " Symbol : " <<
     symbol_ << " Profile : " << profile_ << "\n" ;

      if( profile_ != senderCompID_){

      dbglogger_ << " Report Not Intended For This Session \n" ;
      dbglogger_.DumpCurrentBuffer () ;

      return ;  //must be dropcopy msg intended for other ors
      }

      dbglogger_.DumpCurrentBuffer () ;

      }else{

      symbol_ = symbol_profile_ ; //msg from the current session

      }

      if( on_cxlrej_saos_processed_.find( server_assigned_order_sequence_ ) != on_cxlrej_saos_processed_.end() ){

      dbglogger_ << " Cxl Reject Already Processed Saos : " << server_assigned_order_sequence_ << "\n" ;
      on_cxlrej_saos_processed_.erase ( server_assigned_order_sequence_ ) ;

      }

      on_cxlrej_saos_processed_ [ server_assigned_order_sequence_ ] = true ;
   */

  int cxl_rej_reason_ = cxl_reject_report_.getExecTag102();
  // char cxl_rej_response_to_ = cxl_reject_report_.getExecTag434 ();

  // Reasons are for now categorised into 2 classes: THROTTLING & NON-THROTTLING
  switch (cxl_rej_reason_) {
    case 0:  // CxlRejReason_TOO_LATE_TO_CANCEL
    case 1:  // CxlRejReason_UNKNOWN_ORDER // Non throttling
      p_engine_listener_->OnCxlReject(server_assigned_order_sequence_, kCxlRejectReasonUnknownOrder);
      break;

    default:  // Throttling
      p_engine_listener_->OnCxlReject(server_assigned_order_sequence_, kCxlRejectReasonOther);
      break;
  }
}

// Highly inefficient way of processing the execution report.
// Mostly copy-converted from the old BMFEPEngine.cpp onMessage (ExecutionReport &).
// TODO_OPT.
void BMFEPEngine::onExecReport(HFSAT::ORS::BMFEPFIX::ExecutionReport &exec_report_) {
  HFSAT::ORS::BMFEPFIX::t_FIX_SeqNum server_assigned_order_sequence_ = exec_report_.getExecTag11();
  HFSAT::ORS::BMFEPFIX::t_FIX_OrdStatus ordstatus_ = exec_report_.getExecTag39();

  // first check if the profile is correct then check if the saos is already processed
  std::string symbol_profile_ = exec_report_.getExecTag55();

#if NOT_USING_ZERO_LOGGIN_ORS

  struct timeval tv;
  gettimeofday(&tv, NULL);

  std::ostringstream usec_timestamp_;
  usec_timestamp_ << (tv.tv_sec * 1000000 + tv.tv_usec);

#endif

  std::string symbol_ = "";
  std::string profile_ = "";

  if (symbol_profile_.find("~") != std::string::npos) {
    symbol_ = symbol_profile_.substr(0, symbol_profile_.find("~"));
    profile_ = symbol_profile_.substr(symbol_profile_.find("~") + 1);

#if NOT_USING_ZERO_LOGGIN_ORS
    dbglogger_ << " DropCopy Report Received with SAOS : " << server_assigned_order_sequence_ << " Symbol : " << symbol_
               << " Profile : " << profile_ << "\n";
#endif

    if (profile_ != senderCompID_) {
#if NOT_USING_ZERO_LOGGIN_ORS
      dbglogger_ << " Report Not Intended For This Session \n";
      dbglogger_.DumpCurrentBuffer();
#endif

      return;  // must be dropcopy msg intended for other ors
    }

#if NOT_USING_ZERO_LOGGIN_ORS
    dbglogger_ << " DC SAOS : " << server_assigned_order_sequence_ << " Status : " << ordstatus_ << " @ "
               << (usec_timestamp_.str()) << "\n";
    dbglogger_.DumpCurrentBuffer();
#endif

  } else {
#if NOT_USING_ZERO_LOGGIN_ORS
    dbglogger_ << " SESSION SAOS : " << server_assigned_order_sequence_ << " Status : " << ordstatus_ << " @ "
               << (usec_timestamp_.str()) << "\n";
    dbglogger_.DumpCurrentBuffer();
#endif

    symbol_ = symbol_profile_;  // msg from the current session
  }

  HFSAT::ORS::BMFEPFIX::t_FIX_ExecType exectype_ = exec_report_.getExecTag150();

#if NOT_USING_ZERO_LOGGIN_ORS
  fixlog_ofs_ << "35=9 | 39=" << ordstatus_ << " | 150=" << exectype_ << " | 11=" << server_assigned_order_sequence_
              << std::endl;
#endif

  switch (ordstatus_) {
    case '0': {
      // Order confirmation case
      // Tag 150 must be 0
      if (exectype_ == '0') {
        if (on_conf_saos_processed_.find(server_assigned_order_sequence_) != on_conf_saos_processed_.end()) {
#if NOT_USING_ZERO_LOGGIN_ORS
          dbglogger_ << " Conf Already Processed Saos : " << server_assigned_order_sequence_ << "\n";
#endif
          on_conf_saos_processed_.erase(server_assigned_order_sequence_);

          return;
        }

        on_conf_saos_processed_[server_assigned_order_sequence_] = true;

        // CAUTION: if Length of Tag 37 is more than EXCH_ASSIGNED_ORDER_SEQUENCE_ (17),
        // We are extracting only the first (17) characters, not sure if it will cause
        // issues down the line when calling exchange for an order and we  dont have the
        // the complete orderID with us --  ramkris
        p_engine_listener_->OnOrderConf(server_assigned_order_sequence_, exec_report_.getExecTag37(),
                                        exec_report_.getExecTag44(), exec_report_.getExecTag151(), 0,
                                        exec_report_.GetExchOrderId());

      } else if (exectype_ == '5') {
        if (on_mod_saos_processed_.find(server_assigned_order_sequence_) != on_mod_saos_processed_.end()) {
#if NOT_USING_ZERO_LOGGIN_ORS
          dbglogger_ << " Mod Already Processed Saos : " << server_assigned_order_sequence_ << "\n";
#endif
          on_mod_saos_processed_.erase(server_assigned_order_sequence_);

          return;
        }

        on_mod_saos_processed_[server_assigned_order_sequence_] = true;

        // For Order Modification Execution Report in BMFEP, we get
        // Tag 39= 0, Tag150 = 5
        // The saos will have changed according to the new order that
        // we sent as cancel replace but the exchange_assigned (37)
        // will still correspond to the original placed order
        p_engine_listener_->OnOrderMod(server_assigned_order_sequence_, exec_report_.getExecTag37(),
                                       exec_report_.getExecTag44(), exec_report_.getExecTag151());
      }

    } break;

    case '4':
      // case 'C': Ther
      {  // cancel confirmation

        if (on_cxl_saos_processed_.find(server_assigned_order_sequence_) != on_cxl_saos_processed_.end()) {
#if NOT_USING_ZERO_LOGGIN_ORS
          dbglogger_ << " Cxl Conf Already Processed Saos : " << server_assigned_order_sequence_ << "\n";
#endif
          on_cxl_saos_processed_.erase(server_assigned_order_sequence_);

          return;
        }

        on_cxl_saos_processed_[server_assigned_order_sequence_] = true;

        if (exectype_ == '4' || exectype_ == 'C')  // Canceled or expired
          p_engine_listener_->OnOrderCxl(server_assigned_order_sequence_, exec_report_.GetExchOrderId());
      }
      break;

    case '5': {
      // modify confirmation ( Remove Replace Conf..)

      if (on_mod_saos_processed_.find(server_assigned_order_sequence_) != on_mod_saos_processed_.end()) {
#if NOT_USING_ZERO_LOGGIN_ORS
        dbglogger_ << " Mod Already Processed Saos : " << server_assigned_order_sequence_ << "\n";
#endif
        on_mod_saos_processed_.erase(server_assigned_order_sequence_);

        return;
      }

      on_mod_saos_processed_[server_assigned_order_sequence_] = true;

      p_engine_listener_->OnOrderMod(server_assigned_order_sequence_, exec_report_.getExecTag37(),
                                     exec_report_.getExecTag44(), exec_report_.getExecTag151());

    } break;

    case '1':
    // partial trade notification
    case '2':
      // full trade notification
      {
        if (ordstatus_ == '1') {
          HFSAT::ORS::BMFEPFIX::t_FIX_OrderQty remaining_qty_ = exec_report_.getExecTag151();

          std::ostringstream partial_exec_key_;
          partial_exec_key_ << server_assigned_order_sequence_ << "-" << remaining_qty_;

          if (on_pexec_saos_processed_.find(partial_exec_key_.str()) != on_pexec_saos_processed_.end()) {
#if NOT_USING_ZERO_LOGGIN_ORS
            dbglogger_ << " Partial Exec Already Processed Saos : " << server_assigned_order_sequence_
                       << " Rem Qty : " << remaining_qty_ << "\n";
#endif
            on_pexec_saos_processed_.erase(partial_exec_key_.str());

            return;
          }

          on_pexec_saos_processed_[partial_exec_key_.str()] = true;

        } else if (ordstatus_ == '2') {
          if (on_exec_saos_processed_.find(server_assigned_order_sequence_) != on_exec_saos_processed_.end()) {
#if NOT_USING_ZERO_LOGGIN_ORS
            dbglogger_ << " Exec Already Processed Saos : " << server_assigned_order_sequence_ << "\n";
#endif
            on_exec_saos_processed_.erase(server_assigned_order_sequence_);

            return;
          }

          on_exec_saos_processed_[server_assigned_order_sequence_] = true;
        }

        // Tag 150 must be F
        if (exectype_ == 'F')
          p_engine_listener_->OnOrderExec(server_assigned_order_sequence_, symbol_.c_str(),
                                          (exec_report_.getExecTag54() == 1 ? kTradeTypeBuy : kTradeTypeSell),
                                          exec_report_.getExecTag31(), exec_report_.getExecTag32(),
                                          exec_report_.getExecTag151(), exec_report_.GetExchOrderId());
      }
      break;

    case '8': {
      // Doesnot matter what Tag 150 is (probably 8) we reject just on basis of 39=8
      //
      if (on_rej_saos_processed_.find(server_assigned_order_sequence_) != on_rej_saos_processed_.end()) {
#if NOT_USING_ZERO_LOGGIN_ORS
        dbglogger_ << " Rej Already Processed Saos : " << server_assigned_order_sequence_ << "\n";
#endif
        on_rej_saos_processed_.erase(server_assigned_order_sequence_);

        return;
      }

      on_rej_saos_processed_[server_assigned_order_sequence_] = true;

      p_engine_listener_->OnReject(server_assigned_order_sequence_);

    } break;

    default: {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << ' ' << " Unexpected ordstatus_ = " << ordstatus_ << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
  }
}

void BMFEPEngine::OnAddString(unsigned int t_num_security_id_) {
  std::string group_code_ = " ";
  int this_index = t_num_security_id_ - 1;
  SimpleSecuritySymbolIndexer &indexer = SimpleSecuritySymbolIndexer::GetUniqueInstance();

  std::string exch_sym = indexer.GetSecuritySymbolFromId(this_index);
  if (symbol_to_sec_id_map_.find(exch_sym) == symbol_to_sec_id_map_.end()) {
    std::cerr << "Group code for " << exch_sym << " not known. Assuming ''" << std::endl;
  } else {
    group_code_ = symbol_to_sec_id_map_[exch_sym];
    std::cerr << "Group code for " << exch_sym << " found " << group_code_ << std::endl;
  }

  time_t time_ = time(NULL);
  container_.new_order_map[this_index] = new BMFEPFIX::NewOrder(
      senderCompID_.c_str(),  // Tag 49
      targetCompID_.c_str(),  // Tag 56
      accountID_.c_str(), sender_location_.c_str(), entering_trader_.c_str(), entering_firm_.c_str(),
      group_code_.c_str(),  // secid    Tag 48
      exch_sym.c_str(),     // Symbol  Tag 55
      make_optimize_assumptions_, time_, is_exchange_stp_enabled_);

  container_.cancel_order_map[this_index] =
      new BMFEPFIX::CancelOrder(senderCompID_.c_str(), targetCompID_.c_str(), accountID_.c_str(),
                                sender_location_.c_str(), entering_trader_.c_str(), entering_firm_.c_str(),
                                group_code_.c_str(), exch_sym.c_str(), make_optimize_assumptions_, time_);

  container_.cancel_replace_map[this_index] =
      new BMFEPFIX::CancelReplace(senderCompID_.c_str(), targetCompID_.c_str(), accountID_.c_str(),
                                  sender_location_.c_str(), entering_trader_.c_str(), entering_firm_.c_str(),
                                  group_code_.c_str(), exch_sym.c_str(), make_optimize_assumptions_, time_);

  // For canceling and memory deallocation in destructor
  security_id_list_.push_back(this_index);

  // Add to FastPxConverter map
  HFSAT::SecurityDefinitions &security_definitions_ =
      HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());
  HFSAT::ShortcodeContractSpecificationMap &this_contract_specification_map_ =
      security_definitions_.contract_specification_map_;
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());

  for (auto itr_ = this_contract_specification_map_.begin(); itr_ != this_contract_specification_map_.end(); itr_++) {
    std::string shortcode_ = (itr_->first);
    const char *temp_ptr = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
    if (temp_ptr == NULL) {
      continue;
    }
    std::string this_symbol = temp_ptr;
    if (exch_sym == this_symbol) {
      // This is the corresponding security
      HFSAT::ContractSpecification &contract_spec_ = (itr_->second);
      if (fast_px_convertor_vec_[this_index] == NULL) {
        fast_px_convertor_vec_[this_index] = new HFSAT::FastPriceConvertor(contract_spec_.min_price_increment_);
      }
      break;
    }
  }
}

void BMFEPEngine::SendOrder(Order *order) { SendOrder(order, container_.new_order_map[order->security_id_]); }

void BMFEPEngine::CancelOrder(Order *order) { CancelOrder(order, container_.cancel_order_map[order->security_id_]); }

void BMFEPEngine::ModifyOrder(Order *order, Order *orig_order) {
  ModifyOrder(order, container_.cancel_replace_map[order->security_id_], orig_order);
}

void BMFEPEngine::BeginPlaybackMode() {
  if (!is_resend_request_sent_in_playback_mode_) {
    dbglogger_ << "Starting Playback Mode\n";
    dbglogger_.DumpCurrentBuffer();
    is_resend_request_sent_in_playback_mode_ = true;
    ResendRequest(1);
  }
}
}
}
