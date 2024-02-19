#include "Job.h"

jobs::Job::~Job()
{
}

jobs::Job* jobs::Job::CreateFromLambda(const std::function<void()>& func)
{
	class LambdaJob : public Job
	{
	private:
		std::function<void()> m_func;

	public:
		LambdaJob(const std::function<void()>& func) :
			m_func(func)
		{
		}
		virtual ~LambdaJob()
		{
		}

		void Do() override
		{
			m_func();
		}
	};

	jobs::Job* res = new LambdaJob(func);
	return res;
}