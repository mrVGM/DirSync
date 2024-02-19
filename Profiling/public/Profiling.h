#pragma once

#include "BaseObject.h"

#include "JobSystem.h"

#include <string>
#include <chrono>

namespace profiling
{
	class StopWatch
	{
	private:
		std::chrono::system_clock::time_point m_timeCreated;
		std::string m_profileName;
		std::string m_stopwatchName;
	public:
		StopWatch() = delete;
		StopWatch(const std::string& profileName, const std::string& stopwatchName);
		StopWatch(const StopWatch& other) = delete;
		StopWatch& operator=(StopWatch& other) = delete;
		~StopWatch();

		void StartStopwatch();
	};


	void Boot();
	void ResetProfile(const std::string& name);
	void PrintProfile(const std::string& name);
}