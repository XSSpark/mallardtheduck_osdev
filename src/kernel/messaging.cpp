#include "kernel.hpp"
#include "utils/ministl.hpp"
#include "locks.hpp"

using namespace btos_api;

static uint64_t id_counter=0;
ILock *msg_lock;

static bool msg_get(uint64_t id, bt_msg_header &msg, bt_pid_t pid = CurrentProcess().ID());
static void msg_send_receipt(const bt_msg_header &omsg);

typedef map<bt_pid_t, vector<bt_kernel_messages::Enum> > msg_subscribers_list;
static msg_subscribers_list *msg_subscribers;

static const size_t pool_min = 16;
static const size_t pool_max = pool_min * 2;
//static vector<void*> buffer_pool;
static vector<bt_msg_header*> *header_pool;

static bt_msg_header *GetMessageFromProcess(bt_pid_t pid, uint64_t id){
	auto proc = GetProcess(pid);
	if(proc) return proc->GetMessageByID(id);
	else return nullptr;
}

static bt_msg_header *copy_msg_header(const bt_msg_header &h){
	if(!header_pool->size()){
		for(size_t i = 0; i < pool_min; ++i){
			header_pool->push_back((bt_msg_header*)malloc(sizeof(bt_msg_header)));
		}
	}
	
	size_t backindex = header_pool->size() - 1;
	bt_msg_header *ret = (*header_pool)[backindex];
	header_pool->erase(backindex);
	*ret = h;
	return ret;
}

static void release_msg_header(bt_msg_header *h){
	if(header_pool->size() >= pool_max){
		free(h);
		for(size_t i = header_pool->size() - 1; i > pool_min; --i){
			free((*header_pool)[i]);
			header_pool->erase(i);
		}
		return;
	}
	header_pool->push_back(h);
}

void msg_init(){
	dbgout("MSG: Init messaging...\n");
	msg_lock = NewLock();
	msg_subscribers=new msg_subscribers_list();
	
	//buffer_pool	= new vector<void*>();
	header_pool = new vector<bt_msg_header*>();
	
	for(size_t i = 0; i < pool_min; ++i){
		//buffer_pool.push_back(malloc(BT_MSG_MAX));
		header_pool->push_back((bt_msg_header*)malloc(sizeof(bt_msg_header)));
	}
}

uint64_t msg_send(bt_msg_header &msg){
	if(msg.to != 0 && GetProcessManager().GetProcessStatusByID(msg.to) == btos_api::bt_proc_status::DoesNotExist){
		dbgpf("MSG: Attempted to send message to non-existent process!\n");
		return 0;
	 }
	if(msg.flags & bt_msg_flags::Reply){
		bt_msg_header prev;
		if(!msg_get(msg.reply_id, prev)) {
			dbgout("MSG: Attempted reply to non-existent message!\n");
			return 0;
		}
		if(prev.to != msg.from || prev.from != msg.to) {
			dbgout("MSG: Reply to/from mismatch!\n");
			dbgpf("Expected from: %i to: %i, got from: %i to: %i\n", (int)prev.to, (int)prev.from, (int)msg.from, (int)msg.to);
			return 0;
		}
		if(prev.replied){
			return 0;
		}
		{
			auto hl = msg_lock->LockRecursive();
			bt_msg_header *h = CurrentProcess().GetCurrentMessage();
			if(h && h->id == prev.id) h->replied = true;
			else{
				bt_msg_header *omsg = GetMessageFromProcess(msg.from, prev.id);
				if (omsg) {
					omsg->replied = true;
				}
			}
		}
	}
	{
		auto hl = msg_lock->LockRecursive();
		msg.id = ++id_counter;
		msg.valid = true;
		msg.recieved = msg.replied = false;
		bt_msg_header *ptr = copy_msg_header(msg);
		CurrentProcess().AddMessage(ptr);
	}
	//dbgpf("MSG: Sent message ID %i from PID %i to PID %i.\n", (int)msg.id, (int)msg.from, (int)msg.to);
	//sch_yield_to(msg.to);
	CurrentThread().Yield();
	return msg.id;
}

