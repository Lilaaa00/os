#include "library/syscalls.h"
#include "library/string.h"

int main()
{
    printf("start namedpipe test.\n");

    const char * execs[5] = {"bin/receiver.exe","bin/sender.exe"};
    int priorities[5] = {1,2};

    int argc;
    const char * argv[2];

    for (int i=0;i<2;i++){
        argc = 2;
        argv[0] = execs[i];
        argv[1] = priorities[i];

        int pfd = syscall_open_file(KNO_STDDIR,execs[i],0,0);
        if (pfd >= 0){
            int pid = syscall_process_run(pfd,argc,&argv);
            if(pid > 0){
                printf("started process %d\n",pid);
            } else{
                printf("couldn't run %s: %s\n",argv[0],strerror(pid));
            }
            syscall_object_close(pfd);
        }else{
            printf("couldn't find %s: %s\n",argv[0],strerror(pfd));
        }
		syscall_process_yield();
    }
    struct process_info info;
    syscall_process_wait(&info,-1);

    printf("end namedpipe test\n");

    return 0;
}