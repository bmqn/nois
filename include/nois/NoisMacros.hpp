#pragma once

#if defined(__x86_64__) || defined(_M_X64)
#define NOIS_ARCH_X64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#define NOIS_ARCH_ARM64 1
#else
#error "Unknown architecture"
#endif

#if defined(_WIN32)
#define NOIS_TARGET_WINDOWS 1
#elif defined(__linux__)
#define NOIS_TARGET_LINUX 1
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#define NOIS_TARGET_IOS 1
#elif TARGET_OS_MAC
#define NOIS_TARGET_MAC 1
#else
#error "Unknown Apple platform"
#endif
#endif


#define NOIS_INTERFACE_GETTER(_name, _type) \
	Ref_t<_type> Get##_name() const;

#define NOIS_INTERFACE_SETTER(_name, _type) \
	void Set##_name(Ref_t<_type> value);

#define NOIS_INTERFACE_PARAM(_name, _type) \
	NOIS_INTERFACE_GETTER(_name, _type) \
	NOIS_INTERFACE_SETTER(_name, _type)
