#pragma once
#include <MemoryPool.h>

namespace MemoryPool {

    class CentralCache {
    public:
        static CentralCache& getInstance() { // 单例模式获取CentralCache实例
            return _instance;
        }
        size_t FetchMemoryForThreadCache(void*& start, void*& end, size_t batchnum, size_t size);
        SpanList::Span* getSpanFromSpanList(SpanList& spanlist, size_t size); 
    private:
        CentralCache() = default; // 私有构造函数
        CentralCache(const CentralCache&) = delete; // 禁止拷贝构造
        CentralCache& operator=(const CentralCache&) = delete; // 禁止赋值操作
        static CentralCache _instance; // 单例
        std::vector<SpanList> _spanList(FREE_LIST_SIZE); // 每个元素对应一个SpanList
        void FreeMemoryToSpanList(void* start, size_t size); // 将内存块释放到SpanList中
    };

} // namespace MemoryPool