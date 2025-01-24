#ifndef BLOCK_CACHE_H
#define BLOCK_CACHE_H

#include <vector>
#include <unordered_map>
#include <deque>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

constexpr size_t BLOCK_SIZE = 4096;
constexpr size_t CACHE_SIZE = 16;

struct Block {
    off_t blockNumber;
    std::vector<char> data;
    bool dirty;

    Block(off_t blockNumber, const std::vector<char>& data, bool dirty)
        : blockNumber(blockNumber), data(data), dirty(dirty) {}
};

class BlockCache {
public:
    explicit BlockCache(size_t cacheSize);
    ssize_t read(int fd, off_t offset, char* buf, size_t count);
    ssize_t write(int fd, off_t offset, const char* buf, size_t count);
    void sync(int fd);

private:
    size_t cacheSize;
    std::deque<Block> cache;
    std::unordered_map<off_t, std::deque<Block>::iterator> blockMap;

    bool isBlockInCache(off_t blockNumber) const;
    void loadBlock(int fd, off_t blockNumber);
    Block& getBlockFromCache(off_t blockNumber);
    void writeBlockToDisk(int fd, const Block& block);
    void evictBlock(int fd);
};

extern "C" {
    int lab2_open(const char* path);
    int lab2_close(int fd);
    ssize_t lab2_read(int fd, void* buf, size_t count);
    ssize_t lab2_write(int fd, const void* buf, size_t count);
    off_t lab2_lseek(int fd, off_t offset, int whence);
    int lab2_fsync(int fd);
}

#endif
