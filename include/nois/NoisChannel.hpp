#pragma once

#include "NoisStream.hpp"

#include <memory>

namespace nois {

class Channel
{
public:
	std::shared_ptr<Stream> GetStream()
	{
		return mStream;
	}

private:
	std::shared_ptr<Stream> mStream;
};

};
