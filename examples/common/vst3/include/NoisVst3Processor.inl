#include "NoisVst3Processor.hpp"

#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstprocesscontext.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <pluginterfaces/base/ibstream.h>
#include <pluginterfaces/base/ustring.h>

#include <algorithm>

template<typename T, typename C>
NoisVstProcessor<T, C>::NoisVstProcessor()
	: mSampleRate(0.0f)
	, mTempo(0.0f)
	, mSourceBuffer()
	, mSinkBuffer()
	, mParameters()
	, mTempoParameter(nullptr)
	, mTempoBlockParameter(nullptr)
{
	setControllerClass(C::kUid);

	mTempoParameter = mRegistry.CreateBinder(
		[this](nois::count_t)
		{
			return mTempo;
		});
	mTempoBlockParameter = mRegistry.CreateBlockBinder(
		[this]()
		{
			return mTempo;
		},
		0.0f,
		1000.0f);
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
	if (!state)
	{
		return kResultFalse;
	}

	int count = -1;
	if (state->read(&count, sizeof(count)) != kResultOk)
	{
		return kResultFalse;
	}
	for (int i = 0; i < count; ++i)
	{
		int pid = -1;
		nois::f32_t value = 0.0f;
		if (state->read(&pid, sizeof(pid)) != kResultOk)
		{
			return kResultFalse;
		}
		if (state->read(&value, sizeof(value)) != kResultOk)
		{
			return kResultFalse;
		}
		if (pid != -1)
		{
			// TODO: value min/max check?

			if (auto it = mParameters.find(pid); it != mParameters.end())
			{
				it->second->RequestPlain(value);
			}
		}
	}

	return kResultOk;
}

template<typename T, typename C>
tresult PLUGIN_API NoisVstProcessor<T, C>::getState(IBStream* state)
{
	if (!state)
	{
		return kResultFalse;
	}

	int count = mParameters.size();
	if (state->write(&count, sizeof(count)) != kResultOk)
	{
		return kResultFalse;
	}
	for (const auto& [pid, parameter] : mParameters)
	{
		nois::f32_t value = parameter->GetLastPlain();
		int pide = pid;
		if (state->write(&pide, sizeof(pide)) != kResultOk)
		{
			return kResultFalse;
		}
		if (state->write(&value, sizeof(value)) != kResultOk)
		{
			return kResultFalse;
		}
	}

	return kResultOk;
}

template<typename T, typename C>
tresult PLUGIN_API NoisVstProcessor<T, C>::setupProcessing(Vst::ProcessSetup& setup)
{
	Vst::SpeakerArrangement arrangement;

	if (getBusArrangement(Vst::kInput, 0, arrangement) == kResultTrue)
	{
		nois::s32_t numChannels = Vst::SpeakerArr::getChannelCount(arrangement);
		mSourceBuffer.Reserve(setup.maxSamplesPerBlock, numChannels);
	}

	if (getBusArrangement(Vst::kOutput, 0, arrangement) == kResultTrue)
	{
		nois::s32_t numChannels = Vst::SpeakerArr::getChannelCount(arrangement);
		mSinkBuffer.Reserve(setup.maxSamplesPerBlock, numChannels);
	}

	mSampleRate = setup.sampleRate;

	return AudioEffect::setupProcessing(setup);
}

