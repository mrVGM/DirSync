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

#include "FileServer.h"
#include "FileDownloader.h"

#include "Common.h"

#include <iostream>
#include <string>
#include <thread>
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

void pipe_server::ServerObject::End()
{
	CloseHandle(m_hPipe);
	CloseHandle(m_hPipeOut);
}

void pipe_server::ServerObject::GenPipeNames(std::string& inPipe, std::string& outPipe)
{
	guid::CreateGUID(inPipe);
	guid::CreateGUID(outPipe);

	inPipe = "\\\\.\\pipe\\" + inPipe;
	outPipe = "\\\\.\\pipe\\" + outPipe;

}

void pipe_server::ServerObject::Start(const std::string& inPipe, const std::string& outPipe)
{
	m_hPipe = INVALID_HANDLE_VALUE;
	m_hPipeOut = INVALID_HANDLE_VALUE;

	while (m_hPipe == INVALID_HANDLE_VALUE)
	{
		// Try to open a named pipe; wait for it, if necessary. 
		m_hPipe = CreateFile(
			TEXT(inPipe.c_str()),   // pipe name 
			GENERIC_READ,
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens a pipe 
			0,              // default attributes 
			NULL);          // no template file 

		if (m_hPipe == INVALID_HANDLE_VALUE)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	while (m_hPipeOut == INVALID_HANDLE_VALUE)
	{
		m_hPipeOut = CreateFile(
			TEXT(outPipe.c_str()),   // pipe name 
			GENERIC_WRITE,
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens a pipe
			0,              // default attributes 
			NULL);			// no template file

		if (m_hPipeOut == INVALID_HANDLE_VALUE)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	jobs::Job* serverJob = jobs::Job::CreateFromLambda([=]() {
		const int BUFSIZE = 512;
		TCHAR  chBuf[BUFSIZE + 1];
		BOOL   fSuccess = FALSE;

		DWORD  cbRead, cbToWrite, cbWritten, dwMode;

		int numCB = 0;
		std::string curMessage;
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
						break;
					}
				}
			}
		}

	});

	m_serverJS->ScheduleJob(serverJob);
}

bool pipe_server::ServerObject::HandleReq(const json_parser::JSONValue& req)
{
	using namespace json_parser;

	m_messageReceived = true;

	JSONValue& tmp = const_cast<JSONValue&>(req);
	const auto& map = tmp.GetAsObj();

	std::string op = std::get<std::string>(map.find("op")->second.m_payload);
	if (op == "shutdown")
	{
		return false;
	}
	
	int reqId = std::get<json_parser::JSONNumber>(map.find("id")->second.m_payload).ToInt();
	
	if (op == "hash")
	{
		std::string file = std::get<std::string>(map.find("file")->second.m_payload);
		jobs::RunAsync(jobs::Job::CreateFromLambda([=]() {
			std::string hash;
			ull fileSize;
			crypto::HashBinFile(file, hash, fileSize);
			
			JSONValue res(ValueType::Object);
			auto& resMap = res.GetAsObj();
			resMap["id"] = JSONValue(json_parser::JSONNumber(reqId));
			resMap["hash"] = JSONValue(hash);
			resMap["fileSize"] = JSONValue(json_parser::JSONNumber(fileSize));
			SendResponse(res);
		}));
		
		return true;
	}

	if (op == "set_file_id")
	{
		std::string file = std::get<std::string>(map.find("file")->second.m_payload);
		int fileId = std::get<json_parser::JSONNumber>(map.find("file_id")->second.m_payload).ToInt();

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
			resMap["id"] = JSONValue(json_parser::JSONNumber(reqId));
			SendResponse(res);
		}));

		return true;
	}

	if (op == "run_udp_server")
	{
		jobs::RunSync(jobs::Job::CreateFromLambda([=]() {
			udp::FileServerObject* server = new udp::FileServerObject();
			server->Init();

			JSONValue res(ValueType::Object);
			auto& resMap = res.GetAsObj();
			resMap["id"] = JSONValue(json_parser::JSONNumber(reqId));
			SendResponse(res);
		}));

		return true;
	}

	if (op == "download_file")
	{
		unsigned int fileId = std::get<json_parser::JSONNumber>(map.find("file_id")->second.m_payload).ToInt();
		std::string ipAddr = std::get<std::string>(map.find("ip_addr")->second.m_payload);
		ull fileSize = std::get<json_parser::JSONNumber>(map.find("file_size")->second.m_payload).ToInt();
		std::string path = std::get<std::string>(map.find("path")->second.m_payload);

		jobs::RunSync(jobs::Job::CreateFromLambda([=]() {
			udp::FileDownloaderObject* downloader = new udp::FileDownloaderObject(
				ipAddr,
				fileId,
				fileSize,
				path
			);

			downloader->Init();

			JSONValue res(ValueType::Object);
			auto& resMap = res.GetAsObj();
			resMap["id"] = JSONValue(json_parser::JSONNumber(reqId));
			SendResponse(res);
		}));

		return true;
	}

	if (op == "get_download_progress")
	{
		unsigned int fileId = std::get<json_parser::JSONNumber>(map.find("file_id")->second.m_payload).ToInt();

		jobs::RunSync(jobs::Job::CreateFromLambda([=]() {

			BaseObjectContainer& container = BaseObjectContainer::GetInstance();
			std::list<BaseObject*> tmp;
			container.GetAllObjectsOfClass(udp::FileDownloaderMeta::GetInstance(), tmp);

			udp::FileDownloaderObject* downloader = nullptr;
			for (auto it = tmp.begin(); it != tmp.end(); ++it)
			{
				udp::FileDownloaderObject* cur = static_cast<udp::FileDownloaderObject*>(*it);
				if (cur->m_fileId == fileId)
				{
					downloader = cur;
					break;
				}
			}

			JSONValue res(ValueType::Object);
			auto& resMap = res.GetAsObj();
			resMap["id"] = JSONValue(json_parser::JSONNumber(reqId));

			resMap["progress"] = json_parser::JSONValue(json_parser::ValueType::List);
			auto& l = resMap["progress"].GetAsList();
			if (!downloader)
			{
				l.push_back(json_parser::JSONValue(json_parser::JSONNumber(1)));
				l.push_back(json_parser::JSONValue(json_parser::JSONNumber(1)));
			}
			else
			{
				l.push_back(json_parser::JSONValue(json_parser::JSONNumber(downloader->m_received)));
				l.push_back(json_parser::JSONValue(json_parser::JSONNumber(downloader->m_fileSize)));
			}

			SendResponse(res);
		}));

		return true;
	}

	JSONValue res(ValueType::Object);
	auto& resMap = res.GetAsObj();
	resMap["id"] = JSONValue(json_parser::JSONNumber(reqId));
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

