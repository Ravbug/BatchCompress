#include "MainFrame.hpp"
#include <stdlib.h>

#ifdef _MSC_VER
    #define bswap_32(x) _byteswap_ulong(x)
    #define bswap_64(x) _byteswap_uint64(x)
#elif defined __APPLE__
    // Mac OS X / Darwin features
    #include <libkern/OSByteOrder.h>
    #define bswap_32(x) OSSwapInt32(x)
    #define bswap_64(x) OSSwapInt64(x)

    #elif defined(__sun) || defined(sun)

    #include <sys/byteorder.h>
    #define bswap_32(x) BSWAP_32(x)
    #define bswap_64(x) BSWAP_64(x)

    #elif defined(__FreeBSD__)

    #include <sys/endian.h>
    #define bswap_32(x) bswap32(x)
    #define bswap_64(x) bswap64(x)

    #elif defined(__OpenBSD__)

    #include <sys/types.h>
    #define bswap_32(x) swap32(x)
    #define bswap_64(x) swap64(x)

    #elif defined(__NetBSD__)

    #include <sys/types.h>
    #include <machine/bswap.h>
    #if defined(__BSWAP_RENAME) && !defined(__bswap_32)
    #define bswap_32(x) bswap32(x)
    #define bswap_64(x) bswap64(x)
    #endif

    #else

    #include <byteswap.h>
#endif

using namespace std;

//Adapted from: https://github.com/JayXon/Leanify/blob/master/formats/png.cpp

vector<uint8_t> MainFrame::LoadPNG(const std::filesystem::path& path) {
    constexpr uint8_t header_magic[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    bool is_verbose = false;
    bool keep_icc_profile_ = false;
    size_t size_leanified = 0;

    //read in file
    FILE* f = fopen(path.u8string().c_str(), "rb");
    fseek(f, 0, SEEK_END);
    size_t size_ = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> manage(size_);
    auto fp_ = manage.data();
    fread(fp_, 1, size_, f);
    fclose(f);

    // header
    uint8_t* p_read = fp_;
    uint8_t* p_write = p_read - size_leanified;

    memmove(p_write, p_read, sizeof(header_magic));

    p_read += sizeof(header_magic);
    p_write += sizeof(header_magic);

    // chunk
    uint32_t chunk_type;

    uint8_t* idat_addr = nullptr;

    do {
        // read chunk length
        // use bswap to convert Big-Endian to Little-Endian
        // 12 = length: 4 + type: 4 + crc: 4
        uint32_t chunk_length = bswap_32(*(uint32_t*)p_read) + 12;

        // detect truncated file
        if (p_read + chunk_length > fp_ + size_) {
            memmove(p_write, p_read, fp_ + size_ - p_read);
            fp_ -= size_leanified;
            size_ -= p_read - p_write - size_leanified;
        }

        // read chunk type
        chunk_type = *(uint32_t*)(p_read + 4);

        if (chunk_type == 0x54414449) {
            // save IDAT chunk address
            idat_addr = p_write;
        }

        bool should_remove = [&]() {
            // Check the case of first letter, keep all critical chunks.
            if ((chunk_type & 0x20) == 0)
                return false;

            // Remove all ancillary chunks except the following.
            switch (chunk_type) {
            case 0x4C546361:  // acTL     APNG
            case 0x4C546366:  // fcTL     APNG
            case 0x54416466:  // fdAT     APNG    TODO: use Zopfli to recompress fdAT
            case 0x6354706E:  // npTc     Android 9Patch images (*.9.png)
                return false;

            case 0x534E5274:  // tRNS     transparent
                              // tRNS must be before IDAT according to PNG spec
                return idat_addr != nullptr;
            case 0x50434369:  // iCCP     ICC profile
                return !keep_icc_profile_;
            default:
                return true;
            }
        }();
        if (should_remove) {
            if (is_verbose) {
                // chunk name
                for (int i = 4; i < 8; i++)
                    cout << static_cast<char>(p_read[i]);

                cout << " chunk removed, " << chunk_length << " bytes." << endl;
            }
            // remove this chunk
            p_read += chunk_length;
            continue;
        }

        // move this chunk
        memmove(p_write, p_read, chunk_length);

        // skip whole chunk
        p_write += chunk_length;
        p_read += chunk_length;

    } while (chunk_type != 0x444E4549);  // IEND

    fp_ -= size_leanified;
    size_ = p_write - fp_;

    vector<uint8_t> png(fp_, fp_ + size_);

    return png;
}
