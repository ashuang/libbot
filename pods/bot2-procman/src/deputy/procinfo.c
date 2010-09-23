/*
 * code for reading detailed process information on a GNU/Llinux system
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "procinfo.h"

static void strsplit (char *buf, char **words, int maxwords)
{
    int inword = 0;
    int i;
    int wordind = 0;
    for (i=0; buf[i] != 0; i++) {
        if (isspace (buf[i])) {
            inword = 0;
            buf[i] = 0;
        } else {
            if (! inword) {
                words[wordind] = buf + i;
                wordind++;
                if (wordind >= maxwords) break;
                inword = 1;
            }
        }
    }
    words[wordind] = NULL;
}

int 
procinfo_read_proc_cpu_mem (int pid, proc_cpu_mem_t *s)
{
    memset (s, 0, sizeof (proc_cpu_mem_t));
    char fname[80];
    sprintf (fname, "/proc/%d/stat", pid);
    FILE *fp = fopen (fname, "r");
    if (! fp) { return -1; }

    char buf[4096];
    if(!fgets (buf, sizeof (buf), fp)) {
        return -1;
    }
    char *words[50];
    memset (words, 0, sizeof(words));
    strsplit (buf, words, 50);

    s->user = atoi (words[13]);
    s->system = atoi (words[14]);
    s->vsize = strtoll (words[22], NULL, 10);
    s->rss = strtoll (words[23], NULL, 10) * getpagesize();

    fclose (fp);

    sprintf (fname, "/proc/%d/statm", pid);
    fp = fopen (fname, "r");
    if (! fp) { return -1; }

    if(!fgets (buf, sizeof(buf), fp)) {
        return -1;
    }
    memset (words, 0, sizeof(words));
    strsplit (buf, words, 50);

    s->shared = atoi (words[2]) * getpagesize();
    s->text = atoi (words[3]) * getpagesize();
    s->data = atoi (words[5]) * getpagesize();

    fclose (fp);

    return 0;
}

int 
procinfo_read_sys_cpu_mem (sys_cpu_mem_t *s)
{
    memset (s, 0, sizeof(sys_cpu_mem_t));
    FILE *fp = fopen ("/proc/stat", "r");
    if (! fp) { return -1; }

    char buf[4096];
    char tmp[80];

    while (! feof (fp)) {
        if(!fgets (buf, sizeof (buf), fp)) {
            if(feof(fp))
                break;
            else
                return -1;
        }

        if (! strncmp (buf, "cpu ", 4)) {
            sscanf (buf, "%s %u %u %u %u", 
                    tmp,
                    &s->user,
                    &s->user_low,
                    &s->system,
                    &s->idle);
            break;
        }
    }
    fclose (fp);

    fp = fopen ("/proc/meminfo", "r");
    if (! fp) { return -1; }
    while (! feof (fp)) {
        char units[10];
        memset (units,0,sizeof(units));
        if(!fgets (buf, sizeof (buf), fp)) {
            if(feof(fp))
                break;
            else
                return -1;
        }

        if (! strncmp ("MemTotal:", buf, strlen ("MemTotal:"))) {
            sscanf (buf, "MemTotal: %"PRId64" %9s", &s->memtotal, units);
            s->memtotal *= 1024;
        } else if (! strncmp ("MemFree:", buf, strlen ("MemFree:"))) {
            sscanf (buf, "MemFree: %"PRId64" %9s", &s->memfree, units);
            s->memfree *= 1024;
        } else if (! strncmp ("SwapTotal:", buf, strlen("SwapTotal:"))) {
            sscanf (buf, "SwapTotal: %"PRId64" %9s", &s->swaptotal, units);
            s->swaptotal *= 1024;
        } else if (! strncmp ("SwapFree:", buf, strlen("SwapFree:"))) {
            sscanf (buf, "SwapFree: %"PRId64" %9s", &s->swapfree, units);
            s->swapfree *= 1024;
        } else {
            continue;
        }

        if (0 != strcmp (units, "kB")) {
            fprintf (stderr, "unknown units [%s] while reading "
                    "/proc/meminfo!!!\n", units);
        }
    }

    fclose (fp);

    return 0;
}
