#include <algorithm>
#include <cstdlib>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

#include "cache.hpp"

void generateDataFile(const std::string& filename, size_t numElements) {
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_DIRECT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        std::cerr << "Failed to create file: " << filename << '\n';
        return;
    }

    void *buffer;
    if (posix_memalign(&buffer, BLOCK_SIZE, BLOCK_SIZE)) {
        std::cerr << "Failed to allocate aligned memory\n";
        close(fd);
        return;
    }

    std::srand(std::time(nullptr));

    int *intBuffer = static_cast<int *>(buffer);
    size_t intsPerBlock = BLOCK_SIZE / sizeof(int);

    for (size_t i = 0; i < numElements; i += intsPerBlock) {
        for (size_t j = 0; j < intsPerBlock && i + j < numElements; ++j) {
            intBuffer[j] = std::rand();
        }

        if (write(fd, buffer, BLOCK_SIZE) == -1) {
            std::cerr << "Error writing to file\n";
            free(buffer);
            close(fd);
            return;
        }
    }

    free(buffer);
    close(fd);

    std::cout << "Data file generated: " << filename << '\n';
}

void loadDataFromFile(const std::string& filename, std::vector<int>& data) {
    std::ifstream file(filename, std::ios::binary);
    int num;
    while (file.read(reinterpret_cast<char*>(&num), sizeof(num))) {
        data.push_back(num);
    }
}

void performTask(std::vector<int>& data) {
    std::sort(data.begin(), data.end());
}

void runBenchmark() {
    std::vector<int> data;
    generateDataFile("data.bin", 100000);
    loadDataFromFile("data.bin", data);
    performTask(data);
}

void generateDataFileWithCache(const std::string& filename, size_t numElements, int& fd) {
    fd = lab2_open(filename.c_str());
    if (fd < 0) {
        std::cerr << "Failed to create file: " << filename << '\n';
        return;
    }

    std::srand(std::time(nullptr));
    for (size_t i = 0; i < numElements; ++i) {
        int num = std::rand();
        if (lab2_write(fd, &num, sizeof(num)) == -1) {
            std::cerr << "Error writing to file\n";
            return;
        }
    }


    std::cout << "Data file generated: " << filename << '\n';
}

void loadDataFromFileWithCache(int fd, std::vector<int>& data) {
    int num;
    ssize_t bytesRead;
    while ((bytesRead = lab2_read(fd, &num, sizeof(num))) > 0) {
        data.push_back(num);
        if (num == 0) {
            return;
        }
    }

    if (bytesRead < 0) {
        std::cerr << "Error reading from file\n";
    }
}


void performTaskWithCache(std::vector<int>& data) {
    std::sort(data.begin(), data.end());
}

void runBenchmarkWithCache() {
    std::vector<int> data;
    int fd;
    generateDataFileWithCache("data_cache.bin", 100000, fd);
    loadDataFromFileWithCache(fd, data);

    performTaskWithCache(data);
    
     if (lab2_close(fd) == -1) {
        std::cerr << "Error closing file\n";
    }

}

int main() {
    auto start = std::chrono::high_resolution_clock::now();

    // Without cache
    runBenchmark();
    std::cout << "Without caching \n";

    // With cache 
    runBenchmarkWithCache();
    std::cout << "With caching \n";


    auto end = std::chrono::high_resolution_clock::now();


    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Running time: " << duration << " milliseconds\n";

    return 0;
}
