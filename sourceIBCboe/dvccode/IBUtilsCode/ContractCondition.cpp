/* Copyright (C) 2019 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "dvccode/IBUtils/StdAfx.hpp"
#include "dvccode/IBUtils/ContractCondition.hpp"
#include "dvccode/IBUtils/EDecoder.hpp"
#include "dvccode/IBUtils/EClient.hpp"

std::string ContractCondition::toString() {
    std::string strContract = std::to_string(conId()) + "";

    return std::to_string(type()) + " of " + strContract + OperatorCondition::toString();
}

const char* ContractCondition::readExternal(const char* ptr, const char* endPtr) {
	if (!(ptr = OperatorCondition::readExternal(ptr, endPtr)))
		return 0;

	DECODE_FIELD(m_conId);
	DECODE_FIELD(m_exchange);

	return ptr;
}

void ContractCondition::writeExternal(std::ostream & msg) const {
	OperatorCondition::writeExternal(msg);

	ENCODE_FIELD(m_conId);
	ENCODE_FIELD(m_exchange);
}

int ContractCondition::conId() {
	return m_conId;
}

void ContractCondition::conId(int conId) {
	m_conId = conId;
}

std::string ContractCondition::exchange() {
	return m_exchange;
}

void ContractCondition::exchange(const std::string & exchange) {
	m_exchange = exchange;
}
