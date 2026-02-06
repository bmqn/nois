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
		k_DynamicTanh,
	};

	static Ref_t<Distorter> Create(Kind kind);

	NOIS_INTERFACE_VIRT_PARAM(DriveDb, FloatBlockParameter)
	NOIS_INTERFACE_VIRT_PARAM(MakeupDb, FloatBlockParameter)
	NOIS_INTERFACE_VIRT_PARAM(Wet, FloatBlockParameter)
};

class DynamicTanhDistorter : public Distorter
{
public:
	static Ref_t<DynamicTanhDistorter> Create();

	NOIS_INTERFACE(DynamicTanhDistorter)
	NOIS_INTERFACE_VIRT_PARAM_USE(DriveDb, FloatBlockParameter)
	NOIS_INTERFACE_VIRT_PARAM_USE(MakeupDb, FloatBlockParameter)
	NOIS_INTERFACE_VIRT_PARAM_USE(Wet, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(Shape, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(Asym, FloatBlockParameter)
};

}
