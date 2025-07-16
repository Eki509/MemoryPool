#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <struct>
#include <mutex>

namespace MyMemoryPool {

    #define FREE_LIST_SIZE 216 // 自由链表数组的长度
    #define MAX_BYTES 512 * 1024 // 最大字节数为512KB
    #define MAX_FREELIST_NUMBERS 256 // 每个自由链表的最大节点数
    const vector<size_t> Hash_Buckets = {16, 56, 56, 56, 24, 8}; // 总和为FREE_LIST_SIZE

    static void* ptrNext(void* ptr) { // 获取下一个指针
        return *reinterpret_cast<void**>(ptr);
    }
    // 将指针强转成void**类型，再进行解引用,即可访问void*大小的地址，在64位系统中即为对该内存块头8字节的访问

    class SizeClass { // SizeClass类用于处理内存大小分类
    public:
        // 平衡哈希桶的数量和内碎片的大小，将内碎片的浪费控制在11%左右
        static inline size_t getIndex(size_t size) { // 根据自定规则建立哈希映射--->>>映射到哪一个哈希桶,返回索引
            assert(size > 0 && size <= MAX_BYTES); 
            if(size <= 128){ // 小于等于128字节，按照8字节对齐
                return _getsizeIndex(size, 3);
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
    }

} // namespace MyMemoryPool