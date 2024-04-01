#pragma once

#include "nois/core/NoisStream.hpp"

#include <memory>

namespace nois {

class Gainer : public Stream
{
public:
	virtual ~Gainer() {}

	virtual void SetGain(float gain) = 0;
	virtual void SetGainDb(float gainDb) = 0;
};

std::shared_ptr<Gainer> CreateGainer(std::shared_ptr<Stream> stream);

};
