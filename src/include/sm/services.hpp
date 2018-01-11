#ifndef _SERVICES_HPP
#define _SERVICES_HPP

#include <string>
#include <map>
#include <utility>
#include <vector>
#include <set>

#include <btos/process.hpp>
#include <sm/sm.h>

namespace btos_api{
namespace sm{

class ServiceInstance;

class Service{
private:
	std::string name;
	std::string path;
	std::string cleanupCmd;
	
	std::vector<bt_pid_t> refs;

public:
	Service(const std::string &n, const std::string &p, const std::string &c);
	Service() = default;

	ServiceInstance Start();

	std::string Name();
	std::string Path();
	std::string CleanupCmd();
	
	sm_ServiceInfo Info();
};

class ServiceInstance{
private:
	Process proc;
	Service service;
	std::set<bt_pid_t> refs;
	bool sticky;
	ServiceInstance(const ServiceInstance&) = delete;
public:
	ServiceInstance(Process p, Service s, bool st = false);
	ServiceInstance(ServiceInstance&&) = default;

	Process GetProcess();
	Service GetService();
	
	void AddRef(bt_pid_t pid);
	void RemoveRef(bt_pid_t pid);
	bool HasRef(bt_pid_t pid);
	size_t GetRefCount();

	void Stop();

	void SetSticky(bool s);
	bool IsSticky();
};

}
}

#endif
