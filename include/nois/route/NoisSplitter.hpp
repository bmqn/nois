#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisChannel.hpp"
#include "nois/core/NoisStream.hpp"

#include <memory>

namespace nois {

class Splitter
{
public:
	virtual void SetStream(Stream *stream) = 0;
	virtual Stream *GetStream() = 0;

	virtual void Refresh() = 0;
};

std::shared_ptr<Splitter> CreateSplitter();
std::shared_ptr<Splitter> CreateSplitter(Stream *stream);

}