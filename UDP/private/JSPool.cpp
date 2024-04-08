#include "JSPool.h"

#include "JobSystemMeta.h"

udp::JSPool::JSPool(size_t count, size_t size)
{
	for (size_t i = 0; i < count; ++i)
	{
		jobs::JobSystem* js = new jobs::JobSystem(jobs::JobSystemMeta::GetInstance(), size);
		m_pool.push_back(js);
	}
}

jobs::JobSystem* udp::JSPool::GetJS()
{
	m_mutex.lock();

	jobs::JobSystem* js = m_pool.front();
	m_pool.pop_front();

	m_mutex.unlock();

	return js;
}

void udp::JSPool::ReleaseJS(jobs::JobSystem& js)
{
	m_mutex.lock();

	m_pool.push_back(&js);

	m_mutex.unlock();
}