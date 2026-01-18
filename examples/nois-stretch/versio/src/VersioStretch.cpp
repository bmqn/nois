#include "daisy_versio.h"

#include <nois/Nois.hpp>

using namespace daisy;

DaisyVersio hw;

float scratch[512] = { 0.0f };
float stretchActiveValue = 0.0f;
float stretchFactorValue = 0.0f;

nois::Ref_t<nois::FloatParameter> stretchActive = nullptr;
nois::Ref_t<nois::FloatParameter> stretchFactor = nullptr;
nois::Ref_t<nois::TimeStretcher> timeStretcher = nullptr;

void callback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	stretchActive->Prepare(256, 48000);
	stretchFactor->Prepare(256, 48000);
	timeStretcher->Prepare(256, 2, 48000);

	std::memcpy(scratch       , in[0], size * sizeof(float));
	std::memcpy(scratch + size, in[1], size * sizeof(float));

	nois::ConstFloatBufferView inView(scratch, size, 2);
	nois::FloatBufferView outView(scratch, size, 2);
	timeStretcher->Process(inView, outView);

	std::memcpy(out[0], scratch       , size * sizeof(float));
	std::memcpy(out[1], scratch + size, size * sizeof(float));
}

int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(256);
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

	stretchActive = nois::MakeRef<nois::FloatBinderParameter>(
		[](nois::count_t)
		{
			return stretchActiveValue;
		});

	stretchFactor = nois::MakeRef<nois::FloatBinderParameter>(
		[](nois::count_t)
		{
			return stretchFactorValue;
		});

	timeStretcher = nois::TimeStretcher::Create();
	timeStretcher->SetStretchActive(stretchActive);
	timeStretcher->SetStretchFactor(stretchFactor);

	stretchActive->Prepare(256, 48000);
	stretchFactor->Prepare(256, 48000);
	timeStretcher->Prepare(256, 2, 48000);

	hw.StartAudio(callback);
	hw.StartAdc();

	while (true)
	{
		hw.ProcessAnalogControls();

		stretchActiveValue = hw.GetKnobValue(DaisyVersio::KNOB_0);
		stretchFactorValue = hw.GetKnobValue(DaisyVersio::KNOB_1);

		hw.UpdateLeds();

		System::Delay(1);
	}
}