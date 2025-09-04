#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class PitchDetector : public Stream
{
public:
	virtual ~PitchDetector() = default;

	virtual Ref_t<FloatBlockParameter> GetPitchHz() const = 0;
};

Ref_t<PitchDetector> CreatePitchDetector(Ref_t<Stream> stream);

}
