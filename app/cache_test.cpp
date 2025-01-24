#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

#include "cache.hpp"

void generateDataFile(const std::string& filename, size_t numElements) {
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        std::cerr << "Failed to create file: " << filename << '\n';
        return;
    }

    std::srand(std::time(nullptr));
    for (size_t i = 0; i < numElements; ++i) {
        int num = std::rand();
        if (write(fd, &num, sizeof(num)) == -1) {
            std::cerr << "Error writing to file\n";
            close(fd);
            return;
        }
    }

    if (close(fd) == -1) {
        std::cerr << "Error closing file\n";
        return;
    }

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
    generateDataFile("data.bin", 10000000);
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
            lab2_close(fd);
            return;
        }
    }

    if (lab2_close(fd) == -1) {
        std::cerr << "Error closing file\n";
        return;
    }

    std::cout << "Data file generated: " << filename << '\n';
}

void loadDataFromFileWithCache(int fd, std::vector<int>& data) {
    int num;
    ssize_t bytesRead;
    while ((bytesRead = lab2_read(fd, &num, sizeof(num))) > 0) {
        data.push_back(num);
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
    generateDataFileWithCache("data_cache.bin", 10000000, fd);
    loadDataFromFileWithCache(fd, data);
    performTaskWithCache(data);
}

int main() {
    time_t starttime = 0;
    time_t finishtime = 0;

    if (time(&starttime) == static_cast<time_t>(-1)) {
        std::cerr << "Error: Failed to retrieve the current time.\n";
        return 1;
    }

    // Without cache
    // runBenchmark();

    // With cache 
    runBenchmarkWithCache();

    if (time(&finishtime) == static_cast<time_t>(-1)) {
        std::cerr << "Error: Failed to retrieve the current time.\n";
        return 1;
    }

    const time_t duration = finishtime - starttime;
    std::cout << "Running time: " << duration << " seconds\n";

    return 0;
}
