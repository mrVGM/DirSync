#include "PipeServer.h"

#include "Jobs.h"
#include "Job.h"
#include "JobSystem.h"
#include "JobSystemMeta.h"

#include "BaseObjectContainer.h"

#include "Lock.h"
#include "JSONValue.h"

#include "Hash.h"

#include "FileManager.h"

#include "UDP.h"
#include "FileDownloader.h"

#include <iostream>
#include <string>
#include <Windows.h>


namespace
{
	pipe_server::ServerMeta m_instance;
	jobs::JobSystem* m_serverJS = nullptr;
	jobs::JobSystem* m_serverResponseJS = nullptr;

	udp::FileManagerObject* m_fileManager = nullptr;
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
		TEXT("\\\\.\\pipe\\mynamedpipe_js_to_cpp"),   // pipe name 
		GENERIC_READ,
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
		TCHAR  chBuf[BUFSIZE + 1];
		BOOL   fSuccess = FALSE;

		DWORD  cbRead, cbToWrite, cbWritten, dwMode;

		int numCB = 0;
		std::string curMessage;
		bool shutdown = false;
		while (!shutdown)
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

			for (int i = 0; i < cbRead; ++i)
			{
				if (data[i] == '{')
				{
					++numCB;
				}

				if (data[i] == '}')
				{
					--numCB;
				}
				curMessage.push_back(data[i]);

				if (numCB == 0)
				{
					json_parser::JSONValue req(json_parser::ValueType::Object);
					json_parser::JSONValue::FromString(curMessage, req);
					curMessage.clear();

					if (!HandleReq(req))
					{
						shutdown = true;
						break;
					}
				}
			}
		}

		CloseHandle(m_hPipe);
		CloseHandle(m_hPipeOut);

		jobs::RunSync(jobs::Job::CreateFromLambda([]() {
			BaseObjectContainer& container = BaseObjectContainer::GetInstance();
			lock::LockObject* lock = static_cast<lock::LockObject*>(container.GetObjectOfClass(lock::MainLockMeta::GetInstance()));
			lock->UnLock();
		}));
	});

	m_serverJS->ScheduleJob(serverJob);
}

void pipe_server::ServerObject::StartOut()
{
	// Try to open a named pipe; wait for it, if necessary. 
	m_hPipeOut = CreateFile(
		TEXT("\\\\.\\pipe\\mynamedpipe_cpp_to_js"),   // pipe name 
		GENERIC_WRITE,
		0,              // no sharing 
		NULL,           // default security attributes
		OPEN_EXISTING,  // opens existing pipe 
		0,              // default attributes 
		NULL);          // no template file 

	// Break if the pipe handle is valid. 

	if (m_hPipeOut == INVALID_HANDLE_VALUE)
	{
		return;
	}
}


bool pipe_server::ServerObject::HandleReq(const json_parser::JSONValue& req)
{
	using namespace json_parser;

	JSONValue& tmp = const_cast<JSONValue&>(req);
	const auto& map = tmp.GetAsObj();

	std::string op = std::get<std::string>(map.find("op")->second.m_payload);
	if (op == "shutdown")
	{
		return false;
	}
	
	int reqId = static_cast<int>(std::get<double>(map.find("id")->second.m_payload));
	
	if (op == "hash")
	{
		std::string file = std::get<std::string>(map.find("file")->second.m_payload);
		jobs::RunAsync(jobs::Job::CreateFromLambda([=]() {
			std::string hash;
			size_t fileSize;
			crypto::HashBinFile(file, hash, fileSize);
			
			JSONValue res(ValueType::Object);
			auto& resMap = res.GetAsObj();
			resMap["id"] = JSONValue(static_cast<double>(reqId));
			resMap["hash"] = JSONValue(hash);
			resMap["fileSize"] = JSONValue(static_cast<double>(fileSize));
			SendResponse(res);
		}));
		
		return true;
	}

	if (op == "set_file_id")
	{
		std::string file = std::get<std::string>(map.find("file")->second.m_payload);
		int fileId = static_cast<int>(std::get<double>(map.find("file_id")->second.m_payload));

		jobs::RunSync(jobs::Job::CreateFromLambda([=]() {
			if (!m_fileManager)
			{
				BaseObjectContainer& container = BaseObjectContainer::GetInstance();
				BaseObject* obj = container.GetObjectOfClass(udp::FileManagerMeta::GetInstance());

				m_fileManager = static_cast<udp::FileManagerObject*>(obj);
				if (!m_fileManager)
				{
					m_fileManager = new udp::FileManagerObject();
				}
			}

			m_fileManager->RegisterFile(fileId, file);
			JSONValue res(ValueType::Object);
			auto& resMap = res.GetAsObj();
			resMap["id"] = JSONValue(static_cast<double>(reqId));
			SendResponse(res);
		}));

		return true;
	}

	if (op == "run_udp_server")
	{
		udp::Init();

		jobs::RunSync(jobs::Job::CreateFromLambda([=]() {
			new udp::UDPServerObject();

			JSONValue res(ValueType::Object);
			auto& resMap = res.GetAsObj();
			if (reqId)
			{
				bool t = true;
			}

			resMap["id"] = JSONValue(static_cast<double>(reqId));
			SendResponse(res);
		}));
		
		return true;
	}

	if (op == "run_udp_client")
	{
		udp::Init();
		
		int fileId = static_cast<int>(std::get<double>(map.find("fileId")->second.m_payload));
		size_t fileSize = static_cast<size_t>(std::get<double>(map.find("fileSize")->second.m_payload));
		std::string path = std::get<std::string>(map.find("path")->second.m_payload);

		jobs::RunSync(jobs::Job::CreateFromLambda([=]() {
			new udp::FileDownloaderObject(fileId, fileSize, path);

			JSONValue res(ValueType::Object);
			auto& resMap = res.GetAsObj();
			resMap["id"] = JSONValue(static_cast<double>(reqId));
			SendResponse(res);
		}));

		return true;
	}

	JSONValue res(ValueType::Object);
	auto& resMap = res.GetAsObj();
	resMap["id"] = JSONValue(static_cast<double>(reqId));
	SendResponse(res);
	return true;
}

void pipe_server::ServerObject::SendResponse(const json_parser::JSONValue& res)
{	
	m_serverResponseJS->ScheduleJob(jobs::Job::CreateFromLambda([=]() {
		std::string resp = res.ToString(false);

		DWORD written;
		WriteFile(
			m_hPipeOut,
			resp.c_str(),
			resp.size() * sizeof(char),
			&written,
			NULL
		);
	}));
}

