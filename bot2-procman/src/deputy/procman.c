/*
 * process management core code
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <glib.h>

#include <libgen.h>

#include "procman.h"

#define dbg (args...) fprintf(stderr, args)
//#undef dbg
//#define dbg (args...)

static void dbgt (const char *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);

    char timebuf[1024];
    time_t now;
    time (&now);
    struct tm now_tm;
    localtime_r (&now, &now_tm);
    strftime (timebuf, sizeof (timebuf)-1, "%F %T", &now_tm);

    char buf[4096];
    vsnprintf (buf, sizeof(buf), fmt, ap);

    va_end (ap);

    fprintf (stderr, "%s: %s", timebuf, buf);
}

static procman_cmd_t * procman_cmd_create (const char *cmd, int32_t cmd_id);
static void procman_cmd_destroy (procman_cmd_t *cmd);

struct _procman {
    procman_params_t params;
    GList *commands;
};

void procman_params_init_defaults (procman_params_t *params, int argc,
       char **argv)
{
    memset (params, 0, sizeof (procman_params_t));

    // infer the path of procman.  This will be used with execv to start the
    // child processes, as it's assumed that child executables reside in same
    // directory as procman (or specified as a relative path or absolute path)
    if (argc <= 0) {
        fprintf (stderr, "procman: INVALID argc (%d)\n", argc);
        abort();
    }

    char * dirpath, * argv0;

    argv0 = strdup (argv[0]);
    dirpath = dirname (argv0);
    snprintf ( params->bin_path, sizeof (params->bin_path), "%s/", dirpath);
    free (argv0);
}

procman_t *procman_create (const procman_params_t *params)
{
    procman_t *pm = (procman_t*)calloc(1, sizeof (procman_t));
    if (NULL == pm) return NULL;

    memcpy (&pm->params, params, sizeof (procman_params_t));

    // add the bin path to the PATH environment variable
    //
    // TODO check and see if it's already there
    char *path = getenv ("PATH");
    int newpathlen = strlen (path) + strlen(params->bin_path) + 2;
    char *newpath = calloc(1, newpathlen);
    sprintf (newpath, "%s:%s", params->bin_path, path);
    printf ("setting PATH to %s\n", newpath);
    setenv ("PATH", newpath, 1);
    free (newpath);

    if (strlen (params->config_file) > 0) {
        // parse the config file
        FILE *fp = fopen (params->config_file, "r");
        if (NULL == fp) {
            perror ("fopen");
            return NULL;
        }

        char buf[1024];
        do {
            memset (buf, 0, sizeof(buf));
            if(!fgets (buf, sizeof (buf), fp)) {
                perror("fgets");
                return NULL;
            }

            // remove leading and trailing whitespace
            g_strstrip (buf);

            // skip empty lines and comments
            if (strlen (buf) == 0 || '#' == buf[0]) {
                continue;
            }

            // skip lines starting with '['
            if ('[' == buf[0]) { continue; }

            if (pm->params.verbose)
                printf ("procman: adding [%s]\n", buf);
            procman_add_cmd (pm, buf);

        } while (! feof (fp));
    }

    return pm;
}

void
procman_destroy (procman_t *pm)
{
    GList *iter;
    for (iter = pm->commands; iter != NULL; iter = iter->next) {
        procman_cmd_t *p = (procman_cmd_t*)iter->data;
        procman_cmd_destroy (p);
    }
    g_list_free (pm->commands);
    pm->commands = NULL;

    free (pm);
}

int procman_start_cmd (procman_t *pm, procman_cmd_t *p)
{
    int status;

    if (0 != p->pid) {
        dbgt ("%s has non-zero PID.  not starting again\n", p->cmd->str);
        return -1;
    } else {
        dbgt ("starting [%s]\n", p->cmd->str);

        // close existing fd's
        if (p->stdout_fd >= 0) {
            close (p->stdout_fd);
            p->stdout_fd = -1;
        }
        if (p->stdin_fd >= 0) {
            close (p->stdin_fd);
            p->stdin_fd = -1;
        }
        p->exit_status = 0;

        // setup pipes
        int stdin_fds[2];
        int stdout_fds[2];

        status = pipe (stdin_fds);
        if (status < 0) {
            int eno = errno;
            dbgt ("couldn't create stdin pipe for [%s]: %s (%d)\n",
                    p->cmd->str, strerror (eno), eno);
            return -1;
        }
        status = pipe (stdout_fds);
        if (status < 0) {
            int eno = errno;
            dbgt ("couldn't create stdin pipe for [%s]: %s (%d)\n",
                    p->cmd->str, strerror (eno), eno);
            close (stdin_fds[0]);
            close (stdin_fds[1]);
            return -1;
        }

//        printf("command is: %s\n",p->cmd->str);
//        for (int i=0;i<p->envc;i++){
//          printf("%s = %s\n",p->envp[i][0],p->envp[i][1]);
//        }
//
//        for (int i=0;i<p->argc;i++){
//          printf("%s ",p->argv[i]);
//        }
//        printf("\n");

        int pid;
        pid = fork();
        if (0 == pid) {
            // move stderr to fd 3, in case shit happens during exec
            // if exec succeeds, then we have a dangling file descriptor that
            // gets closed when the child exits... that's okay
            dup2( 2, 3);

            // redirect stdin, stdout, and stderr
            status = close(0);
            status = close(1);
            status = close(2);
            status = dup2( stdin_fds[0], 0);
            status = dup2( stdout_fds[1], 1);

            // make stderr the same as stdout
            status = dup2( stdout_fds[1], 2);

            // close unneeded pipes
            status = close (stdin_fds[1]);
            status = close (stdout_fds[0]);

            // block SIGINT (only allow the procman to kill the process now)
            sigset_t toblock;
            sigemptyset (&toblock);
            sigaddset (&toblock, SIGINT);
            sigprocmask (SIG_BLOCK, &toblock, NULL);

            //set environment variables from the beginning of the command
            for (int i=0;i<p->envc;i++){
              setenv(p->envp[i][0],p->envp[i][1],1);
            }

            // go!
            execvp (p->argv[0], p->argv);

            char ebuf[1024];
            snprintf (ebuf, sizeof(ebuf),
                    "PROCMAN ERROR!!!! couldn't start [%s]!\n"
                    "execv: %s\n", p->cmd->str, strerror (errno));
            fprintf (stderr, "%s\n", ebuf);
            fflush (stderr);

            // if execv returns, the command did not execute successfully
            // (e.g. permission denied or bad path or something)

            // restore stderr so we can barf a real error message
            close(0); close(1); close(2); dup2( 3, 2); close(3);

            fprintf (stderr, "%s\n", ebuf);
            fflush (stderr);

            exit(-1);
        } else if (pid < 0) {
            perror ("fork");
            return -1;
        } else {
            p->pid = pid;

            p->stdin_fd = stdin_fds[1];
            p->stdout_fd = stdout_fds[0];

            // close unneeded pipes
            status = close (stdin_fds[0]);
            status = close (stdout_fds[1]);
        }
    }
    return 0;
}

int procman_start_all_cmds (procman_t *pm)
{
    GList *iter;
    int status;
    for (iter = pm->commands; iter != NULL; iter = iter->next) {
        procman_cmd_t *p = (procman_cmd_t*)iter->data;
        if (0 == p->pid) {
            status = procman_start_cmd (pm, p);
            if (0 != status) {
                // if the command couldn't be started, then abort everything
                // and return
                procman_stop_all_cmds (pm);

                return status;
            }
        }
    }
    return 0;
}

int
procman_kill_cmd (procman_t *pm, procman_cmd_t *p, int signum)
{
    if (0 == p->pid) {
        dbgt ("%s has no PID.  not stopping (already dead)\n", p->cmd->str);
        return -EINVAL;
    }
    dbgt ("sending signal %d to %s\n", signum, p->cmd->str);
    if (0 != kill (p->pid, signum)) {
        return -errno;
    }
    return 0;
}

int procman_stop_cmd (procman_t *pm, procman_cmd_t *p)
{
    return procman_kill_cmd (pm, p, SIGTERM);
}

int procman_stop_all_cmds (procman_t *pm)
{
    GList *iter;
    int ret = 0;
    int status;

    // loop through each managed process and try to stop it
    for (iter = pm->commands; iter != NULL; iter = iter->next) {
        procman_cmd_t *p = (procman_cmd_t*)iter->data;
        status = procman_stop_cmd (pm, p);

        if (0 != status) {
            ret = status;
            // If something bad happened, try to stop the other processes, but
            // still return an error
        }
    }
    return ret;
}

int
procman_check_for_dead_children (procman_t *pm, procman_cmd_t **dead_child)
{
    int status;

    // check for dead children
    *dead_child = NULL;
    int pid = waitpid (-1, &status, WNOHANG);

    if (pid > 0) {
        GList *iter;
        for (iter = pm->commands; iter != NULL; iter = iter->next) {
            procman_cmd_t *p = (procman_cmd_t*)iter->data;
            if (p->pid != 0 && pid == p->pid) {
                dbgt ("reaped [%s]\n", p->cmd->str);
                *dead_child = p;
                p->pid = 0;
                p->exit_status = status;

                if (WIFSIGNALED (status)) {
                    int signum = WTERMSIG (status);
                    dbgt ("[%s] terminated by signal %d (%s)\n",
                            p->cmd->str, signum, strsignal (signum));
                } else if (status != 0) {
                    dbgt ("[%s] exited with nonzero status %d!\n",
                            p->cmd->str, WEXITSTATUS (status));
                }
                return status;
            }
        }

        dbgt ("reaped [%d] but couldn't find process\n", pid);
    }
    return 0;
}

int
procman_close_dead_pipes (procman_t *pm, procman_cmd_t *cmd)
{
    if (cmd->stdout_fd < 0 && cmd->stdin_fd < 0) return 0;

    if (cmd->pid) {
        dbgt ("refusing to close pipes for command "
                "with nonzero pid [%s] [%d]\n",
                cmd->cmd->str, cmd->pid);
    }
    if (cmd->stdout_fd >= 0) {
        close (cmd->stdout_fd);
    }
    if (cmd->stdin_fd >= 0) {
        close (cmd->stdin_fd);
    }
    cmd->stdin_fd = -1;
    cmd->stdout_fd = -1;
    return 0;
}

static char **
strsplit_set_packed(const char *tosplit, const char *delimeters, int max_tokens)
{
    char **tmp = g_strsplit_set(tosplit, delimeters, max_tokens);
    int i;
    int n=0;
    for(i=0; tmp[i]; i++) {
        if(strlen(tmp[i])) n++;
    }
    char **result = calloc(n+1, sizeof(char*));
    int c=0;
    for(i=0; tmp[i]; i++) {
        if(strlen(tmp[i])) {
            result[c] = g_strdup(tmp[i]);
            c++;
        }
    }
    g_strfreev(tmp);
    return result;
}

static void
procman_cmd_split_str (procman_cmd_t *pcmd)
{
    if (pcmd->argv) {
        g_strfreev (pcmd->argv);
        pcmd->argv = NULL;
    }
    if (pcmd->envp) {
        for(int i=0;i<pcmd->envc;i++)
            g_strfreev (pcmd->envp[i]);
        free(pcmd->envp);
        pcmd->envp = NULL;
    }

    char ** argv=NULL;
    int argc = -1;
    GError *err = NULL;
    gboolean parsed = g_shell_parse_argv(pcmd->cmd->str, &argc,
            &argv, &err);

    if(!parsed || err) {
        // unable to parse the command string as a Bourne shell command.
        // Do the simple thing and split it on spaces.
        pcmd->envp = calloc(1, sizeof(char**));
        pcmd->envc = 0;
        pcmd->argv = strsplit_set_packed(pcmd->cmd->str, " \t\n", 0);
        for(pcmd->argc=0; pcmd->argv[pcmd->argc]; pcmd->argc++);
        g_error_free(err);
        return;
    }

    // extract environment variables
    int envCount=0;
    char * equalSigns[512];
    while((equalSigns[envCount]=strchr(argv[envCount],'=')))
        envCount++;
    pcmd->envc=envCount;
    pcmd->argc=argc-envCount;
    pcmd->envp = calloc(pcmd->envc+1,sizeof(char**));
    pcmd->argv = calloc(pcmd->argc+1,sizeof(char*));
    for (int i=0;i<argc;i++) {
        if (i<envCount)
            pcmd->envp[i]=g_strsplit(argv[i],"=",2);
        else
            pcmd->argv[i-envCount]=g_strdup(argv[i]);
    }
    g_strfreev(argv);
}

static procman_cmd_t *
procman_cmd_create (const char *cmd, int32_t cmd_id)
{
    procman_cmd_t *pcmd = (procman_cmd_t*)calloc(1, sizeof (procman_cmd_t));
    pcmd->cmd = g_string_new ("");
    pcmd->cmd_id = cmd_id;
    pcmd->stdout_fd = -1;
    pcmd->stdin_fd = -1;
    g_string_assign (pcmd->cmd, cmd);

    procman_cmd_split_str (pcmd);

    return pcmd;
}

static void
procman_cmd_destroy (procman_cmd_t *cmd)
{
    g_string_free (cmd->cmd, TRUE);
    g_strfreev (cmd->argv);
    for(int i=0;i<cmd->envc;i++)
       g_strfreev (cmd->envp[i]);
    free(cmd->envp);
    free (cmd);
}

const GList *
procman_get_cmds (procman_t *pm) {
    return pm->commands;
}

procman_cmd_t*
procman_add_cmd (procman_t *pm, const char *cmd_str)
{
    // pick a suitable ID
    int32_t cmd_id;

    // TODO make this more efficient (i.e. sort the existing cmd_ids)
    //      this implementation is O (n^2)
    for (cmd_id=1; cmd_id<INT_MAX; cmd_id++) {
        int collision = 0;
        GList *iter;
        for (iter=pm->commands; iter; iter=iter->next) {
            procman_cmd_t *cmd = (procman_cmd_t*)iter->data;
            if (cmd->cmd_id == cmd_id) {
                collision = 1;
                break;
            }
        }
        if (! collision) break;
    }
    if (cmd_id == INT_MAX) {
        dbgt ("way too many commands on the system....\n");
        return NULL;
    }

    procman_cmd_t *newcmd = procman_cmd_create (cmd_str, cmd_id);
    if (newcmd) {
        pm->commands = g_list_append (pm->commands, newcmd);
    }

    dbgt ("added new command [%s] with id %d\n", newcmd->cmd->str, cmd_id);
    return newcmd;
}

int
procman_remove_cmd (procman_t *pm, procman_cmd_t *cmd)
{
    // check that cmd is actually in the list
    GList *toremove = g_list_find (pm->commands, cmd);
    if (! toremove) {
        dbgt ("procman ERRROR: %s does not appear to be managed "
                "by this procman!!\n",
                cmd->cmd->str);
        return -1;
    }

    // stop the command (if it's running)
    if (cmd->pid) {
        dbgt ("procman ERROR: refusing to remove running command %s\n",
                cmd->cmd->str);
        return -1;
    }

    procman_close_dead_pipes (pm, cmd);

    // remove and free
    pm->commands = g_list_remove_link (pm->commands, toremove);
    g_list_free_1 (toremove);
    procman_cmd_destroy (cmd);
    return 0;
}

int32_t
procman_get_cmd_status (procman_t *pm, procman_cmd_t *cmd)
{
    if (cmd->pid > 0) return PROCMAN_CMD_RUNNING;
    if (cmd->pid == 0) return PROCMAN_CMD_STOPPED;

    return 0;
}

procman_cmd_t *
procman_find_cmd (procman_t *pm, const char *cmd_str)
{
    GList *iter;
    for (iter=pm->commands; iter; iter=iter->next) {
        procman_cmd_t *p = (procman_cmd_t*)iter->data;
        if (! strcmp (p->cmd->str, cmd_str)) return p;
    }
    return NULL;
}

procman_cmd_t *
procman_find_cmd_by_id (procman_t *pm, int32_t cmd_id)
{
    GList *iter;
    for (iter=pm->commands; iter; iter=iter->next) {
        procman_cmd_t *p = (procman_cmd_t*)iter->data;
        if (p->cmd_id == cmd_id) return p;
    }
    return NULL;
}

void
procman_cmd_change_str (procman_cmd_t *cmd, const char *cmd_str)
{
    g_string_assign (cmd->cmd, cmd_str);
    procman_cmd_split_str (cmd);
}
