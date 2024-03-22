#pragma once

#include "NoisStream.hpp"

#include <memory>

namespace nois {

class Channel
{
public:
	std::shared_ptr<Stream> GetStream()
	{
		return m_Stream;
	}

	void SetStream(std::shared_ptr<Stream> stream)
	{
		m_Stream = stream;
	}

private:
	std::shared_ptr<Stream> m_Stream;
};

};
