#ifndef PUBLIC_LOGGING_H
#define PUBLIC_LOGGING_H

#include <gflags/gflags.h>
#include "easylogging++.h"

INITIALIZE_EASYLOGGINGPP

DECLARE_string(log_path);

#define CRITIAL_LOGGER_ID "critical"
#define SLOG(LEVEL) CLOG(LEVEL, ELPP_CURR_FILE_LOGGER_ID, CRITIAL_LOGGER_ID)

void init_logging() {
    el::Loggers::configureFromGlobal(FLAGS_log_path);
    el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);
    el::Loggers::addFlag(el::LoggingFlag::MultiLoggerSupport);
}

#endif /*PUBLIC_LOGGING_H*/

