#include "kernel.hpp"
#include "mm2/mm2_internal.hpp"

void mm2_init(multiboot_info_t *mbt){
	dbgout("MM2: Init.\n");
	MM2::mm2_physical_init(mbt);
	MM2::mm2_virtual_init();
	MM2::physical_infofs_register();
	MM2::init_mmap();
	MM2::init_shm();
}

void *mm2_virtual_alloc(size_t pages, uint32_t mode){
	return MM2::mm2_virtual_alloc(pages, mode);
}

void mm2_virtual_free(void *ptr, size_t pages){
	MM2::mm2_virtual_free(ptr, pages);
}