#include "voxigen/processingThread.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace voxigen
{

unsigned int getProcessorCount()
{
    unsigned int numProcessors=1;

#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    
    numProcessors=sysinfo.dwNumberOfProcessors;
#else
    numProcessors=sysconf(_SC_NPROCESSORS_ONLN);
#endif

    return numProcessors;
}

ProcessThread &getProcessThread()
{
    static ProcessThread processThread;

    return processThread;
}

ProcessThread::ProcessThread():
    m_ioThread(&m_event),
    m_workerThread(&m_event)
#ifndef NDEBUG
    , m_requestThreadIdSet(false)
#endif
{
    m_workerThread.setCallback(std::bind(&ProcessThread::processWorkerRequest, this, std::placeholders::_1));

    processChunkRequest=std::bind(&ProcessThread::defaultCallback, this, std::placeholders::_1);
    processMeshRequest=std::bind(&ProcessThread::defaultCallback, this, std::placeholders::_1);
}

void ProcessThread::setSizes(glm::ivec3 &regionSize, glm::ivec3 &chunkSize)
{
    process::Request::regionSize=regionSize;
    process::Request::chunkSize=chunkSize;
}

void ProcessThread::setIoRequestCallback(process::Callback callback)
{
    m_workerThread.setCallback(callback);
}

void ProcessThread::setChunkRequestCallback(process::Callback callback)
{
    processChunkRequest=callback;
}

void ProcessThread::setMeshRequestCallback(process::Callback callback)
{
    processMeshRequest=callback;
}

void ProcessThread::updateQueues(RequestQueue &completedRequests)
{
    assert(checkRequestThread());

    bool update=false;
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);

        //send all cached request to coordination thread
        if(!m_requestQueue.empty())
        {
            m_requestThreadQueue.insert(m_requestThreadQueue.end(), m_requestQueue.begin(), m_requestQueue.end());
            m_requestQueue.clear();
            update=true;
        }

        //get all completed request
        if(!m_completedThreadQueue.empty())
        {
            completedRequests.insert(completedRequests.end(), m_requestsComplete.begin(), m_requestsComplete.end());
            m_requestsComplete.clear();
        }
    }

    if(update)
        m_event.notify_all();
}


void ProcessThread::start()
{
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_run=true;
    }

    m_thread=std::thread(std::bind(&ProcessThread::processThread, this));

    m_ioThread.start();

    //want the number of physical processors vs threads
//    int hardwareThreads=std::thread::hardware_concurrency()-1;
    int hardwareThreads=getProcessorCount()-1;

    if(hardwareThreads<0)
        hardwareThreads=1;

    m_workerThread.start(hardwareThreads);
}


void ProcessThread::stop()
{
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_run=false;
    }

    m_event.notify_all();
    m_thread.join();

    m_ioThread.stop();
    m_workerThread.stop();
}

void ProcessThread::updatePosition(const glm::ivec3 &region, const glm::ivec3 &chunk)
{
    assert(checkRequestThread());

    process::Request *request=m_requests.get();

    assert(request);

    request->type=process::Type::UpdatePos;
    request->priority=process::Priority::UpdatePos;

    request->position.region=region;
    request->position.chunk=chunk;

    m_requestQueue.push_back(request);
    m_event.notify_all();
}

void ProcessThread::requestChunkAction(void *chunkHandle, size_t lod, process::Type type, size_t priority, const glm::ivec3 &regionIndex, const glm::ivec3 &chunkIndex)
{
    assert(checkRequestThread());

    process::Request *request=m_requests.get();

    assert(request);

    request->type=type;
    request->priority=priority;

    request->position.region=regionIndex;
    request->position.chunk=chunkIndex;

    request->data.chunk.handle=(void *)chunkHandle;
    request->data.chunk.lod=lod;

    m_requestQueue.push_back(request);
    m_event.notify_all();
}

void ProcessThread::releaseRequest(process::Request *request)
{
    assert(checkRequestThread());

    m_requests.release(request);
}

void ProcessThread::processThread()
{
    bool run=true;
    RequestQueue requestQueue;
    RequestQueue completedQueue;
    RequestQueue cancelRequests;

    RequestQueue currentQueue;
    RequestQueue ioQueue;
    RequestQueue ioCancelQueue;
    RequestQueue workerQueue;
    RequestQueue workerCancelQueue;

//    RequestQueue priorityQueue;

    while(run)
    {
//        bool completed;

        //check for request
//        if(requestQueue.empty())// || (m_requestCached>0)) <-possible use of atomic rather than lock
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);

            //update status
            run=m_run;

            //check if any new request have been added.
            if(!m_requestQueue.empty())
            {
                requestQueue.insert(requestQueue.end(), m_requestQueue.begin(), m_requestQueue.end());
                m_requestQueue.clear();
            }

            //while we have the lock update anything that is complete
            if(!completedQueue.empty())
            {
                m_requestsComplete.insert(m_requestsComplete.end(), completedQueue.begin(), completedQueue.end());
                completedQueue.clear();
            }

            if(run && requestQueue.empty())
                m_event.wait(lock);

            //            continue;
        }

        //need to sort request into current thread, ioThread and general workerThread
        for(size_t i=0; i<requestQueue.size(); ++i)
        {
            process::Request *request=requestQueue[i];

            switch(request->type)
            {
            case process::Type::UpdatePos:
                currentQueue.push_back(request);
                break;
            case process::Type::Read:
            case process::Type::Write:
                ioQueue.push_back(request);
                break;
            case process::Type::CancelRead:
            case process::Type::CancelWrite:
                ioCancelQueue.push_back(request);
            case process::Type::Generate:
            case process::Type::Mesh:
                workerQueue.push_back(request);
                break;
            case process::Type::CancelGenerate:
            case process::Type::CancelMesh:
                workerCancelQueue.push_back(request);
                break;
            }
        }
        requestQueue.clear();

        bool forceResort=false;
        //process updates
        for(size_t i=0; i<currentQueue.size(); ++i)
        {
            process::Request *request=currentQueue[i];

            process::Compare::currentRegion=request->position.region;
            process::Compare::currentChunk=request->position.chunk;

            forceResort=true;

            completedQueue.push_back(request);
        }
        currentQueue.clear();

        m_ioThread.updateQueue(ioQueue, ioCancelQueue, completedQueue, forceResort);
        m_workerThread.updateQueue(ioQueue, ioCancelQueue, completedQueue, forceResort);
    }
}

bool ProcessThread::processWorkerRequest(process::Request *request)
{
    switch(request->type)
    {
    case process::Type::Generate:
        processChunkRequest(request);
        break;
    case process::Type::Mesh:
        processMeshRequest(request);
        break;
    default:
        break;
    }
    return true;
}

#ifndef NDEBUG
bool ProcessThread::checkRequestThread()
{
    if(!m_requestThreadIdSet)
    {
        m_requestThreadId=std::this_thread::get_id();
        m_requestThreadIdSet=true;
    }
    return (std::this_thread::get_id()==m_requestThreadId);
}
#endif

}//namespace voxigen

