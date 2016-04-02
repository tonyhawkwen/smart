#include <iostream>
#include <gflags/gflags.h>
#include "logging.h"
#include "tcp_service.h"
#include "service_pool.h"
#include "auth_service.h"

using namespace smart;

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

void destroy()
{
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

    AccessService access_service;
    ServicePool::get_instance().add_service(&access_service,"AccessService");
    ServicePool::get_instance().run();

    TcpService service;
    service.run();

    while (!gExitServer) {
        sleep(1);
    }

    destroy();
    ServicePool::get_instance().stop();
    service.stop();

    return 0;
}

