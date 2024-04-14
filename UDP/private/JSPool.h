#pragma once

#include "BaseObjectMeta.h"
#include "BaseObject.h"

#include "Common.h"

#include "JobSystem.h"

#include <list>
#include <mutex>

namespace udp
{
	class JSPoolMeta : public BaseObjectMeta
	{
	public:
		static const JSPoolMeta& GetInstance();
		JSPoolMeta();
	};

	class JSPool : public BaseObject
	{
	private:
		std::mutex m_mutex;
		std::list<jobs::JobSystem*> m_pool;

	public:
		JSPool(size_t poolSize, size_t jsSize);
		virtual ~JSPool();

		jobs::JobSystem* AcquireJS();
		void ReleaseJS(jobs::JobSystem* js);
	};
}