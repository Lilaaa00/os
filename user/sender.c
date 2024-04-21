#include "library/string.h"
#include "library/syscalls.h"
#include "library/stdio.h"

void break_point(){
    return 0;
}
int main()
{
    char *fname = "pipe_";

    int fd = syscall_open_named_pipe(fname);

    char buffer[] = "Hello World\n";
    int write_size = syscall_object_write(fd,buffer,strlen(buffer),0);
    printf("write %d byte.\n",write_size);

}