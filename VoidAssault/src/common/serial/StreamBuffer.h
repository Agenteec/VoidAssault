#pragma once

#include "../Common.h"
#include "raylib.h"

#include <memory>
#include <vector>
#include <string>
#include <ctime>

class StreamBuffer {

public:
    typedef std::shared_ptr<StreamBuffer> Shared;
    static Shared alloc(size_t = 1024);
    static Shared alloc(const uint8_t*, size_t);

    explicit StreamBuffer(size_t = 1024);
    StreamBuffer(const uint8_t*, size_t);

    const std::vector<uint8_t>& buffer() const;
    void seekg(size_t);
    void seekp(size_t);
    size_t tellg() const;
    size_t tellp() const;
    size_t size() const;
    bool eof() const;

    void write(bool);
    void write(uint8_t);
    void write(uint16_t);
    void write(uint32_t);
    void write(uint64_t);
    void write(float32_t);
    void write(float64_t);
    void write(std::time_t);
    void write(const std::string&);

    void write(const Vector2&);
    void write(const Vector3&);
    void write(const Quaternion&);
    void write(const Color&);

    void read(bool&);
    void read(uint8_t&);
    void read(int8_t&);
    void read(uint16_t&);
    void read(int16_t&);
    void read(uint32_t&);
    void read(int32_t&);
    void read(uint64_t&);
    void read(int64_t&);
    void read(float32_t&);
    void read(float64_t&);
    void read(std::string&);
#ifdef __APPLE__
    void read(std::time_t&);
#endif

    void read(Vector2&);
    void read(Vector3&);
    void read(Quaternion&);
    void read(Color&);

    void writeToFile(const std::string&) const;

private:
    StreamBuffer(const StreamBuffer&);

    StreamBuffer& operator=(const StreamBuffer&);

    size_t gpos_;
    size_t ppos_;
    std::vector<uint8_t> buffer_;
};

StreamBuffer::Shared merge(const StreamBuffer::Shared&, const StreamBuffer::Shared&);


template <typename T>
StreamBuffer::Shared& operator<<(StreamBuffer::Shared& stream, const std::vector<T>& data)
{
    stream->write(uint32_t(data.size()));
    for (auto& d : data) {
        stream->write(d);
    }
    return stream;
}

template <typename T>
StreamBuffer::Shared& operator>>(StreamBuffer::Shared& stream, std::vector<T>& data)
{
    uint32_t size = 0;
    stream->read(size);
    data.resize(size);
    for (auto i = uint32_t(0); i < size; i++) {
        stream->read(data[i]);
    }
    return stream;
}

template <typename T>
StreamBuffer::Shared& operator<<(StreamBuffer::Shared& stream, const T& data)
{
    stream->write(data);
    return stream;
}

template <typename T>
StreamBuffer::Shared& operator>>(StreamBuffer::Shared& stream, T& data)
{
    stream->read(data);
    return stream;
}