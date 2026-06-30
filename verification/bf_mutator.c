#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BF_SIZE 8192
static const char SIMPLE_CMDS[] = "+-><.,";
#define NUM_SIMPLE 6

typedef struct {
	unsigned int seed;
} MutState;

static int find_close(const uint8_t *buf, size_t size, int open)
{
	int depth = 1;
	for (int i = open + 1; i < (int)size; i++) {
		if (buf[i] == '[')
			depth++;
		else if (buf[i] == ']') {
			if (--depth == 0)
				return i;
		}
	}
	return -1;
}

static int count_opens(const uint8_t *buf, size_t size)
{
	int n = 0;
	for (size_t i = 0; i < size; i++)
		if (buf[i] == '[')
			n++;
	return n;
}

static size_t buf_insert(uint8_t *dst, const uint8_t *src, size_t size,
			 size_t pos, uint8_t c)
{
	memcpy(dst, src, pos);
	dst[pos] = c;
	memcpy(dst + pos + 1, src + pos, size - pos);
	return size + 1;
}

static size_t buf_delete(uint8_t *dst, const uint8_t *src, size_t size,
			 size_t pos)
{
	memcpy(dst, src, pos);
	memcpy(dst + pos, src + pos + 1, size - pos - 1);
	return size - 1;
}

void *afl_custom_init(void *afl, unsigned int seed)
{
	(void)afl;
	MutState *s = malloc(sizeof(MutState));
	s->seed = seed;
	return s;
}

void afl_custom_deinit(void *data)
{
	free(data);
}

size_t afl_custom_fuzz(void *data, uint8_t *buf, size_t buf_size,
		       uint8_t **out_buf, uint8_t *add_buf,
		       size_t add_buf_size, size_t max_size)
{
	(void)add_buf;
	(void)add_buf_size;
	MutState *s = data;

	static uint8_t mutated[MAX_BF_SIZE + 2];
	if (buf_size == 0 || buf_size > MAX_BF_SIZE) {
		*out_buf = buf;
		return buf_size;
	}

	s->seed = s->seed * 1664525u + 1013904223u;
	int strategy = (int)(s->seed % 5);
	size_t new_size = buf_size;

	switch (strategy) {
	case 0: {
		if (buf_size >= max_size) {
			strategy = 1;
			goto del;
		}
		s->seed = s->seed * 1664525u + 1013904223u;
		size_t pos = s->seed % (buf_size + 1);
		s->seed = s->seed * 1664525u + 1013904223u;
		uint8_t cmd = (uint8_t)SIMPLE_CMDS[s->seed % NUM_SIMPLE];
		new_size = buf_insert(mutated, buf, buf_size, pos, cmd);
		break;
	}
	case 1:
	del : {
		size_t candidates[MAX_BF_SIZE];
		int nc = 0;
		for (size_t i = 0; i < buf_size; i++) {
			uint8_t c = buf[i];
			if (c != '[' && c != ']')
				candidates[nc++] = i;
		}
		if (nc == 0) {
			memcpy(mutated, buf, buf_size);
			break;
		}
		s->seed = s->seed * 1664525u + 1013904223u;
		size_t pos = candidates[s->seed % (unsigned int)nc];
		new_size = buf_delete(mutated, buf, buf_size, pos);
		break;
	}
	case 2: {
		size_t candidates[MAX_BF_SIZE];
		int nc = 0;
		for (size_t i = 0; i < buf_size; i++) {
			uint8_t c = buf[i];
			if (c != '[' && c != ']')
				candidates[nc++] = i;
		}
		memcpy(mutated, buf, buf_size);
		if (nc == 0)
			break;
		s->seed = s->seed * 1664525u + 1013904223u;
		size_t pos = candidates[s->seed % (unsigned int)nc];
		s->seed = s->seed * 1664525u + 1013904223u;
		mutated[pos] = (uint8_t)SIMPLE_CMDS[s->seed % NUM_SIMPLE];
		break;
	}
	case 3: {
		if (buf_size + 2 > max_size) {
			memcpy(mutated, buf, buf_size);
			break;
		}
		s->seed = s->seed * 1664525u + 1013904223u;
		size_t a = s->seed % (buf_size + 1);
		s->seed = s->seed * 1664525u + 1013904223u;
		size_t b = s->seed % (buf_size + 1);
		if (a > b) {
			size_t t = a;
			a = b;
			b = t;
		}
		uint8_t tmp[MAX_BF_SIZE + 2];
		size_t sz = buf_insert(tmp, buf, buf_size, b, ']');
		new_size = buf_insert(mutated, tmp, sz, a, '[');
		break;
	}
	case 4: {
		int n_opens = count_opens(buf, buf_size);
		if (n_opens == 0) {
			memcpy(mutated, buf, buf_size);
			break;
		}
		s->seed = s->seed * 1664525u + 1013904223u;
		int target = (int)(s->seed % (unsigned int)n_opens);
		int seen = 0;
		int open_pos = -1;
		for (int i = 0; i < (int)buf_size; i++) {
			if (buf[i] == '[') {
				if (seen++ == target) {
					open_pos = i;
					break;
				}
			}
		}
		int close_pos = find_close(buf, buf_size, open_pos);
		if (close_pos < 0) {
			memcpy(mutated, buf, buf_size);
			break;
		}
		size_t before = (size_t)open_pos;
		size_t after = buf_size - (size_t)close_pos - 1;
		memcpy(mutated, buf, before);
		memcpy(mutated + before, buf + close_pos + 1, after);
		new_size = before + after;
		break;
	}
	}

	if (new_size == 0) {
		mutated[0] = '+';
		new_size = 1;
	}

	*out_buf = mutated;
	return new_size;
}
