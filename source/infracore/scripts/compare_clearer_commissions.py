#!/usr/bin/env python

import sys

def __main__() :
    if len ( sys.argv ) < 4 :
        print ("Usage: CME EUREX TMX LIFFE");
        sys.exit()

    date_to_CME_volume_map_ = {}
    date_to_EUREX_volume_map_ = {}
    date_to_LIFFE_volume_map_ = {}
    date_to_TMX_volume_map_ = {}

    month_to_CME_volume_ = {}
    month_to_EUREX_volume_ = {}
    month_to_TMX_volume_ = {}
    month_to_LIFFE_volume_ = {}
    month_to_day_list_ = {}

    with open(sys.argv[1]) as CME_v_file_handle_ :
        for CME_v_line_ in CME_v_file_handle_ :
            CME_v_line_words_ = CME_v_line_.strip().split()
            if len ( CME_v_line_words_ ) >= 2 :
                tradingdate_ = (int)(CME_v_line_words_[0])
                date_to_CME_volume_map_[tradingdate_] = (int)(CME_v_line_words_[1])
                yyyymm_ = (int)(tradingdate_/100)
                if yyyymm_ not in month_to_CME_volume_ :
                    month_to_CME_volume_ [ yyyymm_ ] = 0

                month_to_CME_volume_ [ yyyymm_ ] += date_to_CME_volume_map_ [ tradingdate_ ]
                if yyyymm_ not in month_to_day_list_ :
                    month_to_day_list_ [ yyyymm_ ] = []
                if tradingdate_ not in month_to_day_list_ [ yyyymm_ ] :
                    month_to_day_list_ [ yyyymm_ ].append ( tradingdate_ )

    with open(sys.argv[2]) as EUREX_v_file_handle_ :
        for EUREX_v_line_ in EUREX_v_file_handle_ :
            EUREX_v_line_words_ = EUREX_v_line_.strip().split()
            if len ( EUREX_v_line_words_ ) >= 2 :
                tradingdate_ = (int)(EUREX_v_line_words_[0])
                date_to_EUREX_volume_map_[tradingdate_] = (int)(EUREX_v_line_words_[1])
                yyyymm_ = (int)(tradingdate_/100)
                if yyyymm_ not in month_to_EUREX_volume_ :
                    month_to_EUREX_volume_ [ yyyymm_ ] = 0

                month_to_EUREX_volume_ [ yyyymm_ ] += date_to_EUREX_volume_map_ [ tradingdate_ ]
                if yyyymm_ not in month_to_day_list_ :
                    month_to_day_list_ [ yyyymm_ ] = []
                if tradingdate_ not in month_to_day_list_ [ yyyymm_ ] :
                    month_to_day_list_ [ yyyymm_ ].append ( tradingdate_ )

    with open(sys.argv[3]) as TMX_v_file_handle_ :
        for TMX_v_line_ in TMX_v_file_handle_ :
            TMX_v_line_words_ = TMX_v_line_.strip().split()
            if len ( TMX_v_line_words_ ) >= 2 :
                tradingdate_ = (int)(TMX_v_line_words_[0])
                date_to_TMX_volume_map_[tradingdate_] = (int)(TMX_v_line_words_[1])
                yyyymm_ = (int)(tradingdate_/100)
                if yyyymm_ not in month_to_TMX_volume_ :
                    month_to_TMX_volume_ [ yyyymm_ ] = 0

                month_to_TMX_volume_ [ yyyymm_ ] += date_to_TMX_volume_map_ [ tradingdate_ ]
                if yyyymm_ not in month_to_day_list_ :
                    month_to_day_list_ [ yyyymm_ ] = []
                if tradingdate_ not in month_to_day_list_ [ yyyymm_ ] :
                    month_to_day_list_ [ yyyymm_ ].append ( tradingdate_ )

    with open(sys.argv[4]) as LIFFE_v_file_handle_ :
        for LIFFE_v_line_ in LIFFE_v_file_handle_ :
            LIFFE_v_line_words_ = LIFFE_v_line_.strip().split()
            if len ( LIFFE_v_line_words_ ) >= 2 :
                tradingdate_ = (int)(LIFFE_v_line_words_[0])
                date_to_LIFFE_volume_map_[tradingdate_] = (int)(LIFFE_v_line_words_[1])
                yyyymm_ = (int)(tradingdate_/100)
                if yyyymm_ not in month_to_LIFFE_volume_ :
                    month_to_LIFFE_volume_ [ yyyymm_ ] = 0

                month_to_LIFFE_volume_ [ yyyymm_ ] += date_to_LIFFE_volume_map_ [ tradingdate_ ]
                if yyyymm_ not in month_to_day_list_ :
                    month_to_day_list_ [ yyyymm_ ] = []
                if tradingdate_ not in month_to_day_list_ [ yyyymm_ ] :
                    month_to_day_list_ [ yyyymm_ ].append ( tradingdate_ )

    all_dates_ = list ( set(date_to_CME_volume_map_.keys()) | set(date_to_EUREX_volume_map_.keys()) | set(date_to_TMX_volume_map_.keys()) | set(date_to_LIFFE_volume_map_.keys()) )

    savings_ = 0
    set_tmx_same_as_cme_ = 0
    for yyyymm_ in sorted ( month_to_CME_volume_.keys() ) :
        ABN_TMX_rate_ = 12 * 0.01
        ABN_CME_rate_ = 4 * 0.01
        ABN_EUREX_rate_ = 4 * 0.01
        ABN_LIFFE_rate_ = 4 * 0.01
        ABN_tier_one_ = 5*100000
        ABN_tier_two_ = 10*100000

        NE_CME_rate_ = 2 * 0.01 
        NE_EUREX_rate_ = 2 * 0.01 * 1.3
        NE_TMX_rate_ = 5 * 0.01 * 0.93
        NE_LIFFE_rate_ = 2 * 0.01 * 1.3

        total_volume_ = 0
        if yyyymm_ in month_to_CME_volume_.keys() :
            total_volume_ += month_to_CME_volume_ [ yyyymm_ ]
        if yyyymm_ in month_to_EUREX_volume_.keys() :
            total_volume_ += month_to_EUREX_volume_ [ yyyymm_ ]
        if yyyymm_ in month_to_TMX_volume_.keys() :
            total_volume_ += month_to_TMX_volume_ [ yyyymm_ ] 
        if yyyymm_ in month_to_LIFFE_volume_.keys() :
            total_volume_ += month_to_LIFFE_volume_ [ yyyymm_ ] 

        if yyyymm_ in month_to_day_list_ :
            if len ( month_to_day_list_ [ yyyymm_ ] ) < 18 :
                total_volume_ = ( total_volume_ * 22 ) / len ( month_to_day_list_ [ yyyymm_ ] ) 

        if total_volume_ >= ABN_tier_two_ :
            ABN_CME_rate_ = 0.02
            ABN_EUREX_rate_ = 0.02
            ABN_LIFFE_rate_ = 0.02
        else :
            if total_volume_ >= ABN_tier_one_ :
                ABN_CME_rate_ = 0.03
                ABN_EUREX_rate_ = 0.03
                ABN_LIFFE_rate_ = 0.03

        if set_tmx_same_as_cme_ == 1 : ABN_TMX_rate_ = ABN_CME_rate_

        ABN_fees_ = 0
        NE_fees_ = 0

        ABN_CME_fees_ = 0
        ABN_EUREX_fees_ = 0
        ABN_TMX_fees_ = 0
        ABN_LIFFE_fees_ = 0

        NE_CME_fees_ = 0
        NE_EUREX_fees_ = 0
        NE_TMX_fees_ = 0
        NE_LIFFE_fees_ = 0

        if yyyymm_ in month_to_CME_volume_.keys() :
