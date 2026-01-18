#include "NoisVst3Processor.hpp"

#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <pluginterfaces/base/ustring.h>

#include <algorithm>

template<typename T, typename C>
NoisVstProcessor<T, C>::NoisVstProcessor()
	: mSampleRate(0.0)
	, mSourceBuffer()
	, mSinkBuffer()
	, mParameterLookup()
{
	setControllerClass(C::kUid);
}

template<typename T, typename C>
FUnknown* PLUGIN_API NoisVstProcessor<T, C>::createInstance(void*)
{
	return static_cast<IAudioProcessor*>(new T());
}

template<typename T, typename C>
tresult PLUGIN_API NoisVstProcessor<T, C>::initialize(FUnknown* context)
{
	tresult result = AudioEffect::initialize(context);

	if (result != kResultOk)
	{
		return result;
	}

	addAudioInput(STR16("Input"), Vst::SpeakerArr:: kStereo);
	addAudioOutput(STR16("Output"), Vst::SpeakerArr::kStereo);

	return kResultOk;
}

template<typename T, typename C>
tresult PLUGIN_API NoisVstProcessor<T, C>::setState(IBStream* state)
{
	return kResultOk;
}

template<typename T, typename C>
tresult PLUGIN_API NoisVstProcessor<T, C>::getState(IBStream* state)
{
	return kResultOk;
}

template<typename T, typename C>
tresult PLUGIN_API NoisVstProcessor<T, C>::setupProcessing(Vst::ProcessSetup& setup)
{
	mSampleRate = setup.sampleRate;

	return AudioEffect::setupProcessing(setup);
}

template<typename T, typename C>
tresult PLUGIN_API NoisVstProcessor<T, C>::process(Vst::ProcessData& data)
{
	if (data.numInputs == 0 || data.numOutputs == 0 || mSampleRate == 0.0)
	{
		return kResultOk;
	}

	NOIS_PROFILE_MARK();

	{
		NOIS_PROFILE_SCOPE_NAMED("Prepare parameters");

		for (auto it = mParameterLookup.begin(); it != mParameterLookup.end(); ++it)
		{
			it->second->Prepare(data.numSamples, mSampleRate);
		}
	}

	if (data.inputParameterChanges)
	{
		NOIS_PROFILE_SCOPE_NAMED("Process parameter changes");

		int numParamsChanged = data.inputParameterChanges->getParameterCount();
		for (int i = 0; i < numParamsChanged; i++)
		{
			Vst::IParamValueQueue* queue = data.inputParameterChanges->getParameterData(i);
			if (!queue)
			{
				continue;
			}
			
			Vst::ParamID pid = queue->getParameterId();
			auto it = mParameterLookup.find(pid);
			if (it == mParameterLookup.end())
			{
				continue;
			}

			auto& parameter = it->second;
			nois::count_t currentSampleOffset = 0;
			nois::f32_t lastValuePlain = parameter->GetLastPlain();
			int numPoints = queue->getPointCount();
			for (int j = 0; j < numPoints; ++j)
			{
				int sampleOffset;
				Vst::ParamValue valueNormalized;
				if (queue->getPoint(j, sampleOffset, valueNormalized) == kResultTrue)
				{
					nois::f32_t valuePlain = parameter->ToPlain(valueNormalized);

					if (numPoints == 1)
					{
						parameter->WritePlain(
							currentSampleOffset,
							data.numSamples - currentSampleOffset,
							valuePlain);

						break;
					}

					nois::count_t nextSampleOffset = sampleOffset;
					nois::count_t lastSampleOffset = currentSampleOffset;

					for (;
						currentSampleOffset <= nextSampleOffset &&
						currentSampleOffset < data.numSamples;
						++currentSampleOffset)
					{
						if (currentSampleOffset == nextSampleOffset ||
							nextSampleOffset == lastSampleOffset)
						{
							parameter->WritePlain(
								currentSampleOffset,
								valuePlain);
						}
						else
						{
							nois::f32_t t =
								nois::f32_t(currentSampleOffset - lastSampleOffset) /
								nois::f32_t(nextSampleOffset - lastSampleOffset);

							parameter->WritePlain(
								currentSampleOffset,
								valuePlain * t +
								lastValuePlain * (1.0f - t));
						}
					}

					lastValuePlain = valuePlain;
				}
			}
		}
	}

	auto& inSource = data.inputs[0];
	auto& outSink = data.outputs[0];

	mSourceBuffer.Resize(data.numSamples, inSource.numChannels);
	mSinkBuffer.Resize(data.numSamples, outSink.numChannels);

	{
		NOIS_PROFILE_SCOPE_NAMED("Read from source");

		for (int c = 0; c < inSource.numChannels; ++c)
		{
			const float* samples = inSource.channelBuffers32[c];
			for (int f = 0; f < data.numSamples; ++f)
			{
				mSourceBuffer(f, c) = samples[f];
			}
		}
	}

	Process(mSourceBuffer, mSinkBuffer);
	
	{
		NOIS_PROFILE_SCOPE_NAMED("Write to sink");

		for (int c = 0; c < outSink.numChannels; ++c)
		{
			float* samples = outSink.channelBuffers32[c];
			for (int f = 0; f < data.numSamples; ++f)
			{
				samples[f] = mSinkBuffer(f, c);
			}
		}
	}

	return kResultOk;
}

template<typename T, typename C>
void NoisVstProcessor<T, C>::Register(NoisVstProcessorParameter* parameter)
{
	mParameterLookup[parameter->GetPid()] = parameter;
}
