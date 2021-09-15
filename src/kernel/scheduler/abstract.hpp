#ifndef KERNEL_SCHEDULER_ABSTRACT_HPP
#define KERNEL_SCHEDULER_ABSTRACT_HPP

#include <module/kernelsys/scheduler.hpp>

void Scheduler_Init();
bool Scheduler_Ready();
IScheduler &GetScheduler();

inline IThread &CurrentThread(){
	return GetScheduler().CurrentThread();
}

inline ThreadPointer GetThread(uint64_t id){
	auto ptr = GetScheduler().GetByID(id);
	if(!ptr) panic("(THREAD) Thread does not exist!");
	return ptr;
}

#endif