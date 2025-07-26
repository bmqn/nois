#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class Gainer : public Stream
{
public:
	virtual ~Gainer() = default;

	virtual Ref_t<FloatParameter> GetGain() const = 0;
	virtual void SetGain(Ref_t<FloatParameter> gain) = 0;
};

Ref_t<Gainer> CreateGainer(Ref_t<Stream> stream);

}
