#include "nois/core/NoisChannel.hpp"

#include <memory>

namespace nois {

class Splitter
{
public:
	virtual std::shared_ptr<Channel> GetChannel(size_t index) = 0;
};

std::shared_ptr<Splitter> CreateSplitter(std::shared_ptr<Stream> stream, size_t numChannels);

}