#include "PipeServer.h"

#include "Jobs.h"
#include "Job.h"
#include "JobSystem.h"
#include "JobSystemMeta.h"

#include "BaseObjectContainer.h"

#include "Lock.h"
#include "JSONValue.h"

#include <iostream>
#include <Windows.h>


namespace
{
	pipe_server::ServerMeta m_instance;
	jobs::JobSystem* m_serverJS = nullptr;
	jobs::JobSystem* m_serverResponseJS = nullptr;
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
	m_serverResponseJS = new jobs::JobSystem(jobs::JobSystemMeta::GetInstance(), 1);
}

pipe_server::ServerObject::~ServerObject()
{
}

void pipe_server::ServerObject::Start()
{
	// Try to open a named pipe; wait for it, if necessary. 
	m_hPipe = CreateFile(
		TEXT("\\\\.\\pipe\\mynamedpipe"),   // pipe name 
		GENERIC_READ | GENERIC_WRITE,
		0,              // no sharing 
		NULL,           // default security attributes
		OPEN_EXISTING,  // opens existing pipe 
		0,              // default attributes 
		NULL);          // no template file 

	// Break if the pipe handle is valid. 

	if (m_hPipe == INVALID_HANDLE_VALUE)
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
				m_hPipe,    // pipe handle 
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

				json_parser::JSONValue req(json_parser::ValueType::Object);
				json_parser::JSONValue::FromString(message, req);

				HandleReq(req);
			}
		}

		CloseHandle(m_hPipe);
	});

	m_serverJS->ScheduleJob(serverJob);
}


void pipe_server::ServerObject::HandleReq(const json_parser::JSONValue& req)
{
	using namespace json_parser;

	JSONValue& tmp = const_cast<JSONValue&>(req);
	const auto& map = tmp.GetAsObj();

	int reqId = static_cast<int>(std::get<double>(map.find("id")->second.m_payload));
	std::string op = std::get<std::string>(map.find("op")->second.m_payload);

	if (op == "hash")
	{
		return;
	}

	JSONValue res(ValueType::Object);
	auto& resMap = res.GetAsObj();
	resMap["id"] = JSONValue(static_cast<double>(reqId));
	SendResponse(res);
}

void pipe_server::ServerObject::SendResponse(const json_parser::JSONValue& res)
{	
	m_serverResponseJS->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
		std::string resp = res.ToString(false);

		DWORD written;
		WriteFile(
			m_hPipe,
			resp.c_str(),
			resp.size() * sizeof(char),
			&written,
			NULL
		);
	}));
}