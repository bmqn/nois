#include "nois/core/NoisChannel.hpp"

namespace nois {

std::shared_ptr<Channel> CreateChannel()
{
	return std::make_shared<Channel>();
}

};

