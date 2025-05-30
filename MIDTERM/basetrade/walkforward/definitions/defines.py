#!/usr/bin/env python

"""

"""

main_keys = ['cname']

irrelvant_keys = ['min_volume', 'sort_algo', 'max_ttc', 'sign_check', 'skip_days']

regime_keys = ['regime_indicator', 'num_regimes']

pnl_modelling = ['user_matrix', 'thres_step_optim', 'max_sum', 'choose_top_strats']

type3_keys = ['strat_type', 'param_list', 'shortcode', 'start_time', 'event_token',
              'config_type', 'query_id', 'end_time', 'model_list', 'execlogic']
type4_keys = ['param_lookback_days']
type5_keys = ['sample_feature', 'feature_switch_threshold', 'feature_lookback_days']
type6_keys = ['walk_start_date', 'ddays_string', 'trigger_string', 'td_string',
              'rd_string', 'rdata_process_string', 'reg_string', 'model_process_string']

structured_keys = ['sub_model_list', 'sub_param_list', 'sub_strat_list', 'shortcode_list']

list_of_keys = irrelvant_keys + regime_keys + pnl_modelling + \
    type3_keys + type4_keys + type5_keys + type6_keys + structured_keys + main_keys

exec_logic_dict = {"ArbTrading": "at",
                   "ArbTradingTodTom": "attt",
                   "BaseMultipleTrading": "bmt",
                   "BaseOTrading": "bot",
                   "DesiredPositionTrading": "dpt",
                   "DirectionalAggressiveTrading": "dat",
                   "DirectionalAggressiveTradingModify": "datm",
                   "DirectionalAggressiveTradingSingleOrder": "datso",
                   "DirectionalInterventionAggressiveTrading": "diat",
                   "DirectionalInterventionLogisticTrading": "dilt",
                   "DirectionalLogisticTrading": "dlt",
                   "DirectionalPairAggressiveTrading": "dpat",
                   "DirectionalSyntheticTrading": "dst",
                   "EquityTrading2": "et2",
                   "EventBiasAggressiveTrading": "ebat",
                   "EventDirectionalAggressiveTrading": "edat",
                   "EventPriceBasedAggressiveTrading": "epbat",
                   "FEU3MM": "feu3mm",
                   "FillTimeLogger": "ftl",
                   "ImpliedDirectionalAggressiveTrading": "idat",
                   "ImpliedPricePairAggressiveTrading": "ippat",
                   "JP400PMM": "jp400mm",
                   "MinRiskPriceBasedAggressiveTrading": "mrpbat",
                   "NikPricePairBasedAggressiveTrading": "nppbat",
                   "NKTrader": "nkt",
                   "OSEPMM": "osepmm",
                   "PairsTrading": "pt",
                   "PriceBasedAggressiveScalper": "pbas",
                   "PriceBasedAggressiveTrading2": "pbat2",
                   "PriceBasedAggressiveTrading": "pbat",
                   "PriceBasedAggressiveProRataTrading": "pbaprt",
                   "PriceBasedInterventionAggressiveTrading": "pbiat",
                   "PriceBasedLegTrading": "pblt",
                   "PriceBasedScalper": "pbs",
                   "PriceBasedSecurityAggressiveTrading": "pbsat",
                   "PriceBasedSpreadTrading": "pbst",
                   "PriceBasedTrading": "pbt",
                   "PriceBasedVolTrading": "pbvt",
                   "PricePairBasedAggressiveTrading": "ppbat",
                   "ProjectedPricePairBasedAggressiveTrading": "pppbat",
                   "RegimeBasedVolDatTrading": "rbvdt",
                   "RegimeTrading": "rt",
                   "RetailFlyTrading": "rft",
                   "RetailTrading": "rt",
                   "ReturnsBasedAggressiveTrading": "rbat",
                   "RiskBasedStructuredTrading": "rbst",
                   "SGXNiftyPricePairAggressiveTrading": "sgxnppat",
                   "SgxNK1MMTrading": "sgxnkmmt",
                   "SGXNKPricePairAggressiveTrading": "sgxnkppat",
                   "SgxNKSpreadMMTrading": "sgxnksmt",
                   "StructuredPriceBasedAggressiveTrading": "spbat",
                   "TradeBasedAggressiveTrading": "tbat",
                   "VolMMTrading": "vmmt",
                   "IndexFuturesMeanRevertingTrading": "mrt",
                   "DI1TradingManager-StructuredGeneralTrading-PriceBasedAggressiveTrading": "di1cbtpbat",
                   "DI1TradingManager-StructuredGeneralTrading-DirectionalAggressiveTrading": "di1cbtdat"}
