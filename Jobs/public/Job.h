#pragma once

#include <functional>

namespace jobs
{
	class Job
	{
	public:
		static Job* CreateFromLambda(const std::function<void()>& func);

		virtual void Do() = 0;
		virtual ~Job();
	};
}