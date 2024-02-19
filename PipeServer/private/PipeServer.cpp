#include "PipeServer.h"

#include "Jobs.h"
#include "Job.h"
#include "JobSystem.h"
#include "JobSystemMeta.h"

#include "BaseObjectContainer.h"

#include "Lock.h"

#include <iostream>
#include <Windows.h>


namespace
{
	pipe_server::ServerMeta m_instance;
	jobs::JobSystem* m_serverJS = nullptr;
}

pipe_server::ServerMeta::ServerMeta() :
	BaseObjectMeta(nullptr)
{
}

const pipe_server::ServerMeta& pipe_server::ServerMeta::GetInstance()
{
	return m_instance;
}

pipe_server::ServerObject::ServerObject() :
	BaseObject(ServerMeta::GetInstance())
{
	m_serverJS = new jobs::JobSystem(jobs::JobSystemMeta::GetInstance(), 1);
}

pipe_server::ServerObject::~ServerObject()
{
}

void pipe_server::ServerObject::Start()
{
	HANDLE hPipe;

	// Try to open a named pipe; wait for it, if necessary. 
	hPipe = CreateFile(
		TEXT("\\\\.\\pipe\\mynamedpipe"),   // pipe name 
		GENERIC_READ | GENERIC_WRITE,
		0,              // no sharing 
		NULL,           // default security attributes
		OPEN_EXISTING,  // opens existing pipe 
		0,              // default attributes 
		NULL);          // no template file 

	// Break if the pipe handle is valid. 

	if (hPipe == INVALID_HANDLE_VALUE)
	{
		return;
	}

	jobs::Job* serverJob = jobs::Job::CreateFromLambda([=]() {
		const int BUFSIZE = 512;
		TCHAR  chBuf[BUFSIZE];
		BOOL   fSuccess = FALSE;

		DWORD  cbRead, cbToWrite, cbWritten, dwMode;

		while (true)
		{
			// Read from the pipe. 

			fSuccess = ReadFile(
				hPipe,    // pipe handle 
				chBuf,    // buffer to receive reply 
				BUFSIZE * sizeof(TCHAR),  // size of buffer 
				&cbRead,  // number of bytes read 
				NULL);    // not overlapped 

			char* data = reinterpret_cast<char*>(chBuf);
			data[cbRead] = 0;

			if (cbRead > 0) {
				std::string message(data);

				if (message == "shutdown") {
					jobs::RunSync(jobs::Job::CreateFromLambda([]() {
						BaseObjectContainer& container = BaseObjectContainer::GetInstance();
						lock::LockObject* lock = static_cast<lock::LockObject*>(container.GetObjectOfClass(lock::LockMeta::GetInstance()));
						lock->UnLock();
					}));

					break;
				}

				DWORD written;
				WriteFile(
					hPipe,
					message.c_str(),
					message.size() * sizeof(char),
					&written,
					NULL
				);
			}
		}

		CloseHandle(hPipe);
	});

	m_serverJS->ScheduleJob(serverJob);
}
