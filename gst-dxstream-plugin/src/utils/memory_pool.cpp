#include "memory_pool.hpp"
#include <algorithm>
#include <cstring>

MemoryPool::MemoryPool() : m_blockSize(0), m_poolSize(0) {}

MemoryPool::MemoryPool(size_t blockSize, size_t poolSize)
    : m_blockSize(blockSize), m_poolSize(poolSize) {
    allocatePool();
}

MemoryPool::~MemoryPool() { releasePool(); }

MemoryPool::MemoryPool(MemoryPool &&other) noexcept
    : m_blockSize(other.m_blockSize), m_poolSize(other.m_poolSize),
      m_freeList(std::move(other.m_freeList)),
      m_memoryBlocks(std::move(other.m_memoryBlocks)) {}

MemoryPool &MemoryPool::operator=(MemoryPool &&other) noexcept {
    if (this != &other) {
        releasePool();

        m_blockSize = other.m_blockSize;
        m_poolSize = other.m_poolSize;
        m_freeList = std::move(other.m_freeList);
        m_memoryBlocks = std::move(other.m_memoryBlocks);
    }
    return *this;
}

void MemoryPool::initialize(size_t blockSize, size_t poolSize) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_memoryBlocks.empty()) {
        releasePool();
    }
    m_blockSize = blockSize;
    m_poolSize = poolSize;
    allocatePool();
}

void MemoryPool::deinitialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    releasePool();
}

void *MemoryPool::allocate() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_freeList.empty()) {
        void *newBlock = ::operator new(m_blockSize);
        m_memoryBlocks.push_back(newBlock);
        m_poolSize += 1;
        return newBlock;
    }

    void *obj = m_freeList.back();
    m_freeList.pop_back();
    return obj;
}

void MemoryPool::deallocate(void *obj) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (std::find(m_freeList.begin(), m_freeList.end(), obj) !=
        m_freeList.end()) {
        return;
    }
    m_freeList.push_back(obj);
}

void MemoryPool::allocatePool() {
    for (size_t i = 0; i < m_poolSize; ++i) {
        void *ptr = ::operator new(m_blockSize);
        m_freeList.push_back(ptr);
        m_memoryBlocks.push_back(ptr);
    }
}

void MemoryPool::releasePool() {
    for (void *ptr : m_memoryBlocks) {
        if (ptr) {
            ::operator delete(ptr);
        }
    }
    m_memoryBlocks.clear();
    m_freeList.clear();
}
