#pragma once

#include "NoisTypes.hpp"
#include "NoisUtil.hpp"
#include "NoisStream.hpp"

#include <memory>

namespace nois {

class Gainer : public Stream
{
public:
	virtual ~Gainer() {}

	void SetGain(float gain) { m_Gain = gain; };
	void SetGainDb(float gainDb) { m_Gain = FromDb(gainDb); };

protected:
	float m_Gain = 1.0f;

};

std::shared_ptr<Gainer> CreateGainer(std::shared_ptr<Stream> stream);

};
