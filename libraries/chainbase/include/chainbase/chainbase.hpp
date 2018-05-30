#pragma once

#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>

#include <chainbase/db_state.hpp>

namespace chainbase {

class database : public db_state
{
    boost::interprocess::file_lock _flock;

    boost::filesystem::path _data_dir;

    std::unique_ptr<boost::interprocess::managed_mapped_file> _meta;

private:
    void check_dir_existance(const boost::filesystem::path& dir, bool read_only);
    void create_meta_file(const boost::filesystem::path& file);

public:
    virtual ~database();

    enum open_flags
    {
        read_only = 0,
        read_write = 1
    };

    void open(const boost::filesystem::path& dir, uint32_t write = read_only, uint64_t shared_file_size = 0);
    void close();
    void flush();
    void wipe();
};

} // namespace chainbase
