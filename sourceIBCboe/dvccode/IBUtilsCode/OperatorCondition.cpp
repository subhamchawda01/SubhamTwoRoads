/* Copyright (C) 2019 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "dvccode/IBUtils/StdAfx.hpp"
#include "dvccode/IBUtils/OperatorCondition.hpp"
#include "dvccode/IBUtils/EDecoder.hpp"
#include "dvccode/IBUtils/EClient.hpp"

const char* OperatorCondition::readExternal(const char* ptr, const char* endPtr) {
	if (!(ptr = OrderCondition::readExternal(ptr, endPtr)))
		return 0;

	DECODE_FIELD(m_isMore);

	std::string str;

	DECODE_FIELD(str);

	valueFromString(str);

	return ptr;
}

std::string OperatorCondition::toString() {
	return " is " + std::string(isMore() ? ">= " : "<= ") + valueToString();
}

void OperatorCondition::writeExternal(std::ostream & msg) const {
	OrderCondition::writeExternal(msg);

	ENCODE_FIELD(m_isMore);
	ENCODE_FIELD(valueToString());
}

bool OperatorCondition::isMore() {
	return m_isMore;
}

void OperatorCondition::isMore(bool isMore) {
	m_isMore = isMore;
}
