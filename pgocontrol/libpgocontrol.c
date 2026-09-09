/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
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
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern void __gcov_dump(void);
extern void __gcov_reset(void);

#define PGO_FIFO "/tmp/pgo/pgo_control.fifo"

static int fifo_fd = -1;
static pthread_t worker_thread;

static void *pgo_worker(void *arg)
{
    (void)arg;

    char command[128];

    while (1) {
        fprintf(stderr,"PGO: READY For read\n");
        ssize_t n = read(fifo_fd, command,sizeof(command) - 1);
        fprintf(stderr,"PGO: READY For read done:%s\n", command);

        if (n < 0) {

            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(100000);
                continue;
            }

            fprintf(stderr, "pgo: FIFO read");
            break;
        }

        if (n == 0) {
            /*
             * Writer closed FIFO.
             *
             * Continue waiting.
             */
            continue;
        }

        command[n] = '\0';

        /*
         * Remove newline.
         */
        command[strcspn(command, "\r\n")] = '\0';


        if (strcmp(command, "DUMP") == 0) {

            fprintf(stderr,"PGO: dumping profile\n");
            __gcov_dump();
            fprintf(stderr,"PGO: profile dump completed\n");
        }
        else if (strcmp(command, "RESET") == 0) {

            fprintf(stderr,"PGO: resetting profile\n");
            __gcov_reset();
        }
        else if (strcmp(command, "DUMP_RESET") == 0) {

            fprintf(stderr,"PGO: dump profile\n");
            __gcov_dump();
            fprintf(stderr,"PGO: reset profile\n");
            __gcov_reset();
        }
        else {

            fprintf(stderr,"PGO: unknown command: %s\n",command);
        }
    }

    return NULL;
}


__attribute__((constructor))
static void pgo_init(void)
{
    /*
     * Create directory if necessary.
     *
     * Production implementation should handle
     * ownership/permissions carefully.
     */
    mkdir("/tmp/pgo", 0755);

    /*
     * Create FIFO.
     *
     * EEXIST is okay because the FIFO may already exist.
     */
    if (mkfifo(PGO_FIFO, 0600) != 0 && errno != EEXIST) {

        fprintf(stderr, "PGO: mkfifo failed: %s\n", strerror(errno));
        return;
    }

    /*
     * Open FIFO.
     *
     * O_RDWR avoids getting EOF whenever the external
     * writer closes the FIFO.
     */
    fifo_fd = open(PGO_FIFO, O_RDWR);

    if (fifo_fd < 0) {

        fprintf(stderr, "PGO: FIFO open failed: %s\n", strerror(errno));
        return;
    }

    /*
     * Create worker.
     */
    int ret = pthread_create(&worker_thread,NULL,pgo_worker,NULL);

    if (ret != 0) {

        fprintf(stderr,"PGO: pthread_create failed: %s\n",strerror(ret));
        close(fifo_fd);
        fifo_fd = -1;
        return;
    }

    fprintf(stderr, "PGO: FIFO control initialized: %s\n",PGO_FIFO);
}
