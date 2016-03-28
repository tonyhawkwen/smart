#include <iostream>
#include <gflags/gflags.h>
#include "logging.h"
#include "tcp_service.h"

DEFINE_string(log_path, "./conf/log.conf", "log conf");

INITIALIZE_EASYLOGGINGPP

bool gExitServer = false;

void signal_quit(int signo)
{
    //here write need catch signal
    SLOG(FATAL) <<"signal " << signo <<" caught, search server will quit";
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

void init_logging() {
    el::Loggers::configureFromGlobal(FLAGS_log_path);
    el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);
    el::Loggers::addFlag(el::LoggingFlag::MultiLoggerSupport);
}

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    init_logging();
    singal_handle();
    if (!global_init()) {
        std::cout << "global init fail!" << std::endl;
        return -1;
    }

    smart::TcpService service;
    SLOG(INFO) << std::this_thread::get_id();
    service.run();

    while (!gExitServer) {
        sleep(1);
    }

    SLOG(INFO) << std::this_thread::get_id();
    service.stop();

    return 0;
}

