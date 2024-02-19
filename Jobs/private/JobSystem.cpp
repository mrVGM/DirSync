#include "JobSystem.h"

#include "Job.h"

#include "JobSystemMeta.h"
#include "Thread.h"

jobs::JobSystem::JobSystem(const BaseObjectMeta& meta, int numThreads) :
	BaseObject(meta)
{
	for (int i = 0; i < numThreads; ++i)
	{
		m_threads.push_back(new Thread(*this));
	}
}

jobs::JobSystem::~JobSystem()
{
	for (auto it = m_threads.begin(); it != m_threads.end(); ++it)
	{
		delete (*it);
	}

	while (!m_jobQueue.empty())
	{
		Job* cur = m_jobQueue.front();
		m_jobQueue.pop();
		delete cur;
	}
}

void jobs::JobSystem::BootThread()
{
	for (auto it = m_threads.begin(); it != m_threads.end(); ++it)
	{
		if ((*it)->IsBusy())
		{
			continue;
		}

		(*it)->Boot();
		return;
	}
}

void jobs::JobSystem::ScheduleJob(Job* job)
{
	m_mutex.lock();
	m_jobQueue.push(job);
	m_mutex.unlock();

	m_bootMutex.lock();
	BootThread();
	m_bootMutex.unlock();
}

jobs::Job* jobs::JobSystem::AcquireJob()
{
	Job* job = nullptr;
	m_mutex.lock();
	if (!m_jobQueue.empty())
	{
		job = m_jobQueue.front();
		m_jobQueue.pop();
	}
	m_mutex.unlock();

	return job;
}