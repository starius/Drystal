/**
 * This file is part of Drystal.
 *
 * Drystal is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Drystal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Drystal.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <errno.h>

#include "util.h"
#include "macro.hpp"

int mkdir_p(const char *path)
{
	char *p;
	char t[strlen(path) + 1];

	assert(path);

	strcpy(t, path);
	for (p = strchr(t, '/'); p && *p; p = strchr(p + 1, '/')) {
		int r;

		*p = 0;

		r = mkdir(t, 0777);
		if (r < 0 && errno != EEXIST)
			return -errno;

		*p = '/';
	}

	return 0;
}

char *strjoin(const char *s, ...)
{
	va_list ap;
	size_t l = 0;
	char *join, *p;

	va_start(ap, s);
	for (;;) {
		const char *t;

		t = va_arg(ap, const char *);
		if (!t) {
			break;
		}

		l += strlen(t);
	}
	va_end(ap);

	join = calloc(l + 1, sizeof(char));
	if (!join) {
		return NULL;
	}

	va_start(ap, s);
	p = stpcpy(join, s);
	for (;;) {
		const char *t;

		t = va_arg(ap, const char *);
		if (!t) {
			break;
		}

		p = stpcpy(p, t);
	}
	va_end(ap);

	return join;
}

void msleep(unsigned long milisec)
{
	struct timespec ts;
	ts.tv_sec = milisec / MSEC_PER_SEC;
	ts.tv_nsec = (milisec % MSEC_PER_SEC) * MSEC_PER_SEC;

	/* we sleep until the time requested has passed */
	while (nanosleep(&ts, &ts) == -1)
		continue;
}

bool endswith(const char *s, const char *postfix)
{
	size_t s_length;
	size_t p_length;

	assert(s);
	assert(postfix);

	s_length = strlen(s);
	p_length = strlen(postfix);

	if (p_length == 0)
		return true;

	if (s_length < p_length)
		return false;

	if (memcmp(s + s_length - p_length, postfix, p_length) != 0)
		return false;

	return true;
}

bool is_directory(const char *directory)
{
	struct stat s;

	assert(directory);

	if (stat(directory, &s) < 0) {
		return false;
	}

	if (S_ISDIR(s.st_mode)) {
		return true;
	}

	return false;
}

void *xrealloc(void **p, size_t *nmemb, size_t need, size_t size, unsigned min_nmemb)
{
	size_t new_totalsize;
	size_t new_nmemb;
	void *q;

	assert(p);
	assert(nmemb);
	assert(size > 0);

	if (*nmemb >= need)
		return *p;

	new_nmemb = MAX(need * 2, min_nmemb);
	new_totalsize = new_nmemb * size;

	if (new_totalsize < size * need)
		return NULL;

	q = realloc(*p, new_totalsize);
	if (!q)
		return NULL;

	*p = q;
	*nmemb = new_nmemb;
	return q;
}

