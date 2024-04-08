#include "ChunkDownloader.h"

#include "JobSystem.h"
#include "Job.h"

#include "FileDownloader.h"
#include "FileManager.h"

udp::ChunkDownloader::ChunkDownloader(FileDownloaderObject& downloader) :
    m_downloader(downloader)
{
}

void udp::ChunkDownloader::DownloadChunk(ull offset, Tracker& tracker, jobs::JobSystem* js)
{
    Tracker* trackerPtr = &tracker;

    m_chunk = new Chunk(offset, m_downloader);

    bool tmp;
    Bucket* bucket = m_downloader.m_buckets.GetOrCreateBucket(offset, tmp);

    struct Context
    {
        Packet m_pingTemplate;
        Packet m_maskTemplate;

        Packet* m_mask = nullptr;

        std::mutex m_maskMutex;

        ull m_counter = 0;

        bool m_chunkComplete = false;
    };

    Context* ctx = new Context();

    ctx->m_pingTemplate.m_packetType = PacketType::m_ping;
    ctx->m_pingTemplate.m_id = m_downloader.m_fileId;
    ctx->m_pingTemplate.m_chunkId = offset;

    ctx->m_maskTemplate.m_packetType = PacketType::m_bitmask;
    ctx->m_maskTemplate.m_id = m_downloader.m_fileId;
    ctx->m_maskTemplate.m_offset = offset;
    ctx->m_maskTemplate.m_chunkId = offset;
    
    js->ScheduleJob(jobs::Job::CreateFromLambda([=]() {

        Packet* pak = nullptr;

        while (!ctx->m_chunkComplete)
        {
            ctx->m_maskMutex.lock();

            pak = ctx->m_mask;
            ctx->m_mask = nullptr;
            if (!pak)
            {
                ctx->m_pingTemplate.m_offset = ctx->m_counter++;
                pak = &ctx->m_pingTemplate;
            }

            ctx->m_maskMutex.unlock();

            m_downloader.Send(*pak);

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        delete ctx;
        m_downloader.m_buckets.DestroyBucket(offset);
    }));

    js->ScheduleJob(jobs::Job::CreateFromLambda([=]() {

        ull maskId = 0;
        ull fence = 0;

        auto genMask = [&]() {
            m_chunk->CreateDataMask(ctx->m_maskTemplate.m_payload);

            ctx->m_maskMutex.lock();

            maskId = ctx->m_counter++;
            ctx->m_mask = &ctx->m_maskTemplate;

            ctx->m_maskMutex.unlock();
        };

        genMask();

        while (true)
        {
            std::list<Packet>& paks = bucket->GetAccumulated();

            for (auto it = paks.begin(); it != paks.end(); ++it)
            {
                Packet& cur = *it;

                if (cur.m_packetType.GetPacketType() == EPacketType::Ping)
                {
                    fence = std::max(fence, cur.m_offset);
                    continue;
                }

                if (cur.m_packetType.GetPacketType() == EPacketType::Full)
                {
                    ull offset = cur.m_offset - m_chunk->m_offset;
                    if (m_chunk->m_packets[offset].m_packetType.GetPacketType() == EPacketType::Empty)
                    {
                        trackerPtr->IncrementProgress();
                        m_chunk->m_packets[offset] = cur;
                    }
                }
            }

            paks.clear();

            if (m_chunk->IsComplete())
            {
                ctx->m_chunkComplete = true;

                trackerPtr->Finished(this, m_chunk, js);
                break;
            }

            if (maskId < fence)
            {
                genMask();
            }
        }
    }));
}

ull udp::ChunkDownloader::GetOffset() const
{
    return m_chunk->m_offset;
}
