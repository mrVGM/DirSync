#include "BaseFramework.h"

#include "JSONParser.h"
#include "Jobs.h"
#include "Job.h"
#include "Lock.h"
#include "PipeServer.h"

#include <filesystem>
#include <iostream>


int main(int args, const char** argv)
{
	lock::LockObject* lock = new lock::LockObject(lock::MainLockMeta::GetInstance());
	lock->Lock();

	json_parser::Boot();
	jobs::Boot();

	jobs::RunSync(jobs::Job::CreateFromLambda([=]() {
		pipe_server::ServerObject* server = new pipe_server::ServerObject();
		server->Start();
	}));

	lock->Lock();

	return 0;
}
