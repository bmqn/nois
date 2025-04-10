#include "nois/core/NoisParameter.hpp"

namespace nois {

Ref_t<FloatBinderParameter> CreateParameter(BinderFunc_t<float> binder)
{
	return MakeRef<FloatBinderParameter>(binder);
}

}
