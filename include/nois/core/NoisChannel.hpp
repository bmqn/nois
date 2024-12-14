#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

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

	void SetStream(Ref_t<Stream> stream)
	{
		if (m_Stream != stream)
		{
			OnUnsetStream(m_Stream);
			OnSetStream(stream);

			m_Stream = stream;
		}
	}

	Ref_t<Stream> GetStream()
	{
		return m_Stream;
	}

protected:
	virtual void OnUnsetStream(Ref_t<nois::Stream> stream) {}
	virtual void OnSetStream(Ref_t<nois::Stream> stream) {}

private:
	Ref_t<Stream> m_Stream = nullptr;
};

Ref_t<Channel> CreateChannel();

}
