#pragma once

#include "UDP.h"

#include "FileDownloader.h"
#include "JobSystem.h"

namespace udp
{
	class ChunkDownloader
	{
	private:
		FileDownloaderObject& m_downloader;
		Chunk* m_chunk = 0;

	public:

		struct Tracker
		{
			virtual void Finished(ChunkDownloader* downloader, Chunk* chunk, jobs::JobSystem* js) = 0;
			virtual void IncrementProgress() = 0;
		};

		ChunkDownloader(FileDownloaderObject& downloader);

		void DownloadChunk(ull offset, Tracker& tracker, jobs::JobSystem* js);
		ull GetOffset() const;
	};
}