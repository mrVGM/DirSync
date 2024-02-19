#include "ProfilerMeta.h"

namespace
{
	profiling::ProfilerMeta m_instance;
}

profiling::ProfilerMeta::ProfilerMeta() :
	BaseObjectMeta(nullptr)
{
}

const profiling::ProfilerMeta& profiling::ProfilerMeta::GetInstance()
{
	return m_instance;
}