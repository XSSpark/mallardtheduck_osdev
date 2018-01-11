#include <btos/envvars.hpp>
#include <btos.h>

#include <memory>
#include <sstream>

using namespace std;

namespace btos_api{

string GetEnv(const string &var){
	char value[128];
	string ret;
	size_t size = bt_getenv(var.c_str(), value, 128);
	ret = value;
	if(size > 128){
		auto buf = unique_ptr<char>{new char[size]};
		bt_getenv(var.c_str(), buf.get(), size);
		ret = buf.get();
	}
	if(size) return ret;
	else return "";
}

void SetEnv(const string &var, const string &val, uint32_t flags){
	bt_setenv(var.c_str(), val.c_str(), flags);
}

string Interpolate(const string &tmpl, function<string(const string&)> lookup){
	const char delim = '$';
	const char esc = '\\';
	stringstream out;
	stringstream varname;
	bool invar = false;
	bool inesc = false;

	for(char c : tmpl){
		if(invar){
			if(inesc){
				varname << c;
				inesc = false;
			}else if(c == esc){
				inesc = true;
			}else if(c != delim) varname << c;
			else{
				invar = false;
				out << lookup(varname.str());
				varname.str("");
			}
		}else{
			if(inesc){
				out << c;
				inesc = false;
			}else if(c == esc){
				inesc = true;
			}if(c != delim) out << c;
			else invar = true;
		}
	}
	return out.str();
}

string EnvInterpolate(const string &tmpl){
	return Interpolate(tmpl, &GetEnv);
}

}
