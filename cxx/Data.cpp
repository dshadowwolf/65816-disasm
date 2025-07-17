#include "Data.hpp"
#include <memory>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <cstdint>
#include <stdexcept>
#include <memory>

Data::Data(const char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        throw std::runtime_error("Failed to open file");
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        close(fd);
        throw std::runtime_error("Failed to get file size");
    }

    file_size = sb.st_size;
    void* data = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        throw std::runtime_error("Failed to map file");
    }

    file_data = data;
    file_save = data;
    filename = filename;
    close(fd);
}

Data::~Data() {
    file_data = nullptr;
    if (file_save) {
        munmap(file_save, file_size);
    }
}

void* Data::read_8() {
    if (file_data) {
        void* rv = file_data;
        file_data = (uint8_t*)file_data+sizeof(unsigned char);
        return rv;
    }
    return nullptr;
}

void* Data::read_8_16(bool mFlagUse, bool xFlagUse) {
    if (file_data) {
        void* rv = file_data;
        if (mFlagUse || xFlagUse) {
            file_data = (uint8_t*)file_data + sizeof(unsigned short);
        } else {
            file_data = (uint8_t*)file_data + sizeof(unsigned char);
        }
        return rv;
    }
    return nullptr;
}

void* Data::read_16() {
    if (file_data) {
        void* rv = file_data;
        file_data = (uint8_t*)file_data + sizeof(unsigned short);
        return rv;
    }
    return nullptr;
}

void* Data::read_24() {
    if (file_data) {
        uint32_t* value = new uint32_t;
        uint8_t low_word = *((uint8_t*)file_data);
        file_data = (uint8_t*)file_data + sizeof(unsigned short); // move the pointer
        uint16_t high_byte = *((uint16_t*)file_data);
        file_data = (uint8_t*)file_data + 2; // move the pointer
        *value = (low_word | (high_byte << 16));
        void* rv = file_data;
        file_data = (uint8_t*)file_data + (sizeof(unsigned char) * 3); // move the pointer
        return rv;
    }
    return nullptr;
}

void* Data::read_bma() {
    if (file_data) {
        uint8_t* value = new uint8_t[2];
        value[0] = *((uint8_t*)file_data);
        file_data = (uint8_t*)file_data + sizeof(unsigned char); // move the pointer
        value[1] = *((uint8_t*)file_data);
        file_data = (uint8_t*)file_data + sizeof(unsigned char); // move the pointer
        return value;
    }
    return nullptr;
}
