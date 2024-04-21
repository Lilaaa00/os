#include "library/syscalls.h"
#include "library/string.h"

int main()
{
    printf("start scheduler test.\n");

    const char * execs[5] = {"bin/process1.exe","bin/process2.exe","bin/process3.exe","bin/process4.exe","bin/process5.exe"};
    int priorities[5] = {9,7,2,1,5};

    int argc;
    const char * argv[2];

    for (int i=0;i<5;i++){
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
    }
    struct process_info info;
    syscall_process_wait(&info,-1);

    printf("end scheduler test\n");

    return 0;
}