#include <nois/Nois.hpp>

#include <public.sdk/source/main/moduleinit.h>

using namespace Steinberg;

#if NOIS_ENABLE_PROFILING
static ModuleInitializer gInitTracy([]()
{
	tracy::StartupProfiler();
});
static ModuleTerminator gTermTracy([]()
{
	tracy::ShutdownProfiler();
});
#endif // NOIS_ENABLE_PROFILING