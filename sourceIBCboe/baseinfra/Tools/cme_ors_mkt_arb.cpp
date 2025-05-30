#include "baseinfra/MarketAdapter/cme_arb_data_source.hpp"
#include "MarketAdapter/market_orders_view.hpp"

#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/load_our_defined_products.hpp"

/// signal handling
void sighandler(int signum) {
  std::cerr << " Received Termination Signal \n";
  exit(0);
}

int main(int argc, char** argv) {
  struct sigaction sigact;
  memset(&sigact, '\0', sizeof(sigact));
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);

  HFSAT::SimpleLiveDispatcher* simple_live_dispatcher = new HFSAT::SimpleLiveDispatcher();
  HFSAT::DebugLogger dbglogger(1024 * 1024, 1024 * 8);
  std::string logfilename = "/spare/local/MDSlogs/cme_ors_mkt_arb_dbg" + std::string("_") +
                            HFSAT::DateTime::GetCurrentIsoDateLocalAsString() + ".log";
  dbglogger.OpenLogFile(logfilename.c_str(), std::ios::out | std::ios::app);
  dbglogger << "OpenDbglogger\n";
  dbglogger.DumpCurrentBuffer();

  HFSAT::Watch watch(dbglogger, HFSAT::DateTime::GetCurrentIsoDateLocal());

  HFSAT::SecurityNameIndexer& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  std::set<HFSAT::ExchSource_t> exch_source_processing_;
  exch_source_processing_.insert(HFSAT::kExchSourceCME);
  HFSAT::Utils::LoadOurDefinedProducts::GetUniqueInstance(exch_source_processing_).AddExchange(HFSAT::kExchSourceCME);

  unsigned int num_securities = sec_name_indexer.GetNumSecurityId();
  std::cout << "Processing " << num_securities << " securities of CME" << std::endl;

  std::vector<HFSAT::MarketOrdersView*> sid_to_mov_ptr_map;
  sid_to_mov_ptr_map.resize(num_securities, nullptr);
  for (unsigned int sec_id = 0; sec_id < num_securities; sec_id++) {
    sid_to_mov_ptr_map[sec_id] = HFSAT::MarketOrdersView::GetUniqueInstance(dbglogger, watch, sec_id);
  }

  HFSAT::CMEArbDataSource* cme_arbitrate_data_source =
      new HFSAT::CMEArbDataSource(dbglogger, sec_name_indexer, watch, sid_to_mov_ptr_map);
  cme_arbitrate_data_source->SetDataSourceSockets();

  std::vector<int32_t> socket_fd_vec;
  cme_arbitrate_data_source->GetDataSourceSocketsFdList(socket_fd_vec);

  for (auto socket_fd_ : socket_fd_vec) {
    simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(cme_arbitrate_data_source, socket_fd_);
  }

  simple_live_dispatcher->RunLive();

  return 0;
}
