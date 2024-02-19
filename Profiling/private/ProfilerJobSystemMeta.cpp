#include "ProfilerJobSystemMeta.h"

#include "JobSystemMeta.h"

namespace
{
	profiling::ProfilerJobSystemMeta m_instance;
}

profiling::ProfilerJobSystemMeta::ProfilerJobSystemMeta() :
	BaseObjectMeta(&jobs::JobSystemMeta::GetInstance())
{
}

const profiling::ProfilerJobSystemMeta& profiling::ProfilerJobSystemMeta::GetInstance()
{
	return m_instance;
}