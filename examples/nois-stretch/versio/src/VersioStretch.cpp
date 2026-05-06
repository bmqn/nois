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

static float timeSec = 0.0f;

static bool prevGate = false;
static float beatSec = 0.0f;
static float lastBeatSec;

static int swingStepIndex = 0;
static float pendingSwingTrigger = 0.0f;
static float swingLedSec = 0.0f;

static float stretchActiveEnvSec = 0.0f;

static float stretchActiveValue = 0.0f;
static float stretchFactorValue = 0.0f;
static float grainSizeValue = 0.0f;
static float grainBlendValue = 0.5f;
static float grainPhaseIncValue = 1.0f;

static nois::Ref_t<nois::Registry<nois::f32_t>> registry = nullptr;
static nois::Ref_t<nois::TimeStretcher> timeStretcher = nullptr;

enum
{
	KNOB_SWING = DaisyVersio::KNOB_6,
};

static bool IsGatePulse(bool gate)
{
	bool pulseGate = gate && !prevGate;
	prevGate = gate;
	return pulseGate;
}

static void callback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	hw.ProcessAllControls();
	hw.tap.Debounce();

	float deltaSec = static_cast<float>(size) / hw.AudioSampleRate();

	bool gatePulse = IsGatePulse(hw.Gate());

	if (gatePulse)
	{
		float edgeDelta = timeSec - lastBeatSec;
		// smooth estimate (important!)
		beatSec = 0.9f * beatSec + 0.1f * edgeDelta;
		lastBeatSec = timeSec;
	}

	bool swingPulse = false;

	if (gatePulse)
	{
		float swing = hw.GetKnobValue(KNOB_SWING);
		bool isOffBeat = (swingStepIndex % 2 == 1);
		pendingSwingTrigger = isOffBeat ? beatSec * swing : 0.0f;
		++swingStepIndex;
	}
	if (pendingSwingTrigger >= 0.0f)
	{
		pendingSwingTrigger -= deltaSec;
		if (pendingSwingTrigger <= 0.0f)
		{
			swingPulse = true;
			pendingSwingTrigger = -1.0f;
		}
	}

	// If the envelope active
	if (stretchActiveEnvSec > 0.0f)
	{
		stretchActiveEnvSec -= deltaSec;
	}
	// If the gate swing is rising edge
	if (swingPulse)
	{
		// Probability of envelope trigger
		// Offset to account for noise in knob
		float probability = std::max(0.0f, hw.GetKnobValue(DaisyVersio::KNOB_4) - 0.05f);
		if (Random::GetFloat(0.0f, 1.0f) < probability)
		{
			// Envelope length
			// Up to 500 ms long
			float envelopeLength = hw.GetKnobValue(DaisyVersio::KNOB_5);
			stretchActiveEnvSec = 0.5f * envelopeLength;
		}
	}

	if (swingPulse)
	{
		swingLedSec = 5.0f / 1000.0f;
	}
	if (swingLedSec >= 0.0f)
	{
		swingLedSec -= deltaSec;
		if (swingLedSec <= 0.0f)
		{
			swingLedSec = -1.0f;
		}
	}
	hw.SetLed(DaisyVersio::LED_1, 0.0f, swingLedSec >= 0.0f, 0.0f);

	// hw.UpdateExample();
	hw.UpdateLeds();

	stretchActiveValue = (hw.SwitchPressed() || stretchActiveEnvSec > 0.0f) ? 1.0f : 0.0f;
	stretchFactorValue = hw.GetKnobValue(DaisyVersio::KNOB_0);
	grainSizeValue = hw.GetKnobValue(DaisyVersio::KNOB_1);
	grainBlendValue = hw.GetKnobValue(DaisyVersio::KNOB_2);

	float grainPhaseIncKnob = hw.GetKnobValue(DaisyVersio::KNOB_3);
	grainPhaseIncValue = 1.0f + 2.0f * (grainPhaseIncKnob - 0.5f);

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

	timeSec += deltaSec;
}

int main(void)
{
	// Ideas
	// Dry/wet knob
	// Gate idea is the envelope
	// Pre-record 10 seconds and then play effect back
	// Probability for trigger, but manual button press is 100%
	//
	// Switch to C then while you hold the button it records and when you let go it stops
	// Once you have a recording you can go crazy with pitch
	// Maybe let you record into multiple banks and then switch between them
	// Three modes: realtime, bank A, and bank C

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
	}
}