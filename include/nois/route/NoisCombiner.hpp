#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisChannel.hpp"
#include "nois/core/NoisStream.hpp"

#include <memory>

namespace nois {

class Combiner
{
public:
	virtual void AddStream(Ref_t<Stream> stream) = 0;
	virtual void RemoveStream(Ref_t<Stream> stream) = 0;
	virtual Ref_t<Stream> GetStream() = 0;
};

Ref_t<Combiner> CreateCombiner();

}