#ifndef _SHELL_UTILS_HPP
#define _SHELL_UTILS_HPP

#include <memory>
#include <string>
#include <gds/surface.hpp>

namespace btos_api{
namespace gui{
namespace shell{

enum class DefaultIcons{
	Folder, FolderOpen, File, Executable, Computer, Drive, Device, Unknown
};

std::shared_ptr<gds::Surface> GetDefaultIcon(DefaultIcons icon, size_t size);
std::shared_ptr<gds::Surface> GetPathIcon(const std::string &path, size_t size);

std::string TitleCase(const std::string &text);
std::vector<std::string> SplitPath(const std::string &path);

class DirectoryEntryComparator{
public:
	enum class SortBy{
		Name, Size
	};

	DirectoryEntryComparator(SortBy order = SortBy::Name, bool directoriesFirst = true);
	bool operator()(const bt_directory_entry &a, const bt_directory_entry &b);
private:
	SortBy order;
	bool directoriesFirst;
};

}
}
}

#endif