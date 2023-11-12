#include "unistd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <errno.h>

static FILE _stdin  = {.fd = 0};
static FILE _stdout = {.fd = 1};
static FILE _stderr = {.fd = 2};

FILE* stdin  = &_stdin;
FILE* stdout = &_stdout;
FILE* stderr = &_stderr;

void __init_stdio() {
	stdin->read_buffer   = malloc(BUFSIZE);

	stdout->write_buffer = malloc(BUFSIZE);

	stdin->buffer_size  = BUFSIZE;
	stdout->buffer_size = BUFSIZE;
}

void _putchar(char c) {
	fwrite(&c, 1, 1, stdout);
}

int puts(const char *str) {
	uint32_t res = fwrite(str, 1, strlen(str), stdout);
	putchar('\n');
	return res;
}

int fprintf(FILE * stream, const char * format, ... ){
	va_list argptr;
    va_start(argptr, format);
	int result = vfprintf(stream, format, argptr);
	va_end(argptr);
	return result;
}

int fflush(FILE *stream){
	if(!stream->write_buffer) {
		return 0;
	}
	if(stream->write_ptr) {
		uint32_t result = write(stream->fd, stream->write_buffer, stream->write_ptr);
		stream->write_ptr = 0;
		return result;
	}
  	return 0;
}

static uint32_t __mode_to_options(const char* mode) {
	uint32_t len = strlen(mode);

	if(len == 1 || mode[1] != '+') {
		switch(mode[0]){
			case 'r':
				return O_RDONLY;
			case 'w':
				return O_WRONLY;
			case 'a':
				return O_WRONLY | O_APPEND;
		}
	} else if (mode[1] == '+'){
		switch(mode[0]){
			case 'r':
				return O_RDWR;
			case 'w':
				return O_RDWR | O_CREAT;
			case 'a':
				return O_WRONLY | O_APPEND | O_CREAT;
		}
	}

	return 0;
}

FILE* fdopen(int fildes, const char *mode) {
  FILE* file = calloc(1, sizeof(FILE));
  file->fd = fildes;
  file->write_buffer = malloc(BUFSIZE);
  file->read_buffer  = malloc(BUFSIZE);
  file->buffer_size  = BUFSIZE;
  return file;
}

FILE* fopen(const char *path, const char *mode){
  int32_t fd = __sys_open((uint32_t) path, __mode_to_options(mode), 0);
  if(fd < 0) {
	  __set_errno(-fd);
	  return NULL;
  }
  return fdopen(fd, mode);
}

int fclose(FILE * stream){
  fflush(stream);
  __sys_close(stream->fd);
  free(stream->read_buffer);
  free(stream->write_buffer);
  free(stream);
  return 0;
}


int fseek(FILE* stream, long offset, int whence){
	if(stream->read_ptr && whence == SEEK_CUR) {
		offset += stream->real_offset + stream->read_ptr;
		whence = SEEK_SET;	
	}

	if(stream->write_ptr){
		fflush(stream);
	}

	stream->read_ptr  = 0;
	stream->write_ptr = 0;
	stream->available = 0;
	stream->eof       = 0;

	return __sys_seek(stream->fd, offset, whence);
}

long ftell(FILE * stream){
 	return fseek(stream, 0, SEEK_CUR);
}

int vfprintf(FILE* device, const char *format, va_list argptr){
	char* buffer = (char*) malloc(4096); //TODO implement it properly
	vsnprintf(buffer, 4096, format, argptr);
	uint32_t len = strlen(buffer);
    int result = fwrite(buffer, len, 1, device);
	free(buffer);
	return result;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE* stream){
	uint32_t len = size * nmemb;
	uint32_t read_bytes = 0;
	char* buf = (char*) ptr;
	if(!stream->read_buffer) {
		return read(stream->fd, buf, len);
	}
	while(len > 0) {
		if(!stream->available) {
			if(stream->read_buffer_offset == stream->buffer_size) {
				stream->read_buffer_offset = 0;
			}

			stream->real_offset = fseek(stream, 0, SEEK_CUR);
			int32_t r = read(stream->fd, &stream->read_buffer[stream->read_buffer_offset], stream->buffer_size - stream->read_buffer_offset);

			if(r < 0){
				break;
			} else {
				stream->read_ptr            = stream->read_buffer_offset;
				stream->read_buffer_offset += r;
				stream->available           = r;
			}
		}

		if(stream->available) {
			*buf = stream->read_buffer[stream->read_ptr];
			buf++;
			stream->read_ptr++;
			stream->available--;
		} else {
			stream->eof = 1;
			break;
		}

		len--;
		read_bytes++;
	}
	return read_bytes;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE* stream){
	uint32_t len = size * nmemb;
	uint32_t written = len;
	char* buf = (char*) ptr;
	if(!stream->write_buffer) {
		return write(stream->fd, buf, len);
	}
	while(len > 0) {
		if(stream->write_buffer) {
			stream->write_buffer[stream->write_ptr++] = *buf;
			if(stream->write_ptr == stream->buffer_size || (*buf == '\n')) {
				fflush(stream);
			}
			buf++;
			len--;
		} 	
	}
	return written;
}

