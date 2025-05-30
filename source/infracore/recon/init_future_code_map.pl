#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

sub GetFuturecodeToShortcodeMap
{
	my %ne_exch_future_code_to_shortcode_ = ();
    
	$ne_exch_future_code_to_shortcode_{"22NI"} = "NK";
    $ne_exch_future_code_to_shortcode_{"22MN"} = "NKM";
    $ne_exch_future_code_to_shortcode_{"24RV"} = "JGBL";
    $ne_exch_future_code_to_shortcode_{"24TP"} = "TOPIX";
    $ne_exch_future_code_to_shortcode_{"22N4"} = "JP400";
    $ne_exch_future_code_to_shortcode_{"27SD"} = "FGBS";
    $ne_exch_future_code_to_shortcode_{"27BM"} = "FGBL";
    $ne_exch_future_code_to_shortcode_{"27BC"} = "FGBM";
    $ne_exch_future_code_to_shortcode_{"27VS"} = "FVS";
    $ne_exch_future_code_to_shortcode_{"27SF"} = "FESX";
    $ne_exch_future_code_to_shortcode_{"27I*"} = "FOAT";
    $ne_exch_future_code_to_shortcode_{"27FE"} = "FDAX";
    $ne_exch_future_code_to_shortcode_{"27>O"} = "FXXP";
    $ne_exch_future_code_to_shortcode_{"27F7"} = "FBTP";
    $ne_exch_future_code_to_shortcode_{"27FA"} = "FBTS";
    $ne_exch_future_code_to_shortcode_{"27BV"} = "FGBX";
    $ne_exch_future_code_to_shortcode_{"30CA"} = "JFFCE";
    $ne_exch_future_code_to_shortcode_{"28EJ"} = "KFFTI";
    $ne_exch_future_code_to_shortcode_{"05SF"} = "LFI";
    $ne_exch_future_code_to_shortcode_{"05RJ"} = "LFL";
    $ne_exch_future_code_to_shortcode_{"05RH"} = "LFR";
    $ne_exch_future_code_to_shortcode_{"05FT"} = "LFZ";
    $ne_exch_future_code_to_shortcode_{"25B3"} = "YFEBM";
    $ne_exch_future_code_to_shortcode_{"14LA"} = "XFC";
    $ne_exch_future_code_to_shortcode_{"14RC"} = "XFRC";
    $ne_exch_future_code_to_shortcode_{"0125"} = "ZF";
    $ne_exch_future_code_to_shortcode_{"0121"} = "ZN";
    $ne_exch_future_code_to_shortcode_{"0126"} = "ZT";
    $ne_exch_future_code_to_shortcode_{"01TN"} = "TN";
    $ne_exch_future_code_to_shortcode_{"07BZ"} = "BZ";
    $ne_exch_future_code_to_shortcode_{"16N1"} = "NIY";
    $ne_exch_future_code_to_shortcode_{"16NK"} = "NKD";
    $ne_exch_future_code_to_shortcode_{"01CBT 2YR T-NOTE"} = "ZT";
    $ne_exch_future_code_to_shortcode_{"0117"} = "ZB";
    $ne_exch_future_code_to_shortcode_{"01UL"} = "UB";
    $ne_exch_future_code_to_shortcode_{"10BA"} = "BAX";
    $ne_exch_future_code_to_shortcode_{"10CG"} = "CGB";
    $ne_exch_future_code_to_shortcode_{"10CF"} = "CGF";
    $ne_exch_future_code_to_shortcode_{"10CZ"} = "CGZ";
    $ne_exch_future_code_to_shortcode_{"10SX"} = "SXF";
    $ne_exch_future_code_to_shortcode_{"7BVX"} = "VX";
    $ne_exch_future_code_to_shortcode_{"16ES"} = "ES";
    $ne_exch_future_code_to_shortcode_{"16ED"} = "GE";
    $ne_exch_future_code_to_shortcode_{"16MP"} = "6M";
    $ne_exch_future_code_to_shortcode_{"16AD"} = "6A";
    $ne_exch_future_code_to_shortcode_{"16C1"} = "6C";
    $ne_exch_future_code_to_shortcode_{"16J1"} = "6J";
    $ne_exch_future_code_to_shortcode_{"01KW"} = "KE";
    $ne_exch_future_code_to_shortcode_{"33MH"} = "MHI";
    $ne_exch_future_code_to_shortcode_{"33HC"} = "HHI";
    $ne_exch_future_code_to_shortcode_{"33HS"} = "HSI";
    $ne_exch_future_code_to_shortcode_{"07CU"} = "CL";
    $ne_exch_future_code_to_shortcode_{"07RB"} = "RB";
    $ne_exch_future_code_to_shortcode_{"07HO"} = "HO";
    $ne_exch_future_code_to_shortcode_{"17NK"} = "NK";
    $ne_exch_future_code_to_shortcode_{"17CN"} = "CN";
    $ne_exch_future_code_to_shortcode_{"17IN"} = "IN";
    $ne_exch_future_code_to_shortcode_{"17IU"} = "IU";
    $ne_exch_future_code_to_shortcode_{"17AU"} = "AU";
    $ne_exch_future_code_to_shortcode_{"17AJ"} = "AJ";
    $ne_exch_future_code_to_shortcode_{"17TW"} = "TW";
    
    return %ne_exch_future_code_to_shortcode_;
}

