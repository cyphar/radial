/*
 * radial: a radial buffer implementation that's totally rad.
 * Copyright (C) 2016 Aleksa Sarai
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "memfd.h"
#include "radial.h"

/* So we can check that we didn't pass some garbage memory. */
#define RADIAL_MAGIC 0xe7d6f987

/* Assert with a comment. */
#define assert(cond, msg)													\
	if (!(cond)) {															\
		dprintf(STDERR_FILENO, "%d:%d:%s assert(%s) failed! %s\n",			\
				__FILE__, __LINE__, __func__, #cond, msg);					\
		abort();															\
	}

int radial_init(struct radial_t *buffer, size_t size)
{
	/* Assert checks. */
	assert(size > 0, "must allocate some bytes for the radial buffer");
	assert(buffer, "must pass a non-NULL container");
	/* XXX: This should be a static check. */
	assert(size % sysconf(_SC_PAGESIZE) == 0, "must have buffers that are a multiple of _SC_PAGESIZE")

	/*
	 * Container for our buffer. While we could do something horrific with just
	 * passing the pointer to the start of the buffer around, that just sounds
	 * like a bad idea right off the bat.
	 */
	memset(buffer, 0, sizeof(struct radial_t));

	/*
	 * Create an "anonymous file" to use as a file backing for the memory.
	 * Annoyingly, in order to correctly map the same chunk of memory (even
	 * though it exists in RAM or swap and it would only take a second TLB entry
	 * to point to the same chunk of physical ram) you need to use a file
	 * because the mmap(2) interface doesn't permit doing this trick with an
	 * arbitrary block of memory.
	 *
	 * In addition, we're about to use MAP_PRIVATE, rendering *the whole
	 * existence of the file in the first place completely pointless because
	 * it's just some memory that we've "allocated" but all modifications are
	 * being stored in the page cache and won't ever be synced to the underlying
	 * tmpfs. I love UNIX and GNU/Linux, but this is _fucked_.
	 */

	int fd = memfd(size);
	if (fd < 0)
		goto exit;

	/* Create the first mapping. */
	void *start = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (!start)
		goto exit_close;

	/*
	 * Extend the mapping to be twice the size. The user can manually specify to
	 * extend it further but that's just a bad idea.
	 */

	*buffer = (struct radial_t) {
		.magic  = RADIAL_MAGIC,
		.fd     = fd,
		.size   = size,
		.start  = start,
		.copies = 1,
	};

	return 0;

exit_close:
	close(fd);
exit:
	return -1;
}

int radial_map(struct radial_t *buffer, size_t ncopies)
{
	/* Assert checks. */
	assert(buffer && buffer->magic == RADIAL_MAGIC, "radial buffer not initialised");
	assert(ncopies > 0, "must map at least one radial buffer chunk");

	/* Nothing to do. */
	if (buffer->copies == ncopies)
		return 0;

	uintptr_t offset = 0;

	/*
	 * FIXME: This currently causes a lot of issues because we start trying to
	 *        map over other memory. mmap(2) can give us a very large page if
	 *        we ask nicely, but we don't want to allocate that big of a page.
	 */

	/* TODO: This is ugly. */
	if (buffer->copies < ncopies) {
		for (; buffer->copies < ncopies; buffer->copies++) {
			void *new = buffer->start + (uintptr_t)(buffer->copies * buffer->size);
			if (mmap(new, buffer->size, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_PRIVATE, buffer->fd, 0) != new)
				goto exit;
		}
	} else {
		/* munmap(2) for some reason allows you to unmap arbitrary blocks. */
		void *new = buffer->start + (uintptr_t)(ncopies * buffer->size);
		if (munmap(new, (buffer->copies - ncopies) * buffer->size))
			goto exit;
		buffer->copies = ncopies;
	}

	return 0;

exit:
	return -1;
}

void radial_free(struct radial_t *buffer)
{
	assert(buffer && buffer->magic == RADIAL_MAGIC, "radial buffer not initialised");

	/* Unset the magic bits. */
	buffer->magic = 0xDEADBEEF;

	/* Unmap the page and close the file. */
	munmap(buffer->start, buffer->copies*buffer->size);
	close(buffer->fd);
}
