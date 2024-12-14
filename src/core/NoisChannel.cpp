#include "nois/core/NoisChannel.hpp"

namespace nois {

Ref_t<Channel> CreateChannel()
{
	return MakeRef<Channel>();
}

};

