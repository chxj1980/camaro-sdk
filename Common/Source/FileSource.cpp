#include "FileSource.h"

#include <fstream>
#include <set>

#ifdef __linux__
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <dirent.h>
#endif

using namespace TopGear;

DirectorySource::DirectorySource(const std::string &path, const std::string &extension)
{
    std::set<std::string> orderedList;
#ifdef __linux__
    struct stat info;
    if( stat(path.c_str(), &info) != 0 )
        return;
    if ((info.st_mode & S_IFDIR) == 0)
        return;

    DIR *dp = opendir(path.c_str());
    if (dp == nullptr)
        return;

    std::string matchStr = "."+extension;

    dirent *dirp;
    while ((dirp = readdir(dp)) != nullptr)
    {
        if (dirp->d_type != DT_REG)
            continue;
        auto name = std::string(dirp->d_name);
        if (name=="." || name=="..")
            continue;

        auto pos = name.rfind(matchStr);
        if (pos==std::string::npos)
            continue;
        if (pos+matchStr.size()!=name.size())
            continue;

        orderedList.emplace(path+"/"+name);
    }
    closedir(dp);
#endif
    fileList = std::vector<std::string>(orderedList.begin(), orderedList.end());
}

ListFileSource::ListFileSource(const std::string &filepath)
{
    std::ifstream fp(filepath.c_str());
    std::string fileVal;
    while(fp.good())
    {
        std::getline(fp, fileVal);
        fileList.emplace_back(fileVal);
    }
    fp.close();
}
