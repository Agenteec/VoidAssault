#include "StreamBuffer.h"
#include "Serialization.h"

#include <fstream>
#include <cstring>

StreamBuffer::Shared StreamBuffer::alloc(size_t numBytes)
{
    return std::make_shared<StreamBuffer>(numBytes);
}

StreamBuffer::Shared StreamBuffer::alloc(const uint8_t* data, size_t numBytes)
{
    return std::make_shared<StreamBuffer>(data, numBytes);
}

StreamBuffer::StreamBuffer(size_t numBytes)
    : gpos_(0)
    , ppos_(0)
{
    buffer_.reserve(numBytes);
}

StreamBuffer::StreamBuffer(const uint8_t* data, size_t numBytes)
    : gpos_(0)
    , ppos_(0)
{
    buffer_.reserve(numBytes);
    buffer_.assign(data, data + numBytes);
}

const std::vector<uint8_t>& StreamBuffer::buffer() const
{
    return buffer_;
}

void StreamBuffer::seekg(size_t pos)
{
    gpos_ = pos;
}

void StreamBuffer::seekp(size_t pos)
{
    ppos_ = pos;
}

size_t StreamBuffer::tellg() const
{
    return gpos_;
}

size_t StreamBuffer::tellp() const
{
    return ppos_;
}

size_t StreamBuffer::size() const
{
    return buffer_.size();
}

bool StreamBuffer::eof() const
{
    return gpos_ >= buffer_.size();
}

// --- WRITE IMPLEMENTATIONS ---

void StreamBuffer::write(bool data)
{
    buffer_.push_back(data ? 1 : 0);
    ppos_++;
}

void StreamBuffer::write(uint8_t data)
{
    buffer_.push_back(data);
    ppos_++;
}

void StreamBuffer::write(uint16_t data)
{
    uint8_t buff[2];
    buff[0] = (uint8_t)(data >> 8);
    buff[1] = (uint8_t)(data);
    buffer_.insert(buffer_.end(), buff, buff + 2);
    ppos_ += 2;
}

void StreamBuffer::write(uint32_t data)
{
    uint8_t buff[4];
    buff[0] = (uint8_t)(data >> 24);
    buff[1] = (uint8_t)(data >> 16);
    buff[2] = (uint8_t)(data >> 8);
    buff[3] = (uint8_t)(data);
    buffer_.insert(buffer_.end(), buff, buff + 4);
    ppos_ += 4;
}

void StreamBuffer::write(uint64_t data)
{
    uint8_t buff[8];
    buff[0] = (uint8_t)(data >> 56);
    buff[1] = (uint8_t)(data >> 48);
    buff[2] = (uint8_t)(data >> 40);
    buff[3] = (uint8_t)(data >> 32);
    buff[4] = (uint8_t)(data >> 24);
    buff[5] = (uint8_t)(data >> 16);
    buff[6] = (uint8_t)(data >> 8);
    buff[7] = (uint8_t)(data);
    buffer_.insert(buffer_.end(), buff, buff + 8);
    ppos_ += 8;
}

void StreamBuffer::write(float32_t data)
{
    write(pack754_32(data));
}

void StreamBuffer::write(float64_t data)
{
    write(pack754_64(data));
}

void StreamBuffer::write(const std::string& data)
{
    auto len = data.size();
    write(uint32_t(len));
    const char* bytes = data.c_str();
    for (uint32_t i = 0; i < len; i++) {
        write(uint8_t(bytes[i]));
    }
}

void StreamBuffer::write(std::time_t data)
{
    write(uint64_t(data));
}

// --- RAYLIB TYPES WRITE ---

void StreamBuffer::write(const Vector2& data)
{
    write((float32_t)data.x);
    write((float32_t)data.y);
}

void StreamBuffer::write(const Vector3& data)
{
    write((float32_t)data.x);
    write((float32_t)data.y);
    write((float32_t)data.z);
}

void StreamBuffer::write(const Quaternion& data)
{
    write((float32_t)data.x);
    write((float32_t)data.y);
    write((float32_t)data.z);
    write((float32_t)data.w);
}

void StreamBuffer::write(const Color& data)
{
    write((uint8_t)data.r);
    write((uint8_t)data.g);
    write((uint8_t)data.b);
    write((uint8_t)data.a);
}

// --- READ IMPLEMENTATIONS ---

void StreamBuffer::read(bool& data)
{
    if (eof()) { data = false; return; }
    data = buffer_[gpos_++] ? true : false;
}

void StreamBuffer::read(uint8_t& data)
{
    if (eof()) { data = 0; return; }
    data = buffer_[gpos_++];
}

void StreamBuffer::read(int8_t& data)
{
    if (eof()) { data = 0; return; }
    data = (int8_t)buffer_[gpos_++];
}

