#include <iostream>
#include <atomic>
#include <mutex>
#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

namespace MyMemoryPool
{
    #define FREE_LIST_SIZE 64
    #define MAX_BYTES 512
    #define MIN_BYTES 8
    #define BLOCKSIZE 4096

    struct MemorySlot
    {
        MemorySlot* next;
    };

    class MemoryBlock
    {
    public:
        MemoryBlock(size_t blockSize = BLOCKSIZE);
        ~MemoryBlock();
        void initialize(size_t);
        void* allocate();
        void deallocate(void*);
    private:
        size_t alignPointer(char* ptr, size_t size);
        void allocateBlock();
    private:
        size_t _blockSize;
        size_t _slotSize;
        MemorySlot* _firstBlock;
        MemorySlot* _unusedSlot;
        MemorySlot* _endSlot;
        MemorySlot* _freeList;
        std::mutex _mutexFreeList;
        std::mutex _mutexBlock;
    };

    class BlockToHash
    {
    public:
        static void initMemoryBlock();
        static MemoryBlock& getMemoryBlock(size_t index);
        static void* allocateMemory(size_t size){
            if (size <= 0) {
                std::cerr << "Size out of bounds: " << size << std::endl;
                return nullptr;
            }
            if(size > MAX_BYTES){
                return operator new(size); // Use global new for large allocations
            }
            return getMemoryBlock((size + MIN_BYTES - 1) / MIN_BYTES - 1).allocate(); //static_cast<double>(size) / MIN_BYTES
        }
        static void freeMemory(void* ptr, size_t size){
            if(ptr == nullptr) return;
            if(size > MAX_BYTES){
                operator delete(ptr); // Use global delete for large allocations
                return;
            }
            getMemoryBlock((size + MIN_BYTES - 1) / MIN_BYTES - 1).deallocate(ptr);
        }
    };

    template <typename T, typename... Args>
    T* newElement(Args&&... args) {
        T* ptr = nullptr;
        if((ptr = reinterpret_cast<T*>(BlockToHash::allocateMemory(sizeof(T)))) == nullptr) {
            std::cerr << "Memory allocation failed for type: " << typeid(T).name() << std::endl;
            return nullptr;
        }else{
            new (ptr) T(std::forward<Args>(args)...);
        }
        return ptr;
    }
    template <typename T>
    void deleteElement(T* ptr) {
        if(ptr == nullptr) return;
        ptr->~T(); // Call destructor
        BlockToHash::freeMemory(ptr, sizeof(T)); // Memory recycling
    }

} // namespace MyMemoryPool