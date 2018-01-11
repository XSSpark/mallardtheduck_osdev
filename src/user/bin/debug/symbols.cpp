#include "debug.hpp"
#include "symbols.hpp"
#include "commands.hpp"
#include <libelf.h>
#include <gelf.h>
#include <fcntl.h>
#include <fstream>
#include <algorithm>
#include <vector>
#include <unistd.h>
#include <cxxabi.h>
#include <cstdlib>
#include <btos/table.hpp>

#include "lrucache.hpp"

using namespace std;

cache::lru_cache<string, vector<symbol>> symbol_cache(32);

const symbol null_symbol{
	.file = "(Unknown)",
	.name = "(Unkonwn)",
	.raw_name = "(Unkonwn)",
	.short_name = "(Unkonwn)",
	.address = 0,
	.file_address = 0,
	.size = 0
};

string get_pid_filename(bt_pid_t pid){
	ifstream info("INFO:/PROCS");
	table tbl = parsecsv(info);
	for(auto row : tbl.rows){
		if(atoi(row["PID"].c_str()) == (int)pid){
			return row["path"];
		}
	}
	return "";
}

void test_symbols(string filename) {
	if ( elf_version ( EV_CURRENT ) == EV_NONE )
		printf (" ELF library initialization failed : %s " , elf_errmsg ( -1));

	int fd = open(filename.c_str(), O_RDONLY, 0);
	Elf *e;
	if (( e = elf_begin ( fd , ELF_C_READ, NULL )) == NULL )
		printf (" elf_begin () failed : %s . " , elf_errmsg ( -1));

	Elf_Kind ek = elf_kind(e);

	const char *k;
	switch (ek) {
		case ELF_K_AR:
			k = " ar (1) archive ";
			break;
		case ELF_K_ELF:
			k = " elf object ";
			break;
		case ELF_K_NONE:
			k = " data ";
			break;
		default:
			k = " unrecognized ";
	}
	cout << "Elf_Kind: " << k << endl;

	Elf_Scn *scn = NULL;
	GElf_Shdr shdr;
	while ((scn = elf_nextscn(e, scn)) != NULL) {
		gelf_getshdr(scn, &shdr);
		if (shdr.sh_type == SHT_SYMTAB) {
			/* found a symbol table, go print it. */
			break;
		}
	}
	Elf_Data *data = elf_getdata(scn, NULL);
	size_t count = shdr.sh_size / shdr.sh_entsize;
	GElf_Sym sym;
	for(size_t ndx = 0; ndx < count; ++ndx) {
		gelf_getsym(data, ndx, &sym);
		cout << sym.st_value << ":" << elf_strptr(e, shdr.sh_link, sym.st_name) << endl;
	}
}

static string demangle(const string &name){
	if(name.find("_Z") == 0){
		int status = -1;
		char *realname = abi::__cxa_demangle(name.c_str(), NULL, NULL, &status);
		if(status == 0) {
			string ret = realname;
			free(realname);
			return ret;
		}
	}
	return name;
}

static string short_name(const string &name){
	if(name.find("(") != string::npos && name.find(")") != string::npos){
		return name.substr(0, name.find("("));
	}
	return name;
}

void load_symbols(intptr_t base, const string &file, vector<symbol> &vec){
	if(!symbol_cache.exists(file)){
		vector<symbol> symbols;
		if (elf_version(EV_CURRENT) == EV_NONE) throw string("Init fail.");
		int fd = open(file.c_str(), O_RDONLY, 0);
		Elf *e = elf_begin(fd, ELF_C_READ, NULL);
		if(!e) throw string("Symbol load fail.");
		Elf_Scn *scn = NULL;
		GElf_Shdr shdr;
		while((scn = elf_nextscn(e, scn))){
			gelf_getshdr(scn, &shdr);
			if(shdr.sh_type == SHT_SYMTAB){
				Elf_Data *data = elf_getdata(scn, NULL);
				size_t count = shdr.sh_size / shdr.sh_entsize;
				GElf_Sym sym;
				for(size_t ndx = 0; ndx < count; ++ndx){
					symbol s;
					gelf_getsym(data, ndx, &sym);
					s.raw_name = elf_strptr(e, shdr.sh_link, sym.st_name);
					if(s.raw_name == "") continue;
					s.name = demangle(s.raw_name);
					s.short_name = short_name(s.name);
					s.address = 0;
					s.file_address = sym.st_value;
					s.file = file;
					s.size = sym.st_size;
					symbols.push_back(s);
				}
			}
		}
		elf_end(e);
		close(fd);
		symbol_cache.put(file, symbols);
	}
	vector<symbol> symbols = symbol_cache.get(file);
	for(symbol s : symbols){
		s.address = s.file_address + base;
		vec.push_back(s);
	}
}