void StreamBuffer::read(uint16_t& data)
{
    if (gpos_ + 2 > buffer_.size()) { data = 0; return; }
    data = ((uint16_t)buffer_[gpos_] << 8) | buffer_[gpos_ + 1];
    gpos_ += 2;
}

void StreamBuffer::read(int16_t& data)
{
    uint16_t ui = 0;
    read(ui);
    if (ui <= 0x7fffu) {
        data = ui;
    }
    else {
        data = -1 - (int16_t)(0xffffu - ui);
    }
}

void StreamBuffer::read(uint32_t& data)
{
    if (gpos_ + 4 > buffer_.size()) { data = 0; return; }
    data = ((uint32_t)buffer_[gpos_] << 24) |
        ((uint32_t)buffer_[gpos_ + 1] << 16) |
        ((uint32_t)buffer_[gpos_ + 2] << 8) |
        buffer_[gpos_ + 3];
    gpos_ += 4;
}

void StreamBuffer::read(int32_t& data)
{
    uint32_t ui = 0;
    read(ui);
    if (ui <= 0x7fffffffu) {
        data = ui;
    }
    else {
        data = -1 - (int32_t)(0xffffffffu - ui);
    }
}

void StreamBuffer::read(uint64_t& data)
{
    if (gpos_ + 8 > buffer_.size()) { data = 0; return; }
    data = ((uint64_t)buffer_[gpos_] << 56) |
        ((uint64_t)buffer_[gpos_ + 1] << 48) |
        ((uint64_t)buffer_[gpos_ + 2] << 40) |
        ((uint64_t)buffer_[gpos_ + 3] << 32) |
        ((uint64_t)buffer_[gpos_ + 4] << 24) |
        ((uint64_t)buffer_[gpos_ + 5] << 16) |
        ((uint64_t)buffer_[gpos_ + 6] << 8) |
        buffer_[gpos_ + 7];
    gpos_ += 8;
}

void StreamBuffer::read(int64_t& data)
{
    uint64_t ui = 0;
    read(ui);
    if (ui <= 0x7fffffffffffffffu) {
        data = ui;
    }
    else {
        data = -1 - (int64_t)(0xffffffffffffffffu - ui);
    }
}

void StreamBuffer::read(float32_t& data)
{
    uint32_t packed = 0;
    read(packed);
    data = unpack754_32(packed);
}

void StreamBuffer::read(float64_t& data)
{
    uint64_t packed = 0;
    read(packed);
    data = unpack754_64(packed);
}

#ifdef __APPLE__
void StreamBuffer::read(std::time_t& data)
{
    uint64_t val = 0;
    read(val);
    data = val;
}
#endif

void StreamBuffer::read(std::string& data)
{
    uint32_t len = 0;
    read(len);
    if (len > 0 && gpos_ + len <= buffer_.size()) {
        data.assign((const char*)&buffer_[gpos_], len);
        gpos_ += len;
    }
    else {
        data = "";
    }
}

// --- RAYLIB TYPES READ ---

void StreamBuffer::read(Vector2& data)
{
    read((float32_t&)data.x);
    read((float32_t&)data.y);
}

void StreamBuffer::read(Vector3& data)
{
    read((float32_t&)data.x);
    read((float32_t&)data.y);
    read((float32_t&)data.z);
}

void StreamBuffer::read(Quaternion& data)
{
    read((float32_t&)data.x);
    read((float32_t&)data.y);
    read((float32_t&)data.z);
    read((float32_t&)data.w);
}

void StreamBuffer::read(Color& data)
{
    read((uint8_t&)data.r);
    read((uint8_t&)data.g);
    read((uint8_t&)data.b);
    read((uint8_t&)data.a);
}

void StreamBuffer::writeToFile(const std::string& path) const
{
    std::ofstream file(path, std::ios::binary);
    if (file.is_open()) {
        file.write((const char*)buffer_.data(), buffer_.size());
        file.close();
    }
}

StreamBuffer::Shared merge(const StreamBuffer::Shared& a, const StreamBuffer::Shared& b)
{
    std::vector<uint8_t> abuff;
    std::vector<uint8_t> bbuff;
    size_t size = 0;
    if (a) {
        abuff = a->buffer();
        size += abuff.size();
    }
    if (b) {
        bbuff = b->buffer();
        size += bbuff.size();
    }
    auto bytes = std::vector<uint8_t>();
    bytes.reserve(size);
    if (a) {
        bytes.insert(bytes.end(), abuff.begin(), abuff.end());
    }
    if (b) {
        bytes.insert(bytes.end(), bbuff.begin(), bbuff.end());
    }
    return StreamBuffer::alloc(bytes.data(), bytes.size());
}