bool msg_recv(bt_msg_header &msg, bt_pid_t pid){
	auto hl = msg_lock->LockExclusive();
	for(size_t i = 0; bt_msg_header *cmsg = GetMessageFromProcess(pid, i); ++i){
		msg=*cmsg;
		cmsg->recieved = true;
		CurrentProcess().SetCurrentMessage(cmsg);
		CurrentThread().SetMessagingStatus(thread_msg_status::Processing);
		return true;
	}
	return false;
}

static bool msg_recv_nolock(bt_msg_header &msg, bt_pid_t pid){
	if(msg_lock->IsLocked()) return false;
	//TODO: Non-locking code?
	for(size_t i = 0; bt_msg_header *cmsg = GetMessageFromProcess(pid, i); ++i){
		CurrentProcess().SetCurrentMessage(cmsg);
		msg=*cmsg;
		cmsg->recieved = true;
		return true;
	}
	return false;
}

bt_msg_header msg_recv_block(bt_pid_t pid){
	bt_msg_header ret;
	auto &currentThread = CurrentThread();
	if(!msg_recv(ret, pid)){
		currentThread.SetMessagingStatus(thread_msg_status::Waiting);
		currentThread.SetBlock([&]{
			return msg_recv_nolock(ret, pid);
		});
	}
	currentThread.SetMessagingStatus(thread_msg_status::Processing);
	return ret;
}

static bool msg_get(uint64_t id, bt_msg_header &msg, bt_pid_t pid){
	auto hl = msg_lock->LockRecursive();
	bt_msg_header *h = CurrentProcess().GetCurrentMessage();
	if(h && h->id == id){
		msg = *h;
		return true;
	}else{
		auto msg_ptr = GetMessageFromProcess(pid, id);
		if(msg_ptr){
			msg = *msg_ptr;
			return true;	
		}
		else return false;
	}
}

size_t msg_getcontent(bt_msg_header &msg, void *buffer, size_t buffersize){
	bt_msg_header r;
	if(!msg_get(msg.id, r, msg.to)){
		//panic("(MSG) Content request for non-existent message!");
		return 0;
	}
	size_t size = buffersize > r.length ? r.length : buffersize;
	//dbgpf("MSG: Copying %i bytes of content (out of %i total, %i requested) for message %i.\n", (int)size, (int)r.length, (int)buffersize, (int)r.id);
	if(size) memcpy(buffer, r.content, size);
	return size;
}

void msg_acknowledge(bt_msg_header &msg, bool set_status){
	void *found = NULL;
	{
		auto hl = msg_lock->LockRecursive();
		bt_msg_header *h = CurrentProcess().GetCurrentMessage();
		if(h && h->id == msg.id){
			found = h->content;
			CurrentProcess().RemoveMessage(h);
			release_msg_header(h);
			CurrentProcess().SetCurrentMessage(nullptr);
		}else{
			auto msg_ptr = GetMessageFromProcess(msg.to, msg.id);
			if(msg_ptr){
				bt_msg_header &header = *msg_ptr;
				msg = header;
				found = header.content;
				CurrentProcess().RemoveMessage(msg_ptr);
				release_msg_header(msg_ptr);
			}
		}
	}
	if(found){
		if(!(msg.flags & bt_msg_flags::Reply) && (msg.flags & bt_msg_flags::UserSpace)){
			//TODO: Fix???
			//proc_free_message_buffer(msg.to, msg.from);
		}else{
			free(found);
		}
		if(set_status) CurrentThread().SetMessagingStatus(thread_msg_status::Normal);
		msg_send_receipt(msg);
	}
	//else dbgpf("MSG: Attempt to acknowlegde non-existent message %i\n", (int)msg.id);
}

void msg_nextmessage(bt_msg_header &msg){
	msg_acknowledge(msg, false);
	msg=msg_recv_block();
}

void msg_clear(bt_pid_t pid){
	auto hl = msg_lock->LockExclusive();
	for(size_t i = 0; bt_msg_header *cmsg = GetMessageFromProcess(pid, i); ++i){
		bt_msg_header &header=*cmsg;
		//if(header.to==pid || (header.from==pid && !header.recieved)){
			msg_acknowledge(header, false);
		//}
	}
	if(msg_subscribers->has_key(pid)) msg_subscribers->erase(pid);
}

