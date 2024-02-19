#pragma once

#include "BaseObject.h"

#include "JobSystem.h"

#include <map>
#include <string>

namespace profiling
{
	struct StopwatchInfo
	{
		std::string m_name;
		unsigned long long m_numberOfCalls = 0;
		double m_accumulatedTime = 0.0;
	};

	struct Profile
	{
		std::map<std::string, StopwatchInfo> m_stopwatchData;
	};

	class Profiler : public BaseObject
	{
	private:
		jobs::JobSystem* m_profilerJobSystem = nullptr;
		std::map<std::string, Profile> m_profiles;
	public:
		Profiler();
		virtual ~Profiler();

		jobs::JobSystem* GetProfilerJobSystem();
		void RecordTime(const std::string& profileName, const std::string& stopwatchName, double time);

		void ResetProfile(const std::string& profileName);
		void PrintProfile(const std::string& profileName);
	};
}