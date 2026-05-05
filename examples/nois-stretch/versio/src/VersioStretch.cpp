#include "daisy_versio.h"
#include <umm_malloc.h>
#include <nois/Nois.hpp>

using namespace daisy;

DaisyVersio hw;

static uint8_t DSY_SDRAM_BSS sdramHeap[32 * 1024 * 1024];
void* UMM_MALLOC_CFG_HEAP_ADDR = &sdramHeap;
size_t UMM_MALLOC_CFG_HEAP_SIZE = sizeof(sdramHeap);

static float scratchIn[2 * 128];
static float scratchOut[2 * 128];

static float stretchActiveValue = 0.0f;
static float stretchFactorValue = 0.0f;
static float grainSizeValue = 0.0f;
static float grainBlendValue = 0.5f;
static float grainPhaseIncValue = 1.0f;

static float gateStretchActiveValue = 0.0f;

static nois::Ref_t<nois::Registry<nois::f32_t>> registry = nullptr;
static nois::Ref_t<nois::TimeStretcher> timeStretcher = nullptr;

static void callback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	hw.ProcessAllControls();
	hw.tap.Debounce();

	if (gateStretchActiveValue > 0.0f)
	{
		gateStretchActiveValue -= 1.0f;
	}
	if (hw.Gate() && gateStretchActiveValue <= 0.0f)
	{
		float stretchActiveProbability = hw.GetKnobValue(DaisyVersio::KNOB_4);
		if (stretchActiveProbability <= 0.01f)
		{
			gateStretchActiveValue = 0.0f;
		}
		else if (Random::GetFloat(0.0f, 1.0f) < stretchActiveProbability)
		{
			float stretchActiveEnvelopeLength = hw.GetKnobValue(DaisyVersio::KNOB_5);
			gateStretchActiveValue = 500.0f * stretchActiveEnvelopeLength;
		}
	}

	stretchActiveValue = (hw.SwitchPressed() || gateStretchActiveValue > 0.0f) ? 1.0f : 0.0f;
	stretchFactorValue = hw.GetKnobValue(DaisyVersio::KNOB_0);
	grainSizeValue = hw.GetKnobValue(DaisyVersio::KNOB_1);
	grainBlendValue = hw.GetKnobValue(DaisyVersio::KNOB_2);
	grainPhaseIncValue = hw.GetKnobValue(DaisyVersio::KNOB_3);

	registry->Prepare(size, hw.AudioSampleRate());
	timeStretcher->Prepare(size, 2, hw.AudioSampleRate());

	for (size_t i = 0; i < size; ++i)
	{
		scratchIn[i] = IN_L[i];
		scratchIn[i + size] = IN_R[i];
	}

	nois::ConstFloatBufferView inView(scratchIn, size, 2);
	nois::FloatBufferView outView(scratchOut, size, 2);
	timeStretcher->Process(inView, outView);

	for (size_t i = 0; i < size; ++i)
	{
		OUT_L[i] = scratchOut[i];
		OUT_R[i] = scratchOut[i + size];
	}
}

int main(void)
{
	// Ideas
	// Dry/wet knob
	// Gate idea is the envelope
	// Pre-record 10 seconds and then play effect back
	// Probability for trigger, but manual button press is 100%

	hw.Init(true);

	umm_init();

	nois::SetAllocFuncs(umm_malloc, umm_free);

	hw.StartAdc();

	registry = nois::MakeRef<nois::Registry<nois::f32_t>>();

	auto stretchTimeMs = registry->CreateBlockConstant(10000.0f);

	auto stretchActive = registry->CreateBinder(
		[](nois::count_t)
		{
			return stretchActiveValue;
		});

	auto stretchFactor = registry->CreateBinder(
		[](nois::count_t)
		{
			return 1.0f + stretchFactorValue * (64.0f - 1.0f);
		});

	auto grainSize = registry->CreateBinder(
		[](nois::count_t)
		{
			return 250.0f + grainSizeValue * (3000.0f - 250.0f);
		});

	auto grainBlend = registry->CreateBinder(
		[](nois::count_t)
		{
			return grainBlendValue / 2.0f;
		});

	auto grainPhaseInc = registry->CreateBinder(
		[](nois::count_t)
		{
			return grainPhaseIncValue;
		});

	timeStretcher = nois::TimeStretcher::Create();
	timeStretcher->SetStretchTimeMs(stretchTimeMs);
	timeStretcher->SetStretchActive(stretchActive);
	timeStretcher->SetStretchFactor(stretchFactor);
	timeStretcher->SetGrainSize(grainSize);
	timeStretcher->SetGrainBlend(grainBlend);
	timeStretcher->SetGrainPhaseInc(grainPhaseInc);

	hw.StartAudio(callback);

	while (true)
	{
		hw.UpdateExample();
		hw.UpdateLeds();
	}
}