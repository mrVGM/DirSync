#include "Jobs.h"

#include "Job.h"

#include "JobSystemMeta.h"
#include "JobSystem.h"

#include "BaseObjectContainer.h"

namespace
{
	jobs::JobSystem* m_main = nullptr;
	jobs::JobSystem* m_async = nullptr;
}

void jobs::Boot()
{
	m_main = new JobSystem(MainJobSystemMeta::GetInstance(), 1);
	m_async = new JobSystem(AsyncJobSystemMeta::GetInstance(), 25);

	RunSync(Job::CreateFromLambda([]() {
		BaseObjectContainer::GetInstance().StartExclusiveThreadAccess();
	}));
}

void jobs::RunSync(Job* job)
{
	m_main->ScheduleJob(job);
}

void jobs::RunAsync(Job* job)
{
	m_async->ScheduleJob(job);
}