vector<module> get_modules(bt_pid_t pid){
	vector<symbol> symbols;
	load_symbols(0, "INIT:/ELOADER.ELX", symbols);
	symbol el_module_count_sym = get_symbol_by_name(symbols, "loaded_module_count");
	symbol el_modules_sym = get_symbol_by_name(symbols, "loaded_modules");
	size_t el_module_count = 0;
	intptr_t el_modules = 0;
	if(el_module_count_sym.address && el_modules_sym.address){
		debug_peek(&el_module_count, pid, el_module_count_sym.address, sizeof(size_t));
		debug_peek(&el_modules, pid, el_modules_sym.address, sizeof(intptr_t));
	}
	
	vector<module> ret;
	for(size_t i = 0; i < el_module_count; ++i){
		el_loaded_module elmod;
		debug_peek(&elmod, pid, el_modules + (sizeof(el_loaded_module) * i), sizeof(el_loaded_module));
		module mod;
		char buffer[BT_MAX_PATH + 1] = {0};
		debug_peek_string(buffer, pid, elmod.name, BT_MAX_PATH);
		mod.name = buffer;
		debug_peek_string(buffer, pid, elmod.full_path, BT_MAX_PATH);
		mod.path = buffer;
		mod.base = elmod.base;
		mod.limit = elmod.limit;
		ret.push_back(mod);
	}
	return ret;
}

vector<symbol> get_symbols(bt_pid_t pid){
	vector<module> modules = get_modules(pid);
	vector<symbol> ret;
	for(auto m: modules){
		load_symbols(m.base, m.path, ret);
	}
	return ret;
}

static symbol fake_symbol(const string &name){
	if(!name.length() || name[0] != '*') return null_symbol;
	string addrpart = name.substr(1);
	string sizepart;
	if(name.find(',') != string::npos) sizepart = name.substr(name.find(',') + 1);
	intptr_t addr = strtoul(addrpart.c_str(), NULL, 0);
	size_t size = strtoul(sizepart.c_str(), NULL, 0);
	symbol ret;
	ret.file = "(None)";
	ret.name = name;
	ret.raw_name = name;
	ret.short_name = name;
	ret.address = addr;
	ret.file_address = addr;
	ret.size = size;
	return ret;
}

static symbol stack_symbol(const string &name){
	if(!selected_thread) return null_symbol;
	size_t id = strtoul(name.substr(1).c_str(), NULL, 0);
	context ctx = get_context(selected_thread);
	vector<symbol> stack = stack_symbols(selected_pid, ctx);
	if(stack.size() > id) return stack[id];
	else return null_symbol;
}

static bool symbol_name_match(const symbol &sym, string name){
	return (name == sym.name || name == sym.raw_name || name == sym.short_name);
}

symbol get_symbol_by_name(const vector<symbol> &symbols, string name){
	if(name.length() && name[0] == '*') return fake_symbol(name);
	if(name.length() && name[0] == '#') return stack_symbol(name);
	symbol ret = null_symbol;
	for(auto sym : symbols){
		if(symbol_name_match(sym, name)){
			ret = sym;
			return ret;
		}
	}
	cout << "Could not find symbol: " << name << endl;
	return ret;
}

symbol get_symbol(const vector<symbol> &symbols, intptr_t addr){
	symbol ret = null_symbol;
	for(auto sym : symbols){
		if(ret.address < sym.address && sym.address <= addr){
			ret = sym;
		}
	}
	return ret;
}

vector<symbol> get_symbols_by_name(const vector<symbol> &symbols, string name){
	vector<symbol> ret;
	if(name.length() && name[0] == '*'){
		ret.push_back(fake_symbol(name));
	}else if(name.length() && name[0] == '#'){
		ret.push_back(stack_symbol(name));
	}else{
		for(auto sym : symbols){
			if(symbol_name_match(sym, name)){
				ret.push_back(sym);
			}
		}
	}
	return ret;
}
