#ifndef __DATA_HPP__
#define __DATA_HPP__
#include <string>

class Data {
    public:
        Data(const char* filename);
        ~Data();

        void* read_8();
        void *read_8_16(bool mFlagUse, bool xFlagUse);
        void* read_16();
        void* read_24();
        void* read_bma();
    private:
        void* file_data;
        void* file_save;
        size_t file_size;
        std::string filename;
};

#endif // __DATA_HPP__