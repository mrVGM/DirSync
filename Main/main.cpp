#include "BaseFramework.h"

#include "JSONParser.h"
#include "Jobs.h"
#include "Job.h"
#include "Lock.h"
#include "PipeServer.h"

#include <filesystem>
#include <iostream>

#include "UDP.h"


int main(int args, const char** argv)
{
	lock::LockObject* lock = new lock::LockObject(lock::MainLockMeta::GetInstance());
	lock->Lock();

	json_parser::Boot();
	jobs::Boot();

	udp::UDPReq req;
	req.UpBit(34);
	bool t = req.GetBitState(34);
	t = req.GetBitState(33);
	t = req.GetBitState(35);

	jobs::RunSync(jobs::Job::CreateFromLambda([=]() {
		pipe_server::ServerObject* server = new pipe_server::ServerObject();
		server->Start();
		server->StartOut();
	}));

	lock->Lock();
	
	return 0;
}
