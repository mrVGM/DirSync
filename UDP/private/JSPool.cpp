#include "JSPool.h"

#include "JobSystemMeta.h"
#include "Job.h"
#include "Jobs.h"

namespace
{
	udp::JSPoolMeta m_jsPoolMeta;
}

const udp::JSPoolMeta& udp::JSPoolMeta::GetInstance()
{
	return m_jsPoolMeta;
}

udp::JSPoolMeta::JSPoolMeta() :
	BaseObjectMeta(nullptr)
{
}

udp::JSPool::JSPool(size_t poolSize, size_t jsSize) :
	BaseObject(JSPoolMeta::GetInstance())
{
	m_mutex.lock();
	
	for (size_t i = 0; i < poolSize; ++i)
	{
		jobs::JobSystem* js = new jobs::JobSystem(jobs::JobSystemMeta::GetInstance(), jsSize);
		m_pool.push_back(js);
	}

	m_mutex.unlock();
}

udp::JSPool::~JSPool()
{
	for (auto it = m_pool.begin(); it != m_pool.end(); ++it)
	{
		jobs::JobSystem* cur = *it;
		delete cur;
	}
}

jobs::JobSystem* udp::JSPool::AcquireJS()
{
	jobs::JobSystem* res = nullptr;

	m_mutex.lock();

	if (!m_pool.empty())
	{
		res = m_pool.front();
		m_pool.pop_front();
	}

	m_mutex.unlock();

	return res;
}

void udp::JSPool::ReleaseJS(jobs::JobSystem* js)
{
	m_mutex.lock();

	m_pool.push_back(js);

	m_mutex.unlock();
}
