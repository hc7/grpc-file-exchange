#pragma once

#include <cstdint>
#include <string>
#if _WIN32 || _WIN64
#else
#include "sys/errno.h"
#endif

#include "sequential_file_reader.h"
#include "messages.h"
#include "utils.h"

template <class StreamWriter>
class FileReaderIntoStream : public SequentialFileReader {
public:
    FileReaderIntoStream(const std::string& filename, std::int32_t id, StreamWriter& writer)
        : SequentialFileReader(filename)
        , m_writer(writer)
        , m_id(id)
    {
    }

    using SequentialFileReader::SequentialFileReader;
    using SequentialFileReader::operator=;

protected:
    virtual void OnChunkAvailable(const void* data, size_t size) override
    {
        const std::string remote_filename = extract_basename(GetFilePath());
        auto fc = MakeFileContent(m_id, remote_filename, data, size);
        if (! m_writer.Write(fc)) {
            raise_from_system_error_code("The server aborted the connection.", ECONNRESET);
        }
    }

private:
    StreamWriter& m_writer;
    std::uint32_t m_id;
};
