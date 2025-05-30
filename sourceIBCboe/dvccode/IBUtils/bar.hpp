﻿/* Copyright (C) 2019 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef TWS_API_CLIENT_BAR_H
#define TWS_API_CLIENT_BAR_H

#include <string>
#include "dvccode/IBUtils/Decimal.hpp"

struct Bar
{
	std::string time;
	double high;
	double low;
	double open;
	double close;
	Decimal wap;
	Decimal volume;
	int count;
};

#endif