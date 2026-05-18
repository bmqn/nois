#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {
class DynamicTanhDistorter : public Stream<f32_t>
{
public:
	NOIS_INTERFACE(DynamicTanhDistorter)
	NOIS_INTERFACE_PARAM(DriveDb, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(MakeupDb, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(Wet, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(Shape, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(Asym, FloatBlockParameter)
};

}
