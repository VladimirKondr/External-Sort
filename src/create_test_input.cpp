/**
 * @brief Создание тестового файла input.bin для проверки решения
 */

#include <fstream>
#include <iostream>
#include <vector>
#include <cstdint>

int main() {
    const std::string filename = "input.bin";
    
    std::vector<uint64_t> data = {5, 3, 4, 2, 1};
    uint64_t num_elements = data.size();
    
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot create file " << filename << std::endl;
        return 1;
    }
    
    file.write(reinterpret_cast<const char*>(&num_elements), sizeof(uint64_t));
    
    for (uint64_t element : data) {
        file.write(reinterpret_cast<const char*>(&element), sizeof(uint64_t));
    }
    
    file.close();
    
    std::cout << "Created test file " << filename << " with " << num_elements << " elements: ";
    for (uint64_t element : data) {
        std::cout << element << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
