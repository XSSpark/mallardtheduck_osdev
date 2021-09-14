#ifndef KERNEL_UTILS_HPP
#define KERNEL_UTILS_HP

#include "gcc_builtins.h"
#include "string.hpp"
#include "ministl.hpp"

//String
class string;

char* itoa(int num, char* str, int base=10);
string to_upper(const string &str);
bool starts_with(const string &str, const string &cmp);
vector<string> split_string(const string &str, const char c);

string itoa(int num, int base = 10);

#ifndef VSCODE //VSCode reports spruious errors here...
//C++ new/delete operators
inline void *operator new(size_t size)
{
	return malloc(size);
}
 
inline void *operator new[](size_t size)
{
	return malloc(size);
}
 
inline void operator delete(void *p)
{
	free(p);
}

inline void operator delete(void *p, size_t)
{
	free(p);
}
 
inline void operator delete[](void *p)
{
	free(p);
}

inline void operator delete[](void *p, size_t)
{
	free(p);
}

template<typename T> void *operator new (size_t, T* ptr)
{
	return ptr;
}
#endif

template<typename T> T min(T a, T b){
	return (a < b) ? a : b;
}

template<typename T>  T max(T a, T b){
	return (a > b) ? a : b;
}

template<typename T> void New(T *&var){
	var = new T();
}

template<typename T, typename ...Tp> void New(T *&var, Tp... params){
	var = new T(params...);
}

template<typename T> class StaticAlloc{
private:
	alignas(T) char buffer[sizeof(T)];
	T *ptr = nullptr;
public:
	T& operator*(){
		if(!ptr) panic("(SA) Use before allocation!");
		return *ptr;
	}

	T *operator->(){
		if(!ptr) panic("(SA) Use before allocation!");
		return ptr;
	}

	operator T*(){
		if(!ptr) panic("(SA) Use before allocation!");
		return ptr;
	}

	operator bool(){
		return ptr;
	}

	void Init(){
		ptr = new(buffer) T();
	}

	template<typename... Ts>
	void Init(Ts... params){
		ptr = new(buffer) T(params...);
	}
};

#endif