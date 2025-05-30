#include "baseinfra/MarketAdapter/fpga_nse_market_view_manager.hpp"

namespace HFSAT {

FpgaNseMarketViewManager::FpgaNseMarketViewManager(DebugLogger &t_dbglogger_, const Watch &t_watch_,
                                                   const SecurityNameIndexer &t_sec_name_indexer_,
                                                   const std::vector<SecurityMarketView *> &t_security_market_view_map_,
                                                   std::vector<std::string> &source_shortcode_list_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      p_time_keeper_(NULL),
      trade_time_manager_(TradeTimeManager::GetUniqueInstance(
          t_sec_name_indexer_, HFSAT::GlobalSimDataManager::GetUniqueInstance(t_dbglogger_).GetTradingDate())),
      nse_daily_token_symbol_handler_(HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(
          HFSAT::GlobalSimDataManager::GetUniqueInstance(t_dbglogger_).GetTradingDate())),
      nse_refdata_loader_(HFSAT::Utils::NSERefDataLoader::GetUniqueInstance(
          HFSAT::GlobalSimDataManager::GetUniqueInstance(t_dbglogger_).GetTradingDate())),
      source_shortcode_vec_(source_shortcode_list_),
      segment_to_token_secid_map_() {
  MBOCHIP::SiliconMD::EApi::EventConfig eventConfig(Configtoml);
  api = new MBOCHIP::SiliconMD::EApi::EventApi<
      MBOCHIP::SiliconMD::EApi::MessageApi<MBOCHIP::SiliconMD::EApi::NSEFPGAReceiver>>(eventConfig);

  for (std::string shortcode : source_shortcode_vec_) {
    std::string exchange_symbol = HFSAT::ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(shortcode);
    std::string internal_symbol = HFSAT::NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol);
    char segment = HFSAT::NSESecurityDefinitions::GetSegmentTypeFromShortCode(shortcode);
    int32_t token = nse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(internal_symbol.c_str(), segment);
    api->subscribeBook(token);
  }

  for (auto &itr : nse_refdata_loader_.GetNSERefData(NSE_EQ_SEGMENT_MARKING)) {
    std::ostringstream internal_symbol_str;
    internal_symbol_str << "NSE"
                        << "_" << (itr.second).symbol;
    std::string exchange_symbol = NSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol_str.str());
    int32_t security_id = -1;

    if (std::string("INVALID") != exchange_symbol) {
      security_id = sec_name_indexer_.GetIdFromSecname(exchange_symbol.c_str());
      segment_to_token_secid_map_[NSE_EQ_SEGMENT_MARKING].insert(std::make_pair(itr.first, security_id));
    }
  }

  for (auto &itr : nse_refdata_loader_.GetNSERefData(NSE_FO_SEGMENT_MARKING)) {
    std::ostringstream internal_symbol_str;

    if (std::string("XX") == std::string((itr.second).option_type)) {
      internal_symbol_str << "NSE"
                          << "_" << (itr.second).symbol << "_FUT_"
                          << HFSAT::Utils::ConvertNSEExpiryInSecToDate((itr.second).expiry);

    } else {
      internal_symbol_str << "NSE"
                          << "_" << (itr.second).symbol << "_" << (itr.second).option_type << "_";
      internal_symbol_str << std::fixed << std::setprecision(2) << (itr.second).strike_price;
      internal_symbol_str << "_" << HFSAT::Utils::ConvertNSEExpiryInSecToDate((itr.second).expiry);
    }
    std::string exchange_symbol = NSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol_str.str());

    int32_t security_id_ = -1;
    if (std::string("INVALID") != exchange_symbol) {
      security_id_ = sec_name_indexer_.GetIdFromSecname(exchange_symbol.c_str());
      segment_to_token_secid_map_[NSE_FO_SEGMENT_MARKING].insert(std::make_pair(itr.first, security_id_));
    }
  }

  std::unordered_map<std::string, int> spot_index_2_token_map_ =
      HFSAT::SpotTokenGenerator::GetUniqueInstance().GetSpotIndexToTokenMap();
  if (0 == spot_index_2_token_map_.size()) {
    std::cerr << "IX SEGMENT REF DATA NOT PRESENT" << std::endl;
    exit(-1);
  }

  for (auto &itr : spot_index_2_token_map_) {
    std::string exchange_sym = "NSE_IDX" + std::to_string(itr.second);
    int security_id = sec_name_indexer_.GetIdFromSecname(exchange_sym.c_str());
    // std::cout<<"SEC ID: " << security_id << " TOKEN: " << itr.second  << " STR: " << exchange_sym << std::endl;

    segment_to_token_secid_map_[NSE_IX_SEGMENT_MARKING].insert(
        std::make_pair(itr.second, security_id));  // currently wrong// need to check other things
  }
}

void FpgaNseMarketViewManager::SetSMVBestVars(const MBOCHIP::SiliconMD::EApi::Event *event) {
  //  std::cout <<"SMV addr 1" << std::endl;
}

