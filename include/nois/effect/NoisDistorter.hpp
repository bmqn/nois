#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class Distorter : public ProcessStream
{
public:
	enum Kind
	{
		k_Tanh,
	};

	static Ref_t<Distorter> Create(Kind kind);

	NOIS_INTERFACE(Distorter)
	NOIS_INTERFACE_PARAM(DriveDb, FloatParameter)
	NOIS_INTERFACE_PARAM(MakeupDb, FloatParameter)
	NOIS_INTERFACE_PARAM(Bias, FloatParameter)
	NOIS_INTERFACE_PARAM(Wet, FloatParameter)
};

}
