#include "../include/CentralCache.h"

namespace MyMemoryPool {

CentralCache CentralCache::_instance; // 静态实例化CentralCache单例

size_t CentralCache::FetchMemoryForThreadCache(void*& start, void*& end, size_t batchnum, size_t size) {
    assert(size > 0 && size <= MAX_BYTES);
    size_t index = SizeClass::getIndex(size);
    size_t count = 1;
    {
        std::unique_lock<std::mutex> lock(_spanList[index]._mutexSpan); 
        SpanList::Span* span = getSpanFromSpanList(_spanList[index], size);
        assert(span != nullptr && span->_freeList != nullptr);
        start = span->_freeList;
        end = start;
        while(ptrNext(end) != nullptr && count < batchnum) { // 取出要分配的内存块数量，不够就有多少拿多少
            end = ptrNext(end);
            count++;
        }
        span->_freeList = ptrNext(end); // 更新Span的自由链表
        ptrNext(end) = nullptr; // 断开链表
        span->_useCount += count; // 更新Span的使用计数
    }
    return count; // 返回分配的内存块数量
}

SpanList::Span* CentralCache::getSpanFromSpanList(SpanList& spanlist, size_t size) {
    assert(size > 0 && size <= MAX_BYTES);
    SpanList::Span* span = spanlist.Begin();
    while(span != spanlist.End()) {
        if(span->_freeList != nullptr) {
            return span; // 找到合适的Span
        }
        span = span->_next; // 继续遍历
    }
    // 没找到合适的Span，申请新的Span
    spanlist._mutexSpan.unlock();  // 先解CentralCache的互斥锁，避免其他线程释放内存发生阻塞
    PageCache::getInstance()._mutexPage.lock(); // 锁住PageCache的全局互斥锁，防止其他线程申请内存
    SpanList::Span* newSpan = PageCache::getInstance().AllocNewSpanToCentralCache(SizeClass::normPageNum(size));
    newSpan->_isUse = true; // 标记为正在使用
    newSpan->_spanMemorySize = size; // 设置Span下挂载的内存块大小
    PageCache::getInstance()._mutexPage.unlock();
    void* start = (void*)(newSpan->_pageID << PAGE_SHIFT);
    newSpan->_freeList = start; // 将新分配的内存块作为自由链表的头
    void* end = (void*)((newSpan->_pageID + newSpan->_numPages) << PAGE_SHIFT); // 计算Span的结束地址
    char* ptr = (char*)start;
    ptr += size; // 将指针移动到下一个内存块位置
    void* temp = newSpan->_freeList;
    while(ptr < end) { // 将剩余的内存块加入自由链表
        ptrNext(temp) = ptr; // 将当前内存块的下一个指针指向下一个内存块
        temp = ptrNext(temp); // 更新当前指针
        ptr += size; // 移动到下一个内存块位置
    }
    spanlist._mutexSpan.lock(); // 恢复CentralCache的互斥锁，避免在挂载Span后发生其他线程的竞争
    spanlist.PushFront(newSpan); // 将新分配的Span挂载到链表头
    return newSpan; // 返回新分配的Span
}

void CentralCache::FreeMemoryToSpanList(void* start, size_t size) {
    size_t index = SizeClass::getIndex(size);
    {
        std::unique_lock<std::mutex> lock(_spanList[index]._mutexSpan);
        while(start != nullptr){
            void* next = ptrNext(start);
            SpanList::Span* span = PageCache::getInstance().getIdOfSpan(start);
            ptrNext(start) = span->_freeList;
            span->_freeList = start; // 将释放的内存块插入到Span的自由链表头
            span->_useCount--; // 更新Span的使用计数
            if(span->_useCount == 0) { // 如果Span的使用计数为0，说明没有线程在使用它,回收给PageCache
                _spanList[index].pop(span); // 从SpanList中删除该Span
                span->_prev = nullptr; // 清空Span的前驱指针
                span->_next = nullptr; // 清空Span的后继指针
                span->_freeList = nullptr; // 清空Span的自由链表
                _spanList[index]._mutexSpan.unlock(); // 解锁SpanList的互斥锁
                PageCache::getInstance()._mutexPage.lock(); // 锁住PageCache的全局互斥锁，防止其他线程申请内存
                PageCache::getInstance().FreeSpanToPageCache(span); // 将Span释放到PageCache中
                PageCache::getInstance()._mutexPage.unlock(); // 解锁PageCache的全局互斥锁
                _spanList[index]._mutexSpan.lock(); // 恢复SpanList的互斥锁
            }
            start = next; // 继续处理下一个内存块
        }
    }
}

} // namespace MyMemoryPool