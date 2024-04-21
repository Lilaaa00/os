#include "library/syscalls.h"
#include "library/string.h"

void runForSeconds(int seconds)
{
    unsigned int startTime;
    syscall_system_time(&startTime);

    unsigned int timeElapsed;

    do {
        syscall_system_time(&timeElapsed);
        timeElapsed -= startTime;
    } while(timeElapsed < seconds);
}

int main()
{
    printf("pid: %d priority: %d\n",syscall_process_self(),syscall_process_priority());
    runForSeconds(3);
    return 0;
}