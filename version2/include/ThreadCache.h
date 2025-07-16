#pragma once
#include "MemoryPool.h"

namespace MyMemoryPool {

class ThreadCache {
public:
    static ThreadCache& getInstance() { 
        return& _instance;
    }
    void* allocate(size_t size);
    void* deallocate(void* ptr, size_t size);
    static size_t getBatchNum(size_t index) { // 获取批量分配的数量
        if(_batchNum[index] == MAX_FREELIST_NUMBERS) return MAX_FREELIST_NUMBERS;
        return _batchNum[index]++;
    }
private:
    void* getMemoryFromCentralCache(size_t index, size_t alignedSize);
    void returnMemoryToCentralCache(void* ptr, size_t size);
    bool isReturnToCentralCache(size_t index);
private:
    std::vector<void*> _freeList(FREE_LIST_SIZE, nullptr);
    std::vector<size_t> _freeListLength(FREE_LIST_SIZE, 0);
    static std::vector<size_t> _batchNum(FREE_LIST_SIZE); // 批量分配的数量,采用慢开始调节算法
    static ThreadCache _instance; // 单例模式
};

thread_local ThreadCache* ptrTLSThreadCache = nullptr;
} // namespace MyMemoryPool