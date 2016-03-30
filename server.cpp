#include <iostream>
#include <gflags/gflags.h>
#include "logging.h"
#include "tcp_service.h"
#include "service_pool.h"

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
    //ServicePool::get_instance().add_service(xxx,yyy);
    smart::ServicePool::get_instance().run();
    return true;
}

void destroy()
{
    smart::ServicePool::get_instance().stop();
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
    service.run();

    while (!gExitServer) {
        sleep(1);
    }

    destroy();
    service.stop();

    return 0;
}

