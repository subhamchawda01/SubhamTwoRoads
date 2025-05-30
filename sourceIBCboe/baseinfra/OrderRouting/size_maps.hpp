/**
    \file OrderRouting/size_maps.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_ORDERROUTING_SIZE_MAPS_HPP
#define BASE_ORDERROUTING_SIZE_MAPS_HPP

namespace HFSAT {
typedef std::map<int, int, std::greater<int> > BidPriceSizeMap;
typedef std::map<int, int, std::greater<int> >::iterator BidPriceSizeMapIter_t;
typedef std::map<int, int, std::greater<int> >::const_iterator BidPriceSizeMapConstIter_t;
typedef std::map<int, int, std::less<int> > AskPriceSizeMap;
typedef std::map<int, int, std::less<int> >::iterator AskPriceSizeMapIter_t;
typedef std::map<int, int, std::less<int> >::const_iterator AskPriceSizeMapConstIter_t;
}

#endif  // BASE_ORDERROUTING_SIZE_MAPS_HPP
