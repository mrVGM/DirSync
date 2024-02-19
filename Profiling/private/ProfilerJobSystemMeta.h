#pragma once

#include "BaseObjectMeta.h"

namespace profiling
{
	class ProfilerJobSystemMeta : public BaseObjectMeta
	{
	public:
		static const ProfilerJobSystemMeta& GetInstance();
		ProfilerJobSystemMeta();
	};
}