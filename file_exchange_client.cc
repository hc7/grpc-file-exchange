#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <utility>
#include <sysexits.h>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "file_exchange.grpc.pb.h"

#include "sequential_file_reader.h"
#include "utils.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

using fileexchange::FileId;
using fileexchange::FileContent;
using fileexchange::FileExchange;

FileId MakeFileId(std::int32_t id)
{
    FileId fid;
    fid.set_id(id);
    return fid;
}

FileContent MakeFileContent(std::int32_t id, const std::string name, const void* data, size_t data_len)
{
    FileContent fc;
    fc.set_id(id);
    fc.set_name(std::move(name));
    fc.set_content(data, data_len);
    return fc;
}

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
        std::string path = GetFilePath();
        const std::string remote_filename = extract_basename(path);
        FileContent fc = MakeFileContent(m_id, remote_filename, data, size);
        m_writer.Write(fc);
    }

private:
    StreamWriter& m_writer;
    std::uint32_t m_id;
};

class FileExchangeClient {
public:
    FileExchangeClient(std::shared_ptr<Channel> channel)
        : m_stub(FileExchange::NewStub(channel))
    {

    }

    void PutFile(std::int32_t id, const std::string& filename)
    {
        FileId returnedId;
        ClientContext context;

        std::unique_ptr<ClientWriter<FileContent>> writer(m_stub->PutFile(&context, &returnedId));
        try {
            FileReaderIntoStream< ClientWriter<FileContent> > reader(filename, id, *writer);

            // TODO: Make the chunk size configurable
            const size_t chunk_size = 1UL << 20;    // Hardcoded to 1MB, which seems to be recommended from experience.
            reader.Read(chunk_size);
        }
        catch (const std::exception& ex) {
            std::cerr << "Failed to send the file " << filename << ": " << ex.what() << std::endl;
            // TODO: Fail the RPC and return
            // return;
        }

        writer->WritesDone();
        Status status = writer->Finish();
        if (!status.ok()) {
            std::cerr << "File Exchange rpc failed: " << status.error_message() << std::endl;
        }
        else {
            std::cout << "Finished sending file with id " << returnedId.id() << std::endl;
        }

        return;
    }

    void GetFileContent(std::int32_t id)
    {

    }
private:
    std::unique_ptr<fileexchange::FileExchange::Stub> m_stub;
};

int main(int argc, char** argv)
{
    if (4 != argc) {
        std::cerr << "USAGE: " << argv[0] << "[put|get] num_id [filename]" << std::endl;
        return EX_USAGE;
    }

    FileExchangeClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
    
    const std::int32_t id = std::atoi(argv[2]);
    const std::string filename = argv[3];
    client.PutFile(id, filename); 
    return 0;
}
