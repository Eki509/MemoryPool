#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <sys/mman.h>
#include <cstring>
#include <cassert>
#include <unordered_map>

namespace MyMemoryPool {

    #if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
    // 64位系统
        typedef unsigned long long PAGE_ID;
    #elif defined(__i386__) || defined(_M_IX86) || defined(__arm__)
    // 32位系统
        typedef size_t PAGE_ID;
    #endif

    #define FREE_LIST_SIZE 216 // 自由链表数组的长度
    #define MAX_BYTES (512 * 1024) // 最大字节数为512KB
    #define MAX_FREELIST_NUMBERS 256 // 每个自由链表的最大节点数
    #define PAGE_SIZE 4096 // 定义页面大小为4KB
    #define MAX_PAGES 128 // 每个Span最多包含128页
    #define PAGE_SHIFT 12 // 页面大小的位移量，4096 = 2^12
    const std::vector<size_t> Hash_Buckets = {16, 56, 56, 56, 24, 8}; // 总和为FREE_LIST_SIZE

    // 将指针强转成void**类型，再进行解引用,即可访问void*大小的地址，在64位系统中即为对该内存块头8字节的访问
    static void*& ptrNext(void* ptr) { // 获取下一个指针
        return *reinterpret_cast<void**>(ptr);
    }
    
    static inline void* systemAlloc(size_t numPages){ // 直接与操作系统交互通过mmap申请大块内存
        size_t size = numPages * PAGE_SIZE;
        void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if(ptr == MAP_FAILED){
            std::cerr << "Error: Memory allocation failed." << std::endl;
            return nullptr;
        }
        memset(ptr, 0, size); // 清零内存
        return ptr; // 返回分配的内存地址
    }
    
    class SizeClass { // SizeClass类用于处理内存大小分类
    public:
        // 平衡哈希桶的数量和内碎片的大小，将内碎片的浪费控制在11%左右
        static inline size_t getIndex(size_t size) { // 根据自定规则建立哈希映射--->>>映射到哪一个哈希桶,返回索引
            assert(size > 0 && size <= MAX_BYTES); 
            if(size <= 128){ // 小于等于128字节，按照8字节对齐
                return _getIndex(size, 3);
            }else if(size <= 1024){ // 小于等于1024字节，按照16字节对齐
                return _getIndex(size - 128, 4) + Hash_Buckets[0];
            }else if(size <= 8 * 1024){ // 小于等于8KB，按照128字节对齐
                return _getIndex(size - 1024, 7) + Hash_Buckets[0] + Hash_Buckets[1];
            }else if(size <= 64 * 1024){ // 小于等于64KB，按照1024字节对齐
                return _getIndex(size - 8 * 1024, 10) + Hash_Buckets[0] + Hash_Buckets[1] + Hash_Buckets[2];
            }else if(size <= 256 * 1024){ // 小于等于256KB，按照8KB对齐
                return _getIndex(size - 64 * 1024, 13) + Hash_Buckets[0] + Hash_Buckets[1] + Hash_Buckets[2] + Hash_Buckets[3];
            }else if(size <= 512 * 1024){  //小于等于512KB，按照32KB对齐
                return _getIndex(size - 256 * 1024, 15) + Hash_Buckets[0] + Hash_Buckets[1] + Hash_Buckets[2] + Hash_Buckets[3] + Hash_Buckets[4];
            }else{
                std::cerr << "Error: Size exceeds maximum limit." << std::endl;
            }
            return -1;
        }
        static inline size_t alignMemory(size_t size){ // 建立内存对齐规则--->>>返回内存对齐后的size
            assert(size > 0 && size <= MAX_BYTES);
            if(size <= 128)
                return _alignMemory(size, 8);
            else if(size <= 1024)
                return _alignMemory(size, 16);
            else if(size <= 8 * 1024)
                return _alignMemory(size, 128);
            else if(size <= 64 * 1024)
                return _alignMemory(size, 1024);
            else if(size <= 256 * 1024)
                return _alignMemory(size, 8192);
            else if(size <= 512 * 1024)
                return _alignMemory(size, 32768);
            else
                std::cerr << "Error: Size exceeds maximum limit." << std::endl;
            return -1;
        }
        static size_t normBatchNum(size_t size) { // 规范化批量分配的数量
            assert(size > 0 && size <= MAX_BYTES);
            size_t num = MAX_BYTES / size; // 每个批次的数量
            if(num < 2) {
                num = 2; // 最小批次数量为2
            } else if(num > MAX_FREELIST_NUMBERS) {
                num = MAX_FREELIST_NUMBERS; // 最大批次数量为MAX_FREELIST_NUMBERS
            }
            return num;
        }
        static size_t normPageNum(size_t size){
            assert(size > 0 && size <= MAX_BYTES);
            size_t num = normBatchNum(size);
            size_t sizePages = num * size;
            size_t pageNum = (sizePages + PAGE_SIZE - 1) / PAGE_SIZE; // 向上取整计算页数
            if(pageNum < 1) {
                pageNum = 1; // 最小页数为1
            }
            if(pageNum > MAX_PAGES){
                pageNum = MAX_PAGES; // 最大页数为MAX_PAGES
            }
            return pageNum;
        }
    private:
        static inline size_t _getIndex(size_t size, size_t alignsize){ // 获取内存大小对应的索引
            return ((size + (1 << alignsize) - 1) >> alignsize) - 1; // 采用位运算提高效率
            // 等效于
            // if(size % alignsize == 0) {
            //     return size / alignsize - 1;
            // } else {
            //     return size / alignsize;
            // }
            // 如果采用位运算，这里的alignsize是对齐大小，通常是2的幂次方；否则是字节大小
        }
        static inline size_t _alignMemory(size_t size, size_t alignsize){ // 内存对齐,返回对齐后的内存大小
            return (size + alignsize - 1) & ~(alignsize - 1); // 采用位运算提高效率
            // 等效于
            // if(size % alignsize == 0) {
            //    return size;
            // } else {
            //    return (size / alignsize + 1) * alignsize;
            // }
        }
    };

