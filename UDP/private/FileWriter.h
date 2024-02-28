#pragma once

#include "UDP.h"

#include <queue>
#include <string>

#include "Job.h"

namespace udp
{
	class FileWriter
	{
	private:
		std::queue<UDPRes*> m_writeQueue;
		std::string m_path;
		void* m_fHandle = nullptr;
		jobs::Job* m_onFinished = nullptr;

		ull m_fileSize = 0;
		ull m_curByte = 0;
		bool m_busy = false;

		void Write(UDPRes* resp);

	public:
		FileWriter(const std::string& path, ull fileSize);
		~FileWriter();

		void PushToQueue(UDPRes* resp);
		void Finalize(jobs::Job* onFinished);
	};
}