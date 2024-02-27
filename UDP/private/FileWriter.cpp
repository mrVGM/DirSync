#include "FileWriter.h"
#include "Jobs.h"

#include "UDP.h"

#include <Windows.h>

void udp::FileWriter::Write(UDPRes* resp)
{
	jobs::RunAsync(jobs::Job::CreateFromLambda([=]() {

		size_t startByte = m_curByte;
		size_t totalWritten = 0;
		for (size_t j = 0; j < FileChunk::m_chunkKBSize; ++j)
		{
			size_t cur = startByte + j * sizeof(KB);
			if (cur >= m_fileSize)
			{
				break;
			}

			size_t endByte = min(m_fileSize, cur + sizeof(KB));
			DWORD written;
			WriteFile(
				m_fHandle,
				&resp[j].m_data,
				endByte - cur,
				&written,
				NULL
			);

			totalWritten += written;
		}

		delete[] resp;
		m_curByte += totalWritten;

		jobs::RunSync(jobs::Job::CreateFromLambda([=]() {
			m_busy = false;

			if (!m_writeQueue.empty())
			{
				m_busy = true;
				UDPRes* tmp = m_writeQueue.front();
				m_writeQueue.pop();

				Write(tmp);
				return;
			}

			if (m_onFinished)
			{
				jobs::RunSync(m_onFinished);
				return;
			}
		}));
	}));
}

udp::FileWriter::FileWriter(const std::string& path, size_t fileSize) :
	m_path(path),
	m_fileSize(fileSize)
{
	m_fHandle = CreateFile(
		m_path.c_str(),
		GENERIC_WRITE,
		NULL,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
}

udp::FileWriter::~FileWriter()
{
	CloseHandle(m_fHandle);
}

void udp::FileWriter::PushToQueue(UDPRes* resp)
{
	jobs::RunSync(jobs::Job::CreateFromLambda([=]() {
		m_writeQueue.push(resp);

		if (m_busy) {
			return;
		}

		m_busy = true;
		UDPRes* tmp = m_writeQueue.front();
		m_writeQueue.pop();

		Write(tmp);
	}));
}

void udp::FileWriter::Finalize(jobs::Job* onFinished)
{
	jobs::RunSync(jobs::Job::CreateFromLambda([=]() {
		m_onFinished = onFinished;

		if (m_busy) {
			return;
		}
		m_busy = true;

		if (m_writeQueue.empty())
		{
			jobs::RunSync(m_onFinished);
			return;
		}

		UDPRes* tmp = m_writeQueue.front();
		m_writeQueue.pop();

		Write(tmp);
	}));
}
