#include <unistd.h>
#include <sys/types.h>

#include "log.h"

int check_root_euid(log *log)
{
	if (geteuid() != 0) {
		log_error(log, "Must be run as root or sudo\n");
		return 1;
	}
	return 0;
}
