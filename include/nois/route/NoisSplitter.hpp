#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisChannel.hpp"
#include "nois/core/NoisStream.hpp"

#include <memory>

namespace nois {

class Splitter
{
public:
	virtual void SetStream(Ref_t<Stream> stream) = 0;
	virtual Ref_t<Stream> GetStream() = 0;
};

Ref_t<Splitter> CreateSplitter();
Ref_t<Splitter> CreateSplitter(Ref_t<Stream> stream);

}