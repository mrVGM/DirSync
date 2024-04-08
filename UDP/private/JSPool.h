#pragma once

#include "JobSystem.h"

#include <list>
#include <mutex>

namespace udp
{
	class JSPool
	{
	private:
		std::list<jobs::JobSystem*> m_pool;
		std::mutex m_mutex;

	public:
		JSPool(size_t count, size_t size);

		jobs::JobSystem* GetJS();
		void ReleaseJS(jobs::JobSystem& js);
	};
}