    class SpanList{ // 维护一个双向循环链表类，用于中心缓存构建Span
    public:
        std::mutex _mutexSpan; // 互斥锁
        struct Span{
            PAGE_ID _pageID = 0; // 页ID，内存在空间中的编号
            size_t _numPages = 0; //对应的页数
            Span* _next = nullptr; // 指向下一个Span的指针
            Span* _prev = nullptr; // 指向上一个Span的指针
            size_t _useCount = 0; // 分配给ThreadCache的使用计数
            size_t _spanMemorySize = 0; // Span下挂载的的内存块大小
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
        Span* Begin() { // 返回链表的头结点
            return _head->_next;
        }
        Span* End() { // 返回链表的尾结点
            return _head;
        }
        bool isEmpty() { // 判断链表是否为空
            return _head->_next == _head;
        }
        void PushFront(Span* ptr){ // 在链表头部插入一个元素
            push(ptr, Begin());
        }
        Span* PopFront(){ // 删除链表头部的元素并返回它
            Span* front = Begin();
            pop(front);
            return front;
        }
        void clear() { // 清空链表
            Span* current = _head->_next;
            while (current != _head) {
                Span* next = current->_next;
                delete current; // 释放当前节点的内存
                current = next; // 移动到下一个节点
            }
            _head->_next = _head; // 重置头结点
            _head->_prev = _head;
        }
        ~SpanList() { // 析构函数，释放链表内存
            clear();
            delete _head; // 释放头结点内存
        }
    private:
        Span* _head;
    };

    template<typename T>
    class DtLenMemoryPool { // 定长内存池类，用于代替本项目中的new/delete操作
    public:
        T* New(){
            std::lock_guard<std::mutex> lock(_mutex); // 确保线程安全
            T* ptr = nullptr;
            if(_freeList != nullptr){
                void* next = ptrNext(_freeList);
                ptr = static_cast<T*>(_freeList);
                _freeList = next; // 更新自由链表头指针
            }else{
                if(_remainSize < sizeof(T)){ // 剩余空间不足，重新申请
                    _remainSize = 512 * 1024; // 每次申请512KB,实现定长512KB
                    _memory = static_cast<char*>(systemAlloc(_remainSize >> PAGE_SHIFT)); // 申请内存
                    if(_memory == nullptr) {
                        std::cerr << "Error: Memory allocation failed." << std::endl;
                        return nullptr;
                    }
                }
                ptr = reinterpret_cast<T*>(_memory);
                size_t ptrSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T); // 确保指针大小不小于T的大小
                _memory += ptrSize;
                _remainSize -= ptrSize; // 更新剩余空间
            }
            new(ptr)T;
            return ptr;
        }

        void Delete(T* ptr){
            if(ptr == nullptr) return;
            std::lock_guard<std::mutex> lock(_mutex); // 确保线程安全
            ptr->~T(); // 调用析构函数
            ptrNext(ptr) = _freeList; // 将释放的内存块插入回自由链表头
            _freeList = ptr;
        }
    private:
        char* _memory = nullptr; // 内存池的起始地址
        size_t _remainSize = 0; // 剩余空间大小
        void* _freeList = nullptr; // 自由链表头指针
        std::mutex _mutex; // 互斥锁，确保线程安全
    };

} // namespace MyMemoryPool