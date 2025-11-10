#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <cstdint>

namespace fs = std::filesystem;

// Error codes:
// 0 = success
// 1 = cannot access file
// 2 = open failed
// 3 = write failed
// 4 = delete failed

// quick_shred fills the file with zeros and deletes it
int quick_shred(const std::string& file) {
    std::error_code ec;
    std::uintmax_t size = fs::file_size(file, ec);
    if (ec || size == 0) {
        std::cerr << "Cannot access the file: " << file << std::endl;
        return 1; 
    }

    std::fstream file_stream(file, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_stream) {
        std::cerr << "Failed to open file: " << file << std::endl;
        return 2;
    }

    std::vector<unsigned char> zeros(4096, 0x00);
    std::uintmax_t written = 0;
    while (written < size) {
        std::uintmax_t chunk = std::min<std::uintmax_t>(4096, size - written);
        file_stream.write(reinterpret_cast<const char*>(zeros.data()), chunk);
        if (!file_stream) {
            file_stream.close();
            std::cerr << "Write failed: " << file << std::endl;
            return 3;
        }
        written += chunk;
    }
    file_stream.flush();
    file_stream.close();

    fs::remove(file, ec);
    if (ec) {
        std::cerr << "Delete failed: " << file << std::endl;
        return 4;
    }

    std::cout << "Quick shredded: " << file << std::endl;
    return 0;
}

// secure_shred performs DoD 3-pass to overwrite the data and then deletes the file
int secure_shred(const std::string& file) {
    std::error_code ec;
    std::uintmax_t size = fs::file_size(file, ec);
    if (ec || size == 0) {
        std::cerr << "Cannot access file: " << file << std::endl;
        return 1;
    }

    std::fstream file_stream(file, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_stream) {
        std::cerr << "Failed to open file: " << file << std::endl;
        return 2;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned char> dist(0, 255);
    std::vector<unsigned char> buffer(4096);

    try {
        for (int pass = 0; pass < 3; ++pass) {
            file_stream.seekp(0);
            for (std::uintmax_t written = 0; written < size;) {
                std::uintmax_t chunk = std::min<std::uintmax_t>(4096, size - written);
                for (std::uintmax_t i = 0; i < chunk; ++i) {
                    buffer[i] = (pass == 0) ? 0x00 :
                                (pass == 1) ? 0xFF :
                                               dist(gen);
                }
                file_stream.write(reinterpret_cast<const char*>(buffer.data()), chunk);
                if (!file_stream) throw std::ios_base::failure("write failed");
                written += chunk;
            }
            file_stream.flush();
        }
    } catch (...) {
        file_stream.close();
        std::cerr << "Write failed: " << file << std::endl;
        return 3;
    }

    file_stream.close();
    fs::remove(file, ec);
    if (ec) {
        std::cerr << "Delete failed: " << file << std::endl;
        return 4;
    }

    std::cout << " Secure shredded: " << file << std::endl;
    return 0;
}

extern "C" {
    int shred_files(const char** files, int count, int mode) {
        int error_count = 0;
        for (int i = 0; i < count; ++i) {
            if (!files[i]) {
                ++error_count;
                continue;
            }

            int code = (mode == 0)
                        ? quick_shred(files[i])
                        : secure_shred(files[i]);
                        
            if (code != 0)
                ++error_count;
        }

        return error_count;
    }
}