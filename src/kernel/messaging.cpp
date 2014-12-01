#include "kernel.hpp"
#include "ministl.hpp"

using namespace btos_api;

static vector<bt_msg_header> *msg_q;
static uint64_t id_counter=0;

void msg_init(){
    dbgout("MSG: Init messaging...\n");
    msg_q=new vector<bt_msg_header>();
}

uint64_t msg_send(bt_msg_header &msg){
    msg.id=++id_counter;
    msg.valid=true;
    msg_q->push_back(msg);
    return msg.id;
}

bool msg_recv(bt_msg_header &msg, pid_t pid){
    for(size_t i=0; i<msg_q->size(); ++i){
        if((*msg_q)[i].to==pid){
            msg=(*msg_q)[i];
            return true;
        }
    }
    return false;
}

struct msg_blockcheck_params{
    bt_msg_header *msg;
    pid_t pid;
};

bool msg_blockcheck(void *p){
    msg_blockcheck_params *params=(msg_blockcheck_params*)p;
    return msg_recv(*params->msg, params->pid);
}

bt_msg_header msg_recv_block(pid_t pid){
    bt_msg_header ret;
    if(!msg_recv(ret, pid)){
        msg_blockcheck_params params={&ret, pid};
        sch_setblock(&msg_blockcheck, (void*)&params);
    }
    return ret;
}

bool msg_get(uint64_t id, bt_msg_header &msg){
    for(size_t i=0; i<msg_q->size(); ++i){
        if((*msg_q)[i].id==id){
            msg=(*msg_q)[i];
            return true;
        }
    }
    return false;
}

void msg_getcontent(bt_msg_header &msg, void *buffer, size_t buffersize){
    bt_msg_header r;
    if(!msg_get(msg.id, r)) return;
    memcpy(buffer, r.content, buffersize>r.length?r.length:buffersize);
}

void msg_acknowledge(bt_msg_header &msg){
    for(size_t i=0; i<msg_q->size(); ++i) {
        bt_msg_header &header=(*msg_q)[i];
        if(header.id==msg.id){
            if(header.flags & bt_msg_flags::UserSpace){
                proc_free_message_buffer(header.from);
            }else{
                free(header.content);
            }
            msg_q->erase(i);
            return;
        }
    }
}
