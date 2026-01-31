#include "common.h"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
static void usage(const char *a){fprintf(stderr,"Usage: %s <pid>\n",a); exit(1);}
static int isnum(const char*s){for(;*s;s++) if(!isdigit(*s)) return 0; return 1;}
int main(int c,char**v){
    if(c!=2||!isnum(v[1])) usage(v[0]);
    long pid = strtol(v[1], NULL, 10);
    char path[128];
    snprintf(path, sizeof(path), "/proc/%ld/stat", pid);
    if(access(path, R_OK) != 0){
        if(errno == ENOENT){
            fprintf(stderr, "procinfo: pid %ld not found\n", pid);
            return 1;
        }
        if(errno == EACCES){
            fprintf(stderr, "procinfo: permission denied for pid %ld\n", pid);
            return 1;
        }
        DIE("access");
    }

    FILE *f = fopen(path, "r");
    if(!f) DIE("fopen");
    char buf[4096];
    if(!fgets(buf, sizeof(buf), f)){
        DIE_MSG("read stat");
    }
    fclose(f);

    char *rp = strrchr(buf, ')');
    if(!rp){
        DIE_MSG("bad stat");
    }

    char *p = rp +2;
    char state = '?';
    long ppid = -1;
    if(sscanf(p, "%c %ld", &state, &ppid) != 2){
        DIE_MSG("parse stat");
    }

    unsigned long long utime = 0, stime = 0;
    int field = 4;
    char *save = NULL;
    char *tok = strtok_r(p, " ", &save);
    while(tok){
        field++;
        if(field == 14){
            utime = strtoull(tok, NULL, 10);
        }
        if(field == 15){
            stime = strtoull(tok, NULL, 10);
            break;
        }
        tok = strtok_r(NULL, " ", &save);
    }

    long ticks = sysconf(_SC_CLK_TCK);
    double cpu_time = 0.0;
    if(ticks > 0){
        cpu_time = (double) (utime + stime) / (double)ticks;
    }
    char cmdline[4096];
    cmdline[0] = '\0';
    snprintf(path, sizeof(path), "/proc/%ld/cmdline", pid);
    f = fopen(path, "rb");
    if(f){
        size_t n = fread(cmdline, 1, sizeof(cmdline) - 1, f);
        fclose(f);
        cmdline[n] = '\0';
        for(size_t i = 0; i < n; i++){
            if(cmdline[i] == '\0'){
                cmdline[i] = ' ';
            }
        }
        while(n > 0 && cmdline[n-1] == ' '){
            cmdline[n - 1] = '\0';
            n--;
        }
    }

    long vmrss_kb = -1;
    snprintf(path, sizeof(path), "/proc/%ld/status", pid);
    f = fopen(path, "r");
    if(f){
        char line[512];
        while(fgets(line, sizeof(line), f)){
            if(strncmp(line, "VmRSS:", 6) == 0){
                sscanf(line, "VmRSS:%ld", &vmrss_kb);
                break;
            }
        }
        fclose(f);
    }

    printf("Process state: %c\n", state);
    printf("Parent PID: %ld\n", ppid);
    printf("Command line: %s\n", (cmdline[0] ? cmdline : "<empty>"));
    printf("CPU time (user+system): %.3f seconds\n", cpu_time);
    if(vmrss_kb >= 0){
        printf("Resident memory usage (VmRSS): %ld kB\n", vmrss_kb);
    } else {
        printf("Resident memory usage (VmRSS): unknown\n");
    }
    return 0;
}
