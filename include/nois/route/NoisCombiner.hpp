#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisChannel.hpp"
#include "nois/core/NoisStream.hpp"

#include <memory>

namespace nois {

class Combiner
{
public:
	virtual void AddStream(Stream *stream) = 0;
	virtual void RemoveStream(Stream *stream) = 0;
	virtual Stream *GetStream() = 0;
};

Ref_t<Combiner> CreateCombiner();
Ref_t<Combiner> CreateCombiner(std::initializer_list<Stream*> streams);

}