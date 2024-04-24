#pragma once

#include "NoisStream.hpp"

#include <atomic>
#include <memory>

namespace nois {

class Channel
{
public:
	void SetStream(std::shared_ptr<Stream> stream)
	{
		if (m_Stream != stream)
		{
			OnSetStream(stream);
		}

		std::atomic_exchange(&m_Stream, std::move(stream));
	}

	std::shared_ptr<Stream> GetStream()
	{
		return std::atomic_load(&m_Stream);
	}

protected:
	virtual void OnSetStream(std::shared_ptr<nois::Stream> stream) {}

private:
	std::shared_ptr<Stream> m_Stream;
};

std::shared_ptr<Channel> CreateChannel();

};
