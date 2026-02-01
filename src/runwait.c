#include "common.h"
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
static void usage(const char *a){fprintf(stderr,"Usage: %s <command> [args...]\n",a); exit(1);}
int main(int c,char**v){
    if(c < 2){
        usage(v[0]);
    }
    struct timespec t0, t1;
    if(clock_gettime(CLOCK_MONOTONIC, &t0) != 0){
        DIE("clock_gettime");
    }
    pid_t pid = fork();
    if(pid < 0){
        DIE("fork");
    }
    if(pid == 0){
        execvp(v[1], &v[1]);
        perror("execvp");
        _exit(127);
    }
    int status = 0;
    if(waitpid(pid, &status, 0) < 0){
        DIE("waitpid");
    }
    if(clock_gettime(CLOCK_MONOTONIC, &t1) != 0){
        DIE("clock_gettime");
    }
    double elapsed = (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1000000000.0;
    printf("pid=%ld ", (long)pid);
    if(WIFEXITED(status)){
        printf("exit=%d signal=0 ", WEXITSTATUS(status));
    } else if(WIFSIGNALED(status)){
        printf("exit=-1 signal=%d ", WTERMSIG(status));
    } else {
        printf("exit=-1 signal=-1 ");
    }
    printf("time=%.6f\n", elapsed);
    return 0;
}
