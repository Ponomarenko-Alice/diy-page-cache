#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <deque>
#include <iostream>
#include <algorithm>

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
    bool isOpen = false;
public:
    BlockCache(size_t cacheSize) : cacheSize(cacheSize) {}

    int openFile(const char* path) {
        int fd = ::open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (fd == -1) {
            perror("Error opening file");
            return -1;
        }
        isOpen = true;
        return fd;
    }

    int closeFile(int fd) {
        sync(fd); 
        return ::close(fd);
    }

    ssize_t read(int fd, off_t offset, char* buf, size_t count) {
        size_t bytesRead = 0;
        while (bytesRead < count) {
            off_t blockNumber = offset / BLOCK_SIZE;
            size_t blockOffset = offset % BLOCK_SIZE;
            size_t toRead = std::min(count - bytesRead, BLOCK_SIZE - blockOffset);

            if (!isBlockInCache(blockNumber)) {
                loadBlock(fd, blockNumber);
            }

            Block& block = getBlockFromCache(blockNumber);
            std::memcpy(buf + bytesRead, block.data.data() + blockOffset, toRead);

            bytesRead += toRead;
            offset += toRead;
        }
        return bytesRead;
    }

    ssize_t write(int fd, off_t offset, const char* buf, size_t count) {
        size_t bytesWritten = 0;
        while (bytesWritten < count) {
            off_t blockNumber = offset / BLOCK_SIZE;
            size_t blockOffset = offset % BLOCK_SIZE;
            size_t toWrite = std::min(count - bytesWritten, BLOCK_SIZE - blockOffset);

            if (!isBlockInCache(blockNumber)) {
                loadBlock(fd, blockNumber);
            }

            Block& block = getBlockFromCache(blockNumber);
            std::memcpy(block.data.data() + blockOffset, buf + bytesWritten, toWrite);
            block.dirty = true;

            bytesWritten += toWrite;
            offset += toWrite;
        }
        return bytesWritten;
    }

    void sync(int fd) {
        for (auto& block : cache) {
            if (block.dirty) {
                writeBlockToDisk(fd, block);
            }
        }
    }

private:
    size_t cacheSize;
    std::deque<Block> cache; 
    std::unordered_map<off_t, std::deque<Block>::iterator> blockMap;

    bool isBlockInCache(off_t blockNumber) {
        return blockMap.find(blockNumber) != blockMap.end();
    }

    Block& getBlockFromCache(off_t blockNumber) {
        return *blockMap[blockNumber];
    }

    void loadBlock(int fd, off_t blockNumber) {
        if (cache.size() >= cacheSize) {
            evictBlock(fd);
        }

        std::vector<char> data(BLOCK_SIZE, 0);
        pread(fd, data.data(), BLOCK_SIZE, blockNumber * BLOCK_SIZE);
        cache.emplace_back(blockNumber, data, false);
        blockMap[blockNumber] = --cache.end();
    }

    void evictBlock(int fd) {
        Block& block = cache.front();
        if (block.dirty) {
            writeBlockToDisk(fd, block);
        }
        blockMap.erase(block.blockNumber);
        cache.pop_front();
    }

    void writeBlockToDisk(int fd, Block& block) {
        pwrite(fd, block.data.data(), BLOCK_SIZE, block.blockNumber * BLOCK_SIZE);
        block.dirty = false;
    }
};

// API для работы с кэшем
extern "C" {
    static BlockCache cache(CACHE_SIZE);

    int lab2_open(const char* path) {
        return cache.openFile(path);
    }

    int lab2_close(int fd) {
        return cache.closeFile(fd);
    }

    ssize_t lab2_read(int fd, void* buf, size_t count) {
        off_t offset = lseek(fd, 0, SEEK_CUR);
        ssize_t bytesRead = cache.read(fd, offset, static_cast<char*>(buf), count);
        lseek(fd, offset + bytesRead, SEEK_SET);
        return bytesRead;
    }

    ssize_t lab2_write(int fd, const void* buf, size_t count) {
        off_t offset = lseek(fd, 0, SEEK_CUR);
        ssize_t bytesWritten = cache.write(fd, offset, static_cast<const char*>(buf), count);
        lseek(fd, offset + bytesWritten, SEEK_SET);
        return bytesWritten;
    }

    off_t lab2_lseek(int fd, off_t offset, int whence) {
        return lseek(fd, offset, whence);
    }

    int lab2_fsync(int fd) {
        cache.sync(fd);
        return fsync(fd);
    }
}
