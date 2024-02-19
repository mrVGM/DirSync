#pragma once

#include "BaseObjectMeta.h"

namespace jobs
{
	class JobSystemMeta : public BaseObjectMeta
	{
	public:
		static const JobSystemMeta& GetInstance();
		JobSystemMeta();
	};

	class MainJobSystemMeta : public BaseObjectMeta
	{
	public:
		static const MainJobSystemMeta& GetInstance();
		MainJobSystemMeta();
	};

	class AsyncJobSystemMeta : public BaseObjectMeta
	{
	public:
		static const AsyncJobSystemMeta& GetInstance();
		AsyncJobSystemMeta();
	};
}