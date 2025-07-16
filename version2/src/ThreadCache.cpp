#pragma once
#include "MemoryPool.h"
#include "ThreadCache.h"

ThreadCache::_batchNum(FREE_LIST_SIZE, 1); // 初始化批量分配的数量
ThreadCache ThreadCache::_instance; // 静态实例化ThreadCache单例

static void* localAllocate(size_t size) { // 实现线程的独立分配
    if(ptrTLSThreadCache == nullptr) {
        ptrTLSThreadCache = new ThreadCache();
    }
    return ptrTLSThreadCache->allocate(size);
}
static void localDeallocate(void* ptr, size_t size) { // 实现线程的独立释放
    if(ptrTLSThreadCache == nullptr) {
        std::cerr << "Error: ThreadCache not initialized." << std::endl;
        return;
    }
    ptrTLSThreadCache->deallocate(ptr, size);
}

void* ThreadCache::allocate(size_t size) {
    if(size == 0) {
        std::cerr << "Error: Attempt to allocate zero size memory." << std::endl;
        return nullptr;
    }
    if(size > MAX_BYTES){
        return malloc(size);
    }

    size_t index = SizeClass::getIndex(size);
    size_t alignedSize = SizeClass::alignMemory(size);
    if(_freeList[index] == nullptr) { //链表为空，向中心缓存申请内存
        return getMemoryFromCentralCache(index, alignedSize);
    }else{ //从链表头部分配一块空闲的内存
        void* ptr = _freeList[index];
        _freeList[index] = ptrNext(ptr);
        _freeListLength[index]--;
        return ptr;
    }
}

void ThreadCache::deallocate(void* ptr, size_t size){
    assert(ptr != nullptr && size > 0);
    if(size > MAX_BYTES) {
        return free(ptr);
    }
    size_t index = SizeClass::getIndex(size);
    ptrNext(ptr) = _freeList[index]; // 将释放的内存插入回链表头
    _freeList[index] = ptr;
    _freeListLength[index]++;

    if(isReturnToCentralCache(index)) returnMemoryToCentralCache(ptr, size);
}

bool ThreadCache::isReturnToCentralCache(size_t index) {
    return _freeListLength[index] >= MAX_FREELIST_NUMBERS;
}

void* ThreadCache::getMemoryFromCentralCache(size_t index, size_t alignedSize) {
    size_t batchNum = std::min(getBatchNum(index), SizeClass::normBatchNum(alignedSize)); // 批量获取的数量，取规范化和当前批量分配数量的最小值，实现慢开始调节算法
    void* start = nullptr, end = nullptr;
    size_t result = CentralCache::getInstance().FetchMemoryForThreadCache(start, end, batchNum, alignedSize);
    if(result == 1){
        assert(start == end);
        return start; // 只返回了一个内存块，说明头指针和尾指针指向同一个地址
    } else{ // 从CentralCache中获取到了多个连续内存块，将第一个返回，其余的头插到对应的自由链表
        ptrNext(end) = _freeList[index]; // 将尾指针的下一个指针指向当前自由链表的头
        _freeList[index] = ptrNext(start); // 将链表头指针指向批量内存块头指针指向的下一个内存块（保留一个用于返回）
        ptrNext(start) = nullptr; // 将第一个内存块的下一个指针置为nullptr
        _freeListLength[index] += batchNum - 1; // 更新当前自由链表的长度
        return start; // 返回第一个内存块
    }
}

void ThreadCache::returnMemoryToCentralCache(void* ptr, size_t size) {
    
}