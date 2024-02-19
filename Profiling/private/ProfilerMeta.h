#pragma once

#include "BaseObjectMeta.h"

namespace profiling
{
	class ProfilerMeta : public BaseObjectMeta
	{
	public:
		static const ProfilerMeta& GetInstance();
		ProfilerMeta();
	};
}