sub GetFuturecodeToExchangeMap
{
    my %ne_exch_future_code_to_exchange_ = ();

    $ne_exch_future_code_to_exchange_{"22NI"} = "OSE";
    $ne_exch_future_code_to_exchange_{"22MN"} = "OSE";
    $ne_exch_future_code_to_exchange_{"24RV"} = "OSE";
    $ne_exch_future_code_to_exchange_{"24TP"} = "OSE";
    $ne_exch_future_code_to_exchange_{"22N4"} = "OSE";
    $ne_exch_future_code_to_exchange_{"27SD"} = "EUREX";
    $ne_exch_future_code_to_exchange_{"27BC"} = "EUREX";
    $ne_exch_future_code_to_exchange_{"27VS"} = "EUREX";
    $ne_exch_future_code_to_exchange_{"27BM"} = "EUREX";
    $ne_exch_future_code_to_exchange_{"27SF"} = "EUREX";
    $ne_exch_future_code_to_exchange_{"27I*"} = "EUREX";
    $ne_exch_future_code_to_exchange_{"27FE"} = "EUREX";
    $ne_exch_future_code_to_exchange_{"27F7"} = "EUREX";
    $ne_exch_future_code_to_exchange_{"27FA"} = "EUREX";
    $ne_exch_future_code_to_exchange_{"27BV"} = "EUREX";
    $ne_exch_future_code_to_exchange_{"27>O"} = "EUREX";
    $ne_exch_future_code_to_exchange_{"30CA"} = "LIFFE";
    $ne_exch_future_code_to_exchange_{"28EJ"} = "LIFFE";
    $ne_exch_future_code_to_exchange_{"05SF"} = "LIFFE";
    $ne_exch_future_code_to_exchange_{"05RJ"} = "LIFFE";
    $ne_exch_future_code_to_exchange_{"05RH"} = "LIFFE";
    $ne_exch_future_code_to_exchange_{"05FT"} = "LIFFE";
    $ne_exch_future_code_to_exchange_{"25B3"} = "LIFFE";
    $ne_exch_future_code_to_exchange_{"14LA"} = "LIFFE";
    $ne_exch_future_code_to_exchange_{"14RC"} = "LIFFE";
    $ne_exch_future_code_to_exchange_{"0125"} = "CME";
    $ne_exch_future_code_to_exchange_{"0121"} = "CME";
    $ne_exch_future_code_to_exchange_{"0126"} = "CME";
    $ne_exch_future_code_to_exchange_{"16N1"} = "CME";  # one chicago 
    $ne_exch_future_code_to_exchange_{"16NK"} = "CME"; # one chicago 
    $ne_exch_future_code_to_exchange_{"01CBT 2YR T-NOTE"} = "CME";
    $ne_exch_future_code_to_exchange_{"0117"} = "CME";
    $ne_exch_future_code_to_exchange_{"01UL"} = "CME";
    $ne_exch_future_code_to_exchange_{"07CU"} = "CME";
    $ne_exch_future_code_to_exchange_{"07RB"} = "CME";
    $ne_exch_future_code_to_exchange_{"07HO"} = "CME";
    $ne_exch_future_code_to_exchange_{"01TN"} = "CME";
    $ne_exch_future_code_to_exchange_{"07BZ"} = "CME";
    $ne_exch_future_code_to_exchange_{"10BA"} = "TMX";
    $ne_exch_future_code_to_exchange_{"10CG"} = "TMX";
    $ne_exch_future_code_to_exchange_{"10CF"} = "TMX";
    $ne_exch_future_code_to_exchange_{"10CZ"} = "TMX";
    $ne_exch_future_code_to_exchange_{"10SX"} = "TMX";
    $ne_exch_future_code_to_exchange_{"7BVX"} = "CFE";
    $ne_exch_future_code_to_exchange_{"16ES"} = "CME";
    $ne_exch_future_code_to_exchange_{"16ED"} = "CME";
    $ne_exch_future_code_to_exchange_{"16MP"} = "CME";
    $ne_exch_future_code_to_exchange_{"16AD"} = "CME";
    $ne_exch_future_code_to_exchange_{"16C1"} = "CME";
    $ne_exch_future_code_to_exchange_{"16J1"} = "CME";
    $ne_exch_future_code_to_exchange_{"01KW"} = "CME";
    $ne_exch_future_code_to_exchange_{"33MH"} = "HKEX";
    $ne_exch_future_code_to_exchange_{"33HC"} = "HKEX";
    $ne_exch_future_code_to_exchange_{"33HS"} = "HKEX";
    $ne_exch_future_code_to_exchange_{"17NK"} = "SGX";
    $ne_exch_future_code_to_exchange_{"17CN"} = "SGX";
    $ne_exch_future_code_to_exchange_{"17IN"} = "SGX";
    $ne_exch_future_code_to_exchange_{"17IU"} = "SGX";
    $ne_exch_future_code_to_exchange_{"17AU"} = "SGX";
    $ne_exch_future_code_to_exchange_{"17AJ"} = "SGX";
    $ne_exch_future_code_to_exchange_{"17TW"} = "SGX";
    
    return %ne_exch_future_code_to_exchange_;
}

sub GetFutureCodeToTradingHoursMap
{
	my %sgx_exch_future_code_to_trading_hours_=();
	$sgx_exch_future_code_to_trading_hours_{"17NK"} = [ 73000, 142500, 151500 ];
	$sgx_exch_future_code_to_trading_hours_{"17CN"} = [ 90000, 163000, 171500 ];
	$sgx_exch_future_code_to_trading_hours_{"17IN"} = [ 90000, 181000, 191500 ];
	$sgx_exch_future_code_to_trading_hours_{"17IU"} = [ 74000, 194000, 201500 ];
	$sgx_exch_future_code_to_trading_hours_{"17AU"} = [ 74000, 194000, 201500 ];
	$sgx_exch_future_code_to_trading_hours_{"17AJ"} = [ 74000, 194000, 201500 ];
	$sgx_exch_future_code_to_trading_hours_{"17TW"} = [ 84500, 134500, 143500 ];
	
	return %sgx_exch_future_code_to_trading_hours_;
}	

1;