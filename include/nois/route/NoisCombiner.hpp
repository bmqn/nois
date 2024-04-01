#include "nois/core/NoisChannel.hpp"

#include <memory>
#include <vector>

namespace nois {

class Combiner
{
public:
	virtual std::shared_ptr<Channel> GetChannel() = 0;

};

std::shared_ptr<Combiner> CreateCombiner(std::vector<std::shared_ptr<Stream>> streams);

}