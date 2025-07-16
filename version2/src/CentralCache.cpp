#pragma once
#include "MemoryPool.h"
#include "CentralCache.h"

CentralCache CentralCache::_instance; // 静态实例化CentralCache单例

size_t CentralCache::FetchMemoryForThreadCache(void* start, void* end, size_t batchnum, size_t size) {
    assert(alignedSize > 0 && alignedSize <= MAX_BYTES);
    size_t index = SizeClass::getIndex(size);
    size_t count = 0;
    {
        std::lock_guard<std::mutex> lock(_spanList[index]._mutexSpan); 
        SpanList::Span* span = getSpanFromSpanList(_spanList[index], size);
        assert(span != nullptr && span->_freeList != nullptr);
        start = span->_freeList;
        end = start;
        while(ptrNext(end) != nullptr && count < batchnum) {
            end = ptrNext(end);
            count++;
        }
        span->_freeList = ptrNext(end); // 更新Span的自由链表
        ptrNext(end) = nullptr; // 断开链表
    }
    span->_useCount += count; // 更新Span的使用计数
    span->_isUse = true; // 标记Span为正在使用
    return count; // 返回分配的内存块数量
}

Span* CentralCache::getSpanFromSpanList(SpanList& spanlist, size_t size) {

}