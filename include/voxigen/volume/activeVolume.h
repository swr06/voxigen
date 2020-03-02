#ifndef _voxigen_activeVolume_h_
#define _voxigen_activeVolume_h

#include "voxigen/volume/regularGrid.h"
#include "voxigen/freeQueue.h"

#include "voxigen/volume/regionIndex.h"
#include "voxigen/volume/regionChunkIndex.h"

#include <memory>
#include <functional>

namespace voxigen
{

template<typename _Grid, typename _Container, typename _Index>
class ActiveVolume//:public RegularGridTypes<_Grid>
{
public:
    typedef typename _Grid::GridType GridType;
    typedef typename _Grid::DescriptorType DescriptorType;

    typedef _Container ContainerType;
    typedef std::vector<ContainerType> Renderers;

    typedef _Index Index;

    typedef std::function<_Container *()> GetContainer;
    typedef std::function<void (_Container *)> ReleaseContainer;

    struct ContainerInfo
    {
        //keep data after meshing
        bool keepData; 
        //mesh chunk, only mesh if neighbors will be loaded
        bool mesh;
        //what lod to load and mesh
        size_t lod;
        //render container
        ContainerType *container;
    };
    typedef std::vector<ContainerInfo> VolumeInfo;

    struct LoadContainer
    {
        LoadContainer():lod(1), container(nullptr) {}
        LoadContainer(size_t lod, ContainerType *container):lod(lod), container(container) {}
        size_t lod;
        ContainerType *container;
    };
    typedef std::vector<LoadContainer> LoadRequests;

    ActiveVolume(GridType *grid, DescriptorType *descriptors, GetContainer getContainer, ReleaseContainer releaseContainer);
    ~ActiveVolume();

    void setViewRadius(const glm::ivec3 &radius);
    size_t getContainerCount() { return m_containerCount; }

    void setOutlineInstance(unsigned int outlineInstanceId);

    void init(const Index &index, LoadRequests &load, std::vector<ContainerType *> &release);
    void updateCamera(const Index &index);
    void update(const Index &index, LoadRequests &load, std::vector<ContainerType *> &release);

//    void draw();
//    void drawInfo(const glm::mat4x4 &projectionViewMat);
//    void drawOutline();

    glm::ivec3 relativeCameraIndex();
//    ChunkRenderInfoType *getChunkRenderInfo(const Key &key);
    ContainerType *getRenderInfo(const Index &index);// const Key &key);
    VolumeInfo &getVolume() { return m_volume; }

    void releaseInfo(_Container *containerInfo);

    void generateUpdateOrder();

private:
    void initVolumeInfo();

    glm::ivec3 calcVolumeSize(const glm::ivec3 &radius);

    _Container *getFreeContainer();
    void releaseFreeContainer(_Container *container);

    void getMissingContainers(LoadRequests &load);

    GetContainer getContainer;
    ReleaseContainer releaseContainer;

//    void updateRegion(glm::ivec3 &startRegionIndex, glm::ivec3 &startChunkIndex, glm::ivec3 &size);
    void releaseRegion(const glm::ivec3 &start, const glm::ivec3 &size, std::vector<ContainerType *> &release);
//    void getRegion(const glm::ivec3 &start, const glm::ivec3 &startRegionIndex, const glm::ivec3 &startChunkIndex, const glm::ivec3 &size);
    void getRegion(const glm::ivec3 &start, const Index &startIndex, const glm::ivec3 &size, LoadRequests &load);

//    void releaseChunkInfo(ChunkRenderInfoType &renderInfo);
    
    
    GridType *m_grid;
    const DescriptorType *m_descriptors;

    glm::ivec3 m_viewRadius;
    size_t m_containerCount;

//    glm::ivec3 m_cameraRegionIndex;
//    glm::ivec3 m_cameraChunkIndex;
    Index m_cameraIndex;

    Index m_index;
//    glm::ivec3 m_regionIndex;
//    glm::ivec3 m_chunkIndex;



//    std::unordered_map<Key::Type, size_t> m_volumeMap;
//    std::vector<ChunkRenderInfoType> m_volume;
    VolumeInfo m_volume;
    glm::ivec3 m_volumeSize;
    glm::ivec3 m_volumeCenterIndex;

    std::vector<size_t> m_updateOrder;

    std::vector<ContainerType *>m_containerReleaseQueue;
    FreeQueue<ContainerType> m_containerQueue;
};

}//namespace voxigen

#include "activeVolume.inl"

#endif //_voxigen_activeVolume_h