void msg_subscribe(bt_kernel_messages::Enum message, bt_pid_t pid){
	auto hl = msg_lock->LockExclusive();
	if(!msg_subscribers->has_key(pid)){
		(*msg_subscribers)[pid]=vector<bt_kernel_messages::Enum>();
	}
	if((*msg_subscribers)[pid].find(message) == (*msg_subscribers)[pid].npos){
		(*msg_subscribers)[pid].push_back(message);
		dbgpf("MSG: Pid %i subscribed to %i.\n", (int)pid, (int)message);
	}
}

void msg_unsubscribe(bt_kernel_messages::Enum message, bt_pid_t pid){
	auto hl = msg_lock->LockExclusive();
	if(!msg_subscribers->has_key(pid)) return;
	if((*msg_subscribers)[pid].find(message) != (*msg_subscribers)[pid].npos){
		(*msg_subscribers)[pid].erase((*msg_subscribers)[pid].find(message));
	}
}

void msg_send_event(bt_kernel_messages::Enum message, void *content, size_t size){
	auto hl = msg_lock->LockExclusive();
	for(msg_subscribers_list::iterator i=msg_subscribers->begin(); i!=msg_subscribers->end(); ++i){
		if(i->second.find(message)!=i->second.npos){
			bt_msg_header msg;
			msg.type=message;
			msg.to=i->first;
			msg.content=malloc(size);
			msg.length = size;
			msg.source=0;
			msg.from=0;
			msg.flags=0;
			msg.critical=false;
			memcpy(msg.content, content, size);
			msg_send(msg);
		}
	}
}

static void msg_send_receipt(const bt_msg_header &omsg){
	auto hl = msg_lock->LockExclusive();
	for(msg_subscribers_list::iterator i=msg_subscribers->begin(); i!=msg_subscribers->end(); ++i){
		if(i->first == omsg.from && i->second.find(bt_kernel_messages::MessageReceipt) != i->second.npos){
			bt_msg_header msg;
			msg.type=bt_kernel_messages::MessageReceipt;
			msg.to=i->first;
			msg.content=malloc(sizeof(bt_msg_header));
			msg.source=0;
			msg.from=0;
			msg.flags=0;
			msg.critical=false;
			msg.length = sizeof(bt_msg_header);
			memcpy(msg.content, &omsg, sizeof(bt_msg_header));
			((bt_msg_header*)msg.content)->content=0;
			//dbgpf("MSG: Sending reciept for %i sent to %i.\n", (int)((bt_msg_header*)msg.content)->id, (int)((bt_msg_header*)msg.content)->to);
			msg_send(msg);
		}
	}
}

bool msg_recv_reply(btos_api::bt_msg_header &msg, uint64_t msg_id){
	auto hl = msg_lock->LockExclusive();
	for(size_t i = 0; bt_msg_header *cmsg = CurrentProcess().GetMessageByID(i); ++i){
		if(cmsg->to==0 && (cmsg->flags & btos_api::bt_msg_flags::Reply) && cmsg->reply_id == msg_id){
			CurrentProcess().SetCurrentMessage(cmsg);
			msg=*cmsg;
			cmsg->recieved = true;
			CurrentThread().SetMessagingStatus(thread_msg_status::Processing);
			return true;
		}
	}
	return false;
}

static bool msg_recv_reply_nolock(bt_msg_header &msg, uint64_t msg_id){
	if(msg_lock->IsLocked()) return false;
	//TODO: Non-locking code???
	for(size_t i = 0; bt_msg_header *cmsg = CurrentProcess().GetMessageByID(i); ++i){
		if(cmsg->to==0 && (cmsg->flags & btos_api::bt_msg_flags::Reply) && cmsg->reply_id == msg_id){
			CurrentProcess().SetCurrentMessage(cmsg);
			msg=*cmsg;
			cmsg->recieved = true;
			return true;
		}
	}
	return false;
}

bt_msg_header msg_recv_reply_block(uint64_t msg_id){
	bt_msg_header ret;
	auto &currentThread = CurrentThread();
	if(!msg_recv_reply(ret, msg_id)){
		currentThread.SetMessagingStatus(thread_msg_status::Waiting);
		currentThread.SetBlock([&]{
			return msg_recv_reply_nolock(ret, msg_id);
		});
	}
	currentThread.SetMessagingStatus(thread_msg_status::Processing);
	return ret;
}

