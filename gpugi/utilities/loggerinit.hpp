#ifdef LOGGER_VAR_DEFINED
#	error "This header has to be include only once in a project."
#endif
#define LOGGER_VAR_DEFINED

#include "logger.hpp"

namespace Logger
{
	Logger g_logger;
} // namespace Logger