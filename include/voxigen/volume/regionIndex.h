#ifndef _voxigen_regionIndex_h_
#define _voxigen_regionIndex_h_

namespace voxigen
{

template<typename _Region>
struct RegionIndex
{
    typedef std::shared_ptr<RegionHandle<_Region>> Handle;

    RegionIndex(){}
    RegionIndex(const glm::ivec3 &index):index(index){}

    template<typename _Grid>
    static Handle getHandle(_Grid *grid, const RegionIndex<_Region> &index)
    {
        return grid->getRegion(index.index);
    }

    template<typename _Grid>
    static bool load(_Grid *grid, Handle handle, size_t lod)
    {
        //lets at least go for 1+ lod
        return grid->loadRegion(handle.get(), lod>0?lod:1);
    }

    template<typename _Grid>
    static bool cancelLoad(_Grid *grid, Handle handle)
    {
        return grid->cancelLoadRegion(handle.get());
    }

    static glm::ivec3 difference(const RegionIndex &index1, const RegionIndex &index2)
    {
        return index2.index-index1.index;
    }

    static RegionIndex offset(const RegionIndex &startIndex, const glm::ivec3 &delta)
    {
        RegionIndex regionIndex;

        regionIndex.index=startIndex.index+delta;
        return regionIndex;
    }

    static glm::ivec3 cells(glm::ivec3 &offset)
    {
        return glm::ivec3(0, 0, 0);
    }

    glm::ivec3 regionIndex()
    {
        return index;
    }

    bool operator==(const RegionIndex &that) const
    {
        return (index==that.index);
    }

    void setX(const RegionIndex &setIndex)
    {
        index.x=setIndex.index.x;
    }

    void setY(const RegionIndex &setIndex)
    {
        index.y=setIndex.index.y;
    }

    void setZ(const RegionIndex &setIndex)
    {
        index.z=setIndex.index.z;
    }

    void incX()
    {
        index.x++;
    }

    void incY()
    {
        index.y++;
    }

    void incZ()
    {
        index.z++;
    }

    std::string pos()
    {
        size_t size=std::snprintf(nullptr, 0, "(%d, %d, %d)", index.x, index.y, index.z);
        std::string value(size, 0);
        std::snprintf(&value[0], size+1, "(%d, %d, %d)", index.x, index.y, index.z);
        return value;
    }

    glm::ivec3 index;
};

template<typename _Region>
glm::ivec3 operator+(const RegionIndex<_Region> &value1, const RegionIndex<_Region> &value2)
{
    return value1.index+value2.index;
}

template<typename _Region>
glm::ivec3 operator+(const RegionIndex<_Region> &value1, const glm::ivec3 &value2)
{
    return value1.index+value2;
}
template<typename _Region>
glm::ivec3 operator-(const RegionIndex<_Region> &value1, const RegionIndex<_Region> &value2)
{
    return value1.index-value2.index;
}

template<typename _Region>
glm::ivec3 operator-(const RegionIndex<_Region> &value1, const glm::ivec3 &value2)
{
    return value1.index-value2;
}

#endif //_voxigen_regionIndex_h_