#include "kernel/types.h"
#include "named_pipe.h"
#include "kmalloc.h"
#include "process.h"
#include "page.h"
#include "syscall_handler.h"

#define NAMEDPIPE_SIZE PAGE_SIZE
struct namedpipe {
	char *buffer;
    char *name; // name of namedpipe.
	int read_pos;
	int write_pos;
	int flushed;
	int refcount;
	struct list queue;
};

struct namedpipe_file_hdr{
    char ident[16];
    int isOpened;               // 0 :Closed.  1: Opened.
    char * named_pipe_addr;  
};

// Creating a namedpipe file.
int namedpipe_create(char* fname)
{
    struct kobject* newobj;
    int result = kobject_lookup(current->ktable[KNO_STDDIR],fname,&newobj);

    if (result >= 0){
        
        printf("namedpipe create error: File name exists.\n");

        kobject_close(newobj);
        return -1;
    }

    struct fs_dirent* named_pipe_file = fs_dirent_mkfile(fs_resolve('.'),fname);
    if (named_pipe_file == 0){
        printf("namedpipe create error: namedpipe creation failed.%s,%s\n",current->ktable[KNO_STDDIR],fname);

        kobject_close(newobj);
        return -1;
    }

    // create new named pipe file.
    struct namedpipe_file_hdr named_pipe_hdr;
    strcpy(named_pipe_hdr.ident,"NAMEDPIPE");
    named_pipe_hdr.isOpened = 0;
    named_pipe_hdr.named_pipe_addr = 0;

    int write_size = fs_dirent_write(named_pipe_file,&named_pipe_hdr,sizeof(named_pipe_hdr),0);
    fs_dirent_close(named_pipe_file);

    kobject_close(newobj);
    return 0;
}

// Delete Named Pipe File
int namedpipe_delete(char* fname)
{
    struct kobject* newobj;
    int result = kobject_lookup(current->ktable[KNO_STDDIR],fname,&newobj);

    if (result >= 0){
        struct namedpipe_file_hdr hdr;
        kobject_read(newobj,&hdr,sizeof(hdr),0);

        if(strcmp(hdr.ident,"NAMEDPIPE")!=0){
            printf("namedpipe delete error: File %s is not namedpipe.\n",fname);

            kobject_close(newobj);
            return -1;
        }
        if (hdr.isOpened == 1){
            printf("namedpipe delete error: namedpipe $s is opened.\n",fname);

            kobject_close(newobj);
            return -1;
        }
        
        fs_dirent_remove(current->ktable[KNO_STDDIR],fname);

        kobject_close(newobj);
        return 0;
    }
    
    return 0;
}

struct namedpipe *namedpipe_addref( struct namedpipe *p )
{
	p->refcount++;
	return p;
}

// Opens the namedpipe file and returns the pipe if it is already open, otherwise, creates the pipe.
struct namedpipe* namedpipe_open(char* fname)
{
    struct kobject* newobj;

    int result = kobject_lookup(current->ktable[KNO_STDDIR],fname,&newobj);

    if (result < 0){
        printf("namedpipe open error: File %s is not exists.\n",fname);

        kobject_close(newobj);
        return 0;
    }

    struct namedpipe_file_hdr hdr;

    int read_size = kobject_read(newobj,&hdr,sizeof(hdr),0);
    if(strcmp(hdr.ident,"NAMEDPIPE")!=0){
        printf("namedpipe open error: File %s is not namedpipe.\n",fname);
        
        kobject_close(newobj);
        return 0;
    }

    if(hdr.isOpened && hdr.named_pipe_addr != 0){
        kobject_close(newobj);
        return hdr.named_pipe_addr;
    }else{
        struct namedpipe *p = kmalloc(sizeof(*p));
	    if(!p){
            kobject_close(newobj);
            return 0;
        }
        p->buffer = page_alloc(1);
	    if(!p->buffer) {
		    kfree(p);
            kobject_close(newobj);
		    return 0;
	    }
        p->name = kmalloc(strlen(fname)+1);
        strcpy(p->name,fname);
	    p->read_pos = 0;
	    p->write_pos = 0;
	    p->flushed = 0;
	    p->queue.head = 0;
	    p->queue.tail = 0;
	    p->refcount = 1;
        hdr.isOpened = 1;
        hdr.named_pipe_addr = p;
        newobj->offset = 0;
        kobject_write(newobj,&hdr,sizeof(hdr),0);
        kobject_close(newobj);
        return p;
    }
}

