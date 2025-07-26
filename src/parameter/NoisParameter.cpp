#include "nois/parameter/NoisParameter.hpp"

namespace nois {

Ref_t<FloatBinderParameter> CreateFloatParameter(
	FloatBinderFunc_t binder)
{
	return CreateParameter<f32_t>(binder);
}

}
