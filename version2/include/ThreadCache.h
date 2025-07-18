#pragma once
#include "MemoryPool.h"

namespace MyMemoryPool {

class ThreadCache {
public:
    // static ThreadCache& getInstance() { 
    //     return _instance;
    // }
    ThreadCache() : _freeList(FREE_LIST_SIZE, nullptr), 
                    _freeListLength(FREE_LIST_SIZE, 0)
                    {}
    void* allocate(size_t size);
    void deallocate(void* ptr, size_t size);
    static size_t getBatchNum(size_t index) { // 获取批量分配的数量
        if(_batchNum[index] == MAX_FREELIST_NUMBERS) return MAX_FREELIST_NUMBERS;
        return _batchNum[index]++;
    }
private:
    void* getMemoryFromCentralCache(size_t index, size_t alignedSize);
    void returnMemoryToCentralCache(void*& freelist, size_t size);
    bool isReturnToCentralCache(size_t index);
private:
    std::vector<void*> _freeList; // 自由链表数组
    std::vector<size_t> _freeListLength; // 自由链表长度数组
    static std::vector<size_t> _batchNum; // 批量分配的数量,采用慢开始调节算法
    static ThreadCache _instance; // 单例模式
};

extern thread_local ThreadCache* ptrTLSThreadCache;
} // namespace MyMemoryPool