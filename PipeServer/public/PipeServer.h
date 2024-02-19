#pragma once

#include "BaseObjectMeta.h"
#include "BaseObject.h"

#include <semaphore>

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
	public:
		ServerObject();
		virtual ~ServerObject();

		void Start();
	};
}