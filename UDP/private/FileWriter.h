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

		size_t m_fileSize = 0;
		size_t m_curByte = 0;
		bool m_busy = false;

		void Write(UDPRes* resp);

	public:
		FileWriter(const std::string& path, size_t fileSize);
		~FileWriter();

		void PushToQueue(UDPRes* resp);
		void Finalize(jobs::Job* onFinished);
	};
}