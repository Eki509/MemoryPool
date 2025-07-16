#pragma once
#include <MemoryPool.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
    // 64位系统
    typedef unsigned long long PAGE_ID;
#elif defined(__i386__) || defined(_M_IX86) || defined(__arm__)
    // 32位系统
    typedef size_t PAGE_ID;
#endif

namespace MemoryPool {

    class SpanList{ // 维护一个双向循环链表
    public:
        std::mutex _mutexSpan; // 互斥锁
        struct Span{
            PAGE_ID _pageid = 0; // 页ID
            size_t _num = 0; //页数
            Span* _next = nullptr; // 指向下一个Span的指针
            Span* _prev = nullptr; // 指向上一个Span的指针
            size_t _useCount = 0; // 分配给ThreadCache的使用计数
            size_t _spanMemorySize = 0; // 每个Span下挂载的自由链表中的内存块大小
            void* _freeList = nullptr; // 每个Span下挂载的自由链表
            bool _isUse = false; // 是否正在使用
        };
        SpanList() : _head() {_head = new Span(); _head->_next = _head; _head->_prev = _head; } // 初始化头结点
        void push(Span* ptr, Span* index){ // 将一个元素插入到链表index之前（不用考虑越界问题）
            if (ptr == nullptr || index == nullptr) return;
            Span* temp = index->_prev;
            index->_prev = ptr;
            temp->_next = ptr;
            ptr->_prev = temp;
            ptr->_next = index;
        }
        void pop(Span* index){ // 删除链表中index处的元素，但是不释放它的内存
            if(index == nullptr) return;
            Span* prev = index->_prev;
            Span* next = index->_next;
            prev->_next = next;
            next->_prev = prev;
        }
    private:
        Span* _head;
    };

    class CentralCache {
    public:
        static CentralCache& getInstance() { // 单例模式获取CentralCache实例
            return& _instance;
        }
        size_t FetchMemoryForThreadCache(void* start, void* end, size_t batchnum, size_t size);
        SpanList::Span* getSpanFromSpanList(SpanList& spanlist, size_t size); 
    private:
        CentralCache() = default; // 私有构造函数
        CentralCache(const CentralCache&) = delete; // 禁止拷贝构造
        CentralCache& operator=(const CentralCache&) = delete; // 禁止赋值操作
        static CentralCache _instance; // 单例
        std::vector<SpanList> _spanList(FREE_LIST_SIZE); // 每个元素对应一个SpanList
    };

} // namespace MemoryPool