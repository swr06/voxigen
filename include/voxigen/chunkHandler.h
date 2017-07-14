#ifndef _voxigen_chunkHandler_h_
#define _voxigen_chunkHandler_h_

#include "voxigen/chunkHandle.h"
#include "voxigen/worldDescriptors.h"
#include "voxigen/worldGenerator.h"

#include <thread>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <queue>
#include <boost/filesystem.hpp>

namespace voxigen
{

namespace fs=boost::filesystem;

template<typename _Chunk>
class ChunkHandler
{
public:
    typedef _Chunk ChunkType;
    typedef std::unique_ptr<ChunkType> UniqueChunk;

    typedef ChunkHandle<ChunkType> ChunkHandleType;
    typedef std::shared_ptr<ChunkHandleType> SharedChunkHandle;
    typedef std::weak_ptr<ChunkHandleType> WeakChunkHandle;
    typedef std::unordered_map<unsigned int, WeakChunkHandle> WeakChunkHandleMap;
    typedef std::unordered_map<unsigned int, SharedChunkHandle> ChunkHandleMap;

    ChunkHandler(WorldDescriptors *descriptors);

    void initialize();
    void terminate();

    bool load(const std::string &name);

    void ioThread();
    void generatorThread();
    
    SharedChunkHandle getChunk(unsigned int hash);
    void removeHandle(ChunkHandleType *chunkHandle);

    std::vector<unsigned int> getUpdatedChunks();

private:
    void addToGenerateQueue(SharedChunkHandle chunkHandle);
    void addToUpdatedQueue(unsigned int hash);

    void buildConfig();
    void loadWorldCache();
    void verifyDirectory();

