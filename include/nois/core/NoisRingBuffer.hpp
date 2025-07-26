#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisBuffer.hpp"

namespace nois {

template<typename>
class RingBuffer;

using FloatRingBuffer = RingBuffer<f32_t>;

template<typename T>
class RingBuffer
{
public:
	RingBuffer()
		: m_NumFrames(0)
		, m_NumChannels(0)
		, m_Size(0)
		, m_LatencySize(0)
		, m_InternalSize(0)
		, m_LogicalBase(0)
		, m_LogicalWriteOffset(0)
		, m_InternalPlayOffset(0)
		, m_InternalData(0, T{ 0 })
	{
	}

	RingBuffer(count_t numFrames, count_t numChannels)
		: m_NumFrames(numFrames)
		, m_NumChannels(numChannels)
		, m_Size(numFrames * numChannels)
		, m_LatencySize(numFrames * numChannels)
		, m_InternalSize(m_Size + m_LatencySize)
		, m_LogicalBase(0)
		, m_LogicalWriteOffset(m_LatencySize)
		, m_InternalPlayOffset(0)
		, m_InternalData(m_InternalSize, T{ 0 })
	{
	}

	void Consume(Buffer<T> &buffer)
	{
		count_t sizeToConsume = buffer.GetSize();

		count_t internalPlayOffset = m_InternalPlayOffset.load(std::memory_order_acquire);
		count_t newInternalPlayEndOffset = (internalPlayOffset + sizeToConsume);

		if (internalPlayOffset < newInternalPlayEndOffset || newInternalPlayEndOffset == 0)
		{
			const T *dataToCopy = m_Data.data() + internalPlayOffset;
			count_t sizeToCopy = sizeToConsume;

			buffer.Copy(dataToCopy, sizeToCopy);
		}
		else
		{
			const T *dataToCopy1 = m_Data.data() + internalPlayOffset;
			count_t sizeToCopy1 = m_InternalSize - internalPlayOffset;
			const T *dataToCopy2 = m_Data.data();
			count_t sizeToCopy2 = sizeToConsume - sizeToCopy1;

			buffer.Copy(dataToCopy1, sizeToCopy1);
			buffer.Copy(dataToCopy2, sizeToCopy2);
		}

		count_t newInternalPlayOffset = newInternalPlayEndOffset % m_InternalSize;
		
		m_InternalPlayOffset.store(newInternalPlayOffset, std::memory_order_release);
	
		if ((internalPlayOffset + m_InternalSize - m_LogicalBase) % m_InternalSize >= m_Size)
		{
			m_LogicalBase = (m_LogicalBase + m_Size) % m_Size;
		}
	}

	void GetOffsets(count_t &playOffset, count_t &writeOffset) const
	{
		count_t internalPlayOffset = m_InternalPlayOffset.load(std::memory_order_acquire);
		count_t logicalPlayOffset = InternalToLogical(internalPlayOffset);
		
		playOffset = logicalPlayOffset;
		writeOffset = m_LogicalWriteOffset;
	}

	bool Lock(count_t offset, count_t size, T *&ptr1, count_t &size1, T *&ptr2, count_t &size2)
	{
		if (size > m_Size)
		{
			return false;
		}

		count_t internalOffset = LogicalToInternal(offset);
		count_t endOffset = (internalOffset + size) % m_InternalSize;

		if (internalOffset < endOffset || endOffset == 0)
		{
			ptr1 = &m_InternalData[internalOffset];
			size1 = size;
			ptr2 = nullptr;
			size2 = 0;
		}
		else
		{
			ptr1 = &m_InternalData[internalOffset];
			size1 = m_InternalSize - internalOffset;
			ptr2 = &m_InternalData[0];
			size2 = size - size1;
		}

		return true;
	}

	bool Unlock(T *ptr1, count_t size1, T *ptr2, count_t size2)
	{
		if (!ptr1)
		{
			return false;
		}

		m_LogicalWriteOffset = (m_LogicalWriteOffset + size1 + size2) % m_Size;

		return true;
	}

	inline count_t GetSize() const
	{
		return m_Size;
	}

	inline count_t GetBytes() const
	{
		return m_Size * sizeof(T);
	}

	inline void Zero()
	{
		Fill(T{ 0 });
	}

	inline void Fill(T sample)
	{
		std::fill(m_InternalData.begin(), m_InternalData.begin() + m_Size, sample);
	}

private:
	count_t LogicalToInternal(count_t logicalOffset) const
	{
		return (m_LogicalBase + logicalOffset) % m_InternalSize;
	}

	count_t InternalToLogical(count_t internalOffset) const
	{
		if (internalOffset < m_LogicalBase)
		{
			internalOffset += m_InternalSize;
		}

		return (internalOffset - m_LogicalBase) % m_Size;
	}

private:
	count_t m_NumFrames;
	count_t m_NumChannels;
	count_t m_Size;

	count_t m_LatencySize;
	count_t m_InternalSize;

	count_t m_LogicalBase;
	count_t m_LogicalWriteOffset;

	std::atomic<count_t> m_InternalPlayOffset;

	std::vector<T> m_InternalData;
};

}