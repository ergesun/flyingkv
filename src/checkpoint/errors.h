/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_CHECKPOINT_ERRORS_H
#define FLYINGKV_CHECKPOINT_ERRORS_H

#include <string>

using std::string;

namespace flyingkv {
namespace checkpoint {
enum class Code {
    OK   = 0,
    Uninited,
    FileCorrupt,
    FileSystemError,
    EncodeEntryError,
    DecodeEntryError,
    MissingFile
};

const char* const FileCorruptError = " file corrupt";
const char* const UninitializedError    = " has not been initialized";
const char* const EncodeEntryError = " encode entry error";
const char* const DecodeEntryError = " decode entry error";
const char* const MissingFileError       = " file is missing";
}
}

#endif //FLYINGKV_CHECKPOINT_ERRORS_H
