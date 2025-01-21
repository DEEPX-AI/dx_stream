#ifndef MEMORY_POOL_HPP
#define MEMORY_POOL_HPP

#include <iostream>
#include <mutex>
#include <vector>

class MemoryPool {
  public:
    MemoryPool();
    MemoryPool(size_t blockSize, size_t poolSize);
    ~MemoryPool();

    void *allocate();
    void deallocate(void *obj);

    void initialize(size_t blockSize, size_t poolSize);
    void deinitialize();

    size_t get_block_size() const { return this->m_blockSize; }
    size_t get_free_pool_size() const { return this->m_freeList.size(); }
    size_t get_tot_pool_size() const { return this->m_memoryBlocks.size(); }

    MemoryPool(const MemoryPool &) = delete;
    MemoryPool &operator=(const MemoryPool &) = delete;

    MemoryPool(MemoryPool &&other) noexcept;
    MemoryPool &operator=(MemoryPool &&other) noexcept;

  private:
    void allocatePool();
    void releasePool();

    size_t m_blockSize = 0;
    size_t m_poolSize = 0;
    std::vector<void *> m_freeList;
    std::vector<void *> m_memoryBlocks;
    std::mutex m_mutex;
};

#endif // MEMORY_POOL_HPP