void FpgaNseMarketViewManager::NotifyListenersOnLevelChange() {
  SecurityMarketView &smv_ = *market_view_ptr_;
  smv_.is_ready_ = true;

  //  CheckL1Consistency(t_security_id_);
  if (CheckValidTime(security_id)) {
    // Notify relevant listeners about the update
    if (event_type_ == k_l1price) {
      smv_.NotifyL1PriceListeners();
    } else if (event_type_ == k_l1size) {
      smv_.NotifyL1SizeListeners();
    } else if (event_type_ == k_l2change) {
      // disabled it currently only L1 updates
      smv_.NotifyL2Listeners();
      smv_.NotifyL2OnlyListeners();
    }
  }
}

void FpgaNseMarketViewManager::NotifyListenersOnTrade(const uint32_t t_security_id_, const int t_trade_int_price_,
                                                      const int t_trade_size_) {
  SecurityMarketView &smv_ = *market_view_ptr_;
  smv_.is_ready_ = true;

  double trade_price_ = smv_.GetDoublePx(t_trade_int_price_);

  smv_.trade_print_info_.trade_price_ = trade_price_;
  smv_.trade_print_info_.size_traded_ = t_trade_size_;
  smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;
  // smv_.trade_print_info_.buysell_ = t_buysell_;
  // smv_.trade_print_info_.is_intermediate_ = is_intermediate;
  // smv_.trade_print_info_.num_levels_cleared_ = _num_levels_cleared_;

  //  CheckL1Consistency(t_security_id_);
  if ((CheckValidTime(t_security_id_)) && ((t_trade_int_price_ <= smv_.upper_int_trade_range_limit_) &&
                                           (t_trade_int_price_ >= smv_.lower_int_trade_range_limit_))) {
    smv_.NotifyTradeListeners();
    smv_.NotifyOnReadyListeners();
  }
}

void FpgaNseMarketViewManager::ProcessHWBookMessage(const MBOCHIP::SiliconMD::EApi::Event *event) {
  if (segment_to_token_secid_map_[NSE_FO_SEGMENT_MARKING].find(event->message.HwBook_->token) ==
      segment_to_token_secid_map_[NSE_FO_SEGMENT_MARKING].end())
    return;
  security_id = segment_to_token_secid_map_[NSE_FO_SEGMENT_MARKING][event->message.HwBook_->token];
  if (security_id < 0) return;
  // ::cout <<"Token: " << event->message.HwBook_->token << " ID: " << security_id << std::endl;
  // std::cout <<"ASK SIZE: " << event->message.HwBook_->ask.size() << " Bid Size " <<
  // event->message.HwBook_->bid.size() <<std::endl;
  if (event->message.HwBook_->ask.size() == MAXMBOFPGALEVEL &&
      event->message.HwBook_->bid.size() ==
          MAXMBOFPGALEVEL) {  // assumption Top 5 level will always exist in the book for valid update
    SecurityMarketView &smv_ = *(security_market_view_map_[security_id]);
    market_view_ptr_ = &smv_;
    if (event->message.HwBook_->bid[0].price != smv_.market_update_info_.bestbid_price_ || event->message.HwBook_->ask[0].price != smv_.market_update_info_.bestask_price_) {
      // update l1.
      event_type_ = k_l1price;
      smv_.market_update_info_.bestbid_price_ = event->message.HwBook_->bid[0].price;
      smv_.market_update_info_.bestbid_int_price_ = event->message.HwBook_->bid[0].price / 0.05; // smv_.GetIntPx(event->message.HwBook_->bid[0].price);  //   // check
      smv_.market_update_info_.bestask_price_ = event->message.HwBook_->ask[0].price;
      smv_.market_update_info_.bestask_int_price_ = event->message.HwBook_->ask[0].price/ 0.05; // smv_.GetIntPx(event->message.HwBook_->ask[0].price);  // l1.bid.price / 0.05;
    } else if ((int)event->message.HwBook_->bid[0].quantity != smv_.market_update_info_.bestbid_size_ || 
    (int)event->message.HwBook_->ask[0].quantity != smv_.market_update_info_.bestask_size_) {
      smv_.market_update_info_.bestbid_size_ = event->message.HwBook_->bid[0].quantity;
      smv_.market_update_info_.bestask_size_ = event->message.HwBook_->ask[0].quantity;
      event_type_ = k_l1size;
    } else {
      event_type_ = k_l2change;
      return;
    }

    for (int level = 0; level <= MAXMBOFPGALEVEL; level++) {
      smv_.market_update_info_.bidlevels_[level] = MarketUpdateInfoLevelStruct(
          0, smv_.GetIntPx(event->message.HwBook_->bid[level].price), event->message.HwBook_->bid[level].price,
          event->message.HwBook_->bid[level].quantity, 0, watch_.tv());
      smv_.market_update_info_.asklevels_[level] = MarketUpdateInfoLevelStruct(
          0, smv_.GetIntPx(event->message.HwBook_->ask[level].price), event->message.HwBook_->ask[level].price,
          event->message.HwBook_->ask[level].quantity, 0, watch_.tv());
    }

    // mid/mkt prices and spread increments might be used in pnl etc classes so those need to be set as well
    if (smv_.price_type_subscribed_[kPriceTypeMidprice]) {
      smv_.market_update_info_.mid_price_ =
          (smv_.market_update_info_.bestbid_price_ + smv_.market_update_info_.bestask_price_) * 0.5;
    }
    //  std::cout <<"Update SMV Values" << std::endl;
    smv_.market_update_info_.spread_increments_ =
        smv_.market_update_info_.bestask_int_price_ - smv_.market_update_info_.bestbid_int_price_;

    if (smv_.price_type_subscribed_[kPriceTypeMktSizeWPrice]) {
      if (smv_.market_update_info_.spread_increments_ <= 1) {
        smv_.market_update_info_.mkt_size_weighted_price_ =
            (smv_.market_update_info_.bestbid_price_ * smv_.market_update_info_.bestask_size_ +
             smv_.market_update_info_.bestask_price_ * smv_.market_update_info_.bestbid_size_) /
            (smv_.market_update_info_.bestbid_size_ + smv_.market_update_info_.bestask_size_);
      } else {
        smv_.market_update_info_.mkt_size_weighted_price_ =
            ((smv_.market_update_info_.bestbid_price_ * smv_.market_update_info_.bestask_size_ +
              smv_.market_update_info_.bestask_price_ * smv_.market_update_info_.bestbid_size_) /
                 (smv_.market_update_info_.bestbid_size_ + smv_.market_update_info_.bestask_size_) +
             (smv_.market_update_info_.mid_price_)) /
            2.0;
      }
    }
    // notify listeners.
// temp    NotifyListenersOnLevelChange();
  }
  //  std::cout << "NotifyListenersOnLevelChange Done" << std::endl;
}

