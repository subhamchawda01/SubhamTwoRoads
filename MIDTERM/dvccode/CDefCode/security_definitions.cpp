/**
   \file CDefCode/security_definitions.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/bse_security_definition.hpp"
#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CDef/refdata_locator.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"
#include "dvccode/Utils/exchange_names.hpp"
#include "dvccode/Utils/holiday_manager.hpp"

namespace HFSAT {

SecurityDefinitions* SecurityDefinitions::p_uniqueinstance_ = NULL;
int SecurityDefinitions::tradingdate_ = 0;
ShortcodeContractSpecificationMap SecurityDefinitions::contract_specification_map_;

SecurityDefinitions::SecurityDefinitions(int t_intdate_) {
  tradingdate_ = t_intdate_;
  CurrencyConvertor::SetUniqueInstance(tradingdate_);
  _dummy_ = ContractSpecification(1, 1, kExchSourceInvalid, 1);
  contract_specification_map_["HYB_ESPY"] = ContractSpecification(25, .50, kExchSourceHYB, 1);
  contract_specification_map_["HYB_NQQQ"] = ContractSpecification(25, .20, kExchSourceHYB, 1);
  contract_specification_map_["HYB_YDIA"] = ContractSpecification(1, 5, kExchSourceHYB, 1);
  CMESecurityDefinitions(t_intdate_);
  EUREXSecurityDefinitions(t_intdate_);
  ASXSecurityDefinitions(t_intdate_);
  CFESecurityDefinitions(t_intdate_);
  TMXSecurityDefinitions(t_intdate_);
  BMFSecurityDefinitions(t_intdate_);
  SGXSecurityDefinitions(t_intdate_);
  OSESecurityDefinitions(t_intdate_);

  LIFFEISESecurityDefintions(t_intdate_);
  contract_specification_map_["usg_02Y"] = ContractSpecification(0.0078125, 10000, kExchSourceESPEED, 1);
  contract_specification_map_["usg_03Y"] = ContractSpecification(0.0078125, 10000, kExchSourceESPEED, 1);
  contract_specification_map_["usg_05Y"] = ContractSpecification(0.0078125, 10000, kExchSourceESPEED, 1);
  contract_specification_map_["usg_07Y"] = ContractSpecification(0.015625, 10000, kExchSourceESPEED, 1);
  contract_specification_map_["usg_10Y"] = ContractSpecification(0.015625, 10000, kExchSourceESPEED, 1);
  contract_specification_map_["usg_30Y"] = ContractSpecification(0.015625, 10000, kExchSourceESPEED, 1);

  BatsSecurityDefinitions(t_intdate_);
  BMFEQSecurityDefinitions(t_intdate_);

  // NASDAQ
  contract_specification_map_["FXE"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["IEF"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["TLT"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["USO"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["GLD"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["OIH"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["QQQ"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["SPY"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["DIA"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["IWM"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["EEM"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["EWZ"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["XLF"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["SDS"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["EWG"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["EWU"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["PBR"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["VALE"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["ITUB"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["GLD"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["EFA"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["BBD"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["WEAT"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["MOO"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);
  contract_specification_map_["DBA"] = ContractSpecification(0.01, 1, kExchSourceNASDAQ, 1);

  // RTS products
  contract_specification_map_["RI_0"] = ContractSpecification(
      10, 0.65 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // RTS Index future

  if (t_intdate_ > 20140727) {
    contract_specification_map_["MX_0"] = ContractSpecification(
        25, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // MICEX Index future

  } else {
    contract_specification_map_["MX_0"] = ContractSpecification(
        10, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // MICEX Index future
  }

  contract_specification_map_["Si_0"] = ContractSpecification(
      1, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // USD/RUB Futures
  contract_specification_map_["Eu_0"] = ContractSpecification(
      1, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // EUR/RUB Futures
  contract_specification_map_["ED_0"] = ContractSpecification(
      0.0001, 32401 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // EUR/RUB Futures
  contract_specification_map_["JP_0"] = ContractSpecification(
      0.01, 32401 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // USD/JPY Futures

  contract_specification_map_["GD_0"] = ContractSpecification(
      0.1, 32.401 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // GOLD Futures
  contract_specification_map_["BR_0"] = ContractSpecification(
      0.01, 324.01 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // brent oil Futures
  contract_specification_map_["BR_1"] = ContractSpecification(
      0.01, 324.01 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // brent oil Futures

  contract_specification_map_["OX_0"] = ContractSpecification(
      1, 10 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // Bond futures
  contract_specification_map_["OV_0"] = ContractSpecification(
      1, 10 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // Bond futures
  contract_specification_map_["O6_0"] = ContractSpecification(
      1, 10 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // Bond futures
  contract_specification_map_["O4_0"] = ContractSpecification(
      1, 10 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // Bond futures
  contract_specification_map_["O2_0"] = ContractSpecification(
      1, 10 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // Bond futures

  contract_specification_map_["LK_0"] = ContractSpecification(
      1, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // LUKOil Futures
  contract_specification_map_["GZ_0"] = ContractSpecification(
      1, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // Gazprom futures
  contract_specification_map_["SR_0"] = ContractSpecification(
      1, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // Sberbank futures
  contract_specification_map_["GM_0"] = ContractSpecification(
      1, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // Norilsk Nickel futures
  contract_specification_map_["RN_0"] = ContractSpecification(
      1, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // Rosneft futures
  contract_specification_map_["VB_0"] = ContractSpecification(
      1, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // VTB Bank futures
  contract_specification_map_["SN_0"] = ContractSpecification(
      1, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // Surgutneftegas furutes
  contract_specification_map_["TT_0"] = ContractSpecification(
      1, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // Tatneft futures
  contract_specification_map_["MN_0"] = ContractSpecification(
      1, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // Magnit futures
  contract_specification_map_["MT_0"] = ContractSpecification(
      1, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // MTS futures
  // Using this symbol for CME. TODO Add prefix based on exchange
  //  contract_specification_map_["TN_0"] = ContractSpecification(
  //      1, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // Transneft prefered
  //      futures

  // Novateck ticker is same as NK, so using NT_0 as shc and a hack is added to exchange_symbol_manager to get the
  // correct exchange symbol
  contract_specification_map_["NT_0"] = ContractSpecification(
      1, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceRTS, 1);  // Novatek prefered futures,

  // MICEX products
  contract_specification_map_["USD000TODTOM"] =
      ContractSpecification(0.0001, 100000 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD),
                            kExchSourceMICEX_CR, 1);  // MICEX USD/RUB
  contract_specification_map_["EURUSD000TOM"] = ContractSpecification(
      0.0001, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceMICEX_CR, 1);
  contract_specification_map_["EURUSD000TOD"] = ContractSpecification(
      0.0001, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceMICEX_CR, 1);

  if (tradingdate_ < 20150202) {
    contract_specification_map_["USD000000TOD"] =
        ContractSpecification(0.0005, 1000 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD),
                              kExchSourceMICEX_CR, 1);  // MICEX USD/RUB
    contract_specification_map_["USD000UTSTOM"] =
        ContractSpecification(0.0005, 1000 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD),
                              kExchSourceMICEX_CR, 1);  // MICEX USD/RUB

    contract_specification_map_["EUR_RUB__TOD"] =
        ContractSpecification(0.0005, 1000 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD) *
                                          CurrencyConvertor::Convert(kCurrencyUSD, kCurrencyEUR),
                              kExchSourceMICEX_CR, 1);
    contract_specification_map_["EUR_RUB__TOM"] =
        ContractSpecification(0.0005, 1000 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD) *
                                          CurrencyConvertor::Convert(kCurrencyUSD, kCurrencyEUR),
                              kExchSourceMICEX_CR, 1);
    contract_specification_map_["SPUSDRUBSPOTFX"] = ContractSpecification(
        0.0005, 1000 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceMICEX_CR, 1);
  } else if (tradingdate_ < 20160530) {
    // Tick size changed on 20150202 for USD/RUB, EUR/RUB and GBP/RUB FX spot instruments
    contract_specification_map_["USD000000TOD"] = ContractSpecification(
        0.001, 1000 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceMICEX_CR, 1);  // MICEX USD/RUB
    contract_specification_map_["USD000UTSTOM"] = ContractSpecification(
        0.001, 1000 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceMICEX_CR, 1);  // MICEX USD/RUB

    contract_specification_map_["EUR_RUB__TOD"] =
        ContractSpecification(0.001, 1000 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD) *
                                         CurrencyConvertor::Convert(kCurrencyUSD, kCurrencyEUR),
                              kExchSourceMICEX_CR, 1);
    contract_specification_map_["EUR_RUB__TOM"] =
        ContractSpecification(0.001, 1000 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD) *
                                         CurrencyConvertor::Convert(kCurrencyUSD, kCurrencyEUR),
                              kExchSourceMICEX_CR, 1);
    contract_specification_map_["SPUSDRUBSPOTFX"] = ContractSpecification(
        0.001, 1000 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceMICEX_CR, 1);
  } else {
    // Tick size changed on 20160530 for USD/RUB, EUR/RUB and GBP/RUB FX spot instruments
    contract_specification_map_["USD000000TOD"] =
        ContractSpecification(0.0025, 1000 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD),
                              kExchSourceMICEX_CR, 1);  // MICEX USD/RUB
    contract_specification_map_["USD000UTSTOM"] =
        ContractSpecification(0.0025, 1000 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD),
                              kExchSourceMICEX_CR, 1);  // MICEX USD/RUB

    contract_specification_map_["EUR_RUB__TOD"] =
        ContractSpecification(0.0025, 1000 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD) *
                                          CurrencyConvertor::Convert(kCurrencyUSD, kCurrencyEUR),
                              kExchSourceMICEX_CR, 1);
    contract_specification_map_["EUR_RUB__TOM"] =
        ContractSpecification(0.0025, 1000 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD) *
                                          CurrencyConvertor::Convert(kCurrencyUSD, kCurrencyEUR),
                              kExchSourceMICEX_CR, 1);
    contract_specification_map_["SPUSDRUBSPOTFX"] = ContractSpecification(
        0.0025, 1000 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceMICEX_CR, 1);
  }

  contract_specification_map_["SBER"] =
      ContractSpecification(0.01, 10 * 3.0 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD),
                            kExchSourceMICEX_EQ, 1);  // MICEX SBER bank
  contract_specification_map_["GMKN"] =
      ContractSpecification(1.00, 1 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceMICEX_EQ,
                            1);  // MICEX Norilsk Nickel
  contract_specification_map_["LKOH"] = ContractSpecification(
      0.1, 0.03 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceMICEX_EQ, 1);  // MICEX LUKOil
  contract_specification_map_["GAZP"] =
      ContractSpecification(0.01, 10 * 5.0 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD),
                            kExchSourceMICEX_EQ, 1);  // MICEX Gazprom
  contract_specification_map_["VTBR"] =
      ContractSpecification(0.00001, 10000 * 0.01 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD),
                            kExchSourceMICEX_EQ, 1);  // MICEX VTB bank share
  contract_specification_map_["SNGS"] =
      ContractSpecification(0.001, 10 * 0.01 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD),
                            kExchSourceMICEX_EQ, 1);  // MICEX Surgutneftegas
  contract_specification_map_["TATN"] =
      ContractSpecification(0.01, 10 * 0.01 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD),
                            kExchSourceMICEX_EQ, 1);  // MICEX Tatneft

  if (tradingdate_ < 20141201) {
    contract_specification_map_["MGNT"] =
        ContractSpecification(0.1, 0.01 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceMICEX_EQ,
                              1);  // MICEX Magnit Company stocksTB bank share
    contract_specification_map_["ROSN"] =
        ContractSpecification(0.01, 10 * 0.01 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD),
                              kExchSourceMICEX_EQ, 1);  // MICEX Rosneft
  } else {
    contract_specification_map_["MGNT"] =
        ContractSpecification(1, 0.01 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD), kExchSourceMICEX_EQ,
                              1);  // MICEX Magnit Company stocksTB bank share
    contract_specification_map_["ROSN"] =
        ContractSpecification(0.05, 10 * 0.01 * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD),
                              kExchSourceMICEX_EQ, 1);  // MICEX Rosneft
  }

  // HKEX
  contract_specification_map_["HHI_0"] =
      ContractSpecification(1, 50 * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD), kExchSourceHONGKONG, 1);
  contract_specification_map_["HHI_1"] =
      ContractSpecification(1, 50 * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD), kExchSourceHONGKONG, 1);
  contract_specification_map_["HHI_2"] =
      ContractSpecification(1, 50 * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD), kExchSourceHONGKONG, 1);
  contract_specification_map_["HHI_3"] =
      ContractSpecification(1, 50 * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD), kExchSourceHONGKONG, 1);
  contract_specification_map_["HSI_0"] =
      ContractSpecification(1, 50 * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD), kExchSourceHONGKONG, 1);
  contract_specification_map_["HSI_1"] =
      ContractSpecification(1, 50 * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD), kExchSourceHONGKONG, 1);
  contract_specification_map_["HSI_2"] =
      ContractSpecification(1, 50 * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD), kExchSourceHONGKONG, 1);
  contract_specification_map_["HSI_3"] =
      ContractSpecification(1, 50 * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD), kExchSourceHONGKONG, 1);

  contract_specification_map_["MCH_0"] =
      ContractSpecification(1, 10 * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD), kExchSourceHONGKONG, 1);
  contract_specification_map_["MCH_1"] =
      ContractSpecification(1, 10 * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD), kExchSourceHONGKONG, 1);
  contract_specification_map_["MHI_0"] =
      ContractSpecification(1, 10 * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD), kExchSourceHONGKONG, 1);
  contract_specification_map_["MHI_1"] =
      ContractSpecification(1, 10 * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD), kExchSourceHONGKONG, 1);

  // EBS
  contract_specification_map_["USD/RUB"] = ContractSpecification(0.0005, 1000, kExchSourceEBS, 1000000);  // EBS USD/RUB
  contract_specification_map_["USD/JPY"] = ContractSpecification(0.005, 1000, kExchSourceEBS, 1000000);  //  EBS USD/JPY
  contract_specification_map_["EUR/USD"] =
      ContractSpecification(0.00005, 1000, kExchSourceEBS, 1000000);  //  EBS EUR/USD
  contract_specification_map_["EUR/GBP"] =
      ContractSpecification(0.00005, 1000, kExchSourceEBS, 1000000);  //  EBS EUR/GBP
  contract_specification_map_["EUR/RUB"] =
      ContractSpecification(0.0005, 1000, kExchSourceEBS, 1000000);                                      //  EBS EUR/RUB
  contract_specification_map_["EUR/NOK"] = ContractSpecification(0.001, 1000, kExchSourceEBS, 1000000);  //  EBS EUR/NOK
  contract_specification_map_["EUR/CHF"] =
      ContractSpecification(0.00005, 1000, kExchSourceEBS, 1000000);  //  EBS EUR/CHF
  contract_specification_map_["EUR/CAD"] =
      ContractSpecification(0.0001, 1000, kExchSourceEBS, 1000000);  //  EBS EUR/CAD
  contract_specification_map_["EUR/AUD"] =
      ContractSpecification(0.00005, 1000, kExchSourceEBS, 1000000);  //  EBS EUR/AUD
  contract_specification_map_["BKT/RUB"] =
      ContractSpecification(0.0005, 1000, kExchSourceEBS, 1000000);  //  EBS BKT/RUB

  // Combined Securities

  contract_specification_map_["C_BOVESPA"] = ContractSpecification(1, 1, kExchSourceJPY, 1);

  // TODO: KRX Securities Add currency and check min price inc
  contract_specification_map_["KOSPI_0"] =
      ContractSpecification(0.05, 500000 * CurrencyConvertor::Convert(kCurrencyKRW, kCurrencyUSD), kExchSourceKRX, 1);
  contract_specification_map_["KOSPI_1"] =
      ContractSpecification(0.05, 500000 * CurrencyConvertor::Convert(kCurrencyKRW, kCurrencyUSD), kExchSourceKRX, 1);
  contract_specification_map_["KOSPI_2"] =
      ContractSpecification(0.05, 500000 * CurrencyConvertor::Convert(kCurrencyKRW, kCurrencyUSD), kExchSourceKRX, 1);
  contract_specification_map_["KOSPI_3"] =
      ContractSpecification(0.05, 500000 * CurrencyConvertor::Convert(kCurrencyKRW, kCurrencyUSD), kExchSourceKRX, 1);

  GetRTSContractSpecifications();
  GetNonSelfEnabledSecurities();

  if (tradingdate_ > 20150802) {
    ResetDI1MPI();
  }
}

void SecurityDefinitions::CMESecurityDefinitions(int t_intdate_) {
  /* CME Combined Securities*/
  // Interest Rate
  contract_specification_map_["C_GE"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_GE_SP"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_ZT"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_ZF"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_ZN"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_ZB"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_UB"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_TN"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  // Index Futures
  contract_specification_map_["C_ES"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_NQ"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_YM"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  // Currencies
  contract_specification_map_["C_6A"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_6B"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_6C"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_6E"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_6M"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_6N"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_6J"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_6R"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_6Z"] = ContractSpecification(1, 1, kExchSourceCME, 1);

  // Energy
  contract_specification_map_["C_CL"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_BZ"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_RB"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_NG"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_HO"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_QM"] = ContractSpecification(1, 1, kExchSourceCME, 1);

  // Commodities
  contract_specification_map_["C_GC"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_SI"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_HG"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_ZC"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_ZW"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_KE"] = ContractSpecification(1, 1, kExchSourceCME, 1);
  contract_specification_map_["C_ZS"] = ContractSpecification(1, 1, kExchSourceCME, 1);

  // OUTRIGHT YIELDS
  contract_specification_map_["GEY_0"] = ContractSpecification(0.25, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_1"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_2"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_3"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_4"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_5"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_6"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_7"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_8"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_9"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_10"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_11"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_12"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_13"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_14"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_15"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_16"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_17"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_18"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_19"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_20"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_21"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_22"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_23"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_24"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_25"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_26"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_27"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_28"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_29"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_30"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_31"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_32"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_33"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_34"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_35"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_36"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_37"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_38"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GEY_39"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);

  /* OUTRIGHTS */
  contract_specification_map_["GE_0"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);  // TODO 0.25 for front month
  contract_specification_map_["GE_1"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_2"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_3"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_4"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_5"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_6"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_7"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_8"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_9"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_10"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_11"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_12"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_13"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_14"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_15"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_16"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_17"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_18"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_19"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_20"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_21"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_22"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_23"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_24"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_25"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_26"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_27"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_28"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_29"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_30"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_31"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_32"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_33"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_34"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_35"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_36"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_37"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_38"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["GE_39"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);

  contract_specification_map_["SP_GE0_GE1"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE1_GE2"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);  // GE spreads
  contract_specification_map_["SP_GE2_GE3"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE3_GE4"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE4_GE5"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE5_GE6"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE6_GE7"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE7_GE8"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE8_GE9"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE9_GE10"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE10_GE11"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE11_GE12"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE12_GE13"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE13_GE14"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE14_GE15"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE15_GE16"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE16_GE17"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE17_GE18"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE18_GE19"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE19_GE20"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE20_GE21"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE21_GE22"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE22_GE23"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);
  contract_specification_map_["SP_GE23_GE24"] = ContractSpecification(0.5, 25, kExchSourceCME, 1);

  contract_specification_map_["ZTY_0"] = ContractSpecification(0.0078125, 2000, kExchSourceCME, 1);
  contract_specification_map_["ZTY_1"] = ContractSpecification(0.0078125, 2000, kExchSourceCME, 1);
  contract_specification_map_["ZFY_0"] = ContractSpecification(0.0078125, 1000, kExchSourceCME, 1);
  contract_specification_map_["ZFY_1"] = ContractSpecification(0.0078125, 1000, kExchSourceCME, 1);
  contract_specification_map_["ZNY_0"] = ContractSpecification(0.015625, 1000, kExchSourceCME, 1);
  contract_specification_map_["ZNY_1"] = ContractSpecification(0.015625, 1000, kExchSourceCME, 1);
  contract_specification_map_["ZBY_0"] = ContractSpecification(0.031250, 1000, kExchSourceCME, 1);
  contract_specification_map_["ZBY_1"] = ContractSpecification(0.031250, 1000, kExchSourceCME, 1);
  contract_specification_map_["UBY_0"] = ContractSpecification(0.031250, 1000, kExchSourceCME, 1);
  contract_specification_map_["UBY_1"] = ContractSpecification(0.031250, 1000, kExchSourceCME, 1);
  contract_specification_map_["ZT_0"] = ContractSpecification(0.0078125, 2000, kExchSourceCME, 1);
  contract_specification_map_["ZT_1"] = ContractSpecification(0.0078125, 2000, kExchSourceCME, 1);
  contract_specification_map_["ZF_0"] = ContractSpecification(0.0078125, 1000, kExchSourceCME, 1);
  contract_specification_map_["ZF_1"] = ContractSpecification(0.0078125, 1000, kExchSourceCME, 1);
  contract_specification_map_["ZN_0"] = ContractSpecification(0.015625, 1000, kExchSourceCME, 1);
  contract_specification_map_["ZN_1"] = ContractSpecification(0.015625, 1000, kExchSourceCME, 1);
  contract_specification_map_["TN_0"] = ContractSpecification(0.015625, 1000, kExchSourceCME, 1);
  contract_specification_map_["TN_1"] = ContractSpecification(0.015625, 1000, kExchSourceCME, 1);
  contract_specification_map_["ZB_0"] = ContractSpecification(0.031250, 1000, kExchSourceCME, 1);
  contract_specification_map_["ZB_1"] = ContractSpecification(0.031250, 1000, kExchSourceCME, 1);
  contract_specification_map_["UB_0"] = ContractSpecification(0.031250, 1000, kExchSourceCME, 1);
  contract_specification_map_["UB_1"] = ContractSpecification(0.031250, 1000, kExchSourceCME, 1);
  contract_specification_map_["ES_0"] = ContractSpecification(25, .50, kExchSourceCME, 1);
  contract_specification_map_["ES_1"] = ContractSpecification(25, .50, kExchSourceCME, 1);
  contract_specification_map_["NQ_0"] = ContractSpecification(25, .20, kExchSourceCME, 1);
  contract_specification_map_["NQ_1"] = ContractSpecification(25, .20, kExchSourceCME, 1);
  contract_specification_map_["YM_0"] = ContractSpecification(1, 5, kExchSourceCME, 1);
  contract_specification_map_["YM_1"] = ContractSpecification(1, 5, kExchSourceCME, 1);
  contract_specification_map_["EMD_0"] = ContractSpecification(0.1, 100, kExchSourceCME, 1);

  contract_specification_map_["6A_0"] = ContractSpecification(1.0, 10.0, kExchSourceCME, 1);
  contract_specification_map_["6A_1"] = ContractSpecification(1.0, 10.0, kExchSourceCME, 1);
  contract_specification_map_["6B_0"] = ContractSpecification(1.0, 6.25, kExchSourceCME, 1);
  contract_specification_map_["6B_1"] = ContractSpecification(1.0, 6.25, kExchSourceCME, 1);

  if (t_intdate_ > SIXC_TICKCHANGE_DATE) {
    contract_specification_map_["6C_0"] = ContractSpecification(0.5, 10.0, kExchSourceCME, 1);
    contract_specification_map_["6C_1"] = ContractSpecification(0.5, 10.0, kExchSourceCME, 1);
  } else {
    contract_specification_map_["6C_0"] = ContractSpecification(1.0, 10.0, kExchSourceCME, 1);
    contract_specification_map_["6C_1"] = ContractSpecification(1.0, 10.0, kExchSourceCME, 1);
  }

  if (t_intdate_ > SIXJ_TICKCHANGE_DATE) {
    contract_specification_map_["6J_0"] = ContractSpecification(0.5, 12.5, kExchSourceCME, 1);
    contract_specification_map_["6J_1"] = ContractSpecification(0.5, 12.5, kExchSourceCME, 1);
  } else {
    contract_specification_map_["6J_0"] = ContractSpecification(1.0, 12.5, kExchSourceCME, 1);
    contract_specification_map_["6J_1"] = ContractSpecification(1.0, 12.5, kExchSourceCME, 1);
  }

  if (t_intdate_ > SIXE_TICKCHANGE_DATE) {
    contract_specification_map_["6E_0"] = ContractSpecification(0.5, 12.5, kExchSourceCME, 1);
    contract_specification_map_["6E_1"] = ContractSpecification(0.5, 12.5, kExchSourceCME, 1);
  } else {
    contract_specification_map_["6E_0"] = ContractSpecification(1.0, 12.5, kExchSourceCME, 1);
    contract_specification_map_["6E_1"] = ContractSpecification(1.0, 12.5, kExchSourceCME, 1);
  }

  if (t_intdate_ > 20140713) {
    contract_specification_map_["6M_0"] =
        ContractSpecification(10.0, 0.5, kExchSourceCME, 1);  // KP : value is 0.5 because CME send 5 digits price
    contract_specification_map_["6M_1"] = ContractSpecification(10.0, 0.5, kExchSourceCME, 1);

    contract_specification_map_["6R_0"] =
        ContractSpecification(5.0, 12.50, kExchSourceCME, 1);  // KP : this has to verfied once we have raw data
    contract_specification_map_["6R_1"] = ContractSpecification(5.0, 12.50, kExchSourceCME, 1);
  } else {
    contract_specification_map_["6M_0"] =
        ContractSpecification(25.0, 0.5, kExchSourceCME, 1);  // KP : value is 0.5 because CME send 5 digits price
    contract_specification_map_["6M_1"] = ContractSpecification(25.0, 0.5, kExchSourceCME, 1);

    contract_specification_map_["6R_0"] =
        ContractSpecification(10.0, 12.50, kExchSourceCME, 1);  // KP : this has to verfied once we have raw data
    contract_specification_map_["6R_1"] = ContractSpecification(10.0, 12.50, kExchSourceCME, 1);
  }

  contract_specification_map_["6N_0"] = ContractSpecification(1.0, 10, kExchSourceCME, 1);
  contract_specification_map_["6N_1"] = ContractSpecification(1.0, 10, kExchSourceCME, 1);
  contract_specification_map_["6S_0"] = ContractSpecification(1.0, 12.5, kExchSourceCME, 1);
  contract_specification_map_["6S_1"] = ContractSpecification(1.0, 12.5, kExchSourceCME, 1);
  contract_specification_map_["SEK_0"] = ContractSpecification(1.0, 20, kExchSourceCME, 1);
  contract_specification_map_["SEK_1"] = ContractSpecification(1.0, 20, kExchSourceCME, 1);

  // Brazilian Real
  contract_specification_map_["6L_0"] = ContractSpecification(0.00005, 100000, kExchSourceCME, 1);
  contract_specification_map_["6L_1"] = ContractSpecification(0.00005, 100000, kExchSourceCME, 1);

  // Indian rupee
  contract_specification_map_["SIR_0"] = ContractSpecification(0.01, 5000000, kExchSourceCME, 1);
  contract_specification_map_["SIR_1"] = ContractSpecification(0.01, 5000000, kExchSourceCME, 1);

  // South African rand
  contract_specification_map_["6Z_0"] = ContractSpecification(0.000025, 5000000, kExchSourceCME, 1);
  contract_specification_map_["6Z_1"] = ContractSpecification(0.000025, 5000000, kExchSourceCME, 1);

  //    contract_specification_map_ [ "NG_0" ] = ContractSpecification ( 0.001, 10000, kExchSourceCME, 1 );

  //    contract_specification_map_ [ "RB_0" ] = ContractSpecification ( 0.0001, 42000, kExchSourceCME, 1 );
  //    contract_specification_map_ [ "HG_0" ] = ContractSpecification ( 0.0005, 25000, kExchSourceCME, 1 );
  //    contract_specification_map_ [ "SI_0" ] = ContractSpecification ( 0.005, 5000, kExchSourceCME, 1 );
  //    contract_specification_map_ [ "PA_0" ] = ContractSpecification ( 0.05, 100, kExchSourceCME, 1 );
  //    contract_specification_map_ [ "PD_0" ] = ContractSpecification ( 0.10, 50, kExchSourceCME, 1 );

  // Gold GC
  contract_specification_map_["GC_0"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["GC_1"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["GC_2"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  // Silver SI
  contract_specification_map_["SI_0"] = ContractSpecification(5, 5, kExchSourceCME, 1);
  contract_specification_map_["SI_1"] = ContractSpecification(5, 5, kExchSourceCME, 1);
  contract_specification_map_["SI_2"] = ContractSpecification(5, 5, kExchSourceCME, 1);
  // Copper HG
  contract_specification_map_["HG_0"] = ContractSpecification(5, 2.5, kExchSourceCME, 1);
  contract_specification_map_["HG_1"] = ContractSpecification(5, 2.5, kExchSourceCME, 1);
  contract_specification_map_["HG_2"] = ContractSpecification(5, 2.5, kExchSourceCME, 1);

  // OIL CL @ Size = 1000 , MinFluct = $0.01
  contract_specification_map_["CL_0"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["CL_1"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["CL_2"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["CL_3"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["CL_4"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["CL_5"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["CL_6"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["CL_7"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["CL_8"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["CL_9"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["CL_10"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["CL_11"] = ContractSpecification(1, 10, kExchSourceCME, 1);

  contract_specification_map_["SP_CL0_CL1"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_CL1_CL2"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_CL2_CL3"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_CL3_CL4"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_CL4_CL5"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_CL5_CL6"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_CL6_CL7"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_CL7_CL8"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_CL8_CL9"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_CL9_CL10"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_CL10_CL11"] = ContractSpecification(1, 10, kExchSourceCME, 1);

  // E-Mini CrudeOil
  contract_specification_map_["QM_0"] = ContractSpecification(25, 0.5, kExchSourceCME, 1);
  contract_specification_map_["QM_1"] = ContractSpecification(25, 0.5, kExchSourceCME, 1);
  contract_specification_map_["QM_2"] = ContractSpecification(25, 0.5, kExchSourceCME, 1);

  // Heating oil
  contract_specification_map_["HO_0"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["HO_1"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["HO_2"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["HO_3"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["HO_4"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["HO_5"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["HO_6"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["HO_7"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["HO_8"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["HO_9"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["HO_10"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["HO_11"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);

  contract_specification_map_["SP_HO0_HO1"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_HO1_HO2"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_HO2_HO3"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_HO3_HO4"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_HO4_HO5"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_HO5_HO6"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_HO6_HO7"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_HO7_HO8"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_HO8_HO9"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_HO9_HO10"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_HO10_HO11"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);

  // Henry hub natural gas
  contract_specification_map_["NN_0"] = ContractSpecification(1, 2.5, kExchSourceCME, 1);
  contract_specification_map_["NN_1"] = ContractSpecification(1, 2.5, kExchSourceCME, 1);
  // Natural Gas NG
  contract_specification_map_["NG_0"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["NG_1"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["NG_2"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["NG_3"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["NG_4"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["NG_5"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["NG_6"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["NG_7"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["NG_8"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["NG_9"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["NG_10"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["NG_11"] = ContractSpecification(1, 10, kExchSourceCME, 1);

  contract_specification_map_["SP_NG0_NG1"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_NG1_NG2"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_NG2_NG3"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_NG3_NG4"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_NG4_NG5"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_NG5_NG6"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_NG6_NG7"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_NG7_NG8"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_NG8_NG9"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_NG9_NG10"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["SP_NG10_NG11"] = ContractSpecification(1, 10, kExchSourceCME, 1);

  // Gasoline RB
  contract_specification_map_["RB_0"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["RB_1"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["RB_2"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["RB_3"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["RB_4"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["RB_5"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["RB_6"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["RB_7"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["RB_8"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["RB_9"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["RB_10"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["RB_11"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);

  contract_specification_map_["SP_RB0_RB1"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_RB1_RB2"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_RB2_RB3"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_RB3_RB4"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_RB4_RB5"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_RB5_RB6"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_RB6_RB7"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_RB7_RB8"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_RB8_RB9"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_RB9_RB10"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);
  contract_specification_map_["SP_RB10_RB11"] = ContractSpecification(1, 4.2, kExchSourceCME, 1);

  // Brent BZ
  contract_specification_map_["BZ_0"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["BZ_1"] = ContractSpecification(1, 10, kExchSourceCME, 1);
  contract_specification_map_["BZ_2"] = ContractSpecification(1, 10, kExchSourceCME, 1);

  contract_specification_map_["IBV_0"] = ContractSpecification(25, 1, kExchSourceCME, 1);

  contract_specification_map_["ZW_0"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);  // chicago wheat
  contract_specification_map_["ZW_1"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["ZW_2"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["ZW_3"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["ZW_4"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["ZW_5"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);

  contract_specification_map_["KE_0"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["KE_1"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["KE_2"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["KE_3"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["KE_4"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);

  contract_specification_map_["ZC_0"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);  // corn
  contract_specification_map_["ZC_1"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["ZC_2"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["ZC_3"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["ZC_4"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["ZC_5"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);

  contract_specification_map_["ZS_0"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);  // Soybean
  contract_specification_map_["ZS_1"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["ZS_2"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["ZS_3"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["ZS_4"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["ZS_5"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["ZS_6"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);
  contract_specification_map_["ZS_7"] = ContractSpecification(0.25, 50, kExchSourceCME, 1);

  contract_specification_map_["ZL_0"] = ContractSpecification(0.01, 600, kExchSourceCME, 1);  // soybean oil
  contract_specification_map_["ZL_1"] = ContractSpecification(0.01, 600, kExchSourceCME, 1);
  contract_specification_map_["ZL_2"] = ContractSpecification(0.01, 600, kExchSourceCME, 1);
  contract_specification_map_["ZL_3"] = ContractSpecification(0.01, 600, kExchSourceCME, 1);
  contract_specification_map_["ZL_4"] = ContractSpecification(0.01, 600, kExchSourceCME, 1);
  contract_specification_map_["ZL_5"] = ContractSpecification(0.01, 600, kExchSourceCME, 1);
  contract_specification_map_["ZL_6"] = ContractSpecification(0.01, 600, kExchSourceCME, 1);
  contract_specification_map_["ZL_7"] = ContractSpecification(0.01, 600, kExchSourceCME, 1);

  contract_specification_map_["ZM_0"] = ContractSpecification(0.1, 100, kExchSourceCME, 1);  // soybean meal
  contract_specification_map_["ZM_1"] = ContractSpecification(0.1, 100, kExchSourceCME, 1);
  contract_specification_map_["ZM_2"] = ContractSpecification(0.1, 100, kExchSourceCME, 1);
  contract_specification_map_["ZM_3"] = ContractSpecification(0.1, 100, kExchSourceCME, 1);
  contract_specification_map_["ZM_4"] = ContractSpecification(0.1, 100, kExchSourceCME, 1);
  contract_specification_map_["ZM_5"] = ContractSpecification(0.1, 100, kExchSourceCME, 1);
  contract_specification_map_["ZM_6"] = ContractSpecification(0.1, 100, kExchSourceCME, 1);
  contract_specification_map_["ZM_7"] = ContractSpecification(0.1, 100, kExchSourceCME, 1);

  contract_specification_map_["XW_0"] = ContractSpecification(0.125, 1, kExchSourceCME, 1);

  // nikkei 225 ( USD & YEN )
  contract_specification_map_["NKD_0"] = ContractSpecification(5, 5, kExchSourceCME, 1);
  contract_specification_map_["NKD_1"] = ContractSpecification(5, 5, kExchSourceCME, 1);
  contract_specification_map_["NIY_0"] =
      ContractSpecification(5, 500 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceCME, 1);
  contract_specification_map_["NIY_1"] =
      ContractSpecification(5, 500 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceCME, 1);
}

void SecurityDefinitions::EUREXSecurityDefinitions(int t_intdate_) {
  /*Combined */

  // FI
  contract_specification_map_["C_FGBS"] = ContractSpecification(1, 1, kExchSourceEUREX, 1);
  contract_specification_map_["C_FGBM"] = ContractSpecification(1, 1, kExchSourceEUREX, 1);
  contract_specification_map_["C_FGBL"] = ContractSpecification(1, 1, kExchSourceEUREX, 1);
  contract_specification_map_["C_FGBX"] = ContractSpecification(1, 1, kExchSourceEUREX, 1);
  contract_specification_map_["C_FOAT"] = ContractSpecification(1, 1, kExchSourceEUREX, 1);
  contract_specification_map_["C_FBTP"] = ContractSpecification(1, 1, kExchSourceEUREX, 1);
  contract_specification_map_["C_FBTS"] = ContractSpecification(1, 1, kExchSourceEUREX, 1);
  contract_specification_map_["C_FEU3"] = ContractSpecification(1, 1, kExchSourceEUREX, 1);

  // Equity
  contract_specification_map_["C_FESX"] = ContractSpecification(1, 1, kExchSourceEUREX, 1);
  contract_specification_map_["C_FDAX"] = ContractSpecification(1, 1, kExchSourceEUREX, 1);
  contract_specification_map_["C_FXXP"] = ContractSpecification(1, 1, kExchSourceEUREX, 1);
  contract_specification_map_["C_FDXM"] = ContractSpecification(1, 1, kExchSourceEUREX, 1);

  // VOl
  contract_specification_map_["C_FVS"] = ContractSpecification(1, 1, kExchSourceEUREX, 1);
  contract_specification_map_["C_FVS_SP"] = ContractSpecification(1, 1, kExchSourceEUREX, 1);

  /* EUREX */
  contract_specification_map_["FGBSY_0"] =
      ContractSpecification(0.005, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FGBSY_1"] =
      ContractSpecification(0.005, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FGBMY_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FGBMY_1"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FGBMY_2"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FGBLY_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FGBLY_1"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FGBXY_0"] =
      ContractSpecification(0.02, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FBTSY_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FBTPY_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FBTMY_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FOATY_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FOAMY_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FGBS_0"] =
      ContractSpecification(0.005, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FGBS_1"] =
      ContractSpecification(0.005, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FGBM_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FGBM_1"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FGBM_2"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FGBL_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FGBL_1"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FGBX_0"] =
      ContractSpecification(0.02, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FGBX_1"] =
      ContractSpecification(0.02, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FBTS_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FBTS_1"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FBTP_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FBTP_1"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FBTM_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FOAT_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FOAT_1"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FBON_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FOAM_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["CONF_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FESX_0"] =
      ContractSpecification(1, 10 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FESX_1"] =
      ContractSpecification(1, 10 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  // dollar denominated euro stox50
  contract_specification_map_["FESQ_0"] = ContractSpecification(1, 10, kExchSourceEUREX, 1);

  contract_specification_map_["FDAX_0"] =
      ContractSpecification(0.5, 25 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FDAX_1"] =
      ContractSpecification(0.5, 25 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FDXM_0"] =
      ContractSpecification(1, 5 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FDXM_1"] =
      ContractSpecification(1, 5 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FSMI_0"] =
      ContractSpecification(1, 10 * CurrencyConvertor::Convert(kCurrencyCHF, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FSMI_1"] =
      ContractSpecification(1, 10 * CurrencyConvertor::Convert(kCurrencyCHF, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FXXP_0"] =
      ContractSpecification(0.1, 50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FXXP_1"] =
      ContractSpecification(0.1, 50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FEXF_0"] =
      ContractSpecification(0.5, 10 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FESB_0"] =
      ContractSpecification(0.1, 50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FESB_1"] =
      ContractSpecification(0.1, 50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FSTB_0"] =
      ContractSpecification(0.1, 50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FSTS_0"] =
      ContractSpecification(0.1, 50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FSTO_0"] =
      ContractSpecification(0.1, 50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FSTG_0"] =
      ContractSpecification(0.1, 50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FSTI_0"] =
      ContractSpecification(0.1, 50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FSTM_0"] =
      ContractSpecification(0.1, 50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["F2MX_0"] =
      ContractSpecification(1, 5 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  //    contract_specification_map_ [ "OKS2_0" ] = ContractSpecification ( 0.01, 500000 * CurrencyConvertor::Convert (
  //    kCurrencyEUR, kCurrencyUSD ), kExchSourceEUREX, 1 );
  contract_specification_map_["FEXD_0"] =
      ContractSpecification(0.1, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FRDX_0"] =
      ContractSpecification(0.5, 10 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FEU3_0"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_1"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_2"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_3"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FEU3_4"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_5"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_6"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_7"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FEU3_8"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_9"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_10"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_11"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FEU3_12"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_13"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_14"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_15"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FEU3_16"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_17"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_18"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_19"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["FEU3_20"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_21"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_22"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FEU3_23"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  // TODO check if there are any assumptions about the shortcode length for eurex products,
  contract_specification_map_["FVS_0"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FVS_1"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FVS_2"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FVS_3"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FVS_4"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FVS_5"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FVS_6"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FVS_7"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["SP_FVS0_FVS1"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS0_FVS2"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS0_FVS3"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS0_FVS4"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["SP_FVS1_FVS2"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS1_FVS3"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS1_FVS4"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS1_FVS5"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["SP_FVS2_FVS3"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS2_FVS4"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS2_FVS5"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS2_FVS6"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["SP_FVS3_FVS4"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS3_FVS5"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS3_FVS6"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS3_FVS7"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  contract_specification_map_["SP_FVS4_FVS5"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS4_FVS6"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS4_FVS7"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS5_FVS6"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS5_FVS7"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["SP_FVS6_FVS7"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceEUREX, 1);

  // SOS : EUREX FX FUTURES
  // notional : AAA/BBB -> ( 100000 AAA )
  // settlement : physical
  // min_px : 0.00005 ( == 5 AAA )
  // contrats : 3 serial, 3 quarterly, 4 semi-annual
  // expiry : 3rd Wed, if holiday, preceding bday
  // trading hrs : CET_800-CET_2200 ( except ltd )
  // FCEU - EUR/USD;
  contract_specification_map_["FCEU_0"] = ContractSpecification(0.00005, 100000, kExchSourceEUREX, 1);
  contract_specification_map_["FCEU_1"] = ContractSpecification(0.00005, 100000, kExchSourceEUREX, 1);
  contract_specification_map_["FCEU_2"] = ContractSpecification(0.00005, 100000, kExchSourceEUREX, 1);
  // FCPU - GBP/USD;
  contract_specification_map_["FCPU_0"] = ContractSpecification(0.00005, 100000, kExchSourceEUREX, 1);
  contract_specification_map_["FCPU_1"] = ContractSpecification(0.00005, 100000, kExchSourceEUREX, 1);
  contract_specification_map_["FCPU_2"] = ContractSpecification(0.00005, 100000, kExchSourceEUREX, 1);
  // FCUF - USD/CHF;
  contract_specification_map_["FCUF_0"] = ContractSpecification(
      0.00005, 100000 * CurrencyConvertor::Convert(kCurrencyCHF, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FCUF_1"] = ContractSpecification(
      0.00005, 100000 * CurrencyConvertor::Convert(kCurrencyCHF, kCurrencyUSD), kExchSourceEUREX, 1);
  contract_specification_map_["FCUF_2"] = ContractSpecification(
      0.00005, 100000 * CurrencyConvertor::Convert(kCurrencyCHF, kCurrencyUSD), kExchSourceEUREX, 1);
  // EOS : EUREX FX FUTURES
}

void SecurityDefinitions::ASXSecurityDefinitions(int t_intdate_) {
  // Combined
  contract_specification_map_["C_IR"] = ContractSpecification(1, 1, kExchSourceASX, 1);

  // ASX
  contract_specification_map_["AP_0"] =
      ContractSpecification(1, 25 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["AP_1"] =
      ContractSpecification(1, 25 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["XT_0"] =
      ContractSpecification(0.005, 9400 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["XT_1"] =
      ContractSpecification(0.005, 9400 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  // To do : Reset N2D of XX based on its yield value
  contract_specification_map_["XX_0"] =
        ContractSpecification(0.005, 9400 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
    contract_specification_map_["XX_1"] =
        ContractSpecification(0.005, 9400 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["XTY_0"] =
      ContractSpecification(0.005, 9400 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["XTY_1"] =
      ContractSpecification(0.005, 9400 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  if (t_intdate_ >= YT_TICKCHANGE_DATE) {
    contract_specification_map_["YT_0"] =
        ContractSpecification(0.005, 3000 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
    contract_specification_map_["YTY_0"] =
        ContractSpecification(0.005, 3000 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
    contract_specification_map_["YTY_1"] =
        ContractSpecification(0.005, 3000 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
    contract_specification_map_["YT_1"] =
        ContractSpecification(0.005, 3000 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  } else {
    contract_specification_map_["YT_0"] =
        ContractSpecification(0.01, 3000 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
    contract_specification_map_["YTY_0"] =
        ContractSpecification(0.01, 3000 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
    contract_specification_map_["YTY_1"] =
        ContractSpecification(0.01, 3000 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
    contract_specification_map_["YT_1"] =
        ContractSpecification(0.01, 3000 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  }

  contract_specification_map_["IR_0"] =
      ContractSpecification(0.01, 2400 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["IR_1"] =
      ContractSpecification(0.01, 2400 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["IR_2"] =
      ContractSpecification(0.01, 2400 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["IR_3"] =
      ContractSpecification(0.01, 2400 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["IR_4"] =
      ContractSpecification(0.01, 2400 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["IB_0"] =
      ContractSpecification(0.005, 2466 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["IB_1"] =
      ContractSpecification(0.005, 2466 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["IB_2"] =
      ContractSpecification(0.005, 2466 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["IB_3"] =
      ContractSpecification(0.005, 2466 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["IB_4"] =
      ContractSpecification(0.005, 2466 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);

  // ASX spreads (TODO: Add correct values)
  contract_specification_map_["SP_XT0_YT0"] =
      ContractSpecification(0.005, 9400 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);

  // ASX Expiry Shortcodes
  contract_specification_map_["XTE_0"] =
      ContractSpecification(0.0025, 9400 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["XTE_1"] =
      ContractSpecification(0.0025, 9400 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["YTE_0"] =
      ContractSpecification(0.005, 3000 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
  contract_specification_map_["YTE_1"] =
      ContractSpecification(0.005, 3000 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD), kExchSourceASX, 1);
}

void SecurityDefinitions::CFESecurityDefinitions(int t_intdate_) {
  // Combined
  contract_specification_map_["C_VX"] = ContractSpecification(1, 1, kExchSourceCFE, 1);
  contract_specification_map_["C_VX_SP"] = ContractSpecification(1, 1, kExchSourceCFE, 1);

  // CFE
  contract_specification_map_["VXE_0"] = ContractSpecification(0.05, 1000, kExchSourceCFE, 1);
  contract_specification_map_["VX_0"] = ContractSpecification(0.05, 1000, kExchSourceCFE, 1);
  contract_specification_map_["VX_1"] = ContractSpecification(0.05, 1000, kExchSourceCFE, 1);
  contract_specification_map_["VX_2"] = ContractSpecification(0.05, 1000, kExchSourceCFE, 1);
  contract_specification_map_["VX_3"] = ContractSpecification(0.05, 1000, kExchSourceCFE, 1);
  contract_specification_map_["VX_4"] = ContractSpecification(0.05, 1000, kExchSourceCFE, 1);
  contract_specification_map_["VX_5"] = ContractSpecification(0.05, 1000, kExchSourceCFE, 1);
  contract_specification_map_["VX_6"] = ContractSpecification(0.05, 1000, kExchSourceCFE, 1);
  contract_specification_map_["VX_7"] = ContractSpecification(0.05, 1000, kExchSourceCFE, 1);

  // weekly
  contract_specification_map_["VXWEEK_0"] = ContractSpecification(0.05, 1000, kExchSourceCFE, 1);
  contract_specification_map_["VXWEEK_1"] = ContractSpecification(0.05, 1000, kExchSourceCFE, 1);

  // Spreads
  contract_specification_map_["SP_VXE0_VXE1"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX0_VX1"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX1_VX2"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX2_VX3"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX0_VX2"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX0_VX3"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX1_VX3"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX0_VX4"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX1_VX4"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX0_VX5"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX3_VX4"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX1_VX5"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX2_VX4"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX2_VX5"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX4_VX5"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX0_VX6"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX3_VX5"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX1_VX6"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX2_VX6"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX5_VX6"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX3_VX6"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX4_VX6"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX7_VX0"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX7_VX3"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX7_VX1"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX7_VX4"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX7_VX5"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX7_VX2"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);
  contract_specification_map_["SP_VX7_VX6"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);

  // Butterfly
  contract_specification_map_["SP_VX0_VX1_VX2"] = ContractSpecification(0.01, 1000, kExchSourceCFE, 1);

  contract_specification_map_["VSW1_0"] =
      ContractSpecification(0.05, 1000, kExchSourceCFE, 1);  // short term volatility index
  contract_specification_map_["VSW2_0"] =
      ContractSpecification(0.05, 1000, kExchSourceCFE, 1);  // short term volatility index
  contract_specification_map_["VSW3_0"] =
      ContractSpecification(0.05, 1000, kExchSourceCFE, 1);  // short term volatility index
  contract_specification_map_["VSW4_0"] =
      ContractSpecification(0.05, 1000, kExchSourceCFE, 1);  // short term volatility index
}

void SecurityDefinitions::TMXSecurityDefinitions(int t_intdate_) {
  // Combined

  contract_specification_map_["C_BAX"] = ContractSpecification(1, 1, kExchSourceTMX, 1);
  contract_specification_map_["C_BAX_SP"] = ContractSpecification(1, 1, kExchSourceTMX, 1);

  /* TMX */
  contract_specification_map_["CGBY_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["CGBY_1"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["CGB_0"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["CGB_1"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SXF_0"] =
      ContractSpecification(0.10, 200 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["EMF_0"] =
      ContractSpecification(0.05, 100 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["BAX_0"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

  if (t_intdate_ >= 20140908) {
    contract_specification_map_["BAX_1"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["BAX_2"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["BAX_3"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  } else {
    contract_specification_map_["BAX_1"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["BAX_2"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["BAX_3"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  }
  contract_specification_map_["BAX_4"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["BAX_5"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["BAX_6"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["BAX_7"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["BAX_8"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["BAX_9"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

  contract_specification_map_["CGF_0"] = ContractSpecification(
      0.01, 1000 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);  // 5 year
  contract_specification_map_["CGF_1"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["CGZ_0"] = ContractSpecification(
      0.005, 2000 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);  // 2 year
  contract_specification_map_["CGZ_1"] =
      ContractSpecification(0.005, 2000 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

  /* MX Spreads */

  // additional CGBs to include spread contracts
  contract_specification_map_["CGB_2"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["CGB_3"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

  contract_specification_map_["SP_CGB0_CGB1"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_CGB0_CGB2"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_CGB0_CGB3"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

  contract_specification_map_["SP_CGB1_CGB2"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_CGB2_CGB3"] =
      ContractSpecification(0.01, 1000 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

  contract_specification_map_["SP_BAX0_BAX1"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX0_BAX2"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX0_BAX3"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX0_BAX4"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX0_BAX5"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX0_BAX6"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX0_BAX7"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX0_BAX8"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX0_BAX9"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

  if (t_intdate_ >= 20140908) {
    contract_specification_map_["SP_BAX1_BAX2"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX1_BAX3"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX1_BAX4"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX1_BAX5"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX1_BAX6"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX1_BAX7"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX1_BAX8"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX1_BAX9"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

    contract_specification_map_["SP_BAX2_BAX3"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX2_BAX4"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX2_BAX5"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX2_BAX6"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX2_BAX7"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX2_BAX8"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX2_BAX9"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

    contract_specification_map_["SP_BAX3_BAX4"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX3_BAX5"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX3_BAX6"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX3_BAX7"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX3_BAX8"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX3_BAX9"] =
        ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

  } else {
    contract_specification_map_["SP_BAX1_BAX2"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX1_BAX3"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX1_BAX4"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX1_BAX5"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX1_BAX6"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX1_BAX7"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX1_BAX8"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX1_BAX9"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

    contract_specification_map_["SP_BAX2_BAX3"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX2_BAX4"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX2_BAX5"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX2_BAX6"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX2_BAX7"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX2_BAX8"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX2_BAX9"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

    contract_specification_map_["SP_BAX3_BAX4"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX3_BAX5"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX3_BAX6"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX3_BAX7"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX3_BAX8"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
    contract_specification_map_["SP_BAX3_BAX9"] =
        ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  }

  contract_specification_map_["SP_BAX4_BAX5"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX4_BAX6"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX4_BAX7"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX4_BAX8"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX4_BAX9"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

  contract_specification_map_["SP_BAX5_BAX6"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX5_BAX7"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX5_BAX8"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX5_BAX9"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

  contract_specification_map_["SP_BAX6_BAX7"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX6_BAX8"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX6_BAX9"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

  contract_specification_map_["SP_BAX7_BAX8"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["SP_BAX7_BAX9"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

  contract_specification_map_["SP_BAX8_BAX9"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

  // Need to verify the values
  contract_specification_map_["FLY_BAX0_BAX1_BAX2"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["FLY_BAX0_BAX1_BAX3"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["FLY_BAX0_BAX2_BAX4"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);

  contract_specification_map_["FLY_BAX1_BAX2_BAX3"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["FLY_BAX2_BAX3_BAX4"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["FLY_BAX3_BAX4_BAX5"] =
      ContractSpecification(0.005, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["FLY_BAX4_BAX5_BAX6"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["FLY_BAX5_BAX6_BAX7"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["FLY_BAX6_BAX7_BAX8"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
  contract_specification_map_["FLY_BAX7_BAX8_BAX9"] =
      ContractSpecification(0.01, 2500 * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD), kExchSourceTMX, 1);
}

void SecurityDefinitions::BMFSecurityDefinitions(int t_intdate_) {
  // Combined
  contract_specification_map_["C_DOL"] = ContractSpecification(1, 1, kExchSourceBMF, 1);
  contract_specification_map_["C_IND"] = ContractSpecification(1, 1, kExchSourceBMF, 1);
  contract_specification_map_["C_DI"] = ContractSpecification(1, 1, kExchSourceBMF, 1);

  /* BMF */
  contract_specification_map_["BR_IND_0"] =
      ContractSpecification(5, CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["BR_IND_1"] =
      ContractSpecification(5, CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["BR_IND_2"] =
      ContractSpecification(5, CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);

  contract_specification_map_["IR1_0"] =
      ContractSpecification(5, CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["BR_ISP_0"] = ContractSpecification(0.25, 50, kExchSourceBMF, 1);  // SP500
  contract_specification_map_["BR_ISP_1"] = ContractSpecification(0.25, 50, kExchSourceBMF, 1);  // SP500

  contract_specification_map_["BR_WIN_0"] =
      ContractSpecification(5, 0.2 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 1);
  contract_specification_map_["BR_WIN_1"] =
      ContractSpecification(5, 0.2 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 1);
  contract_specification_map_["BR_WIN_2"] =
      ContractSpecification(5, 0.2 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 1);

  contract_specification_map_["BR_DOL_0"] =
      ContractSpecification(0.5, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["BR_DOL_1"] =
      ContractSpecification(0.5, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["BR_DOL_2"] =
      ContractSpecification(0.5, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);

  contract_specification_map_["DR1_0"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);

  contract_specification_map_["BR_WDO_0"] =
      ContractSpecification(0.5, 10 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 1);
  contract_specification_map_["BR_WDO_1"] =
      ContractSpecification(0.5, 10 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 1);
  contract_specification_map_["BR_WDO_2"] =
      ContractSpecification(0.5, 10 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 1);

  contract_specification_map_["BR_CCM_0"] = ContractSpecification(
      0.01, 450 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 1);  // corn
  //  contract_specification_map_ [ "BR_BGM_0" ] = ContractSpecification ( 0.01, 330 * CurrencyConvertor::Convert (
  //  kCurrencyBRL, kCurrencyUSD ), kExchSourceBMF, 1 ); // cattle
  contract_specification_map_["BR_ICF_0"] = ContractSpecification(0.05, 100, kExchSourceBMF, 1);  // coffee
  contract_specification_map_["BR_SJC_0"] =
      ContractSpecification(0.01, 450, kExchSourceBMF, 1);  // soya-cme-replication
  contract_specification_map_["BR_SJC_1"] =
      ContractSpecification(0.01, 450, kExchSourceBMF, 1);  // soya-cme-replication
  contract_specification_map_["BR_SJC_2"] =
      ContractSpecification(0.01, 450, kExchSourceBMF, 1);  // soya-cme-replication

  // DI futures
  //    contract_specification_map_ [ "BR_DI_0" ] = ContractSpecification ( 0.01, 393 * CurrencyConvertor::Convert (
  //    kCurrencyBRL, kCurrencyUSD ), kExchSourceBMF, 5 ); // F13 has min-tick 0.001
  contract_specification_map_["DI1F13"] =
      ContractSpecification(0.01, 1150 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1F14"] =
      ContractSpecification(0.01, 1800 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1F15"] =
      ContractSpecification(0.01, 2300 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1F16"] =
      ContractSpecification(0.01, 2790 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1F17"] =
      ContractSpecification(0.01, 3000 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1F18"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1F19"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1F20"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1F21"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1F22"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1F23"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1F24"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1F25"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1F26"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1F27"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1F28"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1F29"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);

  contract_specification_map_["SPDI1N17F17"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);

  contract_specification_map_["SPDI1F18N17"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["SPDI1F18F17"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);

  contract_specification_map_["SPDI1N18F18"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);

  contract_specification_map_["SPDI1F19N18"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["SPDI1F19F18"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["SPDI1F19F17"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);

  contract_specification_map_["SPDI1N19F19"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);

  contract_specification_map_["SPDI1F20N19"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["SPDI1F20F19"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["SPDI1F20F18"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);

  contract_specification_map_["SPDI1F21F20"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["SPDI1F21F19"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["SPDI1F21F18"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);

  contract_specification_map_["SPDI1F23F22"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["SPDI1F23F21"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["SPDI1F23F20"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["SPDI1F23F19"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);

  contract_specification_map_["SPDI1F25F23"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["SPDI1F25F21"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["SPDI1F25F19"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);

  contract_specification_map_["DI1N13"] =
      ContractSpecification(0.01, 1150 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1N14"] =
      ContractSpecification(0.01, 1800 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1N15"] =
      ContractSpecification(0.01, 2300 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1N16"] =
      ContractSpecification(0.01, 2790 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1N17"] =
      ContractSpecification(0.01, 3000 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1N18"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1N19"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1N20"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1N21"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1N22"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1N23"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1N24"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1N25"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);

  contract_specification_map_["DI1G13"] =
      ContractSpecification(0.01, 1150 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1G14"] =
      ContractSpecification(0.01, 1800 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1G15"] =
      ContractSpecification(0.01, 2300 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1G16"] =
      ContractSpecification(0.01, 2790 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1G17"] =
      ContractSpecification(0.01, 3000 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1G18"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1G19"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1G20"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1G21"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1G22"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1G23"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1G24"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1G25"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);

  contract_specification_map_["DI1J13"] =
      ContractSpecification(0.01, 1150 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1J14"] =
      ContractSpecification(0.01, 1800 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1J15"] =
      ContractSpecification(0.01, 2300 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1J16"] =
      ContractSpecification(0.01, 2790 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1J17"] =
      ContractSpecification(0.01, 3000 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1J18"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1J19"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1J20"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1J21"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1J22"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1J23"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1J24"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1J25"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);

  contract_specification_map_["DI1V13"] =
      ContractSpecification(0.01, 1150 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1V14"] =
      ContractSpecification(0.01, 1800 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1V15"] =
      ContractSpecification(0.01, 2300 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1V16"] =
      ContractSpecification(0.01, 2790 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1V17"] =
      ContractSpecification(0.01, 3000 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1V18"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1V19"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1V20"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1V21"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1V22"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1V23"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1V24"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
  contract_specification_map_["DI1V25"] =
      ContractSpecification(0.01, 50 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMF, 5);
}

void SecurityDefinitions::LIFFEISESecurityDefintions(int t_intdate_) {
  // Combined
  contract_specification_map_["C_LFI"] = ContractSpecification(1, 1, kExchSourceLIFFE, 1);
  contract_specification_map_["C_LFL"] = ContractSpecification(1, 1, kExchSourceLIFFE, 1);
  contract_specification_map_["C_LFI_SP"] = ContractSpecification(1, 1, kExchSourceLIFFE, 1);
  contract_specification_map_["C_LFL_SP"] = ContractSpecification(1, 1, kExchSourceLIFFE, 1);
  contract_specification_map_["C_YFEBM"] = ContractSpecification(1, 1, kExchSourceLIFFE, 1);

  // Paris Futures
  contract_specification_map_["JFFCE_0"] = ContractSpecification(
      0.5, 10 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // CAC 40 Ind. F
  contract_specification_map_["JFFCE_1"] = ContractSpecification(
      0.5, 10 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // CAC 40 Ind. F
  contract_specification_map_["JFMFC_0"] = ContractSpecification(
      0.5, 1 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // Mini CAC 40 Ind. F

  // Amsterdam Futures
  contract_specification_map_["KFFTI_0"] = ContractSpecification(
      0.05, 200 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // AEX Ind. F
  contract_specification_map_["KFMFA_0"] = ContractSpecification(
      0.05, 20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // Mini AEX Ind. F

  // London Futures
  if (tradingdate_ < 20141117) {
    contract_specification_map_["LFZ_0"] = ContractSpecification(
        0.5, 10 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // FTSE 100 IF
    contract_specification_map_["LFZ_1"] = ContractSpecification(
        0.5, 10 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // FTSE 100 IF
  } else {
    contract_specification_map_["LFZ_0"] = ContractSpecification(
        0.5, 10 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // FTSE 100 IF
    contract_specification_map_["LFZ_1"] = ContractSpecification(
        0.5, 10 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // FTSE 100 IF
  }

  if (tradingdate_ < 20141020) {
    // Three months sterling futures, this guy's like BAX, need to have a closer look at symbol mapping for this
    contract_specification_map_["LFL_0"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // Sterling
    contract_specification_map_["LFL_1"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  //
    contract_specification_map_["LFL_2"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  //
    contract_specification_map_["LFL_3"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  //
    contract_specification_map_["LFL_4"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  //
    contract_specification_map_["LFL_5"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  //
    contract_specification_map_["LFL_6"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  //
    contract_specification_map_["LFL_7"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  //
    contract_specification_map_["LFL_8"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  //
    contract_specification_map_["LFL_9"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  //
    contract_specification_map_["LFL_10"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  //
    contract_specification_map_["LFL_11"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  //
    contract_specification_map_["LFL_12"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  //
    contract_specification_map_["LFL_13"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // Q
    contract_specification_map_["LFL_14"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  //
    contract_specification_map_["LFL_15"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  //

    // Spreads contracts of LFL
    contract_specification_map_["SP_LFL0_LFL1"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL1_LFL2"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL2_LFL3"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL3_LFL4"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL4_LFL5"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL5_LFL6"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL6_LFL7"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL7_LFL8"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL8_LFL9"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL9_LFL10"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL10_LFL11"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL11_LFL12"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT

    contract_specification_map_["SP_LFL0_LFL2"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL1_LFL3"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL2_LFL4"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL3_LFL5"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL4_LFL6"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL5_LFL7"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL6_LFL8"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL7_LFL9"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL8_LFL10"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL9_LFL11"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL10_LFL12"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT

    contract_specification_map_["LFRY_0"] = ContractSpecification(
        0.01, 1000 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // GILT Futures
    contract_specification_map_["LFR_0"] = ContractSpecification(
        0.01, 1000 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // GILT Futures
    contract_specification_map_["LFS_0"] = ContractSpecification(
        0.01, 1000 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // GILT Futures
  } else {
    contract_specification_map_["LFRY_0"] = ContractSpecification(
        0.01, 1000 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // GILT Futures
    contract_specification_map_["LFR_0"] = ContractSpecification(
        0.01, 1000 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // GILT Futures
    contract_specification_map_["LFS_0"] = ContractSpecification(
        0.01, 1000 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // GILT Futures

    contract_specification_map_["LFL_0"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // Sterling
    contract_specification_map_["LFL_1"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  //
    contract_specification_map_["LFL_2"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  //
    contract_specification_map_["LFL_3"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  //
    contract_specification_map_["LFL_4"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  //
    contract_specification_map_["LFL_5"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  //
    contract_specification_map_["LFL_6"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  //
    contract_specification_map_["LFL_7"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  //
    contract_specification_map_["LFL_8"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  //
    contract_specification_map_["LFL_9"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  //
    contract_specification_map_["LFL_10"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  //
    contract_specification_map_["LFL_11"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  //
    contract_specification_map_["LFL_12"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  //
    contract_specification_map_["LFL_13"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // Q
    contract_specification_map_["LFL_14"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  //
    contract_specification_map_["LFL_15"] = ContractSpecification(
        0.01, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  //

    // Spreads contracts of LFL
    contract_specification_map_["SP_LFL0_LFL1"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL1_LFL2"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL2_LFL3"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL3_LFL4"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL4_LFL5"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL5_LFL6"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL6_LFL7"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL7_LFL8"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL8_LFL9"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL9_LFL10"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL10_LFL11"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL11_LFL12"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT

    contract_specification_map_["SP_LFL0_LFL2"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL1_LFL3"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL2_LFL4"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL3_LFL5"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL4_LFL6"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL5_LFL7"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL6_LFL8"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL7_LFL9"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL8_LFL10"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL9_LFL11"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFL10_LFL12"] = ContractSpecification(
        0.01, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT

    if (tradingdate_ > 20160123) {
      contract_specification_map_["LFL_0"] = ContractSpecification(
          0.005, 1250 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // Sterling

      // Spreads contracts of LFL
      contract_specification_map_["SP_LFL0_LFL1"] = ContractSpecification(
          0.005, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT

      contract_specification_map_["SP_LFL0_LFL2"] = ContractSpecification(
          0.005, 2500 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    }
  }

  if (tradingdate_ < 20141103) {
    // one more guy like BAX
    contract_specification_map_["LFI_0"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_1"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_2"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_3"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_4"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_5"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_6"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_7"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_8"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_9"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_10"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_11"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_12"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_13"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_14"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_15"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_16"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_17"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_18"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_19"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT

    // Spreads contrats of LFI
    contract_specification_map_["SP_LFI0_LFI1"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI1_LFI2"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI2_LFI3"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI3_LFI4"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI4_LFI5"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI5_LFI6"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI6_LFI7"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI7_LFI8"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI8_LFI9"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI9_LFI10"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI10_LFI11"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI11_LFI12"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT

    contract_specification_map_["SP_LFI0_LFI2"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI1_LFI3"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI2_LFI4"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI3_LFI5"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI4_LFI6"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI5_LFI7"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI6_LFI8"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI7_LFI9"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI8_LFI10"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI9_LFI11"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI10_LFI12"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT

    contract_specification_map_["B_LFI_0_1_2"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["B_LFI_1_2_3"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["B_LFI_2_3_4"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["B_LFI_3_4_5"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
    contract_specification_map_["B_LFI_4_5_6"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // 3 month EURO FUT
  } else {
    // one more guy like BAX
    contract_specification_map_["LFI_0"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_1"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_2"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_3"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_4"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_5"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_6"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_7"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_8"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_9"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_10"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_11"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_12"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_13"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_14"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_15"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_16"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_17"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_18"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["LFI_19"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT

    // Spreads contrats of LFI
    contract_specification_map_["SP_LFI0_LFI1"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI1_LFI2"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI2_LFI3"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI3_LFI4"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI4_LFI5"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI5_LFI6"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI6_LFI7"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI7_LFI8"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI8_LFI9"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI9_LFI10"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI10_LFI11"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI11_LFI12"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT

    contract_specification_map_["SP_LFI0_LFI2"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI1_LFI3"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI2_LFI4"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI3_LFI5"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI4_LFI6"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI5_LFI7"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI6_LFI8"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI7_LFI9"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI8_LFI10"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI9_LFI11"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["SP_LFI10_LFI12"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT

    contract_specification_map_["B_LFI_0_1_2"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["B_LFI_1_2_3"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["B_LFI_2_3_4"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["B_LFI_3_4_5"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
    contract_specification_map_["B_LFI_4_5_6"] = ContractSpecification(
        0.005, 2500 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceICE, 1);  // 3 month EURO FUT
  }

  // LIFFE commodities
  contract_specification_map_["YFEBM_0"] = ContractSpecification(
      0.25, 50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // milling wheat futures
  contract_specification_map_["YFEBM_1"] = ContractSpecification(
      0.25, 50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // milling wheat futures
  contract_specification_map_["YFEBM_2"] = ContractSpecification(
      0.25, 50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // milling wheat futures
  contract_specification_map_["YFEBM_3"] = ContractSpecification(
      0.25, 50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // milling wheat futures
  contract_specification_map_["YFEBM_4"] = ContractSpecification(
      0.25, 50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // milling wheat futures
  contract_specification_map_["YFEBM_5"] = ContractSpecification(
      0.25, 50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceLIFFE, 1);  // milling wheat futures

  contract_specification_map_["XFW_0"] = ContractSpecification(0.10, 50, kExchSourceLIFFE, 1);  // white sugar futures
  contract_specification_map_["XFW_1"] = ContractSpecification(0.10, 50, kExchSourceLIFFE, 1);  // white sugar futures
  contract_specification_map_["XFW_2"] = ContractSpecification(0.10, 50, kExchSourceLIFFE, 1);  // white sugar futures
  contract_specification_map_["XFW_3"] = ContractSpecification(0.10, 50, kExchSourceLIFFE, 1);  // white sugar futures
  contract_specification_map_["XFW_4"] = ContractSpecification(0.10, 50, kExchSourceLIFFE, 1);  // white sugar futures

  contract_specification_map_["XFC_0"] = ContractSpecification(
      1.00, 10 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // cocoa futures
  contract_specification_map_["XFC_1"] = ContractSpecification(
      1.00, 10 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // cocoa futures
  contract_specification_map_["XFC_2"] = ContractSpecification(
      1.00, 10 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // cocoa futures
  contract_specification_map_["XFC_3"] = ContractSpecification(
      1.00, 10 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // cocoa futures
  contract_specification_map_["XFC_4"] = ContractSpecification(
      1.00, 10 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceLIFFE, 1);  // cocoa futures

  contract_specification_map_["XFRC_0"] = ContractSpecification(1.00, 10, kExchSourceLIFFE, 1);  // coffee futures
  contract_specification_map_["XFRC_1"] = ContractSpecification(1.00, 10, kExchSourceLIFFE, 1);  // coffee futures
  contract_specification_map_["XFRC_2"] = ContractSpecification(1.00, 10, kExchSourceLIFFE, 1);  // coffee futures
  contract_specification_map_["XFRC_3"] = ContractSpecification(1.00, 10, kExchSourceLIFFE, 1);  // coffee futures
  contract_specification_map_["XFRC_4"] = ContractSpecification(1.00, 10, kExchSourceLIFFE, 1);  // coffee futures
}

void SecurityDefinitions::SGXSecurityDefinitions(int t_intdate_) {
  // SGX
  contract_specification_map_["C_SGX_CN"] = ContractSpecification(2.5, 1, kExchSourceSGX, 1);
  contract_specification_map_["SGX_CN_0"] = ContractSpecification(2.5, 1, kExchSourceSGX, 1);
  contract_specification_map_["SGX_CN_1"] = ContractSpecification(2.5, 1, kExchSourceSGX, 1);

  contract_specification_map_["SGX_IN_0"] = ContractSpecification(.5, 2, kExchSourceSGX, 1);
  contract_specification_map_["SGX_IN_1"] = ContractSpecification(.5, 2, kExchSourceSGX, 1);

  // contract_specification_map_["SGX_AJ_0"] =
  //    ContractSpecification(0.01, 25000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceSGX, 1);
  // contract_specification_map_["SGX_AJ_1"] =
  //    ContractSpecification(0.01, 25000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceSGX, 1);

  // contract_specification_map_["SGX_AU_0"] = ContractSpecification(0.0001, 25000, kExchSourceSGX, 1);
  // contract_specification_map_["SGX_AU_1"] = ContractSpecification(0.0001, 25000, kExchSourceSGX, 1);

  // contract_specification_map_["SGX_US_0"] = ContractSpecification(0.0001, 25000, kExchSourceSGX, 1);
  // contract_specification_map_["SGX_US_1"] = ContractSpecification(0.0001, 25000, kExchSourceSGX, 1);

  contract_specification_map_["SGX_IU_0"] = ContractSpecification(0.01, 200, kExchSourceSGX, 1);
  contract_specification_map_["SGX_IU_1"] = ContractSpecification(0.01, 200, kExchSourceSGX, 1);

  // contract_specification_map_["SGX_KU_0"] = ContractSpecification(0.0001, 25000, kExchSourceSGX, 1);
  // contract_specification_map_["SGX_KU_1"] = ContractSpecification(0.0001, 25000, kExchSourceSGX, 1);

  contract_specification_map_["C_SGX_NK"] =
      ContractSpecification(5, 500 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceSGX, 1);
  contract_specification_map_["SGX_NK_0"] =
      ContractSpecification(5, 500 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceSGX, 1);
  contract_specification_map_["SGX_NK_1"] =
      ContractSpecification(5, 500 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceSGX, 1);
  contract_specification_map_["SP_SGX_NK0_NK1"] =
      ContractSpecification(1, 500 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceSGX, 1);
  contract_specification_map_["SGX_NKF_0"] =
      ContractSpecification(5, 500 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceSGX, 1);
  contract_specification_map_["SGX_NKF_1"] =
      ContractSpecification(5, 500 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceSGX, 1);

  contract_specification_map_["SGX_TW_0"] = ContractSpecification(0.1, 100, kExchSourceSGX, 1);
  contract_specification_map_["SGX_TW_1"] = ContractSpecification(0.1, 100, kExchSourceSGX, 1);

  contract_specification_map_["SGX_SG_0"] = ContractSpecification(0.05, 100, kExchSourceSGX, 1);
  contract_specification_map_["SGX_SG_1"] = ContractSpecification(0.05, 100, kExchSourceSGX, 1);

  contract_specification_map_["SGX_NU_0"] = ContractSpecification(5, 5, kExchSourceSGX, 1);
  contract_specification_map_["SGX_NU_1"] = ContractSpecification(5, 5, kExchSourceSGX, 1);
  contract_specification_map_["SP_SGX_NU0_NU1"] = ContractSpecification(5, 5, kExchSourceSGX, 1);

  // contract_specification_map_["SGX_CH_0"] = ContractSpecification(2, 5, kExchSourceSGX, 1);
  // contract_specification_map_["SGX_CH_1"] = ContractSpecification(2, 5, kExchSourceSGX, 1);

  // contract_specification_map_["SGX_MD_0"] = ContractSpecification(0.2, 50, kExchSourceSGX, 1);
  // contract_specification_map_["SGX_MD_1"] = ContractSpecification(0.2, 50, kExchSourceSGX, 1);

  contract_specification_map_["SGX_INB_0"] = ContractSpecification(2, 1, kExchSourceSGX, 1);
  contract_specification_map_["SGX_INB_1"] = ContractSpecification(2, 1, kExchSourceSGX, 1);
}

void SecurityDefinitions::OSESecurityDefinitions(int t_intdate_) {
  /// Combined
  contract_specification_map_["C_NK"] = ContractSpecification(1, 1, kExchSourceJPY, 1);
  contract_specification_map_["C_TOPIX"] = ContractSpecification(1, 1, kExchSourceJPY, 1);
  contract_specification_map_["C_JGBL"] = ContractSpecification(1, 1, kExchSourceJPY, 1);
  ///
  ////OSE
  contract_specification_map_["NKM_0"] =
      ContractSpecification(5, 100 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);
  contract_specification_map_["NKM_1"] =
      ContractSpecification(5, 100 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);
  contract_specification_map_["NKM_2"] =
      ContractSpecification(5, 100 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);
  contract_specification_map_["NKM_3"] =
      ContractSpecification(5, 100 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);
  contract_specification_map_["NKM_4"] =
      ContractSpecification(5, 100 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);
  contract_specification_map_["NK_0"] =
      ContractSpecification(10, 1000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);
  contract_specification_map_["NK_1"] =
      ContractSpecification(10, 1000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);
  contract_specification_map_["NK_2"] =
      ContractSpecification(10, 1000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);
  contract_specification_map_["NK_3"] =
      ContractSpecification(10, 1000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);
  contract_specification_map_["NK_4"] =
      ContractSpecification(10, 1000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);
  contract_specification_map_["JP400_0"] =
      ContractSpecification(5, 100 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);
  contract_specification_map_["JN400_0"] =
      ContractSpecification(5, 100 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);
  contract_specification_map_["JP400_1"] =
      ContractSpecification(5, 100 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);
  contract_specification_map_["JP400_2"] =
      ContractSpecification(5, 100 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);
  contract_specification_map_["JP400_3"] =
      ContractSpecification(5, 100 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);

  contract_specification_map_["JGBMY_0"] = ContractSpecification(
      1, 10000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);  // 5-year JGB Futures
  contract_specification_map_["JGBMY_1"] = ContractSpecification(
      1, 10000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);  // 5-year JGB Futures
  contract_specification_map_["JGBLY_0"] = ContractSpecification(
      1, 10000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);  // 10-year JGB Futures
  contract_specification_map_["JGBLY_1"] = ContractSpecification(
      1, 10000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);  // 10-year JGB Futures
  contract_specification_map_["JGBM_0"] = ContractSpecification(
      1, 10000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);  // 5-year JGB Futures
  contract_specification_map_["JGBM_1"] = ContractSpecification(
      1, 10000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);  // 5-year JGB Futures
  contract_specification_map_["JGBL_0"] = ContractSpecification(
      1, 10000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);  // 10-year JGB Futures
  contract_specification_map_["JGBL_1"] = ContractSpecification(
      1, 10000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);  // 10-year JGB Futures
  contract_specification_map_["JGBSL_0"] = ContractSpecification(
      5, 2000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);  // 20-year JGB Futures
  contract_specification_map_["JGBSL_1"] = ContractSpecification(
      5, 2000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);  // 20-year JGB Futures
  contract_specification_map_["JGBLM_0"] =
      ContractSpecification(1, 10000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY,
                            1);  // 10-year mini JGB Futures
  contract_specification_map_["JGBLM_1"] =
      ContractSpecification(1, 10000 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY,
                            1);  // 10-year mini JGB Futures

  contract_specification_map_["TOPIX_0"] = ContractSpecification(
      50, 100 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);  // TOPIX futures
  contract_specification_map_["TOPIX_1"] = ContractSpecification(
      50, 100 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);  // TOPIX futures
  contract_specification_map_["TOPIXM_0"] = ContractSpecification(
      25, 10 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);  // mini TOPIX futures
  contract_specification_map_["TOPIXM_1"] = ContractSpecification(
      25, 10 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);  // mini TOPIX futures
  contract_specification_map_["TPX30_0"] = ContractSpecification(
      50, 10 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);  // TOPIX core30 futures
  contract_specification_map_["JP400_0"] = ContractSpecification(
      5, 100 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);  // JP400 futures

  // NKM - FrontMonth Contract MarketMaking
  contract_specification_map_["NKMF_0"] =
      ContractSpecification(5, 100 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);

  // OSE-TSE Merge Additions
  contract_specification_map_["DJI_0"] =
      ContractSpecification(1, 100 * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD), kExchSourceJPY, 1);
}

void SecurityDefinitions::BatsSecurityDefinitions(int t_intdate_) {
  // LSE - contracts traded both in rts/micex and LSE
  contract_specification_map_["OGZDl"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["VTBRl"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SBERl"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);

  // LSE-100
  contract_specification_map_["ARMl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ADNl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ADMl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["AGKl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["AMECl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["AALl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ANTOl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ASHMl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ABFl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["AZNl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["AVl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BAl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BGl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BLTl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BPl"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BTl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BABl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BARCl"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BATSl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BLNDl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BSYl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BNZLl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BRBYl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["CRHl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["CPIl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["CSCGl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["CCLl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["CNAl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["CPGl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["CRDAl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["DGEl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ENRCl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["EVRAl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["EXPNl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["FRESl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["GFSl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["GKNl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["GSKl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["GLENl"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["HSBAl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["HMSOl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["HLl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["IAPl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["IMIl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ITVl"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["IMTl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["IHGl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["IAGl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ITRKl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["JMATl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["KAZl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["KGFl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["LANDl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["LGENl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["LLOYl"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["MKSl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["MGGTl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["MRWl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["NGl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["NXTl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["OMLl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["PSONl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["PNNl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["PFCl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["POLYl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["PRUl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RSAl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RRSl"] =
      ContractSpecification(5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RBl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RELl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RSLl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["REXl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RIOl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RRl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RBSl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RDSAl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RDSBl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SABl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SSEl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SGEl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SBRYl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SDRl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SRPl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SVTl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SHPl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SNl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SMINl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["STANl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SLl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["TATEl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["TSCOl"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["TLWl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ULVRl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["UUl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["VEDl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["VODl"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["WPPl"] =
      ContractSpecification(0.5, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["WEIRl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["WTBl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["WOSl"] =
      ContractSpecification(1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["XTAl"] =
      ContractSpecification(0.1, CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD), kExchSourceBATSCHI, 1);

  // lse-dax -30
  contract_specification_map_["ADSd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ALVd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BASd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BMWd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BAYNd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BEId"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["CBKd"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["DAId"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["DBKd"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["DB1d"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["DPWd"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["DTEd"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["EOANd"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["FMEd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["FREd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["HEId"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["HEN3d"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["IFXd"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SDFd"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["LINd"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["LHAd"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["MANd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["MRKd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["MEOd"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["MUV2d"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RWEd"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SAPd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SIEd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["TKAd"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["VOW3d"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);

  // lse-aex -25
  contract_specification_map_["AGNa"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["AHa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["AFp"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["AKZAa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["APAMa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["MTa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ASMLa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BOKAa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["CORAa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["DSMa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["FURa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["HEIAa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["INGAa"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["KPNa"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["PHIAa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["PNLa"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RANDa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RENa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RDSAa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SBMOa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["TNTEa"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["TOM2a"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ULp"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["UNAa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["WKLa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);

  // lse-cac 40
  contract_specification_map_["ACp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["AIp"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ALUp"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ALOp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["MTa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["CSp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BNPp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ENp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["CAPp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["CAp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ACAp"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BNp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["EADp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["EDFp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["EIp"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["FTEp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["GSZp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ORp"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["LGp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["LRp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["MCp"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["MLp"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RIp"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["UGp"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["PPp"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["PUBp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RNOp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SAFp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SGOp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SANp"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SUp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["GLEp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["STMp"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["TECp"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["FPp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ULp"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["VKp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["VIEp"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["DGp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["VIVp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);

  // euro-stoxx 50
  contract_specification_map_["AIp"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ALVd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ABIb"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["MTSe"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ASMLa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["Gm"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["CSp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BASd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BAYNd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BBVAe"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SANe"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BMWd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BNPp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["CAp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["CRGi"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["DAId"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["BNp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["DBKd"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["DTEd"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["EOANd"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ENELm"] =
      ContractSpecification(0.002, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ENIm"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["EIp"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["FTEp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["GSZp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["GLEp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["IBEe"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ITXe"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["INGAa"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ISPm"] =
      ContractSpecification(0.001, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ORp"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["MCp"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["MUV2d"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["NOK1Vh"] =
      ContractSpecification(0.002, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["PHIAa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["REPe"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["RWEd"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SGOp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SANp"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SAPd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SUp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["SIEd"] =
      ContractSpecification(0.01, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["TEFe"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["FPp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["ULp"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["UCGm"] =
      ContractSpecification(0.002, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["UNAa"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["DGp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["VIVp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  contract_specification_map_["VOW3d"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);

  // changed Ulp to Ula from 28-Feb-2013
  // We let both symbols live in our system
  contract_specification_map_["ULa"] =
      ContractSpecification(0.05, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
  // changed FTEp -> ORAp from 1st July. Last date for FTEp = 2013-06-28. First date for ORAp = 2013-07-01
  contract_specification_map_["ORAp"] =
      ContractSpecification(0.005, CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD), kExchSourceBATSCHI, 1);
}

void SecurityDefinitions::BMFEQSecurityDefinitions(int t_intdate_) {
  // BMF Equities

  contract_specification_map_["FNAM11"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["RADL3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["BBTG11"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["TOTS3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["EQTL3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["WEGE3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["PSSA3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["ANIM3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["IGTA3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["ODPV3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["VVAR11"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["VLID3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["MYPK3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["MPLU3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["TAEE11"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);

  contract_specification_map_["GETI4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["BRSR6"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["BEEF3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["MDIA3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["SULA11"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["SEER3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["TAEE11"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["PARC3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["QGEP3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);

  contract_specification_map_["BOVA11"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 10);
  contract_specification_map_["RUMO3"] = ContractSpecification(
      0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);  // ALLL3, ticker change

  contract_specification_map_["SMLE3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["ABEV3"] = ContractSpecification(
      0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);  // ABEV3, ticker change

  contract_specification_map_["BBAS3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["BBDC3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["BBDC4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["BISA3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["BRAP4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["BRFS3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["BRKM5"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["BRML3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["BRTO4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["BTOW3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["BVMF3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["CCRO3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["CESP6"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["CIEL3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["CMIG4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["CPFE3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["CPLE6"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["CRUZ3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["CSAN3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["CSNA3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["CYRE3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["DASA3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["DTEX3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["ELET3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["ELET6"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["ELPL4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["EMBR3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["FIBR3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["GFSA3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["GGBR4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["GOAU4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["GOLL4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["HGTX3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["HYPE3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["ITSA4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["ITUB4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["JBSS3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["KLBN4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["LAME4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["LIGT3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["LLXL3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["LREN3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["MMXM3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["MRFG3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["MRVE3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["NATU3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["OGXP3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["PCAR4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["PDGR3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["PETR3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["PETR4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["RDCD3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["RENT3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["RSID3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["SANB11"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["SBSP3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["TAMM4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["TIMP3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["TMAR5"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["TNLP3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["TNLP4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["TRPL4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["UGPA3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["USIM3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["USIM5"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["VAGR3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["VALE3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["VALE5"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["VIVT4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["BBSE3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["BRPR3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["CTIP3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["ECOR3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["ENBR3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["ESTC3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["EVEN3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["KLBN11"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["KROT3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["OIBR4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["OIBR3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["POMO4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["QUAL3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["RLOG3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["SUZB5"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["TBLE3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["MULT3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  // added on 20170613
  contract_specification_map_["RAIL3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["WIZS3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["TEND3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["PETR4T"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["SMTO3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["AZUL4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["ALPA4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["AALR3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["CVCB3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["CSMG3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["SDIL11"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["ITSA4T"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["JSLG3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["JBSS3T"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["PRML3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["MILS3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["ETER3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["SGPS3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["BBTG12"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["TCSA3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["ENGI11"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["PMAM3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["RAPT4"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
  contract_specification_map_["AMAR3"] =
      ContractSpecification(0.01, 1 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD), kExchSourceBMFEQ, 100);
}

void SecurityDefinitions::_ResetASXMpiIfNeeded(const time_t& _start_time_, const bool livetrading_) {
  bool use_half_ticks_ = false;
  int c_month_ = ((tradingdate_ / 100) % 100);
  if (c_month_ % 3 == 0) {
    int start_date_ = DateTime::CalcNextWeekDay(100 * (tradingdate_ / 100) + 7);  // 8th or next business day
    int end_date_ = DateTime::CalcNextWeekDay(100 * (tradingdate_ / 100) + 14);   // 15th or next business day

    if (_start_time_ != 0) {
      time_t st_ = DateTime::GetTimeFromTZHHMMStr(start_date_, "AST_1645");
      time_t et_ = DateTime::GetTimeFromTZHHMMStr(end_date_, "AST_1645");
      use_half_ticks_ = (_start_time_ >= st_ && _start_time_ <= et_);
      if (livetrading_) {
        time_t prev_st_ = DateTime::GetTimeFromTZHHMMStr(start_date_, "AST_830");  // To ensure min price changes take
                                                                                   // place on 9th AS session because
                                                                                   // trading date will be 8th in live
                                                                                   // trading
        time_t prev_et_ = DateTime::GetTimeFromTZHHMMStr(start_date_, "AST_1000");

        if (_start_time_ >= prev_st_ && _start_time_ <= prev_et_) {
          use_half_ticks_ = true;
        }

        time_t next_st_ = DateTime::GetTimeFromTZHHMMStr(
            end_date_, "AST_830");  // To ensure correct functionality at the end of expiry week.
        time_t next_et_ = DateTime::GetTimeFromTZHHMMStr(end_date_, "AST_1000");
        if (_start_time_ >= next_st_ && _start_time_ <= next_et_) {
          use_half_ticks_ = false;
        }
      }
    }
  }

  if (use_half_ticks_) {
    contract_specification_map_["XT_0"].min_price_increment_ = 0.0025;
    contract_specification_map_["YT_0"].min_price_increment_ = 0.005;
    contract_specification_map_["XT_1"].min_price_increment_ = 0.0025;
    contract_specification_map_["YT_1"].min_price_increment_ = 0.005;
    contract_specification_map_["SP_XT0_YT0"].min_price_increment_ = 0.0025;
    contract_specification_map_["XX_0"].min_price_increment_ = 0.0025;
    contract_specification_map_["XX_1"].min_price_increment_ = 0.0025;
  } else {
    time_t st_ = DateTime::GetTimeFromTZHHMMStr(YT_TICKCHANGE_DATE, "AST_1645");
    if (_start_time_ >= st_) {
      contract_specification_map_["YT_0"].min_price_increment_ = 0.005;
      contract_specification_map_["YT_1"].min_price_increment_ = 0.005;
    } else {
      contract_specification_map_["YT_0"].min_price_increment_ = 0.01;
      contract_specification_map_["YT_1"].min_price_increment_ = 0.01;
    }
    contract_specification_map_["XT_0"].min_price_increment_ = 0.005;
    contract_specification_map_["XT_1"].min_price_increment_ = 0.005;
    contract_specification_map_["XX_0"].min_price_increment_ = 0.005;
    contract_specification_map_["XX_1"].min_price_increment_ = 0.005;
    contract_specification_map_["SP_XT0_YT0"].min_price_increment_ = 0.005;
  }
}

double SecurityDefinitions::GetASXSpreadNumbersToDollars() {
  int t_intdate_ = tradingdate_;

  double xt_price_ = HFSAT::SampleDataUtil::GetLastSampleBeforeDate(std::string("XT_0"), t_intdate_,
                                                                    std::string("AvgPrice300"), 30, true);

  // One spread contract is 10 XT and dv01 neutral equivalent of YT. So numbers to dollars is 10 times that of XT.

  return 10 * 100 * fabs(GetASXBondPrice(xt_price_, 10) - GetASXBondPrice((xt_price_ - 0.01), 10)) *
         CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);
}

double SecurityDefinitions::GetASXBondPrice(double price_, int asx_term_) const {
  if (price_ > 0) {
    double yield_ = 100.0 - price_;
    double j8_ = yield_ / 200;
    double j9_ = 1 / (1 + j8_);
    double j10_ = 1000 * (3 * (1 - pow(j9_, (asx_term_ * 2))) / j8_ + 100 * pow(j9_, (asx_term_ * 2)));
    return j10_;
  } else {
    // Error case
    return 0.0;
  }
}

void SecurityDefinitions::GetRTSContractSpecifications() {
  std::map<std::string, bool> shortcode_to_is_n2d_found_;
  shortcode_to_is_n2d_found_["RI_0"] = false;
  shortcode_to_is_n2d_found_["ED_0"] = false;
  shortcode_to_is_n2d_found_["GD_0"] = false;
  shortcode_to_is_n2d_found_["BR_0"] = false;
  shortcode_to_is_n2d_found_["BR_1"] = false;

  std::string tick_value_filename_prefix_ = std::string(DEF_RTS_VALLOC_) + "rts-tick-value.";

  int t_intdate_ = tradingdate_;

  // try looking into past 30 days.
  for (auto i = 0u; i < 30; ++i) {
    std::string tick_value_filename_ = "";

    {
      std::ostringstream t_oss_;
      t_oss_ << t_intdate_;
      tick_value_filename_ = tick_value_filename_prefix_ + t_oss_.str() + ".txt";
    }

    if (FileUtils::ExistsAndReadable(tick_value_filename_)) {
      std::ifstream tick_value_file_;
      tick_value_file_.open(tick_value_filename_.c_str(), std::ifstream::in);

      char readline_buffer_[1024];
      if (tick_value_file_.is_open()) {
        while (tick_value_file_.good()) {
          memset(readline_buffer_, 0, sizeof(readline_buffer_));
          tick_value_file_.getline(readline_buffer_, sizeof(readline_buffer_));
          PerishableStringTokenizer st_(readline_buffer_, sizeof(readline_buffer_));

          const std::vector<const char*>& tokens_ = st_.GetTokens();

          if (tokens_.size() >= 3u) {
            char t_shortcode_[5] = "";
            if (strlen(tokens_[1]) == 4) {
              strncpy(t_shortcode_, tokens_[1], 2);
              t_shortcode_[2] = '_';
              t_shortcode_[3] = '0';
              t_shortcode_[4] = '\0';

              if (shortcode_to_is_n2d_found_.find(t_shortcode_) != shortcode_to_is_n2d_found_.end() &&
                  !shortcode_to_is_n2d_found_[t_shortcode_]) {
                shortcode_to_is_n2d_found_[t_shortcode_] = true;

                contract_specification_map_[t_shortcode_].numbers_to_dollars_ =
                    atof(tokens_[2]) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
                contract_specification_map_[t_shortcode_].numbers_to_dollars_ /=
                    contract_specification_map_[t_shortcode_].min_price_increment_;
              }
            }
          }
        }

        tick_value_file_.close();
      }
    }

    bool are_all_n2ds_found_ = true;
    for (std::map<std::string, bool>::iterator _itr_ = shortcode_to_is_n2d_found_.begin();
         _itr_ != shortcode_to_is_n2d_found_.end(); ++_itr_) {
      if (!_itr_->second) {
        are_all_n2ds_found_ = false;
        break;
      }
    }

    if (are_all_n2ds_found_) {
      break;
    }

    t_intdate_ = DateTime::CalcPrevDay(t_intdate_);
  }
}

void SecurityDefinitions::GetNonSelfEnabledSecurities() {
  std::ifstream t_non_self_securities_file_;
  t_non_self_securities_file_.open("/spare/local/tradeinfo/non_self_securities.txt", std::ifstream::in);

  if (t_non_self_securities_file_.is_open()) {
    const int kL1AvgBufferLen = 1024;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);

    while (t_non_self_securities_file_.good()) {
      bzero(readline_buffer_, kL1AvgBufferLen);
      t_non_self_securities_file_.getline(readline_buffer_, kL1AvgBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);

      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() > 0) {
        if (strncmp(tokens_[0], "NSE_", 4) == 0) continue;
        non_self_securities_.push_back(std::string(tokens_[0]));
      }
    }
  }
}

void SecurityDefinitions::ResetDI1MPI() {
  // get current month && current year
  int c_month_ = ((tradingdate_ / 100) % 100);
  int c_year_ = ((tradingdate_ / 10000) % 100);
  const std::string ExchMonthCode("FGHJKMNQUVXZ");

  std::string t_shc_;
  int i = 0;
  for (; i < 3; i++) {
    if (c_month_ < 12) {
      c_month_++;
      t_shc_ = std::string("DI1") + ExchMonthCode[c_month_ - 1] + std::to_string(c_year_);
      if (contract_specification_map_.find(t_shc_) != contract_specification_map_.end()) {
        contract_specification_map_[t_shc_].min_price_increment_ = 0.001;
      }
    } else {
      c_month_ = 1;
      c_year_++;
      t_shc_ = std::string("DI1") + ExchMonthCode[c_month_ - 1] + std::to_string(c_year_);
      if (contract_specification_map_.find(t_shc_) != contract_specification_map_.end()) {
        contract_specification_map_[t_shc_].min_price_increment_ = 0.001;
      }
    }
  }

  for (; i < 12; i++) {
    if (c_month_ < 12) {
      c_month_++;
      t_shc_ = std::string("DI1") + ExchMonthCode[c_month_ - 1] + std::to_string(c_year_);
      if (contract_specification_map_.find(t_shc_) != contract_specification_map_.end()) {
        contract_specification_map_[t_shc_].min_price_increment_ = 0.005;
      }
    } else {
      c_month_ = 1;
      c_year_++;
      t_shc_ = std::string("DI1") + ExchMonthCode[c_month_ - 1] + std::to_string(c_year_);
      if (contract_specification_map_.find(t_shc_) != contract_specification_map_.end()) {
        contract_specification_map_[t_shc_].min_price_increment_ = 0.005;
      }
    }
  }
}

bool SecurityDefinitions::_GetRemoveSelfOrdersFromBook(const std::string& _shortcode_) {
  for (size_t i = 0; i < non_self_securities_.size(); i++) {
    if (non_self_securities_[i] == _shortcode_) {
      return true;
    }
  }

  return false;
}

int SecurityDefinitions::_GetConfToMarketUpdateMsecs(
    const std::string& _shortcode_) {  // These should be the default values. TODO : Make this a param and optimize it.
  if (!_shortcode_.compare("BR_DOL_0")) {
    return 2;
  }
  if (!_shortcode_.compare("CGB_0")) {
    return 2;
  }
  return 0;
}

bool SecurityDefinitions::_IsTimeProRataSimShortcode(const std::string& _shortcode_) {
  if (!_shortcode_.compare("LFI_0") || !_shortcode_.compare("LFI_1") || !_shortcode_.compare("LFI_2") ||
      !_shortcode_.compare("LFI_3") || !_shortcode_.compare("LFI_4") || !_shortcode_.compare("LFI_5") ||
      !_shortcode_.compare("LFI_6") || !_shortcode_.compare("LFI_7") || !_shortcode_.compare("LFI_8") ||
      !_shortcode_.compare("LFI_9") || !_shortcode_.compare("LFI_10") || !_shortcode_.compare("LFI_11") ||
      !_shortcode_.compare("LFI_12") || !_shortcode_.compare("LFI_13") || !_shortcode_.compare("LFI_14") ||
      !_shortcode_.compare("LFI_15") || !_shortcode_.compare("LFI_16") || !_shortcode_.compare("LFI_17") ||
      !_shortcode_.compare("LFI_18") || !_shortcode_.compare("LFI_19") || !_shortcode_.compare("SP_LFI0_LFI1") ||
      !_shortcode_.compare("SP_LFI1_LFI2") || !_shortcode_.compare("SP_LFI2_LFI3") ||
      !_shortcode_.compare("SP_LFI3_LFI4") || !_shortcode_.compare("SP_LFI4_LFI5") ||
      !_shortcode_.compare("SP_LFI5_LFI6") || !_shortcode_.compare("SP_LFI6_LFI7") ||
      !_shortcode_.compare("SP_LFI7_LFI8") || !_shortcode_.compare("SP_LFI8_LFI9") ||
      !_shortcode_.compare("SP_LFI9_LFI10") || !_shortcode_.compare("SP_LFI10_LFI11") ||
      !_shortcode_.compare("SP_LFI11_LFI12") || !_shortcode_.compare("SP_LFI0_LFI2") ||
      !_shortcode_.compare("SP_LFI1_LFI3") || !_shortcode_.compare("SP_LFI2_LFI4") ||
      !_shortcode_.compare("SP_LFI3_LFI5") || !_shortcode_.compare("SP_LFI4_LFI6") ||
      !_shortcode_.compare("SP_LFI5_LFI7") || !_shortcode_.compare("SP_LFI6_LFI8") ||
      !_shortcode_.compare("SP_LFI7_LFI9") || !_shortcode_.compare("SP_LFI8_LFI10") ||
      !_shortcode_.compare("SP_LFI9_LFI11") || !_shortcode_.compare("SP_LFI10_LFI12") ||
      !_shortcode_.compare("B_LFI0_LFI1_LFI2") || !_shortcode_.compare("B_LFI1_LFI2_LFI3") ||
      !_shortcode_.compare("B_LFI2_LFI3_LFI4") || !_shortcode_.compare("B_LFI3_LFI4_LFI5") ||
      !_shortcode_.compare("B_LFI4_LFI5_LFI6")) {
    return true;
  }

  return false;
}

bool SecurityDefinitions::_IsNewTimeProRataSimShortcode(const std::string& _shortcode_) {
  // LFL and EuroSwiss should be here
  if (!_shortcode_.compare("LFL_0") || !_shortcode_.compare("LFL_1") || !_shortcode_.compare("LFL_2") ||
      !_shortcode_.compare("LFL_3") || !_shortcode_.compare("LFL_4") || !_shortcode_.compare("LFL_5") ||
      !_shortcode_.compare("LFL_6") || !_shortcode_.compare("LFL_7") || !_shortcode_.compare("LFL_8") ||
      !_shortcode_.compare("LFL_9") || !_shortcode_.compare("LFL_10") || !_shortcode_.compare("LFL_11") ||
      !_shortcode_.compare("LFL_12") || !_shortcode_.compare("LFL_13") || !_shortcode_.compare("SP_LFL0_LFL1") ||
      !_shortcode_.compare("SP_LFL1_LFL2") || !_shortcode_.compare("SP_LFL2_LFL3") ||
      !_shortcode_.compare("SP_LFL3_LFL4") || !_shortcode_.compare("SP_LFL4_LFL5") ||
      !_shortcode_.compare("SP_LFL5_LFL6") || !_shortcode_.compare("SP_LFL6_LFL7") ||
      !_shortcode_.compare("SP_LFL7_LFL8") || !_shortcode_.compare("SP_LFL8_LFL9") ||
      !_shortcode_.compare("SP_LFL9_LFL10")) {
    return true;
  }

  return false;
}

bool SecurityDefinitions::_IsSimpleProRataSimShortcode(const std::string& _shortcode_) {
  if (!_shortcode_.compare("GE_0") || !_shortcode_.compare("GE_1") || !_shortcode_.compare("GE_2") ||
      !_shortcode_.compare("GE_3") || !_shortcode_.compare("GE_4") || !_shortcode_.compare("GE_5") ||
      !_shortcode_.compare("GE_6") || !_shortcode_.compare("GE_7") || !_shortcode_.compare("GE_8") ||
      !_shortcode_.compare("GE_9") || !_shortcode_.compare("GE_10") || !_shortcode_.compare("GE_11") ||
      !_shortcode_.compare("GE_12") || !_shortcode_.compare("GE_13") || !_shortcode_.compare("GE_14") ||
      !_shortcode_.compare("GE_15") || !_shortcode_.compare("GE_16") || !_shortcode_.compare("SP_GE0_GE1") ||
      !_shortcode_.compare("SP_GE1_GE2") || !_shortcode_.compare("SP_GE2_GE3") || !_shortcode_.compare("SP_GE3_GE4") ||
      !_shortcode_.compare("SP_GE4_GE5") || !_shortcode_.compare("SP_GE5_GE6") || !_shortcode_.compare("SP_GE6_GE7") ||
      !_shortcode_.compare("SP_GE7_GE8") || !_shortcode_.compare("SP_GE8_GE9") || !_shortcode_.compare("SP_GE9_GE10") ||
      !_shortcode_.compare("SP_GE10_GE11") || !_shortcode_.compare("SP_GE11_GE12") ||
      !_shortcode_.compare("SP_GE12_GE13") || !_shortcode_.compare("SP_GE13_GE14") ||
      !_shortcode_.compare("SP_GE14_GE15") || !_shortcode_.compare("SP_GE15_GE16")) {
    return true;
  }

  return false;
}

bool SecurityDefinitions::_IsSplitFIFOProRataSimShortcode(const std::string& _shortcode_) {
  if (!_shortcode_.compare("ZT_0") || !_shortcode_.compare("ZT_1")) {
    return true;
  }
  return false;
}

bool SecurityDefinitions::_IsICEProRataSimShortcodeLFI(const std::string& _shortcode_) {
  if (_shortcode_.substr(0, 3) == "LFI") {
    return true;
  } else {
    return false;
  }
}

bool SecurityDefinitions::_IsICEProRataSimShortcodeLFL(const std::string& _shortcode_) {
  if (_shortcode_.substr(0, 3) == "LFL") {
    return true;
  } else {
    return false;
  }
}

/**
 * Return the percentage of trade size that's filled FIFO
 * @param _shortcode_
 * @return
 */
double SecurityDefinitions::_GetFIFOPercentageForSimShortcode(const std::string& _shortcode_) {
  if (!_shortcode_.compare("ZT_0") || !_shortcode_.compare("ZT_1")) {
    return 0.4;
  }
  if (_shortcode_.substr(0, 3) == "GE_" || _shortcode_.substr(0, 4) == "LFI_" || _shortcode_.substr(0, 4) == "LFL_") {
    return 0.0;
  } else {
    return 1.0;
  }
}

bool SecurityDefinitions::_GetTradeBeforeQuote(const std::string& _shortcode_) {
  // TODO set this based on research
  switch (GetContractExchSource(_shortcode_, tradingdate_)) {
    case kExchSourceEUREX: {
      return true;
    } break;
    case kExchSourceCME: {
      return true;
    } break;
    case kExchSourceLIFFE: {
      if ((tradingdate_ >= 20141020 &&
           (_shortcode_.find("LFR") != std::string::npos || _shortcode_.find("LFL") != std::string::npos)) ||
          (tradingdate_ >= 20141103 && (_shortcode_.find("LFI") != std::string::npos))) {
        return true;
      }
      return false;
    } break;
    case kExchSourceICE: {
      if ((tradingdate_ >= 20141020 &&
           (_shortcode_.find("LFR") != std::string::npos || _shortcode_.find("LFL") != std::string::npos)) ||
          (tradingdate_ >= 20141103 && (_shortcode_.find("LFI") != std::string::npos))) {
        return true;
      }
      return false;
    } break;
    case kExchSourceNASDAQ: {
      return false;
    } break;
    case kExchSourceESPEED: {
      return false;
    } break;
    case kExchSourceJPY: {
      if ((_shortcode_ == "JGBL_0" || _shortcode_ == "JGBL_1" || _shortcode_ == "TOPIX_0" ||
           _shortcode_ == "TOPIX_1") &&
          tradingdate_ < 20140320) {
        return false;
      } else {
        return true;
      }
    } break;

    default: { return true; }
  }

  return true;
}

bool SecurityDefinitions::_ModifyAfterPartialExecAllowed(const std::string& _shortcode_) {
  switch (GetContractExchSource(_shortcode_, tradingdate_)) {
    case kExchSourceNSE:
    case kExchSourceMICEX:
    case kExchSourceMICEX_CR:
    case kExchSourceMICEX_EQ:
    case kExchSourceASX: {
      return false;
    } break;
    default:
      return true;
  }
}

bool SecurityDefinitions::IsQuincyFeedAvailableForShortcode(const std::string& _shortcode_) {
  if (_shortcode_ == "6A_0" || _shortcode_ == "6B_0" || _shortcode_ == "6C_0" || _shortcode_ == "6E_0" ||
      _shortcode_ == "6J_0" || _shortcode_ == "6M_0" || _shortcode_ == "6N_0" || _shortcode_ == "6S_0") {
    return true;
  }

  return false;
}

void SecurityDefinitions::LoadBSESecurityDefinitions() {
  HFSAT::BSESecurityDefinitions::GetUniqueInstance(tradingdate_).AddContractSpecifications(contract_specification_map_);
}
bool SecurityDefinitions::IsDIHoliday(int yyyymmdd, bool is_weekend_holiday) {
  return (HolidayManagerNoThrow::IsProductHoliday("DI1F13", yyyymmdd, is_weekend_holiday));
}

bool SecurityDefinitions::IsASXHoliday(int yyyymmdd, bool is_weekend_holiday) {
  return (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceASXStr, yyyymmdd, is_weekend_holiday));
}
}
