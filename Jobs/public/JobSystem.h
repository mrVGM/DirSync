#pragma once

#include "BaseObject.h"

#include <queue>
#include <list>

#include <mutex>

namespace jobs
{
	class Job;
	class JobSystem;
	class Thread;

	class JobSystem : public BaseObject
	{
		friend class Thread;
	private:
		std::queue<Job*> m_jobQueue;
		std::list<Thread*> m_threads;
		std::mutex m_mutex;
		std::mutex m_bootMutex;

		jobs::Job* AcquireJob();
		void BootThread();
	public:
		JobSystem(const BaseObjectMeta& meta, int numThreads);
		virtual ~JobSystem();

		void ScheduleJob(Job* job);
	};
}