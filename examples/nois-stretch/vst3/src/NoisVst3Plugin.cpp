#include "NoisVst3Plugin.hpp"

#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <pluginterfaces/base/ustring.h>

#include <algorithm>

nois::Stream::Result NoisVstSource::Consume(nois::FloatBuffer& buffer, nois::f32_t sampleRate)
{
	buffer.Copy(mBuffer);

	return nois::Stream::Result::Success;
}

void NoisVstSource::PrepareToConsume(nois::count_t numFrames, nois::count_t numChannels, nois::f32_t sampleRate)
{
	mBuffer.Resize(numFrames, numChannels);
}

nois::FloatBuffer& NoisVstSource::GetBuffer()
{
	return mBuffer;
}

NoisPlugin::NoisPlugin()
	: mSource(nullptr)
	, mTimeStretcher(nullptr)
	, mSampleRate(0.0)
{
	mSource = nois::MakeRef<NoisVstSource>();

	mTimeStretcher = nois::CreateTimeStretcher(mSource);

	mTimeStretcher->SetStretchActive(mStretchActive);
	mTimeStretcher->SetStretchFactor(mStretchFactor);
	mTimeStretcher->SetGrainPhaseInc(mGrainPhaseInc);
	mTimeStretcher->SetGrainLockActive(mGrainPhaseLockActive);

	mParameterLookup[decltype(mStretchActive)::kPid] = &mStretchActive;
	mParameterLookup[decltype(mStretchFactor)::kPid] = &mStretchFactor;
	mParameterLookup[decltype(mGrainPhaseInc)::kPid] = &mGrainPhaseInc;
	mParameterLookup[decltype(mGrainPhaseLockActive)::kPid] = &mGrainPhaseLockActive;

	setControllerClass(NoisController::kUid);
}

FUnknown* PLUGIN_API NoisPlugin::createInstance(void*)
{
	return static_cast<IAudioProcessor*>(new NoisPlugin());
}

tresult PLUGIN_API NoisPlugin::initialize(FUnknown* context)
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

tresult PLUGIN_API NoisPlugin::setState(IBStream* state)
{
	return kResultOk;
}

tresult PLUGIN_API NoisPlugin::getState(IBStream* state)
{
	return kResultOk;
}

tresult PLUGIN_API NoisPlugin::setupProcessing(Vst::ProcessSetup& setup)
{
	mSampleRate = setup.sampleRate;

	return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API NoisPlugin::process(Vst::ProcessData& data)
{
	if (data.numInputs == 0 || data.numOutputs == 0 || mSampleRate == 0.0)
	{
		return kResultOk;
	}

	NOIS_PROFILE_MARK();

	{
		NOIS_PROFILE_SCOPE_NAMED("Prepare parameters");

		for (auto it = mParameterLookup.begin();
			it != mParameterLookup.end();
			++it)
		{
			std::visit([&](auto &&parameter)
			{
				parameter->Prepare(data.numSamples);
			}, it->second);
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

			std::visit([&](auto &&parameter)
			{
				using T = std::remove_pointer_t<std::decay_t<decltype(parameter)>>;

				nois::f32_t lastValuePlain = parameter->GetLastValue();

				nois::count_t currentSampleOffset = 0;

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
							for (;
								currentSampleOffset < data.numSamples;
								++currentSampleOffset)
							{
								parameter->WriteValue(
									currentSampleOffset,
									valuePlain);
							}

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
								parameter->WriteValue(
									currentSampleOffset,
									valuePlain);
							}
							else
							{
								nois::f32_t t =
									nois::f32_t(currentSampleOffset - lastSampleOffset) /
									nois::f32_t(nextSampleOffset - lastSampleOffset);

								parameter->WriteValue(
									currentSampleOffset,
									valuePlain * t +
									lastValuePlain * (1.0f - t));
							}
						}

						lastValuePlain = valuePlain;
					}
				}
			}, it->second);
		}
	}

	nois::FloatBuffer& sourceBuffer = mSource->GetBuffer();
	nois::FloatBuffer sinkBuffer;

	auto& inSource = data.inputs[0];
	auto& outSink = data.outputs[0];

	sourceBuffer.Resize(data.numSamples, inSource.numChannels);
	sinkBuffer.Resize(data.numSamples, outSink.numChannels);

	{
		NOIS_PROFILE_SCOPE_NAMED("Read from source");

		for (int channel = 0; channel < inSource.numChannels; ++channel)
		{
			float* inSamplesStart  = inSource.channelBuffers32[channel];

			for (int sample = 0; sample < data.numSamples; ++sample)
			{
				sourceBuffer(sample, channel) = inSamplesStart[sample];
			}
		}
	}

	{
		nois::ScopedNoDenorms noDenorms;

		NOIS_PROFILE_SCOPE_NAMED("Time stretch");

		mTimeStretcher->PrepareToConsume(sinkBuffer.GetNumFrames(), sinkBuffer.GetNumChannels(), mSampleRate);
		mTimeStretcher->Consume(sinkBuffer, mSampleRate);
	}
	
	{
		NOIS_PROFILE_SCOPE_NAMED("Write to sink");

		for (int channel = 0; channel < outSink.numChannels; ++channel)
		{
			float* outSamplesStart = outSink.channelBuffers32[channel];

			for (int sample = 0; sample < data.numSamples; ++sample)
			{
				outSamplesStart[sample] = sinkBuffer(sample, channel);
			}
		}
	}

	return kResultOk;
}

FUnknown* PLUGIN_API NoisController::createInstance(void*)
{
	return static_cast<Steinberg::Vst::IEditController*>(new NoisController());
}

tresult PLUGIN_API NoisController::initialize(FUnknown* context)
{
	tresult result = EditController::initialize(context);

	if (result != kResultOk)
	{
		return result;
	}

	parameters.addParameter(new
		NoisVstControllerParameter<parameter::StretchActive>(
		"Stretch"));
	parameters.addParameter(new
		NoisVstControllerParameter<parameter::StretchFactor>(
		"Factor"));
	parameters.addParameter(new
		NoisVstControllerParameter<parameter::GrainPhaseInc>(
		"Grain Increment"));
	parameters.addParameter(new
		NoisVstControllerParameter<parameter::GrainPhaseLockActive>(
		"Grain Lock"));

	return result;
}

tresult PLUGIN_API NoisController::terminate()
{
	return EditController::terminate();
}
