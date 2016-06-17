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

#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>

#if !defined(SYS_memfd_create)
#	error "memfd_create(2) is not available"
#else
/* These come directly from <include/uapi/linux/memfd.h>. */
#	if !defined(MFD_CLOEXEC)
#		define MFD_CLOEXEC 0x0001U
#	endif
#	if !defined(MFD_ALLOW_SEALING)
#		define MFD_ALLOW_SEALING 0x0002U
#	endif
int memfd_create(char *name, unsigned int flags)
{
	return syscall(SYS_memfd_create, name, flags);
}
#endif

#if !defined(SYS_unlinkat)
# 	error "unlinkat(2) is not available"
int unlinkat(int dirfd, const char *pathname, int flags)
{
	return syscall(SYS_unlinkat, dirfd, pathname, flags);
}
#endif

/* These come directly from the Linux kernel source. */
#if !defined(F_ADD_SEALS)
#	define F_LINUX_SPECIFIC_BASE 1024
#	define F_ADD_SEALS (F_LINUX_SPECIFIC_BASE + 9)
#	define F_SEAL_SEAL   0x0001
#	define F_SEAL_SHRINK 0x0002
#	define F_SEAL_GROW   0x0004
#endif
#if !defined(MFD_ALLOW_SEALING)
#	define MFD_ALLOW_SEALING 0x0002U
#endif

/*
 * So people know how I feel about the topic.
 */
#define MEMFD_NAME "memfd_create(2) is a bad interface"

/*
 * A wrapper around memfd_create. It currently only works on Linux 3.17 or
 * later, but in principle you can implement this by just mounting tmpfs and
 * redoing most of this work.
 *
 * TODO: Do that.
 */
int memfd(size_t size)
{
	/*
	 * After reading the kernel source (mm/shmem.c), it looks like the "name"
	 * field here is completely superfluous. Why is it required, given the fact
	 * that files are inodes in UNIX and that the "filename" is a concept that
	 * exists at a directory level (and shmem_file_setup() doesn't actually
	 * link the dentry to the shmem directory)? For "debugging purposes". That
	 * is not a joke (while this interface is).
	 *
	 * Not even the POSIX interface requires something goofy like being able to
	 * open the same anonymous file with (shmem_create). I actually do not
	 * understand what was going through the author's and maintainers' minds
	 * when they collectively decided "this is a good idea".
	 */
	int fd = memfd_create(MEMFD_NAME, MFD_ALLOW_SEALING|MFD_CLOEXEC);
	if (fd < 0)
		return -1;

	/* To "set" the size of the buffer, we ftruncate. */
	if (ftruncate(fd, size) < 0)
		goto error;

	/*
	 * Disable changing the size. Truncating mmap(2)d buffers causes bad things
	 * to happen (such as `tail -f` not working properly).
	 */
	if (fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK|F_SEAL_GROW|F_SEAL_SEAL) < 0)
		goto error;

	return fd;

error:
	close(fd);
	return -1;
}
