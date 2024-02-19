#include "BaseFramework.h"

#include "JSONParser.h"
#include "Jobs.h"
#include "Job.h"
#include "Lock.h"
#include "PipeServer.h"

#include "Hash.h"

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

	crypto::SHA256_CTX ctx;
	crypto::Init(ctx);
	const char* tmp = "Hello";
	crypto::Update(
		ctx,
		(const unsigned char*)tmp,
		strlen(tmp));
	std::string res = crypto::Finalize(ctx);

	std::cout << res << std::endl;

	lock->Lock();


	return 0;
}
