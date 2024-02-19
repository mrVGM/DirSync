#pragma once

namespace jobs
{
	class Job;

	void Boot();

	void RunSync(Job* job);
	void RunAsync(Job* job);
}