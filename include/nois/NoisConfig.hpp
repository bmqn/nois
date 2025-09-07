#pragma once

#include "NoisTypes.hpp"

namespace nois {

// Max number of supported channels
//
// We use this to make assumptions about data for faster code.
constexpr count_t k_MaxChannels = 2;

// Max size of in-place cache buffers for frames
//
// We use in-place small vectors to store a fixed number of cached frames.
// The vectors will dynamically move to heap allocated vectors if exceeded.
// Using buffer sizes equal or less than this should improve cache hits.
constexpr count_t k_CacheOptimisedNumFrames = 1024;

constexpr f32_t k_DcOffset = 0.005f;

}
