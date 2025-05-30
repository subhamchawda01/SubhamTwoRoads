/**
   \file lwfixfast/eobi_md_processor.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "infracore/BSEMD/bse_md_processor.hpp"

namespace BSE_MD_Processor {

BSEMDProcessor::BSEMDProcessor(HFSAT::MulticastSenderSocket* sock_, HFSAT::BSERawMDHandler* rls,
                                 HFSAT::FastMdConsumerMode_t mode_, bool recoveryFlag_, bool full_mode)
    : sock(sock_),
      secondary_sock(NULL),
      eobiLiveSource(rls),
      live_source_products_list_(),
      mdsLogger("BSE2"), //("BSE"),
      send_sequence_(1),
      initial_recovery_not_finished_(true),
      mode(mode_),
      full_mode_(full_mode),
      ref_mode_product_list_(),
      cstr(new EOBI_MDS::EOBICommonStruct()),
      idmap_(),
      sec_id_to_min_px_inc_map_(),
      order_book_for_normal_pricefeed_(),
      mds_shm_interface_(nullptr),
      generic_mds_message_(new HFSAT::MDS_MSG::GenericMDSMessage()),
      spread_security_id_(0) {
      
  if (mode == HFSAT::kModeMax) {
    return;  // dummy arg
  } else if (mode == HFSAT::kLogger) {
    mdsLogger.run();
  } else if (mode == HFSAT::kComShm) {
    mds_shm_interface_ = &HFSAT::Utils::MDSShmInterface::GetUniqueInstance();
  }

  md_time_.tv_sec = 0;

  gettimeofday(&logger_start_time_, NULL);

  if (mode == HFSAT::kReference) {
    ReferenceDataProcessing();
    return;
  }
  mkt_seg_id_to_prod_name_.clear();
  mkt_seg_id_to_seq_num_.clear();
  target_msg_seq_num_.clear();
  sec_to_mkt_seg_id_.clear();
  curr_sec_id_.clear();
  instr_summry_rcvd_.clear();
  security_in_recovery.clear();
  prod_recovery_count_.clear();
  mkt_seg_to_security_.clear();
  message_buffer_.clear();
  price_buffer_.clear();
  sec_id_to_indexed_sec_id_.clear();
  sec_id_to_contract_code_.clear();

  total_recovery_count_ = 0;

  if (mode == HFSAT::kMcast) {
    ReadContractCodes();
    secondary_sock = new HFSAT::MulticastSenderSocket("239.23.0.77", 56999, "eth6.2490");
  }

  //ReadProductCodes();
  
  if (mode == HFSAT::kProShm) {
    eobi_shm_writer_ = new SHM::ShmWriter<EOBI_MDS::EOBICommonStruct>(SHM_KEY_EOBI_RAW, EOBI_RAW_SHM_QUEUE_SIZE);
  }

  if (mode == HFSAT::kComShm || mode == HFSAT::kPriceFeedLogger) {
    InitializeEOBIPriceFeedShortcodes();
  }

  // Start recovery process at the start
//  StartRecoveryForAllProducts();
}

// Read contract codes: contract codes are used for low bandwidth eurex mds structs
void BSEMDProcessor::ReadContractCodes() {
  std::ifstream livesource_contractcode_shortcode_mapping_file_;
  livesource_contractcode_shortcode_mapping_file_.open(DEF_LS_PRODUCTCODE_SHORTCODE_);

  if (!livesource_contractcode_shortcode_mapping_file_.is_open()) {
    std::cerr << " Couldn't open broadcast product list file : " << DEF_LS_PRODUCTCODE_SHORTCODE_ << "\n";
    exit(1);
  }

  std::cerr << " Read Contract Code To Shortcode List File : " << DEF_LS_PRODUCTCODE_SHORTCODE_ << "\n";

  char productcode_shortcode_line_[1024];

  int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

  while (livesource_contractcode_shortcode_mapping_file_.good()) {
    livesource_contractcode_shortcode_mapping_file_.getline(productcode_shortcode_line_, 1024);

    std::string check_for_comment_ = productcode_shortcode_line_;

    if (check_for_comment_.find("#") != std::string::npos) {
      continue;  // Skip comments
    }

    HFSAT::PerishableStringTokenizer st_(productcode_shortcode_line_, 1024);
    const std::vector<const char*>& tokens_ = st_.GetTokens();

    if (tokens_.size() != 2) {
      continue;  // Skip incorrect format
    }

    uint8_t this_contract_code_ = uint8_t(atoi(tokens_[0]));
    std::string this_shortcode_ = tokens_[1];
    std::string this_exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(this_shortcode_);

    if (live_source_products_list_.find(this_contract_code_) == live_source_products_list_.end()) {
      live_source_products_list_[this_contract_code_] = true;
    }

    // don't really need two maps, can use only one
    if (exchange_symbol_to_shortcode_map_.find(this_exch_symbol_) == exchange_symbol_to_shortcode_map_.end()) {
      exchange_symbol_to_shortcode_map_[this_exch_symbol_] = this_shortcode_;

      if (shortcode_to_product_code_map_.find(this_shortcode_) == shortcode_to_product_code_map_.end()) {
        shortcode_to_product_code_map_[this_shortcode_] = this_contract_code_;
      }
    }
  }

  livesource_contractcode_shortcode_mapping_file_.close();
}

// Read product reference data from file "/spare/local/files/EUREX/eobi-prod-codes.txt"
void BSEMDProcessor::InitializeEOBIPriceFeedShortcodes() {
  if (full_mode_) {
    for (auto pair : idmap_) {
      double min_px_inc = sec_id_to_min_px_inc_map_[pair.first];
      std::string dummy_shc = "INVALID";
      order_book_for_normal_pricefeed_[idmap_[pair.first]] =
          new HFSAT::EobiOrderBookForNPF(dummy_shc, idmap_[pair.first], min_px_inc);
    }
  } else {
    HFSAT::SecurityDefinitions& security_definitions_ =
        HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());
    HFSAT::ShortcodeContractSpecificationMap this_contract_specification_map_ =
        security_definitions_.contract_specification_map_;
    HFSAT::ShortcodeContractSpecificationMapCIter_t itr_ = this_contract_specification_map_.begin();

    HFSAT::ExchangeSymbolManager::SetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());

    for (itr_ = this_contract_specification_map_.begin(); itr_ != this_contract_specification_map_.end(); itr_++) {
      std::string shortcode_ = (itr_->first);
      HFSAT::ContractSpecification contract_spec_ = (itr_->second);

      // Only Search For EUREX, EOBI check is redundant
      if (contract_spec_.exch_source_ != HFSAT::kExchSourceEUREX &&
          contract_spec_.exch_source_ != HFSAT::kExchSourceBSE)
        continue;

      std::string this_exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
      order_book_for_normal_pricefeed_[this_exch_symbol_] =
          new HFSAT::EobiOrderBookForNPF(shortcode_, this_exch_symbol_);
    }
  }
}

// Read product reference data from file "/spare/local/files/EUREX/eobi-prod-codes.txt"
void BSEMDProcessor::ReadProductCodes() {
  std::ifstream dfile;

  std::string ref_file_name = full_mode_ ? DEF_EOBI_FULL_REFLOC_ : DEF_EOBI_REFLOC_;

  dfile.open(ref_file_name.c_str(), std::ofstream::in);

  if (!dfile.is_open()) {
    fprintf(stderr, "Cannot open file %s for reading reference data\n", ref_file_name.c_str());
    exit(-1);
  }

  char line[1024];
  while (!dfile.eof()) {
    memset(line, 0, sizeof(line));
    dfile.getline(line, sizeof(line));

    if (strlen(line) == 0 || line[0] == '#')  /// comments etc
    {
      continue;
    }

    int64_t num_id = atoll(strtok(line, "\n\t "));

    char* sec_name_ = (char*)calloc(12, sizeof(char));
    strncpy(sec_name_, strtok(NULL, "\n\t "), 12);

    int mkt_seg_id_ = atoi(strtok(NULL, "\n\t "));

    double min_px_increment = atof(strtok(NULL, "\n\t"));

    if (mode == HFSAT::kRaw || mode == HFSAT::kComShm) {
      int t_secid = eobiLiveSource->getSecurityNameIndexer().GetIdFromSecname(sec_name_);
      if (t_secid < 0) {
        continue;
      }
      sec_id_to_indexed_sec_id_[num_id] = t_secid;
    } else if (mode == HFSAT::kMcast) {
      // Skip the securities that we are not interested in multicasting to other locations
      if (exchange_symbol_to_shortcode_map_.find(sec_name_) == exchange_symbol_to_shortcode_map_.end()) {
        continue;
      }
    }

    if (mkt_seg_id_to_prod_name_.find(mkt_seg_id_) == mkt_seg_id_to_prod_name_.end()) {
      char* product_name_ = (char*)calloc(5, sizeof(char));
      strncpy(product_name_, sec_name_, 5);
      product_name_[4] = '\0';
      if (strcmp(product_name_, "FVS2") == 0) product_name_[3] = '\0';  // Hack to get the prod_name correct for FVS
      mkt_seg_id_to_prod_name_[mkt_seg_id_] = product_name_;
      mkt_seg_to_security_[mkt_seg_id_];
    }

    idmap_[num_id] = sec_name_;
    sec_id_to_min_px_inc_map_[num_id] = min_px_increment;
    target_msg_seq_num_[num_id] = 0;
    sec_to_mkt_seg_id_[num_id] = mkt_seg_id_;
    instr_summry_rcvd_[num_id] = false;
    security_in_recovery[num_id] = true;

    if (exchange_symbol_to_shortcode_map_.find(sec_name_) != exchange_symbol_to_shortcode_map_.end()) {
      std::string shortcode_ = exchange_symbol_to_shortcode_map_[sec_name_];
      uint8_t contract_code_ = shortcode_to_product_code_map_[shortcode_];
      sec_id_to_contract_code_[num_id] = contract_code_;
      order_book_[contract_code_] = new HFSAT::EobiOrderBook(shortcode_, contract_code_);
    }

    mkt_seg_id_to_seq_num_[mkt_seg_id_] = 0;
    curr_sec_id_[mkt_seg_id_] = -1;
    prod_recovery_count_[mkt_seg_id_] = 0;
    mkt_seg_to_security_[mkt_seg_id_].push_back(num_id);
  }
  dfile.close();
}

void BSEMDProcessor::ReferenceDataProcessing() {
  leg_sec_id_ = (int64_t*)malloc(sizeof(int64_t) * 2);

  /// open files for writing in reference mode
  HFSAT::FileUtils::MkdirEnclosing(DEF_EOBI_REFLOC_);  // makes the directory if it does not exist

  std::string ref_file_name = full_mode_ ? DEF_EOBI_FULL_REFLOC_ : DEF_EOBI_REFLOC_;

  reference_info_file_.open(ref_file_name.c_str(), std::ofstream::out);

  if (!reference_info_file_.is_open()) {
    fprintf(stderr, "Cannot open file %s for logging address data\n", ref_file_name.c_str());
    exit(-1);
  }
  reference_info_file_ << "##Numeric_Id\tInternal_Ref\tMktSegId\tMinPxIncrement" << std::endl;

  std::string mcast_file_name = full_mode_ ? DEF_EOBI_FULL_MCASTS_ : DEF_EOBI_MCASTS_;
  multicast_info_file_.open(mcast_file_name.c_str(), std::ofstream::out);

  if (!multicast_info_file_.is_open()) {
    fprintf(stderr, "Cannot open file %s for logging address data\n", mcast_file_name.c_str());
    exit(-1);
  }
  multicast_info_file_ << "##Product:\tStreamType:\tStreamService:\tAddress:\tPort:\tMktSegId:" << std::endl;

  fpga_feed_file_.open(DEF_EOBI_FEEDS_, std::ofstream::out);
  if (!fpga_feed_file_.is_open()) {
    fprintf(stderr, "Cannot open file %s for logging fpga feeds\n", DEF_EOBI_FEEDS_);
    exit(-1);
  }

  prref_seen_.clear();
  prmcast_seen_.clear();
  mktseg_id_2_name_.clear();
  simple_secid_to_expiry_.clear();

  std::ifstream ref_mode_product_list_file_;
  ref_mode_product_list_file_.open(DEF_EOBI_REF_PRODUCTLIST_);
  if (!ref_mode_product_list_file_.is_open()) {
    std::cerr << " Couldn't open broadcast product list file : " << DEF_EOBI_REF_PRODUCTLIST_ << "\n";
    exit(1);
  }

  std::cerr << " Read REFMODE Product List File : " << DEF_EOBI_REF_PRODUCTLIST_ << "\n";

  char product_list_line_[1024];

  while (ref_mode_product_list_file_.good()) {
    ref_mode_product_list_file_.getline(product_list_line_, 1024);
    std::string _this_shortcode_ = product_list_line_;

    if (_this_shortcode_.find("#") != std::string::npos) continue;  // comments

    // all eobi base shortcodes have length = 4
    if (_this_shortcode_.length() < 3) continue;

    std::cerr << " REF MODE Added Product : " << _this_shortcode_ << "\n";

    // store symbols to broadcast
    ref_mode_product_list_[_this_shortcode_] = true;
  }
}

// checks whether the sequence number is valid
// whether
void BSEMDProcessor::CheckSequenceNo(int seq_no_, bool start_recovery_on_mismatch_) {
  if (mkt_seg_id_to_prod_name_.find(current_mkt_seg_id_) == mkt_seg_id_to_prod_name_.end()) {
    return;
  }

  if (prod_recovery_count_[current_mkt_seg_id_] <= 0) {  // product is not in recovery
    if (seq_no_ > (int)(mkt_seg_id_to_seq_num_[current_mkt_seg_id_] + 1)) {
      if (start_recovery_on_mismatch_) {
        std::cout << "\n"
                  << "Seq num mismatch:" << mkt_seg_id_to_seq_num_[current_mkt_seg_id_] + 1 << "\t" << seq_no_
                  << "\n\n";
        StartRecoveryForProduct(current_mkt_seg_id_);
      }
    } else if (seq_no_ < (int)(mkt_seg_id_to_seq_num_[current_mkt_seg_id_] + 1)) {
      // Error case
    } else {
      // std::cout << "\t" << seq_no_ << std::endl;
      mkt_seg_id_to_seq_num_[current_mkt_seg_id_]++;
    }
  }
}

void BSEMDProcessor::CheckSequenceNum() {
  // std::cout << "Product: " << mkt_seg_id_to_prod_name_[current_mkt_seg_id_] << " is in recovery." << std::endl;
  CreateMdsStruct();
  EOBI_MDS::EOBICommonStruct* eobi_cstr_ = (EOBI_MDS::EOBICommonStruct*)malloc(sizeof(EOBI_MDS::EOBICommonStruct));
  memcpy(eobi_cstr_, cstr, sizeof(EOBI_MDS::EOBICommonStruct));

  message_buffer_.push_back(std::make_pair(eobi_cstr_, o_security_id_));
  price_buffer_.push_back(std::make_pair(o_price_, o_prev_price_));

  timeval current_time_;
  gettimeofday(&current_time_, NULL);

  if (current_time_.tv_sec - logger_start_time_.tv_sec > 60) {
    for (auto it_ = security_in_recovery.begin(); it_ != security_in_recovery.end(); it_++) {
      if (it_->second == true) {
        EndRecovery(it_->first);
      }
    }
  }
}

inline void BSEMDProcessor::CreateMdsStruct() {
  memset(cstr, 0, sizeof(EOBI_MDS::EOBICommonStruct));

  cstr->data_.order_.action_ = o_action_;
  cstr->data_.order_.msg_seq_num_ = o_seq_num_;
  cstr->token_ = o_security_id_;

  switch (o_action_) {
    case '0':  // Order Add
    {
      cstr->data_.order_.intermediate_ = o_intermediate_;
      cstr->data_.order_.price = o_price_ / EOBI_PRICE_DIVIDER;
      cstr->data_.order_.priority_ts = o_priority_ts_;
      cstr->data_.order_.side = o_side_ == 1 ? 'B' : 'S';
      cstr->data_.order_.size = o_size_;
      cstr->data_.order_.trd_reg_ts = o_trd_reg_ts_;
    } break;
    case '1':  // Order Modify
    {
      cstr->data_.order_.price = o_price_ / EOBI_PRICE_DIVIDER;
      cstr->data_.order_.priority_ts = o_priority_ts_;
      cstr->data_.order_.side = o_side_ == 1 ? 'B' : 'S';
      cstr->data_.order_.size = o_size_;
      cstr->data_.order_.prev_price = o_prev_price_ / EOBI_PRICE_DIVIDER;
      cstr->data_.order_.prev_size = o_prev_size_;
      cstr->data_.order_.prev_priority_ts = o_prev_priority_ts_;
      cstr->data_.order_.trd_reg_ts = o_trd_reg_ts_;
    } break;
    case '2':  // Order Delete
    {
      cstr->data_.order_.price = o_price_ / EOBI_PRICE_DIVIDER;
      cstr->data_.order_.priority_ts = o_priority_ts_;
      cstr->data_.order_.side = o_side_ == 1 ? 'B' : 'S';
      cstr->data_.order_.size = o_size_;
      cstr->data_.order_.trd_reg_ts = o_trd_reg_ts_;
    } break;
    case '3':  // Order Mass Delete
    {
    } break;
    case '4':  // Partial Execution
    {
      cstr->data_.order_.price = o_price_ / EOBI_PRICE_DIVIDER;
      cstr->data_.order_.priority_ts = o_priority_ts_;
      cstr->data_.order_.side = o_side_ == 1 ? 'B' : 'S';
      cstr->data_.order_.size = o_size_;
    } break;
    case '5':  // Full Execution
    {
      cstr->data_.order_.price = o_price_ / EOBI_PRICE_DIVIDER;
      cstr->data_.order_.priority_ts = o_priority_ts_;
      cstr->data_.order_.side = o_side_ == 1 ? 'B' : 'S';
      cstr->data_.order_.size = o_size_;
    } break;
    case '6':  // Execution Summary
    {
      cstr->data_.order_.price = o_price_ / EOBI_PRICE_DIVIDER;
      cstr->data_.order_.side = o_side_ == 1 ? 'B' : 'S';
      cstr->data_.order_.size = o_size_;
      cstr->data_.order_.prev_size = (int)o_synthetic_match_;  // Hack to get synthetic match
      cstr->data_.order_.trd_reg_ts = o_trd_reg_ts_;
    } break;
    default:
      break;
  }
}

void BSEMDProcessor::ProcessOrder() {
  switch (mode) {
    case HFSAT::kLogger: {
      CreateMdsStruct();

      gettimeofday(&cstr->time_, NULL);
      cstr->msg_ = EOBI_MDS::EOBI_ORDER;
//      strcpy(cstr->data_.order_.contract_, (const char*)idmap_[o_security_id_]);

      mdsLogger.log(*cstr);
      break;
    }
    case HFSAT::kProShm: {
      CreateMdsStruct();

      gettimeofday(&cstr->time_, NULL);
      cstr->msg_ = EOBI_MDS::EOBI_ORDER;
      strcpy(cstr->data_.order_.contract_, (const char*)idmap_[o_security_id_]);

      eobi_shm_writer_->Write(cstr);
    } break;
    default: { break; }
  }
}

void BSEMDProcessor::StartRecovery(const char* p_prod_) { eobiLiveSource->startRecovery(p_prod_); }

void BSEMDProcessor::EndRecovery(const char* p_prod_) { eobiLiveSource->endRecovery(p_prod_); }

void BSEMDProcessor::EndRecoveryForAllProducts() { eobiLiveSource->endRecoveryForAllProducts(); }

void BSEMDProcessor::StartRecoveryForAllProducts() {
  for (std::map<int32_t, char*>::iterator _itr_ = mkt_seg_id_to_prod_name_.begin();
       _itr_ != mkt_seg_id_to_prod_name_.end(); _itr_++) {
    StartRecovery(_itr_->second);
  }

  for (std::map<int64_t, bool>::iterator _itr_ = security_in_recovery.begin(); _itr_ != security_in_recovery.end();
       _itr_++) {
    // StartRecovery (_itr_->first);
    _itr_->second = true;
    total_recovery_count_++;
  }
}

void BSEMDProcessor::StartRecovery(int64_t security_id_) {
  timeval current_time;
  int mkt_seg_id_ = sec_to_mkt_seg_id_[security_id_];

  if (prod_recovery_count_[mkt_seg_id_] == 0) {
    gettimeofday(&current_time, NULL);
    StartRecovery(mkt_seg_id_to_prod_name_[mkt_seg_id_]);
  }
  prod_recovery_count_[mkt_seg_id_]++;
  security_in_recovery[security_id_] = true;
  total_recovery_count_++;
}

void BSEMDProcessor::EndRecovery(int64_t security_id_) {
  timeval current_time;

  total_recovery_count_--;

  if (total_recovery_count_ <= 0) {
    gettimeofday(&current_time, NULL);
    std::cout << "Ending overall recovery at " << ctime(&(current_time.tv_sec)) << std::endl;
    std::cout << "Number of buffered messages: " << message_buffer_.size() << std::endl;
    std::cout << "Number of buffered messages: " << price_buffer_.size() << std::endl;

    for (size_t i = 0; i < message_buffer_.size(); i++) {
      std::pair<EOBI_MDS::EOBICommonStruct*, int64_t> this_pair_ = message_buffer_[i];
      EOBI_MDS::EOBICommonStruct* this_cstr_ = this_pair_.first;

      if (this_cstr_ == NULL) {
        std::cout << "memory error during free" << std::endl;
        return;
      }

      std::pair<uint64_t, uint64_t> price_pair_ = price_buffer_[i];

      o_security_id_ = this_pair_.second;
      o_action_ = this_cstr_->data_.order_.action_;
      o_seq_num_ = this_cstr_->data_.order_.msg_seq_num_;
      o_side_ = this_cstr_->data_.order_.side == 'B' ? 1 : 2;
      o_priority_ts_ = this_cstr_->data_.order_.priority_ts;
      // o_price_            = (uint64_t) (this_cstr_->data_.order_.price * EOBI_PRICE_DIVIDER);
      o_price_ = price_pair_.first;
      o_size_ = this_cstr_->data_.order_.size;
      o_prev_priority_ts_ = this_cstr_->data_.order_.prev_priority_ts;
      // o_prev_price_       = (uint64_t) (this_cstr_->data_.order_.prev_price * EOBI_PRICE_DIVIDER);
      o_prev_price_ = price_pair_.second;
      o_prev_size_ = this_cstr_->data_.order_.prev_size;
      o_intermediate_ = this_cstr_->data_.order_.intermediate_;
      o_trd_reg_ts_ = this_cstr_->data_.order_.trd_reg_ts;

      int market_segment_id_ = sec_to_mkt_seg_id_[this_pair_.second];

      if (this_cstr_->data_.order_.msg_seq_num_ > mkt_seg_id_to_seq_num_[market_segment_id_]) {
        ProcessOrder();
      }

      free(this_cstr_);
    }

    std::cout << "\n"
              << "Buffered messages processed."
              << "\n" << std::endl;

    message_buffer_.clear();
    price_buffer_.clear();
    initial_recovery_not_finished_ = false;

    EndRecoveryForAllProducts();
  }

  security_in_recovery[security_id_] = false;
}

// A product has multiple securities. Msg Seq is maintained at product level. Hence, start recovery for all the
// securities in this product.
void BSEMDProcessor::StartRecoveryForProduct(int32_t mkt_seg_id_) {
  for (std::vector<int64_t>::iterator _itr_ = mkt_seg_to_security_[mkt_seg_id_].begin();
       _itr_ != mkt_seg_to_security_[mkt_seg_id_].end(); ++_itr_) {
    security_in_recovery[*_itr_] = true;
    StartRecovery(*_itr_);
  }
}

// --------
// REFERENCE mode functions
// --------
void BSEMDProcessor::dumpProdRefInfo(const uint32_t mktSegId, const std::string& mktSegment, int feedType,
                                      const std::string& stream_service, const std::string& stream_addr,
                                      unsigned int stream_port) {
  int64_t key = (mktSegId * 100 + feedType) * 2;

  if (mktSegment.size() > 0) {
    mktseg_id_2_name_[mktSegId] = mktSegment;
  }

  if (stream_service == "A") {
    key++;  // Odd, for B its always even
  }

  if (prmcast_seen_[key]) {
    return;
  }

  multicast_info_file_ << mktSegment << "\t\t" << feedType << "\t\t" << stream_service << "\t" << stream_addr << "\t"
                       << stream_port << "\t\t\t" << mktSegId << std::endl;
  prmcast_seen_[key] = true;
}

void BSEMDProcessor::DumpFPGAFeeds(uint32_t t_mkt_seg_id_, int t_feed_type_, std::string t_ip1_, int t_port1_,
                                    std::string t_ip2_, int t_port2_) {
  std::string ip_port_key_ = t_ip1_;
  {
    std::ostringstream t_oss_;
    t_oss_ << t_ip1_ << "." << t_port1_;
    ip_port_key_ = t_oss_.str();
  }

  if (fpga_feed_dumped_[ip_port_key_]) {
    return;
  }

  fpga_feed_file_ << fpga_feed_dumped_.size() + (t_port1_ % 2) << "  " << (t_feed_type_ == 0 ? "Delta" : "Snapshot")
                  << "  " << t_ip1_ << "  " << t_port1_ << "  " << t_ip2_ << "  " << t_port2_ << std::endl
                  << std::flush;

  fpga_feed_dumped_[ip_port_key_] = true;
}

void BSEMDProcessor::dumpSLRefInfo(int64_t cntrid, uint32_t mkt_seg_id, uint32_t expiry_date_month,
                                    double min_px_increment) {
  if (prref_seen_[cntrid]) {
    return;
  }

  if (mktseg_id_2_name_.find(mkt_seg_id) == mktseg_id_2_name_.end()) {
    return;
  }

  std::cout << mktseg_id_2_name_[mkt_seg_id] << " " << expiry_date_month << " " << min_px_increment << std::endl;
  reference_info_file_ << cntrid << "\t\t" << mktseg_id_2_name_[mkt_seg_id] << expiry_date_month << "\t\t" << mkt_seg_id
                       << "\t\t" << min_px_increment << std::endl;
  prref_seen_[cntrid] = true;
  simple_secid_to_expiry_[cntrid] = expiry_date_month;
}

void BSEMDProcessor::DumpComplexInstrument(uint32_t mkt_seg_id) {
  if (prref_seen_[spread_security_id_]) {
    return;
  }

  if (!prref_seen_[leg_sec_id_[0]] || !prref_seen_[leg_sec_id_[1]]) {
    return;
  }

  if (simple_secid_to_expiry_.find(leg_sec_id_[0]) == simple_secid_to_expiry_.end() ||
      simple_secid_to_expiry_.find(leg_sec_id_[1]) == simple_secid_to_expiry_.end()) {
    return;
  }

  // Currently, only include FVS spreads
  if (mktseg_id_2_name_[mkt_seg_id] != "FVS") {
    return;
  }

  if (mktseg_id_2_name_.find(mkt_seg_id) == mktseg_id_2_name_.end()) {
    return;
  }

  reference_info_file_ << spread_security_id_ << "\t\t" << mktseg_id_2_name_[mkt_seg_id]
                       << GetExpiry(simple_secid_to_expiry_[leg_sec_id_[0]])
                       << GetExpiry(simple_secid_to_expiry_[leg_sec_id_[1]]) << "\t\t" << mkt_seg_id << "\t"
                       << leg_sec_id_[0] << "\t" << leg_sec_id_[1] << std::endl
                       << std::flush;

  prref_seen_[spread_security_id_] = true;
}

std::string BSEMDProcessor::GetExpiry(uint32_t t_expiry_) {
  int year_ = t_expiry_ / 100;
  int month_ = t_expiry_ % 100;
  std::stringstream expiry_stream_;

  if (month_ == 1)
    expiry_stream_ << "F";
  else if (month_ == 2)
    expiry_stream_ << "G";
  else if (month_ == 3)
    expiry_stream_ << "H";
  else if (month_ == 4)
    expiry_stream_ << "J";
  else if (month_ == 5)
    expiry_stream_ << "K";
  else if (month_ == 6)
    expiry_stream_ << "M";
  else if (month_ == 7)
    expiry_stream_ << "N";
  else if (month_ == 8)
    expiry_stream_ << "Q";
  else if (month_ == 9)
    expiry_stream_ << "U";
  else if (month_ == 10)
    expiry_stream_ << "V";
  else if (month_ == 11)
    expiry_stream_ << "X";
  else if (month_ == 12)
    expiry_stream_ << "Z";
  else
    expiry_stream_ << "I";  // this is for invalid

  expiry_stream_ << year_ % 100;

  return expiry_stream_.str();
}
}
