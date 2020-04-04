/* ls -l /dev/shm
 * -rw------- 1 b07902143 student 2.2G Apr  4 02:34 criteo_entries
 * -rw------- 1 b07902143 student 184M Apr  4 02:34 sorted_criteo_entries
 */
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>

#include "common.h"
#include "database.h"
#include "fstring.h"

int main(int argc, const char** argv)
{
    preload_criteo_data(argv[1]);
    pause();
    shm_unlink(CRITEO_ENTRIES_SHM_NAME);
    shm_unlink(SORTED_CRITEO_ENTRIES_SHM_NAME);
    return 0;
}