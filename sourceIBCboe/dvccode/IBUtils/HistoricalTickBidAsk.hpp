﻿/* Copyright (C) 2019 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef TWS_API_CLIENT_HISTORICALTICKBIDASK_H
#define TWS_API_CLIENT_HISTORICALTICKBIDASK_H

#include "dvccode/IBUtils/TickAttribBidAsk.hpp"
#include "dvccode/IBUtils/Decimal.hpp"

struct HistoricalTickBidAsk
{
    long long time;
    TickAttribBidAsk tickAttribBidAsk;
    double priceBid;
    double priceAsk;
    Decimal sizeBid;
    Decimal sizeAsk;
};
#endif

