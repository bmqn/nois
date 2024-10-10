#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

#include <memory>

namespace nois {

class Channel
{
public:
	virtual ~Channel()
	{
		if (m_Stream)
		{
			OnUnsetStream(m_Stream);
		}
	}

	void SetStream(std::shared_ptr<Stream> stream)
	{
		if (m_Stream != stream)
		{
			OnUnsetStream(m_Stream);
			OnSetStream(stream);

			m_Stream = stream;
		}
	}

	std::shared_ptr<Stream> GetStream()
	{
		return m_Stream;
	}

protected:
	virtual void OnUnsetStream(std::shared_ptr<nois::Stream> stream) {}
	virtual void OnSetStream(std::shared_ptr<nois::Stream> stream) {}

private:
	std::shared_ptr<Stream> m_Stream = nullptr;
};

std::shared_ptr<Channel> CreateChannel();

}
