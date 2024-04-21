#ifndef NAMED_PIPE_H
#define NAMED_PIPE_H

#include "kernel/types.h"
// /kernel/fs.h file operation

int namedpipe_create(char* fname);
int namedpipe_delete(char* fname);

struct namedpipe *namedpipe_addref( struct namedpipe *p );
struct namedpipe* namedpipe_open(char* fname);

void namedpipe_close(struct namedpipe *p);
void namedpipe_flush(struct namedpipe *p);

int namedpipe_write(struct namedpipe *p, char *buffer, int size);
int namedpipe_write_nonblock(struct namedpipe *p, char *buffer, int size);
int namedpipe_read(struct namedpipe *p, char *buffer, int size);
int namedpipe_read_nonblock(struct namedpipe *p, char *buffer, int size);
int namedpipe_size(struct namedpipe *p);



#endif