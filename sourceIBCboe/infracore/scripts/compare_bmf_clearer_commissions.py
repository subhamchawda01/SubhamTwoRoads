#!/usr/bin/env python

import sys

def __main__() :
    if len ( sys.argv ) < 4 :
        print ("Usage: DOL IND WIN DI");
        sys.exit()

    date_to_DOL_volume_map_ = {}
    date_to_IND_volume_map_ = {}
    date_to_DI_volume_map_ = {}
    date_to_WIN_volume_map_ = {}

    month_to_DOL_volume_ = {}
    month_to_IND_volume_ = {}
    month_to_WIN_volume_ = {}
    month_to_DI_volume_ = {}

    with open(sys.argv[1]) as DOL_v_file_handle_ :
        for DOL_v_line_ in DOL_v_file_handle_ :
            DOL_v_line_words_ = DOL_v_line_.strip().split()
            if len ( DOL_v_line_words_ ) >= 2 :
                tradingdate_ = (int)(DOL_v_line_words_[0])
                date_to_DOL_volume_map_[tradingdate_] = (int)(DOL_v_line_words_[1])
                yyyymm_ = (int)(tradingdate_/100)
                if yyyymm_ not in month_to_DOL_volume_ :
                    month_to_DOL_volume_ [ yyyymm_ ] = 0

                month_to_DOL_volume_ [ yyyymm_ ] += date_to_DOL_volume_map_ [ tradingdate_ ]

    with open(sys.argv[2]) as IND_v_file_handle_ :
        for IND_v_line_ in IND_v_file_handle_ :
            IND_v_line_words_ = IND_v_line_.strip().split()
            if len ( IND_v_line_words_ ) >= 2 :
                tradingdate_ = (int)(IND_v_line_words_[0])
                date_to_IND_volume_map_[tradingdate_] = (int)(IND_v_line_words_[1])
                yyyymm_ = (int)(tradingdate_/100)
                if yyyymm_ not in month_to_IND_volume_ :
                    month_to_IND_volume_ [ yyyymm_ ] = 0

                month_to_IND_volume_ [ yyyymm_ ] += date_to_IND_volume_map_ [ tradingdate_ ]

    with open(sys.argv[3]) as WIN_v_file_handle_ :
        for WIN_v_line_ in WIN_v_file_handle_ :
            WIN_v_line_words_ = WIN_v_line_.strip().split()
            if len ( WIN_v_line_words_ ) >= 2 :
                tradingdate_ = (int)(WIN_v_line_words_[0])
                date_to_WIN_volume_map_[tradingdate_] = (int)(WIN_v_line_words_[1])
                yyyymm_ = (int)(tradingdate_/100)
                if yyyymm_ not in month_to_WIN_volume_ :
                    month_to_WIN_volume_ [ yyyymm_ ] = 0

                month_to_WIN_volume_ [ yyyymm_ ] += date_to_WIN_volume_map_ [ tradingdate_ ]

    with open(sys.argv[4]) as DI_v_file_handle_ :
        for DI_v_line_ in DI_v_file_handle_ :
            DI_v_line_words_ = DI_v_line_.strip().split()
            if len ( DI_v_line_words_ ) >= 2 :
                tradingdate_ = (int)(DI_v_line_words_[0])
                date_to_DI_volume_map_[tradingdate_] = (int)(DI_v_line_words_[1])
                yyyymm_ = (int)(tradingdate_/100)
                if yyyymm_ not in month_to_DI_volume_ :
                    month_to_DI_volume_ [ yyyymm_ ] = 0

                month_to_DI_volume_ [ yyyymm_ ] += date_to_DI_volume_map_ [ tradingdate_ ]

    all_dates_ = list ( set(date_to_DOL_volume_map_.keys()) | set(date_to_IND_volume_map_.keys()) | set(date_to_WIN_volume_map_.keys()) | set(date_to_DI_volume_map_.keys()) )

    savings_ = 0
    for yyyymm_ in sorted ( month_to_DOL_volume_.keys() ) :
        LINK_DOL_rate_ = 0.08 / 2
        LINK_IND_rate_ = 0.10 / 2
        LINK_WIN_rate_ = 0.02 / 2
        LINK_DI_rate_ = 0.05 / 2

        ALPES_DOL_rate_ = 0.05 / 2
        ALPES_IND_rate_ = 0.05 / 2
        ALPES_WIN_rate_ = 0.01 / 2
        ALPES_DI_rate_ = 0.01 / 2

        total_volume_ = 0
        if yyyymm_ in month_to_DOL_volume_.keys() :
            total_volume_ += month_to_DOL_volume_ [ yyyymm_ ]
        if yyyymm_ in month_to_IND_volume_.keys() :
            total_volume_ += month_to_IND_volume_ [ yyyymm_ ]
        if yyyymm_ in month_to_WIN_volume_.keys() :
            total_volume_ += month_to_WIN_volume_ [ yyyymm_ ] 
        if yyyymm_ in month_to_DI_volume_.keys() :
            total_volume_ += month_to_DI_volume_ [ yyyymm_ ] 

        # if total_volume_ >= 1000000 :
        #     LINK_DOL_rate_ = 0.02
        #     LINK_IND_rate_ = 0.02
        #     LINK_WIN_rate_ = 0.08
        #     LINK_DI_rate_ = 0.02
        # else :
        #     if total_volume_ >= 500000 :
        #         LINK_DOL_rate_ = 0.03
        #         LINK_IND_rate_ = 0.03
        #         LINK_WIN_rate_ = 0.08
        #         LINK_DI_rate_ = 0.03

        LINK_fees_ = 0
        ALPES_fees_ = 0
        if yyyymm_ in month_to_DOL_volume_.keys() :
            LINK_fees_ += LINK_DOL_rate_ * month_to_DOL_volume_ [ yyyymm_ ]
            ALPES_fees_ += ALPES_DOL_rate_ * month_to_DOL_volume_ [ yyyymm_ ]
        if yyyymm_ in month_to_IND_volume_.keys() :
            LINK_fees_ += LINK_IND_rate_ * month_to_IND_volume_ [ yyyymm_ ]
            ALPES_fees_ += ALPES_IND_rate_ * month_to_IND_volume_ [ yyyymm_ ]
        if yyyymm_ in month_to_WIN_volume_.keys() :
            LINK_fees_ += LINK_WIN_rate_ * month_to_WIN_volume_ [ yyyymm_ ]
            ALPES_fees_ += ALPES_WIN_rate_ * month_to_WIN_volume_ [ yyyymm_ ]
        if yyyymm_ in month_to_DI_volume_.keys() :
            LINK_fees_ += LINK_DI_rate_ * month_to_DI_volume_ [ yyyymm_ ]
            ALPES_fees_ += ALPES_DI_rate_ * month_to_DI_volume_ [ yyyymm_ ]

        print ( ""+str(yyyymm_)+" Volume: "+str(total_volume_)+" LINK: "+str(LINK_fees_)+" ALPES: "+str(ALPES_fees_)+" Diff: "+str(LINK_fees_ - ALPES_fees_) )
        savings_ += LINK_fees_ - ALPES_fees_

    print ( "Total Savings: "+str(savings_) )

__main__();
