#pragma once

#include "BaseObjectMeta.h"
#include "BaseObject.h"

#include "JSONValue.h"
#include <Windows.h>

namespace pipe_server
{
	class ServerMeta : public BaseObjectMeta
	{
	public:
		static const ServerMeta& GetInstance();
		ServerMeta();
	};

	class ServerObject : public BaseObject
	{
	private:
		HANDLE m_hPipe = nullptr;

	public:
		ServerObject();
		virtual ~ServerObject();

		void Start();
		void HandleReq(const json_parser::JSONValue& req);
		void SendResponse(const json_parser::JSONValue& res);
	};
}