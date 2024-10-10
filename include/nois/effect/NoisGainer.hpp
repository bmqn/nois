#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

#include <memory>

namespace nois {

class Gainer : public Stream
{
public:
	virtual ~Gainer() {}

	virtual data_t GetGain() const = 0;
	virtual data_t GetGainDb() const = 0;

	virtual void SetGain(data_t gain) = 0;
	virtual void SetGainDb(data_t gainDb) = 0;
};

std::shared_ptr<Gainer> CreateGainer(Stream *stream);

}
