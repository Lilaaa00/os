#include "library/string.h"
#include "library/syscalls.h"
#include "library/stdio.h"

void break_point(){
    return 0;
}

int main()
{
    char *fname = "pipe_";

    int res = syscall_make_named_pipe(fname);
    int fd = syscall_open_named_pipe(fname);

    char buffer[20] = {0};
    printf("read\n");
    int read_size = syscall_object_read(fd,buffer,12,0);
    break_point();
    printf("read %d bytes : %s\n",read_size,buffer);
    printf("end receiver\n");
}