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

#include "filesystem.h"

#include <cstdint>

#include "physfs/physfs.h"
#include "debug.h"

namespace two {

File::~File() {
    if (is_open())
        close();
}

bool File::open(FileMode mode, bool buffered) {
    ASSERTS(PHYSFS_isInit(), "Filesystem must be initialized");

    if (is_open()) {
        // File has already been opened...
        return true;
    }
    const char *name = filename.c_str();
    fp = nullptr;

    switch (mode) {
    case FileMode::Read:
        if (!PHYSFS_exists(name)) {
            PANIC("Could not open file '%s'. No such file", name);
            return false;
        }
        fp = PHYSFS_openRead(name);
        break;
    case FileMode::Write:
        if (PHYSFS_getWriteDir() == nullptr) {
            PANIC("Missing write directory");
            return false;
        }
        fp = PHYSFS_openWrite(name);
        break;
    case FileMode::Append:
        if (PHYSFS_getWriteDir() == nullptr) {
            PANIC("Missing write directory");
            return false;
        }
        fp = PHYSFS_openAppend(name);
        break;
    default:
        // Invalid mode
        break;
    }

    if (fp == nullptr) {
        const char *err = PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
        PANIC("Could not open file '%s' (%s)", name, err);
        return false;
    }

    if (buffered && PHYSFS_setBuffer(fp, File::BufferSize) == 0) {
        const char *err = PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
        PANIC("Failed to create buffer for file '%s' (%s)", name, err);
        return false;
    }

    this->mode = mode;
    return true;
}

bool File::close() {
    if (fp == nullptr || !PHYSFS_close(fp)) {
        return false;
    }
    fp = nullptr;
    mode = FileMode::Invalid;
    return true;
}

int64_t File::read(char *buffer, int64_t length) {
    if (!is_open() || mode != FileMode::Read) {
        return -1;
    }
    int64_t total_size = size();
    if (length == ReadAll) {
        length = total_size;
    } else if (length > total_size) {
        length = total_size;
    }

    if (length < 0) {
        return -1;
    }
    return PHYSFS_readBytes(fp, buffer, length);
}

char *File::read_all() {
    int64_t len = size();
    char *buffer = new char[len];
    if (read(buffer, len) == -1) {
        delete[] buffer;
        return nullptr;
    }
    return buffer;
}

bool File::write(const char *buffer, int64_t length) {
    if (!is_open() || (mode != FileMode::Write && mode != FileMode::Append)) {
        return false;
    }
    if (length < 0) {
        return false;
    }
    return PHYSFS_writeBytes(fp, buffer, length) == length;
}

int64_t File::size() {
    if (!is_open()) {
        return -1;
    }
    return PHYSFS_fileLength(fp);
}

bool File::eof() {
    if (!is_open()) {
        return -1;
    }
    return PHYSFS_eof(fp);
}

int64_t File::tell() {
    if (!is_open()) {
        return -1;
    }
    return PHYSFS_tell(fp);
}

bool File::seek(int64_t pos) {
    if (!is_open()) {
        return false;
    }
    return PHYSFS_seek(fp, pos) != 0;
}

bool File::skip(int64_t size) {
    if (!is_open()) {
        return false;
    }
    return seek(tell() + size);
}

bool File::flush() {
    if (!is_open()) {
        return false;
    }
    return PHYSFS_flush(fp) != 0;
}

bool File::is_open() const {
    return fp != nullptr && mode != FileMode::Invalid;
}

const std::string &File::get_filename() const {
    return filename;
}

FileMode File::get_mode() const {
    return mode;
}

bool mount(const char *archive, const char *mountpoint, bool append) {
    return PHYSFS_mount(archive, mountpoint, append);
}

bool mount(const char *archive) {
    return mount(archive, nullptr, false);
}

} // two
