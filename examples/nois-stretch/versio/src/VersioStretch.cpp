#include "daisy_versio.h"

#include <umm_malloc.h>

#include <nois/Nois.hpp>

using namespace daisy;

DaisyVersio hw;

static constexpr size_t kBlockSize = 128;

static uint8_t DSY_SDRAM_BSS sdram_heap[64 * 1024 * 1024];

static float scratchIn[2 * kBlockSize] = { 0.0f };
static float scratchOut[2 * kBlockSize] = { 0.0f };

static float stretchActiveValue = 0.0f;
static float stretchFactorValue = 0.0f;
static float grainSizeValue = 0.0f;

static nois::Ref_t<nois::ParameterRegistry<nois::f32_t>> registry = nullptr;
static nois::Ref_t<nois::FloatParameter> stretchActive = nullptr;
static nois::Ref_t<nois::FloatParameter> stretchFactor = nullptr;
static nois::Ref_t<nois::FloatParameter> grainSize = nullptr;
static nois::Ref_t<nois::TimeStretcher> timeStretcher = nullptr;

void* operator new(size_t size) { return umm_malloc(size); }
void* operator new[](size_t size) { return umm_malloc(size); }
void operator delete(void* ptr) noexcept { umm_free(ptr); }
void operator delete[](void* ptr) noexcept { umm_free(ptr); }
void operator delete(void* ptr, size_t) noexcept { umm_free(ptr); }
void operator delete[](void* ptr, size_t) noexcept { umm_free(ptr); }

void callback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	registry->Prepare(size, hw.AudioSampleRate());
	timeStretcher->Prepare(size, 2, hw.AudioSampleRate());

	std::memcpy(scratchIn             , in[0], size * sizeof(float));
	std::memcpy(scratchIn + kBlockSize, in[1], size * sizeof(float));

	nois::ConstFloatBufferView inView(scratchIn, size, 2);
	nois::FloatBufferView outView(scratchOut, size, 2);
	timeStretcher->Process(inView, outView);

	std::memcpy(out[0], scratchOut             , size * sizeof(float));
	std::memcpy(out[1], scratchOut + kBlockSize, size * sizeof(float));
}

int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(kBlockSize);

	umm_init_heap(sdram_heap, sizeof(sdram_heap));

	registry = nois::MakeRef<nois::ParameterRegistry<nois::f32_t>>();

	stretchActive = registry->CreateBinder(
		[](nois::count_t)
		{
			return stretchActiveValue;
		});

	stretchFactor = registry->CreateBinder(
		[](nois::count_t)
		{
			return 1.0f + stretchFactorValue * (16.0f - 1.0f);
		});

	grainSize = registry->CreateBinder(
		[](nois::count_t)
		{
			return 250.0f + grainSizeValue * (3000.0f - 250.0f);
		});

	timeStretcher = nois::TimeStretcher::Create();
	timeStretcher->SetStretchActive(stretchActive);
	timeStretcher->SetStretchFactor(stretchFactor);
	timeStretcher->SetGrainSize(grainSize);

	hw.StartAudio(callback);
	hw.StartAdc();

	while (true)
	{
		hw.ProcessAnalogControls();

		stretchActiveValue = hw.SwitchPressed() ? 1.0f : 0.0f;
		stretchFactorValue = hw.GetKnobValue(DaisyVersio::KNOB_0);
		grainSizeValue = hw.GetKnobValue(DaisyVersio::KNOB_1);

		hw.UpdateLeds();
	}
}