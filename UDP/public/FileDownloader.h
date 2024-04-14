#pragma once

#include "UDP.h"

#include "Job.h"

#include "BaseObject.h"
#include "BaseObjectMeta.h"

namespace udp
{
	class FileDownloaderObject;
    struct Chunk
    {
        ull m_offset = 0;
        Packet* m_packets = nullptr;

        Chunk(ull offset, const FileDownloaderObject& downloader);

        ~Chunk();

		int RecordPacket(const Packet& pkt);
        

		bool IsComplete();

		void CreateDataMask(KB& outMask);

		size_t GetFull() const;
    };

	class FileDownloaderJSMeta : public BaseObjectMeta
	{
	public:
		static const FileDownloaderJSMeta& GetInstance();
		FileDownloaderJSMeta();
	};

	class FileDownloaderMeta : public BaseObjectMeta
	{
	public:
		static const FileDownloaderMeta& GetInstance();
		FileDownloaderMeta();
	};

	class FileWriter
	{
	private:
		FileDownloaderObject& m_downloader;

		std::list<Chunk*>* m_curBuff = nullptr;
		std::list<Chunk*> m_buff1;
		std::list<Chunk*> m_buff2;

		std::mutex m_mutex;
		std::binary_semaphore m_getChunksSemaphore{ 0 };

		bool m_running = true;

	public:
		FileWriter(FileDownloaderObject& downloader);
		~FileWriter();

		void PushChunk(Chunk* chunk);
		std::list<Chunk*>& GetReceived();
		void Start();
		void Stop();
	};

	class FileDownloaderObject : public BaseObject, public Endpoint
	{
	private:
		jobs::Job* m_onDestroyed = nullptr;

	public:
		bool m_running = true;

		unsigned int m_fileId;

		ull m_fileSize = 0;
		ull m_received = 0;
		std::string m_path;
		std::string m_serverIP;
		int m_serverPort = -1;

		int m_numWorkers = 1;
		int m_downloadWindow = 1;
		int m_pingDelay = 10;

		FileWriter m_writer;

		jobs::Job* m_done = nullptr;

		static void ReleaseJS();

		FileDownloaderObject(
			int serverPort,
			const std::string& serverIP,
			unsigned int fileId,
			ull fileSize,
			const std::string& path,
			int numWorkers,
			int downloadWindow,
			int pingDelay,
			jobs::Job* done);

		virtual ~FileDownloaderObject();

		void Init() override;
		void Shutdown(jobs::Job* onDestroyed);
	};
}