#pragma once

#include "NoisTypes.hpp"
#include "NoisConfig.hpp"

namespace nois {

inline data_t ToDb(data_t amp)
{
	return 10.0f * std::log10(amp - k_DcOffset);
}

inline data_t FromDb(data_t db)
{
	return std::pow(10.0f, (db) / 10.0f) + k_DcOffset;
}

};