template<typename T, typename C>
tresult PLUGIN_API NoisVstProcessor<T, C>::process(Vst::ProcessData& data)
{
	if (data.numInputs == 0 ||
		data.numOutputs == 0 ||
		data.numSamples == 0 ||
		mSampleRate == 0.0)
	{
		return kResultOk;
	}

	if (data.processContext)
	{
		mTempo = data.processContext->tempo;
	}

	NOIS_PROFILE_MARK();

	{
		NOIS_PROFILE_SCOPE_NAMED("Setup parameters");

		for (auto it = mParameters.begin();
			it != mParameters.end();
			++it)
		{
			it->second->Setup(
				data.numSamples,
				mSampleRate);
		}

		if (data.inputParameterChanges)
		{
			NOIS_PROFILE_SCOPE_NAMED("Process changed parameter");

			for (int32 i = 0; i < data.inputParameterChanges->getParameterCount(); i++)
			{
				Vst::IParamValueQueue* queue = data.inputParameterChanges->getParameterData(i);
				if (!queue)
				{
					continue;
				}
				Vst::ParamID pid = queue->getParameterId();
				auto it = mParameters.find(pid);
				if (it == mParameters.end())
				{
					continue;
				}
				auto& parameter = it->second;
				nois::count_t currentSampleOffset = 0;
				nois::f32_t lastValuePlain = parameter->GetLastPlain();
				int32 numPoints = queue->getPointCount();
				for (int j = 0; j < numPoints; ++j)
				{
					int32 sampleOffset;
					Vst::ParamValue valueNormalized;
					if (queue->getPoint(j, sampleOffset, valueNormalized) == kResultTrue)
					{
						nois::f32_t valuePlain =
							parameter->ToPlain(valueNormalized);
						nois::count_t nextSampleOffset =
							sampleOffset == 0
							? data.numSamples
							: sampleOffset;
						nois::count_t prevSampleOffset =
							currentSampleOffset;
						for (;
							currentSampleOffset <= nextSampleOffset &&
							currentSampleOffset < data.numSamples;
							++currentSampleOffset)
						{
							if (nextSampleOffset == prevSampleOffset)
							{
								parameter->WritePlain(
									currentSampleOffset,
									valuePlain);
							}
							else
							{
								nois::f32_t t =
									nois::f32_t(currentSampleOffset - prevSampleOffset) /
									nois::f32_t(nextSampleOffset - prevSampleOffset);
								parameter->WritePlain(
									currentSampleOffset,
									lastValuePlain +
									(valuePlain - lastValuePlain) * t);
							}
						}
						lastValuePlain = valuePlain;
					}
				}
			}
		}
	}

	{
		NOIS_PROFILE_SCOPE_NAMED("Prepare parameters");

		mRegistry.Prepare(data.numSamples, mSampleRate);
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

	{
		NOIS_PROFILE_SCOPE_NAMED("Process");

		Process(mSourceBuffer, mSinkBuffer);
	}
	
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
template<typename Param>
auto NoisVstProcessor<T, C>::CreateParameter() -> nois::Own_t<NoisVstProcessorParameter>
{
	auto parameter = NoisVstProcessorParameter::Create<Param>(mRegistry);
	mParameters[parameter->GetPid()] = parameter.get();
	return parameter;
}

template<typename T, typename C>
auto NoisVstProcessor<T, C>::CreateConstant(nois::f32_t value) -> nois::Ref_t<nois::FloatParameter>
{
	return mRegistry.CreateConstant(value);
}

template<typename T, typename C>
auto NoisVstProcessor<T, C>::CreateBlockConstant(nois::f32_t value) -> nois::Ref_t<nois::FloatBlockParameter>
{
	return mRegistry.CreateBlockConstant(value);
}

template<typename T, typename C>
template<typename F, typename... Params>
auto NoisVstProcessor<T, C>::CreateTransformer(F&& transformer, Params&&... transformees) -> nois::Ref_t<nois::FloatParameter>
{
	return mRegistry.CreateTransformer(std::forward<F>(transformer), std::forward<Params>(transformees)...);
}

template<typename T, typename C>
template<typename F, typename... Params>
auto NoisVstProcessor<T, C>::CreateBlockTransformer(F&& transformer, Params&&... transformees) -> nois::Ref_t<nois::FloatBlockParameter>
{
	return mRegistry.CreateBlockTransformer(std::forward<F>(transformer), std::forward<Params>(transformees)...);
}

template<typename T, typename C>
auto NoisVstProcessor<T, C>::GetTempo() -> nois::Ref_t<nois::FloatParameter>
{
	return mTempoParameter;
}

template<typename T, typename C>
auto NoisVstProcessor<T, C>::GetBlockTempo() -> nois::Ref_t<nois::FloatBlockParameter>
{
	return mTempoBlockParameter;
}

template<typename T>
NoisVstController<T>::NoisVstController()
	: mParameters()
{
}

template<typename T>
FUnknown* PLUGIN_API NoisVstController<T>::createInstance(void*)
{
	return static_cast<Steinberg::Vst::IEditController*>(new T());
}

template<typename T>
tresult PLUGIN_API NoisVstController<T>::initialize(FUnknown* context)
{
	tresult result = EditController::initialize(context);

	if (result != kResultOk)
	{
		return result;
	}

	for (const auto& [pid, parameter] : mParameters)
	{
		parameters.addParameter(*parameter);
	}

	return result;
}

template<typename T>
tresult PLUGIN_API NoisVstController<T>::setComponentState(IBStream* state)
{
	if (!state)
	{
		return kResultFalse;
	}

	int count = -1;
	if (state->read(&count, sizeof(count)) != kResultOk)
	{
		return kResultFalse;
	}
	for (int i = 0; i < count; ++i)
	{
		int pid = -1;
		nois::f32_t value = 0.0f;
		if (state->read(&pid, sizeof(pid)) != kResultOk)
		{
			return kResultFalse;
		}
		if (state->read(&value, sizeof(value)) != kResultOk)
		{
			return kResultFalse;
		}
		if (pid != -1)
		{
			// TODO: value min/max check?

			if (auto it = mParameters.find(pid); it != mParameters.end())
			{
				it->second->SetPlain(value);
			}
		}
	}

	return kResultOk;
}

template<typename T>
template<typename Param>
auto NoisVstController<T>::CreateParameter() -> nois::Own_t<NoisVstControllerParameter>
{
	auto parameter = NoisVstControllerParameter::Create<Param>();
	mParameters[parameter->GetPid()] = parameter.get();
	return parameter;
}