bool msg_is_match(const bt_msg_header &msg, const bt_msg_filter &filter){
	if(msg.critical) return true;
	bool ret = true;
	if((filter.flags & bt_msg_filter_flags::NonReply) && (msg.flags & bt_msg_flags::Reply)) ret = false;
	if((filter.flags & bt_msg_filter_flags::From) && msg.from != filter.pid) ret = false;
	if((filter.flags & bt_msg_filter_flags::Reply) && msg.reply_id != filter.reply_to) ret = false;
	if((filter.flags & bt_msg_filter_flags::Type) && msg.type != filter.type) ret = false;
	if((filter.flags & bt_msg_filter_flags::Source) && msg.source != filter.source) ret = false;
	if((filter.flags & bt_msg_filter_flags::Invert)) ret = !ret;
	return ret;
}

struct msg_filter_blockcheck_params{
	bt_pid_t pid;
	bt_msg_filter filter;
};

bool msg_filter_blockcheck(msg_filter_blockcheck_params &params){
	//TODO: Non-locking code!
	if(msg_lock->IsLocked()) return false;
	auto proc = GetProcess(params.pid);
	if(!proc) return false;
	bt_msg_header *cmsg = proc->GetMessageMatch(params.filter);
	if(cmsg){
		proc->SetCurrentMessage(cmsg);
		return true;
	}
	return false;
}

bt_msg_header msg_recv_filtered(bt_msg_filter filter, bt_pid_t pid, bool block){
	auto hl = msg_lock->LockExclusive();
	auto &currentThread = CurrentThread();
	auto proc = GetProcess(pid);
	if(!proc){
		bt_msg_header ret;
		ret.valid = false;
		return ret;
	}
	while(true){
		bt_msg_header *cmsg = proc->GetMessageMatch(filter);
		if(cmsg){
			bt_msg_header &msg=*cmsg;
			proc->SetCurrentMessage(cmsg);
			currentThread.SetMessagingStatus(thread_msg_status::Processing);
			return msg;
		}
		if(block){
			msg_lock->Release();
			currentThread.SetMessagingStatus(thread_msg_status::Waiting);
			msg_filter_blockcheck_params p = {pid, filter};
			currentThread.SetBlock([&]{
				return msg_filter_blockcheck(p);
			});
			msg_lock->TakeExclusive();
		}else{
			bt_msg_header ret;
			ret.valid = false;
			return ret;
		}
	}
}

void msg_nextmessage_filtered(bt_msg_filter filter, bt_msg_header &msg){
	msg_acknowledge(msg, false);
	msg=msg_recv_filtered(filter);
}

bool msg_query_recieved(uint64_t id){
	auto hl = msg_lock->LockExclusive();
	bt_msg_header *msg = CurrentProcess().GetMessageByID(id);
	if(msg){
		return false;
	}
	return true;
}

struct msg_recv_handle{
	bt_pid_t pid;
	bt_msg_filter filter;
	bt_msg_header msg;
};

static void msg_recv_handle_close(msg_recv_handle *ptr){
	delete ptr;
}

static bool msg_recv_handle_wait(msg_recv_handle *h){
	//TODO: Non-locking code!
	if(h->msg.valid) return true;
	if(!msg_lock->IsLocked()){
		auto proc = GetProcess(h->pid);
		if(!proc) return false;
		bt_msg_header *cmsg = proc->GetMessageMatch(h->filter);
		if(cmsg){
			h->msg = *cmsg;
			proc->SetCurrentMessage(cmsg);
			return true;
		}
	}
	return false;
}

IHandle *msg_create_recv_handle(bt_msg_filter filter){
	auto value = new msg_recv_handle();
	value->pid = CurrentProcess().ID();
	value->filter = filter;
	value->msg.valid = false;
	return MakeKernelGenericHandle<KernelHandles::MessageRecive>(value, &msg_recv_handle_close, &msg_recv_handle_wait);
}

bt_msg_header msg_read_recv_handle(IHandle *h){
	bt_msg_header ret;
	if(auto handle = KernelHandleCast<KernelHandles::MessageRecive>(h)){
		auto v = handle->GetData();
		ret = v->msg;
		v->msg.valid = false;
	}else ret.valid = false;
	return ret;
}
