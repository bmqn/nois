#pragma once

#include "nois/NoisTypes.hpp"

namespace nois {

class Stream
{
public:
	virtual ~Stream() {}

	virtual count_t Consume(data_t *data, count_t len) = 0;
};

};
