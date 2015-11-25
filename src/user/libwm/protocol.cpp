#include <wm/libwm.h>
#include <btos_stubs.h>
#include <crt_support.h>
#include "libwm_internal.hpp"
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <cstring>

using namespace std;

static bt_pid_t wm_pid = 0;

static bool Init(){
	if(!wm_pid){
		string pid_str = get_env("WM_PID");
		wm_pid = strtoull(pid_str.c_str(), NULL, 10);
		if(!wm_pid){
			cout << "ERROR: Could not communicate with WM." << endl;
			return false;
		}
		return true;
	}
	return true;
}

template<typename T> static T GetContent(const bt_msg_header &msg){
	T ret;
	bt_msg_content((bt_msg_header*)&msg, (void*)&ret, sizeof(ret));
	bt_msg_ack((bt_msg_header*)&msg);
	return ret;
}

static bt_msg_header SendMessage(wm_RequestType::Enum type, size_t size, void* content, bool waitreply){
	if(!wm_pid  && !Init()) return bt_msg_header();
	bt_msg_header msg;
	msg.flags = 0;
	msg.to = wm_pid;
	msg.type = type;
	msg.length = size;
	msg.content = content;
	msg.id = bt_send(msg);
	if(waitreply) {
		bt_msg_header ret;
		bt_msg_filter filter;
		filter.flags = bt_msg_filter_flags::Reply;
		filter.reply_to = msg.id;
		ret = bt_recv_filtered(filter);
		while(ret.reply_id != msg.id){
			stringstream ss;
			ss << "LIBWM: Spurious message!" << endl;
			ss << "Message id: " << ret.id << endl;
			ss << "From : " << ret.from << endl;
			ss << "Flags : " << ret.flags << endl;
			ss << "Reply ID: " << ret.reply_id << " (Waiting for: " << msg.id << ")" << endl;
			bt_zero(ss.str().c_str());
			bt_next_msg_filtered(&ret, filter);
		}
		return ret;
	}
	else return bt_msg_header();
}

template<typename T> static bt_msg_header SendMessage(wm_RequestType::Enum type, const T& content, bool waitreply){
	return SendMessage(type, sizeof(T), (void*)&content, waitreply);
}

extern "C" uint64_t WM_SelectWindow(uint64_t id){
	bt_msg_header reply = SendMessage(wm_RequestType::SelectWindow, id, true);
	return GetContent<uint64_t>(reply);
}

extern "C" uint64_t WM_CreateWindow(wm_WindowInfo info){
	bt_msg_header reply = SendMessage(wm_RequestType::CreateWindow, info, true);
	return GetContent<uint64_t>(reply);
}

extern "C" void WM_DestroyWindow(){
	SendMessage(wm_RequestType::DestroyWindow, 0, NULL, false);
}

extern "C" wm_WindowInfo WM_WindowInfo(){
	bt_msg_header reply = SendMessage(wm_RequestType::CreateWindow, 0, NULL, true);
	return GetContent<wm_WindowInfo>(reply);
}

extern "C" void WM_Subscribe(uint32_t events){
	wm_WindowInfo info;
	info.subscriptions = events;
	SendMessage(wm_RequestType::MoveWindow, info, false);
}

extern "C" void WM_Update(){
	SendMessage(wm_RequestType::Update, 0, NULL, false);
}

extern "C" void WM_ReplaceSurface(uint64_t gds_id){
	SendMessage(wm_RequestType::ReplaceSurface, gds_id, false);
}

extern "C" void WM_MoveWindow(int32_t x, int32_t y){
	wm_WindowInfo info;
	info.x = x; info.y = y;
	SendMessage(wm_RequestType::MoveWindow, info, false);
}

extern "C" void WM_ChangeOptions(uint32_t opts){
	wm_WindowInfo info;
	info.options = opts;
	SendMessage(wm_RequestType::MoveWindow, info, false);
}

extern "C" void WM_SetTitle(const char *title){
	wm_WindowInfo info;
	strncpy(info.title, title, WM_TITLE_MAX);
	SendMessage(wm_RequestType::MoveWindow, info, false);
}

void WM_SetTitle(const std::string title){
	WM_SetTitle(title.c_str());
}

extern "C" wm_Event WM_GetEvent(){
	bt_msg_filter filter;
	filter.flags = (bt_msg_filter_flags::Enum)(bt_msg_filter_flags::From | bt_msg_filter_flags::Type);
	filter.pid = wm_pid;
	filter.type = wm_MessageType::Event;
	bt_msg_header msg = bt_recv_filtered(filter);
	wm_Event ret;
	ret = GetContent<wm_Event>(msg);
	bt_msg_ack(&msg);
	return ret;
}