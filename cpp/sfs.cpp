#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <cstdint>
#include <algorithm> // For std::min

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

namespace fs = std::filesystem;

// Error codes:
// 0 = success
// 1 = cannot access file
// 2 = open failed
// 3 = write failed
// 4 = delete failed

// helper to ensure data is flushed to physical disk
bool flush_to_disk_platform(const std::string& file) {
#if defined(_WIN32)
    std::wstring wpath = fs::path(file).wstring();
    
    HANDLE hFile = CreateFileW(
        wpath.c_str(), 
        GENERIC_WRITE, 
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING, 
        FILE_ATTRIBUTE_NORMAL, 
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateFileW failed for flush on: " << file << std::endl;
        return false;
    }
    
    BOOL ok = FlushFileBuffers(hFile);
    CloseHandle(hFile);
    
    if (!ok) {
        std::cerr << "FlushFileBuffers failed for: " << file << std::endl;
        return false;
    }
    return true;
#else
    int fd = ::open(file.c_str(), O_WRONLY);
    if (fd == -1) {
        std::perror("open for fsync");
        return false;
    }
    int rc = ::fsync(fd);
    if (rc != 0) {
        std::perror("fsync");
        ::close(fd);
        return false;
    }
    ::close(fd);
    return true;
#endif
}

// helper to generate random filename
std::string generate_random_filename(size_t length = 16) {
    static const char charset[] = 
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
    
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += charset[dist(gen)];
    }
    return result;
}

// scramble filename by renaming to random name, returns new path or empty string on error
std::string scramble_filename(const std::string& file) {
    fs::path original_path(file);
    fs::path parent = original_path.parent_path();
    
    for (int attempt = 0; attempt < 10; ++attempt) {
        std::string random_name = generate_random_filename();
        fs::path new_path = parent / random_name;
        
        std::error_code ec;
        if (!fs::exists(new_path, ec)) {
            fs::rename(original_path, new_path, ec);
            if (!ec) {
                std::cout << "Scrambled filename: " << file << " -> " << new_path.string() << std::endl;
                return new_path.string();
            }
        }
    }
    
    std::cerr << "Failed to scramble filename after 10 attempts." << std::endl;
    return "";
}

// quick shred
int quick_shred(const std::string& file) {
    std::error_code ec;
    std::uintmax_t size = fs::file_size(file, ec);
    if (ec) {
        std::cerr << "Cannot access the file: " << file << std::endl;
        return 1; 
    }

    if (size > 0) {
        std::fstream file_stream(file, std::ios::in | std::ios::out | std::ios::binary);
        if (!file_stream) {
            std::cerr << "Failed to open file: " << file << std::endl;
            return 2;
        }

        // 64KB buffer for better HDD performance
        std::vector<unsigned char> zeros(65536, 0x00); 
        std::uintmax_t written = 0;
        
        while (written < size) {
            std::uintmax_t chunk = std::min<std::uintmax_t>(zeros.size(), size - written);
            file_stream.write(reinterpret_cast<const char*>(zeros.data()), chunk);
            if (!file_stream) {
                file_stream.close();
                std::cerr << "Write failed: " << file << std::endl;
                return 3;
            }
            written += chunk;
        }
        
        // Close the stream BEFORE flushing to disk to avoid file locking issues
        file_stream.close();

        if (!flush_to_disk_platform(file)) {
            std::cerr << "Failed to flush data to disk for: " << file << std::endl;
            return 3;
        }
    }

    std::string scrambled_path = scramble_filename(file);
    if (scrambled_path.empty()) scrambled_path = file;

    fs::remove(scrambled_path, ec);
    if (ec) return 4;

    std::cout << "Quick shredded: " << file << std::endl;
    return 0;
}

// Secure shred: DoD 3-pass (0x00, 0xFF, Random)
int secure_shred(const std::string& file) {
    std::error_code ec;
    std::uintmax_t size = fs::file_size(file, ec);
    if (ec) return 1;

    if (size > 0) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<unsigned char> dist(0, 255);
        
        std::vector<unsigned char> buffer(65536);

        for (int pass = 0; pass < 3; ++pass) {
            // Open the file for every pass
            std::fstream file_stream(file, std::ios::in | std::ios::out | std::ios::binary);
            if (!file_stream) {
                std::cerr << "Failed to open file for pass " << pass + 1 << std::endl;
                return 2;
            }

            try {
                // Fill buffer based on pass type
                if (pass == 0) {
                    std::fill(buffer.begin(), buffer.end(), 0x00);
                } else if (pass == 1) {
                    std::fill(buffer.begin(), buffer.end(), 0xFF);
                } 
                // Pass 2 (Random) fills inside the loop

                std::uintmax_t written = 0;
                while (written < size) {
                    std::uintmax_t chunk = std::min<std::uintmax_t>(buffer.size(), size - written);
                    
                    if (pass == 2) {
                        for (size_t i = 0; i < chunk; ++i) buffer[i] = dist(gen);
                    }

                    file_stream.write(reinterpret_cast<const char*>(buffer.data()), chunk);
                    if (!file_stream) throw std::ios_base::failure("Write failed");
                    written += chunk;
                }
            } catch (...) {
                file_stream.close();
                std::cerr << "Write failed during pass " << pass + 1 << ": " << file << std::endl;
                return 3;
            }

            file_stream.close();

            if (!flush_to_disk_platform(file)) {
                std::cerr << "Failed to flush pass " << pass + 1 << " to disk" << std::endl;
                return 3;
            }
        }
    }

    std::string scrambled_path = scramble_filename(file);
    if (scrambled_path.empty()) scrambled_path = file;

    fs::remove(scrambled_path, ec);
    if (ec) return 4;

    std::cout << "Secure shredded: " << file << std::endl;
    return 0;
}

extern "C" {
    int shred_files(const char** files, int count, int mode) {
        if (!files || count < 0) return -1;

        // mode: 0 = quick, 1 = secure
        if (mode != 0 && mode != 1) return -1;

        int error_count = 0;
        for (int i = 0; i < count; ++i) {
            if (!files[i]) {
                ++error_count;
                continue;
            }

            int code = (mode == 0)
                        ? quick_shred(files[i])
                        : secure_shred(files[i]);
                        
            if (code != 0) ++error_count;
        }
        return error_count;
    }
}