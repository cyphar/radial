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

#if !defined(RADIAL_H)
#define RADIAL_H

#include <stdint.h>

/*
 * I'm going to come clean and say that using this is not a good idea.
 * Sure, it's somewhat of a cool hack to play with, but if you care
 * about performance of your process at all please do not use this.
 * While TLB lookups are done in hardware, due to the fact that the TLB
 * has a limited number of entries, using mmap(2) to create many
 * mappings causes you to exhaust the number of extents in the TLB --
 * this causes lots of page faults which are an overhead for your
 * program (the kernel needs to drop in like a ninja to swap out some
 * entries to make everything look like it's still working).
 *
 * Put simply, please just use a regular circular buffer implementation.
 */

struct radial_t {
	/* Must be == RADIAL_MAGIC. */
	uint32_t magic;

	/*
	 * A file handle that has no purpose. Read rad_alloc to see why this
	 * whole process is really braindead and an example of a design that
	 * could've been so much nicer. Please don't touch this if you know
	 * what's good for you.
	 */
	int fd;

	/*
	 * This is the number of elements in the buffer * the size of each
	 * element.  Since we're doing all of this opaquely (allowing users
	 * to do funky stuff if they like), we just store the size.
	 * Extensions of the buffer will be mapped at this address:
	 *               (void *)(@start + i * (@size)).
	 *
	 * For technical reasons, @size MUST be a multiple of the system's
	 * page size. You can get this using sysconf(_SC_PAGESIZE).
	 */
	size_t size;

	/*
	 * The "start" of the circular buffer. All of the repeated mappings are
	 * after this one. These are the fields you probably care about. Do
	 * not modify them unless you like broken code.
	 */
	void *start;
	size_t copies; /* Don't touch this if like your program working. */
};

/*
 * Initialises a radial_t buffer. @size *must* be a multiple of the
 * system's page size. If your buffer isn't that size, too bad. Add some
 * padding to your structure.
 *
 * Returns 0 on success, -1 on failure with %errno set.
 */
int radial_init(struct radial_t *buffer, size_t size);

/*
 * Sets the number of copies of a radial_t buffer that have been cloned.
 * @ncopies must be greater than one, and it would be unwise to make it
 * too big since you might accidentally start attempting to map over
 * other pages.
 *
 * Returns 0 on success, -1 on failure with %errno set. If a failure
 * occured when increasing @ncopies, then the buffer may have been
 * mapped to at most (@ncopies - 1). Unmapping copies is atomic.
 */
int radial_map(struct radial_t *buffer, size_t ncopies);

/*
 * Frees all of the internal state of a radial_t buffer. The pointer
 * passed is not freed (the caller is responsible for memory management
 * of the structure).
 */
void radial_free(struct radial_t *buffer);

#endif /* !defined(RADIAL_H) */
