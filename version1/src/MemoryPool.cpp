#include "../include/MemoryPool.h"

namespace MyMemoryPool 
{
MemoryBlock::MemoryBlock(size_t blockSize) : _blockSize(blockSize) {}
MemoryBlock::~MemoryBlock(){
    MemorySlot* cur = _firstBlock;
    while (cur){
        MemorySlot* next = cur->next;
        operator delete(cur);
        cur = next;
    }
}

void MemoryBlock::initialize(size_t slotSize) {
    _slotSize = slotSize;
    _firstBlock = nullptr;
    _unusedSlot = nullptr;
    _endSlot = nullptr;
    _freeList = nullptr;
}

void* MemoryBlock::allocate() {
    {
        std::lock_guard<std::mutex> lock(_mutexFreeList);
        if (_freeList != nullptr) {
            MemorySlot* slot = _freeList;
            _freeList = _freeList->next;
            return slot;
        }
    }
    MemorySlot* slot;
    {
        std::lock_guard<std::mutex> lock(_mutexBlock);
        if (_unusedSlot >= _endSlot) {
            allocateBlock();
        }
        slot = _unusedSlot;
        _unusedSlot = reinterpret_cast<MemorySlot*>(reinterpret_cast<char*>(_unusedSlot) + _slotSize);
    }
    return slot;
}

void MemoryBlock::deallocate(void* ptr) {
    if (ptr == nullptr) return;
    {
        std::lock_guard<std::mutex> lock(_mutexFreeList);
        reinterpret_cast<MemorySlot*>(ptr)->next = _freeList;
        _freeList = reinterpret_cast<MemorySlot*>(ptr);
    }
}

void MemoryBlock::allocateBlock() {
    void* newBlock = operator new(_blockSize);
    reinterpret_cast<MemorySlot*>(newBlock)->next = _firstBlock;
    _firstBlock = reinterpret_cast<MemorySlot*>(newBlock);

    char* ptr = reinterpret_cast<char*>(newBlock) + sizeof(MemorySlot*);
    size_t alignSize = alignPointer(ptr, _slotSize);
    _unusedSlot = reinterpret_cast<MemorySlot*>(ptr + alignSize);
    _endSlot = reinterpret_cast<MemorySlot*>(reinterpret_cast<char*>(newBlock) + _blockSize);
    _freeList = nullptr;
}

size_t MemoryBlock::alignPointer(char* ptr, size_t size) {
    size_t offset = reinterpret_cast<size_t>(ptr) % size;
    return (offset == 0) ? 0 : (size - offset);
}

void BlockToHash::initMemoryBlock() {
    for (size_t i = 0; i < FREE_LIST_SIZE; ++i) {
        getMemoryBlock(i).initialize(MIN_BYTES * (i + 1));
    }
}

MemoryBlock& BlockToHash::getMemoryBlock(size_t index) {
    static std::vector<MemoryBlock> memoryBlocks(FREE_LIST_SIZE);
    if (index >= memoryBlocks.size()) {
        std::cerr << "Index out of bounds: " << index << std::endl;
        throw std::out_of_range("Index out of bounds");
    }
    return memoryBlocks[index];
}

} // namespace MyMemoryPool
