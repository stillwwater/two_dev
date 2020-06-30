// Copyright (c) 2020 stillwwater
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#ifndef TWO_FILESYSTEM_H
#define TWO_FILESYSTEM_H

#include <cstdint>
#include <string>

#include "physfs/physfs.h"

namespace two {

enum class FileMode { Invalid, Read, Write, Append };

class File {
public:

    // Indicates that all data from the file should be read.
    static const int64_t ReadAll = -1;
    static const int64_t BufferSize = 4096;

    File(const std::string &filename)
        : mode{FileMode::Invalid}, filename{filename} {}

    ~File();

    // Open the file, returns true if successful.
    bool open(FileMode mode, bool buffered = true);

    // Closes the file, returns true if successful.
    bool close();

    // Reads data from the file. If length is -1 then the file will
    // be read until EOF. Returns the number of bytes actually read.
    int64_t read(char *buffer, int64_t length = ReadAll);

    // Reads all data from a file and returns a new buffer with the
    // file contents. Returns `nullptr` if the file could not be read.
    char *read_all();

    // Writes data to the file, returns true if successful.
    bool write(const char *buffer, int64_t length);

    // Returns the size of the file in bytes.
    int64_t size();

    // Checks whether we are end-of-file.
    bool eof();

    // Returns the current position in the file.
    int64_t tell();

    // Seek to a position in the file, returns true if sucessful.
    bool seek(int64_t pos);

    // Advance the file position by a number of bytes without reading
    bool skip(int64_t size);

    // Flushes buffered data to disk, returns true if successful.
    // If the file is opened for reading or the file is unbuffered this
    // is a no-op and will report success.
    bool flush();

    bool is_open() const;
    FileMode get_mode() const;
    const std::string &get_filename() const;

private:
    // PHYSFS file handle
    PHYSFS_file *fp = nullptr;
    FileMode mode = FileMode::Invalid;
    std::string filename;
};

bool load(const std::string &developer, const std::string &application);

bool unload();

// Mounts an archive or directory to a given mountpoint.
// The path mounted will be set up as a read-only directory.
bool mount(const char *archive, const char *mountpoint, bool append = false);

// Mounts archive or directory to the default mount point ("/").
// The path mounted will be set up as a read-only directory.
bool mount(const char *archive);

// Mount a directory for reading and writing. This directory should be set
// up with the path returned from io::data_path()
// If the directory has already been mounted this function will not remount
// the directory but will set it up for writing nonetheless.
//
// Note: There can only be one write directory in the filesystem, mounting
// a write dir will override the previous write dir.
bool mount_rw(const char *directory);

// Unmounts an archive or directory from the filesystem. Unlike in Unix
// archive should be the path that was mounted using mount or mount_rw and
// not the path of where it is mounted in the filesystem (the "mountpoint").
bool unmount(const char *archive);

// Determines if a file is a directory.
bool is_directory(const char *filename);

// TODO: Use name of game and stuff it's not ideal to pass these in
// Maybe pass this in io::init(developer, application)...
// A safe path to write files to. This path will be different depending on
// the platform. On Windows this path will be in ...
//
// Note: You should assume that the path returned is the only safe place to
// write files and use with io::mount_rw.
std::string data_path(const std::string &developer,
                      const std::string &application);

// Removes a file or directory. This function will fail and return false if
// the file given is in a read-only directory.
// When removing a directory, the directory must be empty.
bool remove(const char *filename);

// Creates a directory in the write dir.
bool mkdir(const char *dir);

} // two

#endif // TWO_FILESYSTEM_H
