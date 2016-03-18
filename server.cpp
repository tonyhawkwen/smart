#include <iostream>
#include <gflags/gflags.h>
#include "logging.h"

DEFINE_string(log_path, "./conf/log.conf", "log conf");

bool gExitServer = false;

void signal_quit(int signo)
{
    //here write need catch signal
    LOG(FATAL) <<"signal " << signo <<" caught, search server will quit";
    gExitServer = true;
}

void singal_handle()
{
    signal(SIGPIPE, SIG_IGN);//ignore signal of SIGPIPE
    signal(SIGINT, signal_quit);
    signal(SIGTERM, signal_quit);
}

bool global_init()
{

    return true;
}

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    init_logging();
    singal_handle();
    if (!global_init()) {
        std::cout << "global init fail!" << std::endl;
        return -1;
    }

    return 0;
}

