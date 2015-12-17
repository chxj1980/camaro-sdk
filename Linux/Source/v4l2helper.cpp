#include "v4l2helper.h"

#include <sstream>
#include <thread>
#include <utility>
#include <cstring>
#include <fstream>
#include <ios>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

using namespace TopGear;
using namespace Linux;

#define CLEAR(x) memset(&(x), 0, sizeof(x))

void v4l2Helper::EnumVideoDeviceSources(std::vector<SourcePair> &sources,
                                        std::chrono::milliseconds waitTime)
{
    sources.clear();
    for(auto i=0;i<63;++i)
    {
        std::ostringstream ss;
        ss<<"/dev/video"<<i;
        auto dev = ss.str();
        auto fd = ::open(dev.c_str(), O_RDWR|O_NONBLOCK, 0);
        if(fd==-1)
            continue;
        v4l2_capability cap;
        std::memset(&cap,0,sizeof(cap));
        ioctl(fd, VIDIOC_QUERYCAP, &cap);

        if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
           !(cap.capabilities & V4L2_CAP_STREAMING))
        {
            ::close(fd);
            continue;
        }
        sources.push_back(std::make_pair(dev,fd));
    }
    if (sources.size()>0)
        std::this_thread::sleep_for(waitTime);
}

int GetDirs(std::string dir, std::vector<std::string> &list, std::string matchStr={})
{
    dirent *dirp;
    DIR *dp = opendir(dir.c_str());
    if (dp == nullptr)
    {
        //cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }

    list.clear();
    while ((dirp = readdir(dp)) != nullptr)
    {
        if (dirp->d_type != DT_DIR)
            continue;
        auto name = std::string(dirp->d_name);
        if (name=="." || name=="..")
            continue;
        if (!matchStr.empty() && name.find(matchStr)==std::string::npos)
            continue;
        list.push_back(name);
    }
    closedir(dp);
    return 0;
}

bool MatchPath(std::string path, uint32_t flags)
{
    struct stat info;
    if( stat(path.c_str(), &info) != 0 )
        return false;
    return ((info.st_mode & flags) != 0);
}

bool SearchDir(std::string dir, std::string matchStr, std::string &result)
{
    if (MatchPath(dir+"/"+matchStr, S_IFDIR))
    {
        result = dir;
        return true;
    }

    dirent *dirp;
    DIR *dp = opendir(dir.c_str());
    if (dp == nullptr)
    {
        //cout << "Error(" << errno << ") opening " << dir << endl;
        result.clear();
        return false;
    }

    while ((dirp = readdir(dp)) != nullptr)
    {
        if (dirp->d_type != DT_DIR)
            continue;
        auto name = std::string(dirp->d_name);
        if (name=="." || name=="..")
            continue;
        if (SearchDir(dir+"/"+name,matchStr,result))
        {
            closedir(dp);
            return true;
        }
    }
    closedir(dp);
    result.clear();
    return false;

}

std::string GetDescPathFromBusInfo(const std::string busInfo,
                                   const std::string productName)
{
    const std::string BUS_BASE_PATH = "/sys/devices/pci0000:00";
    const std::string DEST_FILE = "descriptors";
    const std::string PRODUCT_FILE = "product";

    if (!MatchPath(BUS_BASE_PATH, S_IFDIR))
        return {};

    //Prepare path strings
    auto pos = busInfo.find('-');
    auto rpos = busInfo.rfind('-');
    if (pos==std::string::npos || rpos==std::string::npos)
        return {};
    auto busStr = busInfo.substr(0,pos);
    auto locationStr = busInfo.substr(pos+1,rpos-pos-1);
    auto terminalStr = busInfo.substr(rpos+1);

    std::string locationPath;
    if (SearchDir(BUS_BASE_PATH,locationStr,locationPath))
    {
        locationPath += "/" +locationStr;
        std::vector<std::string> busList;
        GetDirs(locationPath,busList,busStr);
        for(auto busName : busList)
        {
            auto offset = busName.find(busStr) + busStr.length();
            auto prefix= busName.substr(offset)+"-";
            std::string end;
            size_t pos = 0;
            while((pos = terminalStr.find('.', pos))!=std::string::npos)
            {
                end += prefix + terminalStr.substr(0, pos) + "/";
                pos++;
            }
            end += prefix + terminalStr;
            auto specficPath = locationPath+"/"+busName+"/"+end;

            if (!MatchPath(specficPath,S_IFDIR))
                continue;

            auto destPath = specficPath + "/"+DEST_FILE;
            auto prodPath = specficPath + "/"+PRODUCT_FILE;
            if (MatchPath(destPath,S_IFREG) && MatchPath(prodPath,S_IFREG))
            {
                std::ifstream fp(prodPath.c_str());
                std::string fileVal;
                std::getline (fp, fileVal);
                fp.close();
                if (fileVal!=productName)
                    continue;
                return destPath;
            }
        }
    }
    return {};
}

#define CS_INTERFACE        0x24
#define VC_EXTENSION_UNIT   0x06

std::shared_ptr<ExtensionInfo> v4l2Helper::GetXUFromBusInfo(
        const v4l2_capability &cap)
{
    auto product = std::string(reinterpret_cast<const char *>(cap.card));
    auto bus = std::string(reinterpret_cast<const char *>(cap.bus_info));
    auto filePath = GetDescPathFromBusInfo(bus,product);
    if (filePath.empty())
        return {};
    std::ifstream desc(filePath.c_str(),std::ios_base::binary);
    if (!desc.is_open())
        return {};
    std::vector<char> candidate;
    char byte;
    for(auto i=0;i<3;++i)
    {
        if (!desc.get(byte))
        {
            desc.close();
            return {};
        }
        candidate.push_back(byte);
    }

    std::unique_ptr<char[]> payload;
    auto payloadSize=0;
    while(desc.get(byte))
    {
        candidate.erase(candidate.begin());
        candidate.push_back(byte);
        if (candidate[0]>24 &&
            candidate[1]==CS_INTERFACE &&
            candidate[2]==VC_EXTENSION_UNIT) //Extension Unit Identifiers
        {
            payloadSize=candidate[0]-3;
            payload= std::unique_ptr<char[]>(new char[payloadSize]);
            if (!desc.read(payload.get(),payloadSize))
                payload.reset();
            break;
        }
    }
    desc.close();
    if (payload)
        return ExtensionInfo::Parse(
                    reinterpret_cast<uint8_t *>(payload.get()),payloadSize);
    return {};
}
