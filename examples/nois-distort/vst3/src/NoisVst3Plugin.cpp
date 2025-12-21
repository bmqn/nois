#include "NoisVst3Plugin.hpp"

#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <pluginterfaces/base/ustring.h>

#include <algorithm>

NoisPlugin::NoisPlugin()
	: mSourceBuffer()
	, mSinkBuffer()
	, mHpBuffer()
	, mDistBuffer()
	, mSubFreq(nullptr)
	, mDrive(nullptr)
	, mSource(&mSourceBuffer)
	, mHpFilter(nullptr)
	, mLpFilter(nullptr)
	, mAp1Filter(nullptr)
	, mAp2Filter(nullptr)
	, mDist(nullptr)
	, mLpDistFilter(nullptr)
	, mSampleRate(0.0)
{
	mSubFreq = parameter::CreateProcessor(parameter::kSubFreq, mRegistry);
	mDrive = parameter::CreateProcessor(parameter::kDrive, mRegistry);
	mWet = parameter::CreateProcessor(parameter::kWet, mRegistry);

	auto ap1CutoffRatio =
		mRegistry.CreateBlockTransformer(*mSubFreq,
			[](float x) { return std::clamp(x * 0.7f, 0.002f, 0.02f); });
	auto ap2CutoffRatio =
		mRegistry.CreateBlockTransformer(*mSubFreq,
			[](float x) { return std::clamp(x * 1.2f, 0.002f, 0.03f); });
	auto apQ =
		mRegistry.CreateBlockConstant(0.67f);
	auto distWet =
		mRegistry.CreateConstant(1.0f);
	auto lpDistCutoffRatio =
		mRegistry.CreateBlockTransformer(*mSubFreq,
			[](nois::f32_t x) { return x * 2.0f; });

	mHpFilter = nois::Filter::Create(nois::Filter::k_LR4High);
	mHpFilter->SetCutoffRatio(*mSubFreq);

	mLpFilter = nois::Filter::Create(nois::Filter::k_LR4Low);
	mLpFilter->SetCutoffRatio(*mSubFreq);

	mAp1Filter = nois::AllpassFilter::Create(nois::AllpassFilter::k_RBJBiquad);
	mAp1Filter->SetCutoffRatio(ap1CutoffRatio);
	mAp1Filter->SetQ(apQ);

	mAp2Filter = nois::AllpassFilter::Create(nois::AllpassFilter::k_RBJBiquad);
	mAp2Filter->SetCutoffRatio(ap2CutoffRatio);
	mAp2Filter->SetQ(apQ);

	mDist = nois::Distorter::Create(nois::Distorter::k_Tanh);
	mDist->SetDriveDb(*mDrive);
	mDist->SetWet(distWet);

	mLpDistFilter = nois::Filter::Create(nois::Filter::k_LR4Low);
	mLpDistFilter->SetCutoffRatio(lpDistCutoffRatio);

	mParameterLookup[mSubFreq->GetPid()] = mSubFreq.get();
	mParameterLookup[mDrive->GetPid()] = mDrive.get();
	mParameterLookup[mWet->GetPid()] = mWet.get();

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

	nois::FloatBuffer& sourceBuffer = mSourceBuffer;
	nois::FloatBuffer& sinkBuffer = mSinkBuffer;
	nois::FloatBuffer& hpBuffer = mHpBuffer;
	nois::FloatBuffer& distBuffer = mDistBuffer;

	auto& inSource = data.inputs[0];
	auto& outSink = data.outputs[0];

	sourceBuffer.Resize(data.numSamples, inSource.numChannels);
	sinkBuffer.Resize(data.numSamples, outSink.numChannels);
	hpBuffer.Resize(data.numSamples, inSource.numChannels);
	distBuffer.Resize(data.numSamples, inSource.numChannels);

	{
		NOIS_PROFILE_SCOPE_NAMED("Read from source");

		for (int c = 0; c < inSource.numChannels; ++c)
		{
			const float* samples = inSource.channelBuffers32[c];
			for (int f = 0; f < data.numSamples; ++f)
			{
				sourceBuffer(f, c) = samples[f];
			}
		}
	}

	{
		NOIS_PROFILE_SCOPE_NAMED("Prepare parameters");

		mRegistry.Prepare(hpBuffer.GetNumFrames(), mSampleRate);
	}

	{
		NOIS_PROFILE_SCOPE_NAMED("Prepare processors");

		mHpFilter->Prepare(hpBuffer.GetNumFrames(), hpBuffer.GetNumChannels(), mSampleRate);
		mLpFilter->Prepare(distBuffer.GetNumFrames(), distBuffer.GetNumChannels(), mSampleRate);
		mAp1Filter->Prepare(distBuffer.GetNumFrames(), distBuffer.GetNumChannels(), mSampleRate);
		mAp2Filter->Prepare(distBuffer.GetNumFrames(), distBuffer.GetNumChannels(), mSampleRate);
		mDist->Prepare(distBuffer.GetNumFrames(), distBuffer.GetNumChannels(), mSampleRate);
		mLpDistFilter->Prepare(distBuffer.GetNumFrames(), distBuffer.GetNumChannels(), mSampleRate);
	}

	{
		nois::ScopedNoDenorms noDenorms;

		NOIS_PROFILE_SCOPE_NAMED("Process processors");

		mHpFilter->Process(sourceBuffer, hpBuffer);
		mLpFilter->Process(sourceBuffer, distBuffer);
		mAp1Filter->Process(distBuffer, distBuffer);
		mAp2Filter->Process(distBuffer, distBuffer);
		mDist->Process(distBuffer, distBuffer);
		mLpDistFilter->Process(distBuffer, distBuffer);
	}
	
	{
		NOIS_PROFILE_SCOPE_NAMED("Write to sink");

		for (int c = 0; c < outSink.numChannels; ++c)
		{
			float* samples = outSink.channelBuffers32[c];
			for (int f = 0; f < data.numSamples; ++f)
			{
				nois::f32_t wet = mWet->GetLastPlain();
				nois::f32_t x = sourceBuffer(f, c);
				nois::f32_t z = hpBuffer(f, c) + distBuffer(f, c);
				samples[f] = x * (1.0f - wet) + z * wet;
			}
		}
	}

	return kResultOk;
}

NoisController::NoisController()
	: mSubFreq(nullptr)
	, mDrive(nullptr)
{
	mSubFreq = parameter::CreateController(parameter::kSubFreq);
	mDrive = parameter::CreateController(parameter::kDrive);
	mWet = parameter::CreateController(parameter::kWet);
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

	parameters.addParameter(*mSubFreq);
	parameters.addParameter(*mDrive);
	parameters.addParameter(*mWet);

	return result;
}

tresult PLUGIN_API NoisController::terminate()
{
	return EditController::terminate();
}
