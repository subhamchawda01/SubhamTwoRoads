﻿/* Copyright (C) 2019 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef TWS_API_CLIENT_HISTORICALTICK_H
#define TWS_API_CLIENT_HISTORICALTICK_H

#include "dvccode/IBUtils/Decimal.hpp"

struct HistoricalTick
{
    long long time;
    double price;
    Decimal size;
};  
#endif // !historicaltickendtry_def


