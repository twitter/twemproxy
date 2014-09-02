/*
 * twemproxy - A fast and lightweight proxy for memcached protocol.
 * Copyright (C) 2011 Twitter, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <signal.h>

#include <nc_core.h>
#include <nc_signal.h>

static sig_atomic_t nc_reload;
static sig_atomic_t nc_exiting;
static int64_t nc_reload_ts;

/* how many ms should we delay since we received reload signal */
#define NC_RELOAD_DELAYED 3000

static struct signal signals[] = {
    { SIGUSR1, "SIGUSR1", 0,                 signal_handler },
    { SIGUSR2, "SIGUSR2", 0,                 signal_handler },
    { SIGTTIN, "SIGTTIN", 0,                 signal_handler },
    { SIGTTOU, "SIGTTOU", 0,                 signal_handler },
    { SIGHUP,  "SIGHUP",  0,                 signal_handler },
    { SIGINT,  "SIGINT",  0,                 signal_handler },
    { SIGSEGV, "SIGSEGV", (int)SA_RESETHAND, signal_handler },
    { SIGPIPE, "SIGPIPE", 0,                 SIG_IGN },
    { 0,        NULL,     0,                 NULL }
};

rstatus_t
signal_init(void)
{
    struct signal *sig;

    for (sig = signals; sig->signo != 0; sig++) {
        rstatus_t status;
        struct sigaction sa;

        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = sig->handler;
        sa.sa_flags = sig->flags;
        sigemptyset(&sa.sa_mask);

        status = sigaction(sig->signo, &sa, NULL);
        if (status < 0) {
            log_error("sigaction(%s) failed: %s", sig->signame,
                      strerror(errno));
            return NC_ERROR;
        }
    }

    return NC_OK;
}

void
signal_deinit(void)
{
}

void
signal_handler(int signo)
{
    struct signal *sig;
    void (*action)(void);
    char *actionstr;
    bool done;

    for (sig = signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }
    ASSERT(sig->signo != 0);

    actionstr = "";
    action = NULL;
    done = false;

    switch (signo) {
    case SIGUSR1:
        if (nc_exiting || nc_reload) {
            actionstr = ", is in exiting, ignored";
        } else {
            actionstr = ", reloading";
            nc_reload = 1;
        }
        break;

    case SIGUSR2:
        break;

    case SIGTTIN:
        actionstr = ", up logging level";
        action = log_level_up;
        break;

    case SIGTTOU:
        actionstr = ", down logging level";
        action = log_level_down;
        break;

    case SIGHUP:
        actionstr = ", reopening log file";
        action = log_reopen;
        break;

    case SIGINT:
        done = true;
        actionstr = ", exiting";
        break;

    case SIGSEGV:
        log_stacktrace();
        actionstr = ", core dumping";
        raise(SIGSEGV);
        break;

    default:
        NOT_REACHED();
    }

    log_safe("signal %d (%s) received%s", signo, sig->signame, actionstr);

    if (action != NULL) {
        action();
    }

    if (done) {
        exit(1);
    }
}

rstatus_t
signal_check(struct instance *nci)
{
    if (nc_reload) {
        nc_reload = 0;
        nc_exiting = 1;
        nc_reload_ts = nc_msec_now();

        log_debug(LOG_INFO, "reloading");
        core_exec_new_binary(nci);

        /* for parent: close listen fd here */
        proxy_deinit(nci->ctx);

        return NC_OK;
    }

    if (nc_exiting) {
        if (nc_msec_now() - nc_reload_ts > NC_RELOAD_DELAYED) {
            /* this is not a error, means we will exit now */
            return NC_ERROR;
        }
    }

    return NC_OK;
}
