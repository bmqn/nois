#pragma once

#include "NoisTypes.hpp"

#include <new>

namespace nois {

// Size of a cache line
//
// We use this to align data for faster access.
#ifndef __cpp_lib_hardware_interference_size
constexpr count_t kCacheLineSize = 64;
#else
constexpr count_t kCacheLineSize = std::hardware_destructive_interference_size;
#endif

// Max number of supported channels
//
// We use this to make assumptions about data for faster code.
constexpr count_t k_MaxChannels = 2;

// Max size of in-place data
//
// We use in-place small vectors to store a fixed number of frames.
// The vectors will dynamically move to heap allocated vectors if exceeded.
// Using buffer sizes equal or less than give faster access.
constexpr count_t k_MaxNumInplaceFrames = 1024;

}