    WorldDescriptors *m_descriptors;
    WorldGenerator<ChunkType> m_worldGenerator;

//World files
    fs::path m_worldDirectory;
    fs::path m_configFile;
    std::string m_cacheDirectory;

//IO thread
    std::thread m_ioThread;
    std::mutex m_ioMutex;
    std::queue<SharedChunkHandle> m_ioQueue;
    std::condition_variable m_ioEvent;
    bool m_ioThreadRun;

//generator thread/queue
    std::mutex m_generatorMutex;
    std::thread m_generatorThread;
    std::queue<SharedChunkHandle> m_generatorQueue;
    std::condition_variable m_generatorEvent;
    bool m_generatorThreadRun;

//Data store
    std::mutex m_chunkMutex;
    ChunkHandleMap m_chunkHandles;
    WeakChunkHandleMap m_weakChunkHandles;

//Status updates
    std::mutex m_chunkUpdatedMutex;
    std::vector<unsigned int> m_chunksUpdated;
};

template<typename _Chunk>
ChunkHandler<_Chunk>::ChunkHandler(WorldDescriptors *descriptors):
m_descriptors(descriptors),
m_worldGenerator(descriptors)
{

}

template<typename _Chunk>
void ChunkHandler<_Chunk>::initialize()
{
    m_ioThreadRun=true;
    m_ioThread=std::thread(std::bind(&ChunkHandler<_Chunk>::ioThread, this));

    m_generatorThreadRun=true;
    m_generatorThread=std::thread(std::bind(&ChunkHandler<_Chunk>::generatorThread, this));
}

template<typename _Chunk>
void ChunkHandler<_Chunk>::terminate()
{
    //thread flags are not atomic so we need the mutexes to coordinate the setting, 
    //otherwise would have to loop re-notifiying thread until it stopped
    {
        std::unique_lock<std::mutex> lock(m_ioMutex);
        m_ioThreadRun=false;
    }
    

    {
        std::unique_lock<std::mutex> lock(m_generatorMutex);
        m_generatorThreadRun=false;
    }
    
    m_ioEvent.notify_all();
    m_generatorEvent.notify_all();

    m_ioThread.join();
    m_generatorThread.join();
    
}

template<typename _Chunk>
bool ChunkHandler<_Chunk>::load(const std::string &directory)
{
    m_worldDirectory=fs::path(directory);

    if(!fs::isDirectory(m_worldDirectory))
    {
        if(fs::exists(m_worldDirectory))
            return false;

        fs::create_directory(m_worldDirectory);
    }

    m_configFile=fs::path(m_worldDirectory.string()+"/worldConfig.json");

    if(!fs::exists(m_configFile))
        buildConfig()
    else
        loadWorldCache();

    verifyDirectory();
    
}

template<typename _Chunk>
void ChunkHandler<_Chunk>::buildConfig()
{}

template<typename _Chunk>
void ChunkHandler<_Chunk>::loadWorldCache()
{}

template<typename _Chunk>
void ChunkHandler<_Chunk>::verifyDirectory()
{
//    for(auto &entry:fs::directory_iterator(m_worldDirectory))
//    {
//
//    }
}

template<typename _Chunk>
void ChunkHandler<_Chunk>::ioThread()
{
    std::unique_lock<std::mutex> lock(m_ioMutex);

    while(m_ioThreadRun)
    {
        if(m_ioQueue.empty())
        {
            m_ioEvent.wait(lock);
            continue;
        }
    }
}

template<typename _Chunk>
void ChunkHandler<_Chunk>::generatorThread()
{
    std::unique_lock<std::mutex> lock(m_generatorMutex);

    while(m_generatorThreadRun)
    {
        if(m_generatorQueue.empty())
        {
            m_generatorEvent.wait(lock);
            continue;
        }

        SharedChunkHandle chunkHandle=m_generatorQueue.front();

        if(!chunkHandle)
            return;

        m_generatorQueue.pop();

        lock.unlock();//drop lock while working

        chunkHandle->chunk=m_worldGenerator.generateChunk(chunkHandle->hash);
        chunkHandle->status=ChunkHandleType::Memory;
        addToUpdatedQueue(chunkHandle->hash);
        chunkHandle.reset();//release pointer while not holding lock as there is a chance this will call removeHandle
                            //which will lock m_chunkMutex and safer to only have one lock at a time
        lock.lock();
    }
}

template<typename _Chunk>
typename ChunkHandler<_Chunk>::SharedChunkHandle ChunkHandler<_Chunk>::getChunk(unsigned int hash)
{
    std::unique_lock<std::mutex> lock(m_chunkMutex);

    auto iter=m_weakChunkHandles.find(hash);

    if(iter!=m_weakChunkHandles.end())
    {
        if(!iter->second.expired())
            return iter->second.lock(); //we already have it and somebody else has it as well
    }

    SharedChunkHandle returnHandle;

    //see if we already know about the chunk
    auto handleIter=m_chunkHandles.find(hash);

    if(handleIter==m_chunkHandles.end())
    {
        //we dont know about this one, create and send for generation
        SharedChunkHandle handle;
        ChunkHandleType *chunkHandle=new ChunkHandleType(hash);

        //ceate local handle for data storage
        handle.reset(chunkHandle);
        //Create shared handle to notify handler when it is no longer in use
        returnHandle.reset(chunkHandle, std::bind(&ChunkHandler<_Chunk>::removeHandle, this, std::placeholders::_1));

        m_chunkHandles[hash]=handle;

        addToGenerateQueue(returnHandle);
    }
    else
    {
        ChunkHandleType *chunkHandle=handleIter->second.get();

        //Create shared handle to notify handler when it is no longer in use
        returnHandle.reset(chunkHandle, std::bind(&ChunkHandler<_Chunk>::removeHandle, this, std::placeholders::_1));

        if(chunkHandle->status!=ChunkHandleType::Memory)
        {
            //we dont have it in memory so we need to load it
            if(!chunkHandle->cachedOnDisk) //shouldn't happen if cache working but in for debug
                addToGenerateQueue(returnHandle);
            else
            {

            }
        }
    }

    m_weakChunkHandles[hash]=returnHandle;
    return returnHandle;
}

template<typename _Chunk>
void ChunkHandler<_Chunk>::removeHandle(ChunkHandleType *chunkHandle)
{
    std::unique_lock<std::mutex> lock(m_chunkMutex);

    auto iter=m_weakChunkHandles.find(chunkHandle->hash);

    if(iter!=m_weakChunkHandles.end())
        m_weakChunkHandles.erase(iter);

    //update cache if not already saved

    //we are not releasing the handle here as we hold it in a different map
    //but we want to release the chunk as no one is using it now
    chunkHandle->chunk.reset(nullptr);
    chunkHandle->status=ChunkHandleType::Unknown;
}

template<typename _Chunk>
std::vector<unsigned int> ChunkHandler<_Chunk>::getUpdatedChunks()
{
    
    std::unique_lock<std::mutex> lock(m_chunkUpdatedMutex);

    std::vector<unsigned int> updatedChunks(m_chunksUpdated);
    m_chunksUpdated.clear();
    lock.unlock();

    return updatedChunks;
}

template<typename _Chunk>
void ChunkHandler<_Chunk>::addToGenerateQueue(SharedChunkHandle chunkHandle)
{
    std::unique_lock<std::mutex> lock(m_generatorMutex);

    chunkHandle->status=ChunkHandleType::Generating;
    m_generatorQueue.push(chunkHandle);
    m_generatorEvent.notify_all();
}

template<typename _Chunk>
void ChunkHandler<_Chunk>::addToUpdatedQueue(unsigned int hash)
{
    std::unique_lock<std::mutex> lock(m_chunkUpdatedMutex);

    m_chunksUpdated.push_back(hash);
}

} //namespace voxigen

#endif //_voxigen_chunkHandler_h_