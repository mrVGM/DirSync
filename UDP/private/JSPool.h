#pragma once

#include "Common.h"

#include "JobSystem.h"

#include <list>
#include <mutex>

namespace udp
{
	class JSPool
	{
	private:
		std::mutex m_mutex;
		std::list<jobs::JobSystem*> m_pool;

	public:
		JSPool(size_t poolSize, size_t jsSize);

		jobs::JobSystem* AcquireJS();
		void ReleaseJS(jobs::JobSystem* js);
	};
}