#pragma once

#include "IGenericVCDevice.h"
#include <set>
#include <string>

namespace TopGear
{
    class FileSource : public ISource
    {
    public:
        std::set<std::string> &GetList() { return fileList; }
        virtual ~FileSource() = default;
    protected:
        FileSource() = default;
        std::set<std::string> fileList;
    };

    class DirectorySource : public FileSource
    {
    public:
        DirectorySource(const std::string &path, const std::string &extension);
        virtual ~DirectorySource() = default;
    };

    class ListFileSource : public FileSource
    {
    public:
        explicit ListFileSource(const std::string &filepath);
        virtual ~ListFileSource() = default;
    };
}

