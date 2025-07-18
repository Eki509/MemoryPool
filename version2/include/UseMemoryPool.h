#pragma once
#include "MemoryPool.h"
#include "ThreadCache.h"
#include "PageCache.h"

namespace MyMemoryPool {
    
static void* localAllocate(size_t size) { // 实现线程的独立分配
    if(ptrTLSThreadCache == nullptr) {
        static DtLenMemoryPool<ThreadCache> _tcPool; // 定长内存池，用于分配ThreadCache的内存
        ptrTLSThreadCache = _tcPool.New();
    }
    return ptrTLSThreadCache->allocate(size);
}

static void localDeallocate(void* ptr) { // 实现线程的独立释放
    SpanList::Span* span = PageCache::getInstance().getIdOfSpan(ptr);
    if(span == nullptr) {
        std::cerr << "Error: Span not found for ptr " << ptr << std::endl;
        return;
    }
    size_t size = span->_spanMemorySize; // 获取Span下挂载的内存块大小
    if(ptrTLSThreadCache == nullptr) {
        std::cerr << "Error: ThreadCache not initialized." << std::endl;
        return;
    }
    ptrTLSThreadCache->deallocate(ptr, size);
}

} // namespace MyMemoryPool