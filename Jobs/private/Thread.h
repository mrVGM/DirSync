#pragma once

#include <thread>
#include <semaphore>

namespace jobs
{
	class Job;
	class JobSystem;

	class Thread
	{
	private:
		bool m_busy = false;
		std::binary_semaphore m_semaphore{ 1 };
		std::thread* m_thread = nullptr;

		bool m_stopped = false;
		jobs::JobSystem& m_jobSystem;
	public:
		Thread(JobSystem& jobSystem);
		~Thread();

		void Start();
		void Boot();
		void Stop();
		bool ShouldStop();

		jobs::Job* GetJob();

		bool IsBusy() const;
		void SetBusy(bool busy);
		std::binary_semaphore& GetSemaphore();
		jobs::JobSystem& GetJobSystem();
	};
}