#include "Thread.h"

#include "Job.h"
#include "JobSystem.h"


namespace
{
	std::binary_semaphore m_initThreadSemaphore{ 1 };
	std::mutex m_initThreadMutex;

	jobs::Thread* m_curThread = nullptr;

	void run()
	{
		jobs::Thread& thread = *m_curThread;
		m_initThreadSemaphore.release();

		while (true)
		{
			thread.GetSemaphore().acquire();
			if (thread.ShouldStop())
			{
				return;
			}

			while (true)
			{
				jobs::Job* job = thread.GetJob();
				if (!job)
				{
					thread.SetBusy(false);
					break;
				}

				job->Do();
				delete job;
			}
		}
	}
}

jobs::Thread::Thread(JobSystem& jobSystem) :
	m_jobSystem(jobSystem)
{
	Start();
}

jobs::Thread::~Thread()
{
	Stop();
	m_thread->join();
	delete m_thread;
}

void jobs::Thread::Start()
{
	m_initThreadMutex.lock();
	m_initThreadSemaphore.acquire();
	m_semaphore.acquire();

	m_curThread = this;
	m_thread = new std::thread(run);

	m_initThreadSemaphore.acquire();
	m_initThreadSemaphore.release();

	m_initThreadMutex.unlock();
}

void jobs::Thread::Boot()
{
	if (m_busy)
	{
		throw "Already Busy!";
	}

	m_busy = true;
	m_semaphore.release();
}

void jobs::Thread::Stop()
{
	m_stopped = true;
}

bool jobs::Thread::ShouldStop()
{
	return m_stopped;
}

jobs::Job* jobs::Thread::GetJob()
{
	return m_jobSystem.AcquireJob();
}

std::binary_semaphore& jobs::Thread::GetSemaphore()
{
	return m_semaphore;
}

jobs::JobSystem& jobs::Thread::GetJobSystem()
{
	return m_jobSystem;
}

bool jobs::Thread::IsBusy() const
{
	return m_busy;
}

void jobs::Thread::SetBusy(bool busy)
{
	m_busy = busy;
}
