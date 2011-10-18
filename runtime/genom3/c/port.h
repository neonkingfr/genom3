/*
 * Copyright (c) 2011 LAAS/CNRS
 * All rights reserved.
 *
 * Redistribution and use  in source  and binary  forms,  with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   1. Redistributions of  source  code must retain the  above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice,  this list of  conditions and the following disclaimer in
 *      the  documentation  and/or  other   materials provided  with  the
 *      distribution.
 *
 * THIS  SOFTWARE IS PROVIDED BY  THE  COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND  ANY  EXPRESS OR IMPLIED  WARRANTIES,  INCLUDING,  BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES  OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR  PURPOSE ARE DISCLAIMED. IN  NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR      CONTRIBUTORS  BE LIABLE FOR   ANY    DIRECT, INDIRECT,
 * INCIDENTAL,  SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE   OF THIS SOFTWARE, EVEN   IF ADVISED OF   THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 *					Anthony Mallet on Tue Oct 18 2011
 */

#ifndef H_GENOM3_C_PORT
#define H_GENOM3_C_PORT

#include <stdint.h>

typedef uint32_t g3port_handle;

struct g3port_pollhandle {
  g3port_handle fd;		/* port handle */
  uint16_t events;		/*  events to look for */
  uint16_t revents;		/* events returned */
};

#define G3PORT_POLLIN	0x1	/* data may be read without blocking */
#define G3PORT_POLLOUT	0x2	/* data may be written without blocking */

g3port_handle	g3port_open(const char *path, int mode);
int		g3port_close(g3port_handle h);

ssize_t		g3port_read(g3port_handle h, void *buf, size_t size);
ssize_t		g3port_pread(g3port_handle h, void *buf, size_t size,
			size_t off);
ssize_t		g3port_write(g3port_handle h, const void *buf, size_t size);
ssize_t		g3port_pwrite(g3port_handle h, void *buf, size_t size,
			size_t off);

int		g3port_poll(struct g3port_pollhandle *hs, int nfds,
			int timeout);

#endif /* H_GENOM3_C_PORT */