#            if yyyymm_ > 201300 :
#                print ( "CMEV in %d %d" % ( yyyymm_, month_to_CME_volume_ [ yyyymm_ ] ) )
            ABN_CME_fees_ = ABN_CME_rate_ * month_to_CME_volume_ [ yyyymm_ ]
            NE_CME_fees_ = NE_CME_rate_ * month_to_CME_volume_ [ yyyymm_ ]
        if yyyymm_ in month_to_EUREX_volume_.keys() :
#            if yyyymm_ > 201300 :
#                print ( "EUREXV in %d %d" % ( yyyymm_, month_to_EUREX_volume_ [ yyyymm_ ] ) )
            ABN_EUREX_fees_ = ABN_EUREX_rate_ * month_to_EUREX_volume_ [ yyyymm_ ]
            NE_EUREX_fees_ = NE_EUREX_rate_ * month_to_EUREX_volume_ [ yyyymm_ ]
        if yyyymm_ in month_to_TMX_volume_.keys() :
#            if yyyymm_ > 201300 :
#                print ( "TMXV in %d %d" % ( yyyymm_, month_to_TMX_volume_ [ yyyymm_ ] ) )
            ABN_TMX_fees_ = ABN_TMX_rate_ * month_to_TMX_volume_ [ yyyymm_ ]
            NE_TMX_fees_ = NE_TMX_rate_ * month_to_TMX_volume_ [ yyyymm_ ]
        if yyyymm_ in month_to_LIFFE_volume_.keys() :
#            if yyyymm_ > 201300 :
#                print ( "LIFFEV in %d %d" % ( yyyymm_, month_to_LIFFE_volume_ [ yyyymm_ ] ) )
            ABN_LIFFE_fees_ = ABN_LIFFE_rate_ * month_to_LIFFE_volume_ [ yyyymm_ ]
            NE_LIFFE_fees_ = NE_LIFFE_rate_ * month_to_LIFFE_volume_ [ yyyymm_ ]

        ABN_fees_ = ABN_CME_fees_ + ABN_EUREX_fees_ + ABN_TMX_fees_ + ABN_LIFFE_fees_
        NE_fees_ = NE_CME_fees_ + NE_EUREX_fees_ + NE_TMX_fees_ + NE_LIFFE_fees_

        diff_abn_minus_newedge_ = ABN_fees_ - NE_fees_
        if yyyymm_ in month_to_day_list_ :
            if len ( month_to_day_list_ [ yyyymm_ ] ) < 18 :
                diff_abn_minus_newedge_ = ( diff_abn_minus_newedge_ * 22 ) / len ( month_to_day_list_ [ yyyymm_ ] ) 

        print ( "%s Volume: %8d ABN: %6d NE: %6d Diff: %6d BreakDown: %6d %6d %6d %6d" % ( yyyymm_, total_volume_, ABN_fees_, NE_fees_, diff_abn_minus_newedge_, (ABN_CME_fees_-NE_CME_fees_), (ABN_EUREX_fees_-NE_EUREX_fees_), (ABN_TMX_fees_-NE_TMX_fees_), (ABN_LIFFE_fees_-NE_LIFFE_fees_), ) )
        savings_ += ABN_fees_ - NE_fees_

    print ( "Total Savings: %d" %(savings_) )

__main__();
