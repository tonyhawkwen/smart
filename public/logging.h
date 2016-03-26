#ifndef PUBLIC_LOGGING_H
#define PUBLIC_LOGGING_H

#include <gflags/gflags.h>
#include "easylogging++.h"


#define CRITIAL_LOGGER_ID "critical"
#define SLOG(LEVEL) CLOG(LEVEL, ELPP_CURR_FILE_LOGGER_ID, CRITIAL_LOGGER_ID)


#endif /*PUBLIC_LOGGING_H*/

