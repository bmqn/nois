#include "daisy_versio.h"

#include <nois/Nois.hpp>

using namespace daisy;

DaisyVersio hw;

static constexpr size_t kBlockSize = 128;

static float scratch[2 * kBlockSize] = { 0.0f };
static float stretchActiveValue = 0.0f;
static float stretchFactorValue = 0.0f;

static nois::Ref_t<nois::FloatParameter> stretchActive = nullptr;
static nois::Ref_t<nois::FloatParameter> stretchFactor = nullptr;
static nois::Ref_t<nois::TimeStretcher> timeStretcher = nullptr;

void callback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	stretchActive->Prepare(size, hw.AudioSampleRate());
	stretchFactor->Prepare(size, hw.AudioSampleRate());
	timeStretcher->Prepare(size, 2, hw.AudioSampleRate());

	std::memcpy(scratch             , in[0], size * sizeof(float));
	std::memcpy(scratch + kBlockSize, in[1], size * sizeof(float));

	nois::ConstFloatBufferView inView(scratch, size, 2);
	nois::FloatBufferView outView(scratch, size, 2);
	timeStretcher->Process(inView, outView);

	std::memcpy(out[0], scratch             , size * sizeof(float));
	std::memcpy(out[1], scratch + kBlockSize, size * sizeof(float));
}

int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(kBlockSize);

	stretchActive = nois::MakeRef<nois::FloatBinderParameter>(
		[](nois::count_t)
		{
			return stretchActiveValue;
		});

	stretchFactor = nois::MakeRef<nois::FloatBinderParameter>(
		[](nois::count_t)
		{
			return 1.0f + stretchFactorValue * (16.0f - 1.0f);
		});

	timeStretcher = nois::TimeStretcher::Create();
	timeStretcher->SetStretchActive(stretchActive);
	timeStretcher->SetStretchFactor(stretchFactor);

	hw.StartAudio(callback);
	hw.StartAdc();

	while (true)
	{
		hw.ProcessAnalogControls();

		hw.UpdateExample();

		stretchActiveValue = hw.SwitchPressed() ? 1.0f : 0.0f;
		stretchFactorValue = hw.GetKnobValue(DaisyVersio::KNOB_0);

		hw.UpdateLeds();
	}
}