#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <errno.h>

FILE _stdin  = {.fd = 0};
FILE _stdout = {.fd = 1};
FILE _stderr = {.fd = 2};

FILE* stdin  = &_stdin;
FILE* stdout = &_stdout;
FILE* stderr = &_stderr;

void __init_stdio() {
	stdin->read_buffer  = malloc(BUFSIZE);

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
		uint32_t result = __sys_write(stream->fd, stream->write_ptr, (uint32_t) stream->write_buffer);
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
  uint32_t fd = __sys_open((uint32_t) path, __mode_to_options(mode), 0);
  if(!fd) {
	  return NULL;
  }
  return fdopen(fd, mode);
}

int fclose(FILE * stream){
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
	uint32_t read = 0;
	char* buf = (char*) ptr;
	if(!stream->read_buffer) {
		return __sys_read(stream->fd, len, (uint32_t) buf);
	}
	while(len > 0) {
		if(!stream->available) {
			if(stream->read_buffer_offset == stream->buffer_size) {
				stream->read_buffer_offset = 0;
			}

			stream->real_offset = fseek(stream, 0, SEEK_CUR);
			int32_t r = __sys_read(stream->fd, stream->buffer_size - stream->read_buffer_offset, 
					(uint32_t) &stream->read_buffer[stream->read_buffer_offset]);

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
		read++;
	}
	return read;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE* stream){
	uint32_t len = size * nmemb;
	uint32_t written = len;
	char* buf = (char*) ptr;
	if(!stream->write_buffer) {
		return __sys_write(stream->fd, len, (uint32_t) buf);
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