void FpgaNseMarketViewManager::ProcessTradeMessage(const MBOCHIP::SiliconMD::EApi::Event *event) {
  // token -> security_id.
  std::string internal_symbol =
      nse_daily_token_symbol_handler_.GetInternalSymbolFromToken(event->message.TradeMessage_->token, 'F');
  if (std::string("INVALID") == internal_symbol) return;

  std::string exchange_symbol = NSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol);
  if (std::string("INVALID") == exchange_symbol) return;

  int32_t security_id = sec_name_indexer_.GetIdFromSecname(exchange_symbol.c_str());
  if (security_id < 0) return;

  SecurityMarketView &smv_ = *(security_market_view_map_[security_id]);
  market_view_ptr_ = &smv_;

  NotifyListenersOnTrade(security_id, event->message.TradeMessage_->tradePrice, event->message.TradeMessage_->quantity);
}

bool FpgaNseMarketViewManager::CheckValidTime(int sec_id) {
  return trade_time_manager_.isValidTimeToTrade(sec_id, watch_.tv().tv_sec % 86400);
}

void FpgaNseMarketViewManager::Run() {
  // start the receiving loop
  unsigned int level = 1;  // Top level exist forward the update
  bool running = true;
  while (running) {
    const MBOCHIP::SiliconMD::EApi::Event *event = api->receiveEvent();
    if (event) {
//       HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
      gettimeofday(&source_time, NULL);
      p_time_keeper_->OnTimeReceived(source_time);
      switch (event->type) {
        case MBOCHIP::SiliconMD::EApi::EventMessageType::HwBook: {
          /*
                    int32_t msg_seq_no = (event->message.HwBook_->msgSeqNum);
                    int32_t token = (event->message.HwBook_->token);
                    struct timespec ts;
                    timespec_get(&ts, TIME_UTC);
                    std::cout << "FpgaLatencyTimeStamping " << token << " " << msg_seq_no << " " << ts.tv_sec << "."
                              << std::setw(9) << std::setfill('0') << ts.tv_nsec << " " << std::endl;
          */
          if (level <= event->message.HwBook_->ask.size() && level <= event->message.HwBook_->bid.size()) {
            //         HFSAT::DetailsDumping::GetUniqueInstance().updateToken(event->message.HwBook_->token); // Testing
            //         Numbers HFSAT::DetailsDumping::GetUniqueInstance().updateSeq(event->message.HwBook_->msgSeqNum);
            //         // Testing Numbers
            ProcessHWBookMessage(event);
//            HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
          }
        } break;
        case MBOCHIP::SiliconMD::EApi::EventMessageType::TradeMessage: {
          ProcessTradeMessage(event);
        } break;
        default:
          break;
      }
    }
  }
}

void FpgaNseMarketViewManager::DummyFunc() { std::cout << "I am dummy func()"; }
}  // namespace HFSAT
