﻿/* Copyright (C) 2019 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef TWS_API_CLIENT_HISTORICALTICKLAST_H
#define TWS_API_CLIENT_HISTORICALTICKLAST_H

#include "dvccode/IBUtils/TickAttribLast.hpp"
#include "dvccode/IBUtils/Decimal.hpp"
#include <string>

struct HistoricalTickLast
{
    long long time;
    TickAttribLast tickAttribLast;
    double price;
    Decimal size;
    std::string exchange;
    std::string specialConditions;
};
#endif

