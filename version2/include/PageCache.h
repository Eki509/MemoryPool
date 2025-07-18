#pragma once
#include "MemoryPool.h"

namespace MyMemoryPool {
    class PageCache {
    public:
        std::mutex _mutexPage; // 互斥锁
        static PageCache& getInstance() { // 单例模式获取PageCache实例
            return _instance;
        }
        SpanList::Span* AllocNewSpanToCentralCache(size_t numPages);
        SpanList::Span* getIdOfSpan(void* ptr);
        void FreeSpanToPageCache(SpanList::Span* span);
    private:
        PageCache() : _spanList(MAX_PAGES) {} // 私有构造函数
        PageCache(const PageCache&) = delete; // 禁止拷贝构造
        PageCache& operator=(const PageCache&) = delete; // 禁止赋值操作
        static PageCache _instance; // 单例
        std::vector<SpanList> _spanList; // Span链表,对应页数的Span挂载到页数-1的下标链表上
        std::unordered_map<PAGE_ID, SpanList::Span*> _spanMap; // 用于快速查找Span
        // void* systemAlloc(size_t numPages); // 直接与操作系统交互通过mmap申请大块内存
        DtLenMemoryPool<SpanList::Span> _spanPool; // 定长内存池，用于Span的分配
    };

} // namespace MyMemoryPool