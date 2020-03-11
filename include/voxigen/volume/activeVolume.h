#ifndef _voxigen_activeVolume_h_
#define _voxigen_activeVolume_h

#include "voxigen/volume/regionIndex.h"
#include "voxigen/volume/regionChunkIndex.h"
#include "voxigen/volume/containerVolume.h"

#include <generic/objectHeap.h>

#include <memory>
#include <functional>

namespace voxigen
{

template<typename _Container>
struct ChunkContainerInfo
{
    //keep data after meshing
    bool keepData;
    //mesh chunk, only mesh if neighbors will be loaded
    bool mesh;
    //what lod to load and mesh
    size_t lod;
    //render container
    _Container *container;
};

template<typename _Container>
struct RegionContainerInfo
{
    _Container *container;
};

template<typename _Container, typename _Mesh>
struct MeshUpdate
{
    MeshUpdate(){}
    MeshUpdate(_Container *container, _Mesh *mesh):container(container), mesh(mesh){}

    _Container *container;
    _Mesh *mesh;
};

template<typename _Grid, typename _ChunkContainer, typename _RegionContainer>//typename _Index>
class ActiveVolume
{
public:
    typedef typename _Grid::Type Grid;
    typedef typename _Grid::DescriptorType Descriptor;

    typedef typename _Grid::SharedChunkHandle SharedChunkHandle;

    typedef _RegionContainer RegionContainer;
    typedef std::vector<RegionContainer *> RegionContainers;
    typedef _ChunkContainer ChunkContainer;
    typedef std::vector<ChunkContainer *> ChunkContainers;

    typedef RegionIndex<typename Grid::Region> RegionIndex;
    typedef RegionChunkIndex<typename Grid::Region, typename Grid::Chunk> RegionChunkIndex;

    typedef ContainerVolume<Grid, RegionIndex, RegionContainerInfo<RegionContainer>, RegionContainer> RegionVolume;

    typedef typename ChunkContainerInfo<ChunkContainer> ChunkContainerInfo;
    typedef ContainerVolume<Grid, RegionChunkIndex, ChunkContainerInfo, ChunkContainer> ChunkVolume;
    typedef typename ChunkVolume::LoadContainer ChunkLoadContainer;
    typedef typename ChunkVolume::LoadRequests ChunkLoadRequests;
    typedef std::vector<ChunkContainer *> ChunkContainers;
    
    typedef ChunkTextureMesh Mesh;
    typedef MeshUpdate<ChunkContainer, Mesh> MeshUpdate;
    typedef std::vector<MeshUpdate> MeshUpdates;
//    typedef std::function<_Container *()> GetContainer;
//    typedef std::function<void (_Container *)> ReleaseContainer;

    ActiveVolume(Grid *grid, Descriptor *descriptors);
    ~ActiveVolume();

    void setViewRadius(const glm::ivec3 &radius);
    size_t getContainerCount() { return 0;/* m_containerCount;*/ }

    void init(const glm::ivec3 &regionIndex, const glm::ivec3 &chunkIndex);
    void updatePosition(const glm::ivec3 &regionIndex, const glm::ivec3 &chunkIndex);

    void update(MeshUpdates &loadedMeshes, MeshUpdates &releaseMeshes);
//    void updateRegions(RegionContainers &newContainers, ChunkContainers &releasedContainers);
//    void updateChunks(RegionContainers &newRegionContainers, ChunkContainers &newChunkContainers);
//    void updateRegions(RegionContainers &newRegionContainers, ChunkContainers &newChunkContainers);

//container allocation
    RegionContainer *getRegionContainer();
    void releaseRegionContainer(RegionContainer *container);

    ChunkContainer *getChunkContainer();
    void releaseChunkContainer(ChunkContainer *container);

//stats
    size_t getLoadingChunkCount() { return m_loadingChunks; }
    size_t getMeshingWaitChunkCount() { return m_chunkMeshQueue.size(); }
    size_t getMeshingChunkCount() { return m_meshingChunks; }

private:
    void initRegionVolumeInfo();
    void initChunkVolumeInfo();

    void updateRegions();
    void updateChunks();
    void updateMeshes(MeshUpdates &loadedMeshes, MeshUpdates &releaseMeshes);
    void completeMeshRequest(process::Request *request, MeshUpdates &loadedMeshes);
    void generateMeshRequest();

    Grid *m_grid;
    const Descriptor *m_descriptors;

    //region/chunk memory
    generic::ObjectHeap<RegionContainer> m_regionContainers;
    generic::ObjectHeap<ChunkContainer> m_chunkContainers;
    generic::ObjectHeap<typename Grid::Chunk> m_chunks;
    generic::ObjectHeap<Mesh> m_chunkMeshes;

    std::vector<RegionHash> m_updatedRegions;
    std::vector<Key> m_updatedChunks;
    RequestQueue m_completedRequests;

    ChunkContainers m_chunkMeshQueue;

    RegionIndex m_regionIndex;
    RegionChunkIndex m_chunkIndex;

    RegionVolume m_regionVolume;
    ChunkVolume m_chunkVolume;
    ChunkLoadRequests m_chunkLoadRequests;
    ChunkContainers m_chunkReleases;

//    std::vector<MeshRequestInfo> m_meshUpdate;

    int m_loadingChunks;
    int m_meshingChunks;
};

}//namespace voxigen

#include "activeVolume.inl"

#endif //_voxigen_activeVolume_h