#include "load_elf.hpp"

#define hasflag(x, y) (((x) & (y)) == (y))

aligned_memory al_alloc(size_t bytes, size_t align){
	aligned_memory ret;
	void *ptr=malloc(bytes+align);
	ret.alloc=ptr;
	if(align!=0){
		uint32_t pre=((uint32_t)ptr/align)*align;
		ret.aligned=(void*)(pre+align);
	}else{
		ret.aligned=ptr;
	}
	return ret;
}

void al_free(aligned_memory al){
	free(al.alloc);
}

Elf32_Ehdr elf_read_header(file_handle &file){
	Elf32_Ehdr ret;
	fs_seek(file, 0, false);
	fs_read(file, sizeof(ret), (char*)&ret);
	return ret;
}

bool elf_verify_header(const Elf32_Ehdr &header){
	char expected[]={0x7f, 'E', 'L', 'F'};
	for(size_t i=0; i<sizeof(expected); ++i){
		if(header.ident[i]!=expected[i]){
			return false;
		}
	}
	if(header.ident[EI_CLASS] != ELFCLASS32) {
		panic("(ELF) Unsupported ELF File Class.\n");
		return false;
	}
	if(header.ident[EI_DATA] != ELFDATA2LSB) {
		panic("(ELF) Unsupported ELF File byte order.\n");
		return false;
	}
	if(header.machine != EM_386) {
		panic("(ELF) Unsupported ELF File target.\n");
		return false;
	}
	if(header.ident[EI_VERSION] != EV_CURRENT) {
		panic("(ELF) Unsupported ELF File version.\n");
		return false;
	}
	if(header.type != ET_REL && header.type != ET_EXEC) {
		panic("(ELF) Unsupported ELF File type.\n");
		return false;
	}
	return true;
}

Elf32_Shdr elf_read_sectionheader(file_handle &file, const Elf32_Ehdr &header, size_t index){
	size_t offset=header.shoff+(index*sizeof(Elf32_Shdr));
	Elf32_Shdr ret;
	fs_seek(file, offset, false);
	fs_read(file, sizeof(ret), (char*)&ret);
	return ret;
}

Elf32_Phdr elf_read_progheader(file_handle &file, const Elf32_Ehdr &header, size_t index){
	size_t offset=header.phoff+(index*sizeof(Elf32_Phdr));
	Elf32_Phdr ret;
	fs_seek(file, offset, false);
	fs_read(file, sizeof(ret), (char*)&ret);
	return ret;
}

size_t elf_get_stringoffset(file_handle &file, const Elf32_Ehdr &header){
	if(header.shstrndx == SHN_UNDEF) return 0;
	return elf_read_sectionheader(file, header, header.shstrndx).offset;
}

Elf32_Rel elf_read_rel(file_handle &file, const Elf32_Shdr &section, size_t index){
	Elf32_Rel ret;
	fs_seek(file, section.offset+(sizeof(Elf32_Rel)*index), false);
	fs_read(file, sizeof(ret), (char*)&ret);
	return ret;
}

bool elf_getstring(file_handle &file, const Elf32_Ehdr &header, size_t offset, char *buf, size_t bufsize){
	size_t strpos=elf_get_stringoffset(file, header);
	if(strpos){
		size_t readpos=strpos + offset;
		fs_seek(file, readpos, false);
		fs_read(file, bufsize, buf);
		return true;
	}else{
		return false;
	}
}

void elf_test(){
	file_handle file=fs_open("INITFS:TEST.SYS");
	Elf32_Ehdr header=elf_read_header(file);
	if(elf_verify_header(header)){
		dbgpf("ELF: Valid header.\n");
	}else{
		panic("(ELF) Invalid header!\n");
	}
	dbgpf("ELF: Signature:%x, %c%c%c\n", 
		(int)header.ident[0], header.ident[1], header.ident[2], header.ident[3]);
	dbgpf("ELF: Type:%i, Machine:%i\n", (int)header.type, (int)header.machine);
	dbgpf("ELF: Number of sections: %i\n", header.shnum);
	dbgpf("ELF: Numer of program headers: %i\n", header.phnum);
	for(int i=0; i<header.shnum; ++i){
		char buf[64]={0};
		Elf32_Shdr section=elf_read_sectionheader(file, header, i);
		elf_getstring(file, header, section.name, buf, 63);
		dbgpf("ELF: Section: %s type:%x, flags:%x, offset:%x, size:%i\n", buf,
			section.type, section.flags, section.offset, section.size);
		dbgpf("ELF: Writable: %i, Load: %i\n", 
			(int)hasflag(section.flags, SHF_WRITE), (int)hasflag(section.flags, SHF_ALLOC));
	}
	for(int i=0; i<header.phnum; ++i){
		Elf32_Phdr prog=elf_read_progheader(file, header, i);
		dbgpf("ELF: Prog: %i type: %i, offset: %i, vaddr: %x, paddr: %x, filesz: %i, memsz: %i, flags: %i, align: %i\n", i, prog.type, prog.offset, prog.vaddr, prog.paddr, prog.filesz, prog.memsz, prog.flags, prog.align);
	}
	dbgpf("ELF: Image RAM size: %i\n", elf_getsize(file));
	loaded_elf_module tstelf=elf_load_module(file);
	dbgpf("ELF: test %i\n", tstelf.entry(&MODULE_SYSCALL_TABLE));
	fs_close(file);
}