// Close the pipe.
void namedpipe_close(struct namedpipe *p)
{
    if(!p) return;
	p->refcount--;
	if(p->refcount==0) {
		if(p->buffer) {
			page_free(p->buffer);
		}
        struct kobject* newobj;
        int result = kobject_lookup(current->ktable[KNO_STDDIR],p->name,&newobj);

        struct namedpipe_file_hdr hdr;
        kobject_read(newobj,&hdr,sizeof(hdr),0);
        hdr.isOpened = 0;
        hdr.named_pipe_addr = 0;
        kobject_write(newobj,&hdr,sizeof(hdr),0);
        if(p->name)
            kfree(p->name);
        kobject_close(newobj);
		kfree(p);
	}
}

void namedpipe_flush(struct namedpipe *p)
{
    if(p) {
		p->flushed = 1;
	}
}

// The namedpipe reads and writes in the same way as a pipe.
static int namedpipe_write_internal(struct namedpipe *p, char *buffer, int size, int blocking )
{
	if(!p || !buffer) {
		return -1;
	}
	int written = 0;
	if(blocking) {
		for(written = 0; written < size; written++) {
			while((p->write_pos + 1) % NAMEDPIPE_SIZE == p->read_pos) {
				if(p->flushed) {
					p->flushed = 0;
					return written;
				}
				process_wait(&p->queue);
			}
			p->buffer[p->write_pos] = buffer[written];
			p->write_pos = (p->write_pos + 1) % NAMEDPIPE_SIZE;
		}
		process_wakeup_all(&p->queue);
	} else {
		while(written < size && p->write_pos != (p->read_pos - 1) % NAMEDPIPE_SIZE) {
			p->buffer[p->write_pos] = buffer[written];
			p->write_pos = (p->write_pos + 1) % NAMEDPIPE_SIZE;
			written++;
		}
	}
	p->flushed = 0;
	return written;
}

int namedpipe_write(struct namedpipe *p, char *buffer, int size)
{
    return namedpipe_write_internal(p, buffer, size, 1);
}
int namedpipe_write_nonblock(struct namedpipe *p, char *buffer, int size)
{
    return namedpipe_write_internal(p, buffer, size, 0);
}

void break_point()
{
    return 0;
}

// The namedpipe reads and writes in the same way as a pipe.
static int namedpipe_read_internal(struct namedpipe *p, char *buffer, int size, int blocking)
{
	if(!p || !buffer) {
		return -1;
	}
	int read = 0;
	if(blocking) {
		for(read = 0; read < size; read++) {
			while(p->write_pos == p->read_pos) {
                break_point();
				if(p->flushed) {
					p->flushed = 0;
					return read;
				}
				if (blocking == 0) {
					return -1;
				}
				process_wait(&p->queue);
			}
			buffer[read] = p->buffer[p->read_pos];
			p->read_pos = (p->read_pos + 1) % NAMEDPIPE_SIZE;
		}
		process_wakeup_all(&p->queue);
	} else {
		while(read < size && p->read_pos != p->write_pos) {
			buffer[read] = p->buffer[p->read_pos];
			p->read_pos = (p->read_pos + 1) % NAMEDPIPE_SIZE;
			read++;
		}
	}
	p->flushed = 0;
	return read;
}

int namedpipe_read(struct namedpipe *p, char *buffer, int size)
{
    return namedpipe_read_internal(p, buffer, size, 1);
}
int namedpipe_read_nonblock(struct namedpipe *p, char *buffer, int size)
{
    return namedpipe_read_internal(p, buffer, size, 0);
}

int namedpipe_size(struct namedpipe *p)
{
    return NAMEDPIPE_SIZE;
}