#include <iostream>
#include <cstring>
#include <cstdio>
#include "../../include/btos_stubs.h"

using namespace std;

int main(int argc, char **argv){
    if(argc==1){
        cout << "Listening on PID " << bt_getpid() << "." << endl;
        while(true){
            bt_msg_header msg=bt_recv(true);
            cout << "Message from: " << msg.from << endl;
            cout << "Length: " << msg.length << endl;
            char* data=new char[msg.length+1];
            bt_msg_content(&msg, data, msg.length);
            cout << "Data: \"" << data << "\"" << endl;
            bt_msg_ack(&msg);
            delete data;
        }
    }else if(argc==3){
        bt_msg_header msg;
        int to;
        sscanf(argv[1], "%i", &to);
        msg.to=to;
        msg.length=strlen(argv[2]);
        msg.content=argv[2];
        bt_send(msg);
        bt_msgwait();
    }
}