char fgetc(FILE* s) {
	char r;
	size_t read = fread(&r, 1, 1, s);
	if(read == 0) {
		s->eof = 1;
		return EOF;
	}
	return r;
}

int fputc(int ch, FILE *stream) {
	size_t written = fwrite(&ch, 1, 1, stream);
	if(written == 0) {
		stream->eof = 1;
		return EOF;
	}
	return ch;
}

char*  fgets(char* s, int size, FILE* stream) {
	if(size <= 1) {
		return NULL;
	}

	char* ptr = s;
	char  c;

	size -= 1;

	while((c = fgetc(stream)) != EOF) {
		*s = c;
		s++;
		size--;
		if(!size || c == '\n') {
			*s = '\0';
			return ptr;
		}
	}

	if(c == EOF) {
		if(ptr == s) {
			return NULL;
		} else {
			return ptr;
		}
	}

	return NULL;
}

void clearerr(FILE *stream) {
	stream->eof = 0;
}

int feof(FILE *stream) {
	return stream->eof;
}

int ferror(FILE *stream) {
	return 0;
}

int fileno(FILE *stream) {
	return stream->fd;
} 

void  setbuf(FILE* stream, char* buf) {
	stream->read_ptr = 0;
	stream->write_ptr = 0;
	stream->read_buffer_offset = 0;
	stream->available = 0;
	if(!buf) {
		if(stream->read_buffer) {
			free(stream->read_buffer);
			stream->read_buffer = 0;
		}
		if(stream->write_buffer) {
			free(stream->write_buffer);
			stream->write_buffer = 0;
		}
		stream->buffer_size = 0;
	} else {
		stream->write_buffer = buf;
		stream->read_buffer  = buf;
		stream->buffer_size  = BUFSIZE;
	}
}

void rewind(FILE* stream) {
	fseek(stream, 0, SEEK_SET);
	stream->read_buffer_offset = 0;
	stream->available = 0;
	stream->eof = 0;
	stream->ungetc = 0;
}

