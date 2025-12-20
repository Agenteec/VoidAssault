#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include "lz4.h"

class CompressionHelper {
public:
    static std::vector<uint8_t> Compress(const std::vector<uint8_t>& input) {
        if (input.empty()) return {};

        int maxCompressedSize = LZ4_compressBound((int)input.size());
        std::vector<uint8_t> compressed(4 + maxCompressedSize);

        uint32_t originalSize = (uint32_t)input.size();
        std::memcpy(compressed.data(), &originalSize, 4);

        int compressedSize = LZ4_compress_default(
            (const char*)input.data(),
            (char*)(compressed.data() + 4),
            (int)input.size(),
            maxCompressedSize
        );

        if (compressedSize <= 0) return {};

        compressed.resize(4 + compressedSize);
        return compressed;
    }

    static std::vector<uint8_t> Decompress(const std::vector<uint8_t>& input) {
        if (input.size() < 4) return {};

        uint32_t originalSize;
        std::memcpy(&originalSize, input.data(), 4);

        std::vector<uint8_t> decompressed(originalSize);
        int result = LZ4_decompress_safe(
            (const char*)(input.data() + 4),
            (char*)decompressed.data(),
            (int)(input.size() - 4),
            (int)originalSize
        );

        if (result < 0) return {};
        return decompressed;
    }
};