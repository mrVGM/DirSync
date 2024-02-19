#include "Profiling.h"

#include "Profiler.h"

#include "Job.h"

namespace
{
	profiling::Profiler* m_profiler = nullptr;
}

void profiling::Boot()
{
	if (m_profiler)
	{
		return;
	}

	m_profiler = new profiling::Profiler();
}

void profiling::PrintProfile(const std::string& name)
{
	struct Context
	{
		std::string m_profileName;
	};

	class PrintJob : public jobs::Job
	{
	private:
		Context m_ctx;
	public:
		PrintJob(const Context& ctx) :
			m_ctx(ctx)
		{
		}

		void Do() override
		{
			m_profiler->PrintProfile(m_ctx.m_profileName);
		}
	};

	Context ctx{ name };
	m_profiler->GetProfilerJobSystem()->ScheduleJob(new PrintJob(ctx));
}

void profiling::ResetProfile(const std::string& name)
{
	struct Context
	{
		std::string m_profileName;
	};

	class ResetProfileJob : public jobs::Job
	{
	private:
		Context m_ctx;
	public:
		ResetProfileJob(const Context& ctx) :
			m_ctx(ctx)
		{
		}

		void Do() override
		{
			m_profiler->ResetProfile(m_ctx.m_profileName);
		}
	};

	Context ctx{ name };
	m_profiler->GetProfilerJobSystem()->ScheduleJob(new ResetProfileJob(ctx));
}

profiling::StopWatch::StopWatch(const std::string& profileName, const std::string& stopwatchName) :
	m_profileName(profileName),
	m_stopwatchName(stopwatchName)
{
	StartStopwatch();
}

profiling::StopWatch::~StopWatch()
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	auto nowNN = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
	auto timeCreatedNN = std::chrono::time_point_cast<std::chrono::nanoseconds>(m_timeCreated);
	long long deltaNN = nowNN.time_since_epoch().count() - timeCreatedNN.time_since_epoch().count();
	double dt = deltaNN / 1000000000.0;

	struct Context
	{
		std::string m_profileName;
		std::string m_stopwatchName;
		double m_timeElapsed = 0.0;
	};

	class RecordTimeJob : public jobs::Job
	{
	private:
		Context m_ctx;
	public:
		RecordTimeJob(const Context& ctx) :
			m_ctx(ctx)
		{
		}

		void Do() override
		{
			m_profiler->RecordTime(m_ctx.m_profileName, m_ctx.m_stopwatchName, m_ctx.m_timeElapsed);
		}
	};

	Context ctx{ m_profileName, m_stopwatchName, dt };
	m_profiler->GetProfilerJobSystem()->ScheduleJob(new RecordTimeJob(ctx));
}


void profiling::StopWatch::StartStopwatch()
{
	m_timeCreated = std::chrono::system_clock::now();
}