size_t elf_getsize(file_handle &file){
	size_t limit=0;
	size_t base=0xFFFFFF;
	Elf32_Ehdr header=elf_read_header(file);
	for(int i=0; i<header.phnum; ++i){
		Elf32_Phdr prog=elf_read_progheader(file, header, i);
		if(prog.vaddr+prog.memsz>limit) limit=(prog.vaddr+prog.memsz);
		if(prog.vaddr<base) base=prog.vaddr;
	}
	return limit-base;
}

size_t elf_getbase(file_handle &file){
	size_t base=0xFFFFFF;
	Elf32_Ehdr header=elf_read_header(file);
	for(int i=0; i<header.phnum; ++i){
		Elf32_Phdr prog=elf_read_progheader(file, header, i);
		if(prog.vaddr<base) base=prog.vaddr;
	}
	return base;
}

Elf32_Addr elf_symbol_value(file_handle &file, const Elf32_Ehdr &header, size_t symbol){
	for(size_t i=0; i<header.shnum; ++i){
		Elf32_Shdr section=elf_read_sectionheader(file, header, i);
		if(section.type==SHT_SYMTAB){
			size_t offset=section.offset+(symbol * sizeof(Elf32_Sym));
			Elf32_Sym symbolentry;
			fs_seek(file, offset, false);
			fs_read(file, sizeof(symbolentry), (char*)&symbolentry);
			return symbolentry.value;
		}
	}
	return 0;
}

void elf_do_reloc_module(file_handle &file, const Elf32_Ehdr &header, Elf32_Shdr &section, void *base){
	size_t n_relocs=section.size/sizeof(Elf32_Rel);
	for(size_t i=0; i<n_relocs; ++i){
		Elf32_Rel rel=elf_read_rel(file, section, i);
		Elf32_Addr symval=elf_symbol_value(file, header, ELF32_R_SYM(rel.info));
		if(symval) symval+=(uint32_t)base;
		/*dbgpf("ELF: Reloc: offset: %x, info: %x, symbol: %i (%x), type: %i\n",
			rel.offset, rel.info, ELF32_R_SYM(rel.info), symval, ELF32_R_TYPE(rel.info));*/
		uint32_t *ref=(uint32_t*)((char*)base+rel.offset);
		uint32_t newval=-1;
		switch(ELF32_R_TYPE(rel.info)){
			case R_386_NONE:
				break;
			case R_386_32:
				newval=*ref+(size_t)base;
				//dbgpf("ELF: Value %x (originally %x) at %x\n", newval, *ref, ref);
                *ref=newval;
				break;
			case R_386_PC32:
				newval=*ref;
				//dbgpf("ELF: Value %x (originally %x) at %x\n", newval, *ref, ref);
				*ref=newval;
				break;
		}
	}
}

loaded_elf_module elf_load_module(file_handle &file){
	loaded_elf_module ret;
	Elf32_Ehdr header=elf_read_header(file);
	size_t ramsize=elf_getsize(file);
	ret.mem=al_alloc(ramsize, 4096); //TODO: Proper alignment calculation
	ret.entry=(module_entry)((char*)ret.mem.aligned+header.entry);
	memset(ret.mem.aligned, 0, ramsize);
	for(int i=0; i<header.phnum; ++i){
		Elf32_Phdr prog=elf_read_progheader(file, header, i);
		if(prog.type==PT_LOAD){
			fs_seek(file, prog.offset, false);
			fs_read(file, prog.filesz, (char*)ret.mem.aligned+prog.vaddr);
		}
	}
	for(int i=0; i<header.shnum; ++i){
		Elf32_Shdr section=elf_read_sectionheader(file, header, i);
		if(section.type==SHT_REL){
			char buf[128];
			elf_getstring(file, header, section.name, buf, 128);
			string name(buf);
			if(name==".rel.text" || name==".rel.data" || name==".rel.rodata"){
				elf_do_reloc_module(file, header, section, ret.mem.aligned);
			}
		}
	}
	return ret;
}

loaded_elf_proc elf_load_proc(pid_t pid, file_handle &file){
	loaded_elf_proc ret;
	pid_t oldpid=proc_current_pid;
	proc_switch(pid);
	Elf32_Ehdr header=elf_read_header(file);
	size_t ramsize=elf_getsize(file);
	size_t pages=ramsize/VMM_PAGE_SIZE;
	size_t baseaddr=elf_getbase(file);
	ret.mem=vmm_alloc_at(pages, baseaddr);
	for(int i=0; i<header.phnum; ++i){
		Elf32_Phdr prog=elf_read_progheader(file, header, i);
		if(prog.type==PT_LOAD){
			fs_seek(file, prog.offset, false);
			if(prog.vaddr < VMM_KERNELSPACE_END) panic("ELF: Attempt to load process into kernel space!");
			fs_read(file, prog.filesz, (char*)prog.vaddr);
		}
	}
	ret.entry=(proc_entry)(header.entry);
	proc_switch(oldpid);
	return ret;
}