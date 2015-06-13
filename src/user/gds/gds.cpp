#include "gds.hpp"
#include "screen.hpp"
#include "drawingop.hpp"
#include <iostream>

using namespace std;

char dbgbuf[128];

void set_env(const string &name, const string &value){
	bt_setenv(name.c_str(), value.c_str(), 0);
}

int main(int argc, char **argv){
	cout << "BT/OS GDS" << endl;
	bt_pid pid = bt_getpid();
	char pid_str[64] = {0};
	snprintf(pid_str, 64, "%i", pid);
	set_env("GDS_PID", pid_str);
	if(argc > 1){
		int s_argc = 0;
		char **s_argv = NULL;
		if(argc > 2) {
			s_argc = argc - 2;
			s_argv = &argv[2];
		}
		bt_spawn(argv[1], s_argc, s_argv);
	}
	Service();
    return 0;
}