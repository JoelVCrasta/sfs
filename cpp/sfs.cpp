#include <iostream>
#include <vector>
#include <string>

extern "C" {
    void shred_files(const char** files, int count, int mode) {
        std::vector<std::string> files_vec;
        files_vec.reserve(count);

        for (int i = 0; i < count; ++i) 
            if (files[i]) 
                files_vec.emplace_back(files[i]);
        
        // Simulate file shredding operation
        for (const auto& file : files_vec) {
            std::cout << "cpp: Shredding file: " << file << " with mode: " << mode << std::endl;
            // Actual shredding logic would go here
        }
    }
}