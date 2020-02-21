#ifndef _voxigen_processingThread_h_
#define _voxigen_processingThread_h_

#include "voxigen/voxigen_export.h"
#include "voxigen/defines.h"
#include "voxigen/updateQueue.h"
#include "voxigen/volume/chunkHandle.h"
#include "voxigen/processRequests.h"
#include "voxigen/queueThread.h"

#include <generic/objectHeap.h>

#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>

namespace voxigen
{

VOXIGEN_EXPORT unsigned int getProcessorCount();

class ProcessThread
{
public:
    typedef std::vector<process::Request *> RequestQueue;

    ProcessThread();

    void setSizes(glm::ivec3 &regionSize, glm::ivec3 &chunkSize);

    void setIoRequestCallback(process::Callback callback);
    void setChunkRequestCallback(process::Callback callback);
    void setMeshRequestCallback(process::Callback callback);

    void updateQueues(RequestQueue &completedRequests);

    void start();
    void stop();

    //thread actions
    void updatePosition(const glm::ivec3 &region, const glm::ivec3 &chunk);

    template<typename _Object>
    void requestChunkGenerate(_Object *chunkHandle, size_t lod)
    {   requestChunkAction((void *)chunkHandle, lod, process::Type::Generate, process::Priority::Generate, chunkHandle->regionIndex(), chunkHandle->chunkIndex());}
    template<typename _Object>
    void cancelChunkGenerate(_Object *chunkHandle)
    {   requestChunkAction((void *)chunkHandle, 0, process::Type::CancelGenerate, process::Priority::CancelGenerate, chunkHandle->regionIndex(), chunkHandle->chunkIndex());}
    template<typename _Object>
    void requestChunkRead(_Object *chunkHandle, size_t lod)
    {   requestChunkAction((void *)chunkHandle, lod, process::Type::Read, process::Priority::Read, chunkHandle->regionIndex(), chunkHandle->chunkIndex());}
    template<typename _Object>
    void cancelChunkRead(_Object *chunkHandle)
    {   requestChunkAction((void *)chunkHandle, 0, process::Type::CancelRead, process::Priority::CancelRead, chunkHandle->regionIndex(), chunkHandle->chunkIndex());}
    template<typename _Object>
    void requestChunkWrite(_Object *chunkHandle, size_t lod)
    {   requestChunkAction((void *)chunkHandle, lod, process::Type::Write, process::Priority::Write, chunkHandle->regionIndex(), chunkHandle->chunkIndex());}
    template<typename _Object>
    void cancelChunkWrite(_Object *chunkHandle)
    {   requestChunkAction((void *)chunkHandle, 0, process::Type::CancelWrite, process::Priority::CancelWrite, chunkHandle->regionIndex(), chunkHandle->chunkIndex());}
    template<typename _Object>
    void requestChunkMesh(_Object *chunkHandle)
    {   requestChunkAction((void *)chunkHandle, 0, process::Type::Mesh, process::Priority::Mesh, chunkHandle->regionIndex(), chunkHandle->chunkIndex());}
    template<typename _Object>
    void cancelChunkMesh(_Object *chunkHandle)
    {   requestChunkAction((void *)chunkHandle, 0, process::Type::CancelMesh, process::Priority::CancelMesh, chunkHandle->regionIndex(), chunkHandle->chunkIndex());}

    void releaseRequest(process::Request *request);

    //coordination thread
    void processThread();
    
    // callback for worker threads
    bool processWorkerRequest(process::Request *request);

    bool defaultCallback(process::Request *request) { return true; }

private:
    void requestChunkAction(void *chunkHandle, size_t lod, process::Type type, size_t priority, const glm::ivec3 &regionIndex, const glm::ivec3 &chunkIndex);

//    void updatePriorityQueue();

#ifndef NDEBUG
    bool checkRequestThread();
#endif

    std::thread m_thread;
    std::condition_variable m_event;

    process::Callback processChunkRequest;
    process::Callback processMeshRequest;

//only accessible from 1 thread, generally main thread
    generic::ObjectHeap<process::Request> m_requests;
    RequestQueue m_requestQueue;
    RequestQueue m_requestsComplete;
#ifndef NDEBUG
    //used to verify single thread access
    std::thread::id m_requestThreadId;
    bool m_request ThreadIdSet;
#endif

//can only be accessed under lock
    std::mutex m_queueMutex;
    bool m_run;
    RequestQueue m_requestThreadQueue;
    RequestQueue m_completedThreadQueue;
#ifndef NDEBUG
    //used to verify single thread access
    std::thread::id m_requestThreadId;
#endif
    
    QueueThread m_ioThread;
    QueueThread m_workerThread;
};

VOXIGEN_EXPORT ProcessThread &getProcessThread();

}//namespace voxigen

#endif //_voxigen_processingThread_h_