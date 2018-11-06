/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_WAL_ERRORS_H
#define FLYINGKV_WAL_ERRORS_H

#include <string>

using std::string;

namespace flyingkv {
namespace wal {
enum class Code {
    OK   = 0,
    Uninited,
    Unloaded,
    FileCorrupt,
    FileMameCorrupt,
    FileSystemError,
    EncodeEntryError,
    DecodeEntryError,
    EntryBytesSizeOverflow,
    InvalidEntryId
};

const char* const FileCorruptError = " file corrupt";
const char* const FileNameCorruptError = " file name corrupt";
const char* const UninitializedError    = " has not been initialized";
const char* const UnloadedError    = " has not been loaded";
const char* const EncodeEntryError = " encode entry error";
const char* const DecodeEntryError = " decode entry error";
const char* const EntryBytesSizeOverflowError = " entry encoded bytes larger than max limit size";
const char* const InvalidEntryIdError = " invalid entry id";
}
}

#endif //FLYINGKV_WAL_ERRORS_H