static const char* const __errno_values[] = {
	[0]       = "No error",
	[EPERM]   = "Not super-user",
	[ENOENT]  = "No such file or directory",
	[ESRCH ]  = "No such process",
	[EINTR ]  = "Interrupted system call",
	[EIO   ]  = "I/O error",
	[ENXIO ]  = "No such device or address",
	[E2BIG ]  = "Arg list too long",
	[ENOEXEC] = "Exec format error",
	[EBADF  ] = "Bad file number",
	[ECHILD ] = "No children",
	[EAGAIN ] = "No more processes",
	[ENOMEM ] = "Not enough core",
	[EACCES ] = "Permission denied",
	[EFAULT ] = "Bad address",
	[ENOTBLK] = "Block device required",
	[EBUSY  ] = "Mount device busy",
	[EEXIST ] = "File exists",
	[EXDEV  ] = "Cross-device link",
	[ENODEV ] = "No such device",
	[ENOTDIR] = "Not a directory",
	[EISDIR ] = "Is a directory",
	[EINVAL ] = "Invalid argument",
	[ENFILE ] = "Too many open files in system",
	[EMFILE ] = "Too many open files",
	[ENOTTY ] = "Not a typewriter",
	[ETXTBSY] = "Text file busy",
	[EFBIG ]  = "File too large",
	[ENOSPC]  = "No space left on device",
	[ESPIPE]  = "Illegal seek",
	[EROFS ]  = "Read only file system",
	[EMLINK]  = "Too many links",
	[EPIPE ]  = "Broken pipe",
	[EDOM  ]  = "Math arg out of domain of func",
	[ERANGE]  = "Math result not representable",
	[ENOMSG]  = "No message of desired type",
	[EIDRM ]  = "Identifier removed",
	[ECHRNG]  = "Channel number out of range",
	[EL2NSYNC] = "Level 2 not synchronized",
	[EL3HLT ] =  "Level 3 halted",
	[EL3RST ] =  "Level 3 reset",
	[ELNRNG ] =  "Link number out of range",
	[EUNATCH] =  "Protocol driver not attached",
	[ENOCSI ] =  "No CSI structure available",
	[EL2HLT ] =  "Level 2 halted",
	[EDEADLK] =  "Deadlock condition",
	[ENOLCK ] =  "No record locks available",
	[EBADE  ] =  "Invalid exchange",
	[EBADR  ] =  "Invalid request descriptor",
	[EXFULL ] =  "Exchange full",
	[ENOANO ] =  "No anode",
	[EBADRQC] =  "Invalid request code",
	[EBADSLT] =  "Invalid slot",
	[EDEADLOCK] =   "File locking deadlock error",
	[EBFONT ] =  "Bad font file fmt",
	[ENOSTR ] =  "Device not a stream",
	[ENODATA] =  "No data (for no delay io)",
	[ETIME  ] =  "Timer expired",
	[ENOSR  ] =  "Out of streams resources",
	[ENONET ] =  "Machine is not on the network",
	[ENOPKG ] =  "Package not installed",
	[EREMOTE] =  "The object is remote",
	[ENOLINK] =  "The link has been severed",
	[EADV]    =   "Advertise error",
	[ESRMNT   ] = "Srmount error",
	[ECOMM    ] = "Communication error on send",
	[EPROTO   ] = "Protocol error",
	[EMULTIHOP] =  "Multihop attempted",
	[ELBIN    ] =  "Inode is remote (not really error)",
	[EDOTDOT  ] =  "Cross mount point (not really error)",
	[EBADMSG  ] =  "Trying to read unreadable message",
	[EFTYPE   ] =  "Inappropriate file type or format",
	[ENOTUNIQ ] =  "Given log. name not unique",
	[EBADFD   ] =  "f.d. invalid for this operation",
	[EREMCHG  ] =  "Remote address changed",
	[ELIBACC  ] =  "Can't access a needed shared lib",
	[ELIBBAD  ] =  "Accessing a corrupted shared lib",
	[ELIBSCN  ] =  ".lib section in a.out corrupted",
	[ELIBMAX  ] =  "Attempting to link in too many libs",
	[ELIBEXEC ] =  "Attempting to exec a shared library",
	[ENOSYS   ] =  "Function not implemented",
	[ENOTEMPTY] =  "Directory not empty",
	[ENAMETOOLONG] = "File or path name too long",
	[ELOOP       ] = "Too many symbolic links",
	[EOPNOTSUPP  ] = "Operation not supported on transport endpoint",
	[EPFNOSUPPORT] = "Protocol family not supported",
	[ECONNRESET  ] = "Connection reset by peer",
	[ENOBUFS     ] = "No buffer space available",
	[EAFNOSUPPORT] = "Address family not supported by protocol family",
	[EPROTOTYPE  ] = "Protocol wrong type for socket",
	[ENOTSOCK    ] = "Socket operation on non-socket",
	[ENOPROTOOPT ] = "Protocol not available",
	[ESHUTDOWN   ] = "Can't send after socket shutdown",
	[ECONNREFUSED] = "Connection refused",
	[EADDRINUSE  ] = "Address already in use",
	[ECONNABORTED] = "Connection aborted",
	[ENETUNREACH ] = "Network is unreachable",
	[ENETDOWN    ] = "Network interface is not configured",
	[ETIMEDOUT   ] = "Connection timed out",
	[EHOSTDOWN   ] = "Host is down",
	[EHOSTUNREACH] = "Host is unreachable",
	[EINPROGRESS ] = "Connection already in progress",
	[EALREADY    ] = "Socket already connected",
	[EDESTADDRREQ] = "Destination address required",
	[EMSGSIZE    ] = "Message too long",
	[EPROTONOSUPPORT] = "Unknown protocol",
	[ESOCKTNOSUPPORT] = "Socket type not supported",
	[EADDRNOTAVAIL] = "Address not available",
	[ENETRESET    ] = "",
	[EISCONN      ] = "Socket is already connected",
	[ENOTCONN     ] = "Socket is not connected",
	[ETOOMANYREFS ] = "",
	[EPROCLIM     ] = "",
	[EUSERS       ] = "",
	[EDQUOT       ] = "",
	[ESTALE       ] = "",
	[ENOTSUP      ] = "Not supported",
	[EILSEQ       ] = "",
	[EOVERFLOW    ] = "Value too large for defined data type",
	[ECANCELED    ] = "Operation canceled",
	[ENOTRECOVERABLE] = "State not recoverable",
	[EOWNERDEAD]  = "Previous owner died",
	[ESTRPIPE]    = "Streams pipe error",
	[ERESTARTSYS] = "Syscall requires restart" 
};

void perror(const char *str) {
	if(str) {
		fprintf(stderr, "%s: %s\n", str, __errno_values[errno]);
	} else {
		fprintf(stderr, "%s\n", __errno_values[errno]);
	}
}
