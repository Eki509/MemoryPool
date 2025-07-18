#include "../include/PageCache.h"

namespace MyMemoryPool {
    PageCache PageCache::_instance; // 静态实例化PageCache单例

    SpanList::Span* PageCache::AllocNewSpanToCentralCache(size_t numPages){
        assert(numPages > 0 && numPages <= MAX_PAGES);
        if(!_spanList[numPages - 1].isEmpty()) return _spanList[numPages - 1].PopFront(); // 有对应页数的Span直接返回
        for(size_t i = numPages; i < MAX_PAGES; i++){ // 依次向后查找页数更大的Span
            if(!_spanList[i].isEmpty()){ // 找到了返回numPages对应大小的Span,剩余的页数挂载到相应的链表前面
                SpanList::Span* temp = _spanList[i].PopFront();
                SpanList::Span* span = _spanPool.New(); // 从定长内存池中分配一个Span
                span->_pageID = temp->_pageID; // 继承原Span的页ID
                span->_numPages = numPages; // 设置新的Span页数
                temp->_pageID += numPages; // 更新剩余Span的页ID,相当于右移，因为拿走的是左边的页
                temp->_numPages -= numPages; // 更新剩余Span的页数
                _spanList[temp->_numPages - 1].PushFront(temp); // 将切分后得到的Span挂载到对应的链表上
                _spanMap[temp->_pageID] = temp; // 更新首页号
                _spanMap[temp->_pageID + temp->_numPages - 1] = temp; // 更新末尾页号
                for(PAGE_ID i = 0; i < span->_numPages; i++){
                    _spanMap[span->_pageID + i] = span; // 更新每一页对应的页号
                }
                return span; // 返回numPages对应的Span
            }
        }
        // 没找到，直接向系统申请一个最大页数的Span
        SpanList::Span* newSpan = _spanPool.New();
        void* ptr = systemAlloc(MAX_PAGES);
        newSpan->_pageID = (PAGE_ID)((uintptr_t)ptr >> PAGE_SHIFT);
        newSpan->_numPages = MAX_PAGES;
        _spanList[MAX_PAGES - 1].PushFront(newSpan); // 将新Span挂载到对应的链表上
        return AllocNewSpanToCentralCache(numPages); // 递归调用，返回对应页数的Span,对刚分配的大块内存进行切分
    }

    SpanList::Span* PageCache::getIdOfSpan(void* ptr) {
        PAGE_ID id = ((PAGE_ID)ptr >> PAGE_SHIFT); 
        std::unique_lock<std::mutex> lock(_mutexPage);
        auto it = _spanMap.find(id);
        if(it != _spanMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    void PageCache::FreeSpanToPageCache(SpanList::Span* span) {
        while(1){ // 向前合并
            PAGE_ID prevID = span->_pageID - 1;
            auto it = _spanMap.find(prevID);
            if(it == _spanMap.end()) break; // 没有前一个页，直接退出向前合并
            if(it->second->_isUse) break; // 前一个页所属的Span正在使用，不能合并
            if(it->second->_numPages + span->_numPages > MAX_PAGES) break; // 合并后页数超过最大页数，不能合并
            span->_pageID = it->second->_pageID; // 更新当前Span的页ID
            span->_numPages += it->second->_numPages; // 更新当前Span的页数
            _spanList[it->second->_numPages - 1].pop(it->second); // 从对应的链表中删除前一个Span
            _spanPool.Delete(it->second); // 归还节点，防止内存泄漏
        }
        while(1){ // 向后合并
            PAGE_ID nextID = span->_pageID + span->_numPages;
            auto it = _spanMap.find(nextID);
            if(it == _spanMap.end()) break; // 没有后一个页，直接退出向后合并
            if(it->second->_isUse) break; // 后一个页所属的Span正在使用，不能合并
            if(it->second->_numPages + span->_numPages > MAX_PAGES) break; // 合并后页数超过最大页数，不能合并
            span->_numPages += it->second->_numPages; // 更新当前Span的页数
            _spanList[it->second->_numPages - 1].pop(it->second); // 从对应的链表中删除后一个Span
            _spanPool.Delete(it->second); // 归还节点，防止内存泄漏
        }
        // 将合并后的Span释放到PageCache中
        _spanList[span->_numPages - 1].PushFront(span); // 将合并后的Span挂载到对应的哈希桶上
        span->_isUse = false; // 标记Span为未使用
        _spanMap[span->_pageID] = span; // 更新首页号
        _spanMap[span->_pageID + span->_numPages - 1] = span; // 更新末尾页号
    }

} // namespace MyMemoryPool