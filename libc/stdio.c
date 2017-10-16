#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys_api.h>

void iputs(const char *str);
char *_itoa(int, char *, int, int);
char *_ltoa(long, char *, int, int);
//char *_lltoa(long long, char *, int, int);

FILE file_list[128];
int files_ptr = 0;
char pool[4096];
int pool_ptr = 0;

int _sscanf_calls = 0;
int sscanf(const char *str, const char *format, ...) {

    fputs("libc: `sscanf` is just a stub function.\n", stderr);
    fprintf(stderr, "libc: %d calls to sscanf so far.\n", ++_sscanf_calls);

    return -1;
}

int ungetc(int c, FILE* stream) {
    long orig_pos = ftell(stream);

    if (stream->stream_end == -1) return EOF;
    if (!stream->stream_ptr) return EOF;
    
    fseek(stream, -1, SEEK_CUR);
    if (fputc(c, stream) == EOF) {
        fseek(stream, orig_pos, SEEK_SET);
        return EOF;
    }
    fseek(stream, -1, SEEK_CUR);
    
    return c;
}

int ferror(FILE* stream) {
    return 0;
}

int fflush(FILE* stream) {
    return 0;
}

int feof(FILE* stream) {

    if (stream->stream_ptr == stream->stream_end)
        return EOF;

    return 0;

}

char* fgets(char* buf, int limit, FILE* stream) {
    int i;
    for (i = 0; i < (limit - 1); i++) {
        int c = fgetc(stream);
        if (c == '\n') break;
        buf[i] = c;
    }
    buf[i] = 0;
    return buf;
}

void perror(const char* errmsg) {

    fputs(errmsg, stderr);
    
    return;

}

static size_t _fwrite_writememb(const char* memb_ptr, size_t size, FILE* stream) {
    size_t i;

    for (i = 0; i < size; i++)
        fputc(memb_ptr[i], stream);

    return i;
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
    // this function is a stub
    size_t i;

    for (i = 0; i < nmemb; i++)
        _fwrite_writememb((char*)(ptr + i * size), size, stream);
    
    return i;
}

static size_t _fread_readmemb(char* memb_ptr, size_t size, FILE* stream) {
    size_t i;

    for (i = 0; i < size; i++)
        memb_ptr[i] = fgetc(stream);

    return i;
}

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    // this function is a stub
    size_t i;

    for (i = 0; i < nmemb; i++)
        _fread_readmemb((char*)(ptr + i * size), size, stream);
    
    return i;
}

static char* _getline_growbuf(char* buf, size_t* n) {
    char* new_ptr = realloc(buf, *n + 256);
    
    if (!new_ptr) return (char*)0;
    
    *n += 256;
    
    return new_ptr;
}

ssize_t getline(char** lineptr, size_t* n, FILE* stream) {
    ssize_t i;
    
    int c = fgetc(stream);
    
    if (c == EOF) return -1;
    
    if ((!*lineptr) && (!*n)) {
        *lineptr = _getline_growbuf(*lineptr, n);
        if (!*lineptr) return -1;
    }
    
    for (i = 0; ; i++) {
        if (i == (*n - 2)) {
            char* tmp_ptr = _getline_growbuf(*lineptr, n);
            if (!tmp_ptr) return -1;
            *lineptr = tmp_ptr;
        }
        if (c == EOF) goto clean_exit;
        (*lineptr)[i] = (char)c;
        if (c == '\n') goto clean_exit;
        c = fgetc(stream);
    }

clean_exit:
    i++;
    (*lineptr)[i] = 0;
    return i;

}

int remove(const char* filename) {
    if (OS_vfs_remove(filename) == VFS_FAILURE) return -1;
    else return 0;
}

FILE* fopen(const char* path, const char* mode) {
    vfs_metadata_t metadata;

    // use malloc (TODO)
    //FILE* file_ptr = malloc(sizeof(FILE));
    FILE* file_ptr = &file_list[files_ptr++];
    
    if (!strcmp(mode, "r") || !strcmp(mode, "rb") || !strcmp(mode, "rt") ||
        !strcmp(mode, "r+") || !strcmp(mode, "r+b") || !strcmp(mode, "rb+") || !strcmp(mode, "r+t") || !strcmp(mode, "rt+")) {
        strcpy(file_ptr->mode, mode);
        // fetch metadata
        if (OS_vfs_get_metadata(path, &metadata, VFS_FILE_TYPE) == VFS_FAILURE) {
            if (OS_vfs_get_metadata(path, &metadata, VFS_DEVICE_TYPE) != VFS_FAILURE) {
                if (metadata.size)
                    file_ptr->stream_end = metadata.size;
                else
                    file_ptr->stream_end = -1;
            } else goto fail;
        } else
            file_ptr->stream_end = metadata.size;

        file_ptr->stream_ptr = 0;
        //file_ptr->path = malloc(strlen(path) + 1);
        file_ptr->path = &pool[pool_ptr];
        pool_ptr += strlen(path) + 1;
        strcpy(file_ptr->path, path);
        file_ptr->stream_begin = 0;
    
        return file_ptr;
    }
    else if (!strcmp(mode, "w") || !strcmp(mode, "wb") || !strcmp(mode, "wt") ||
             !strcmp(mode, "w+") || !strcmp(mode, "w+b") || !strcmp(mode, "wb+") || !strcmp(mode, "w+t") || !strcmp(mode, "wt+")) {
        strcpy(file_ptr->mode, mode);
        // fetch metadata
        if (OS_vfs_get_metadata(path, &metadata, VFS_FILE_TYPE) == VFS_FAILURE) {
            /* the file doesn't exist, create it */
            
            /* attempt to create file */
            if (OS_vfs_create(path, 0))
                goto fail;
        } else {
            /* the file does exist, remove it and recreate it */
            if (remove(path))
                goto fail;
            
            /* attempt to create file */
            if (OS_vfs_create(path, 0))
                goto fail;
        }
        
        file_ptr->stream_end = 0;
        file_ptr->stream_ptr = 0;
        //file_ptr->path = malloc(strlen(path) + 1);
        file_ptr->path = &pool[pool_ptr];
        pool_ptr += strlen(path) + 1;
        strcpy(file_ptr->path, path);
        file_ptr->stream_begin = 0;
    
        return file_ptr;
    }
    else if (!strcmp(mode, "a") || !strcmp(mode, "ab") || !strcmp(mode, "at") ||
             !strcmp(mode, "a+") || !strcmp(mode, "a+b") || !strcmp(mode, "ab+") || !strcmp(mode, "a+t") || !strcmp(mode, "at+")) {
        strcpy(file_ptr->mode, mode);
        // fetch metadata
        if (OS_vfs_get_metadata(path, &metadata, VFS_FILE_TYPE) == VFS_FAILURE) {
            /* the file doesn't exist, create it */
            
            /* attempt to create file */
            if (OS_vfs_create(path, 0))
                goto fail;
            
            file_ptr->stream_end = 0;
            file_ptr->stream_ptr = 0;
            file_ptr->stream_begin = 0;
        } else {
            file_ptr->stream_end = metadata.size;
            file_ptr->stream_ptr = metadata.size;
            file_ptr->stream_begin = metadata.size;
        }
        
        //file_ptr->path = malloc(strlen(path) + 1);
        file_ptr->path = &pool[pool_ptr];
        pool_ptr += strlen(path) + 1;
        strcpy(file_ptr->path, path);
    
        return file_ptr;
    } else {
        fprintf(stderr, "libc: fopen runtime error: invalid mode `%s`.\n", mode);
        goto fail;
    }
    
fail:
    // free memory
    return (FILE*)0;
        
}

int fclose(FILE* stream) {
    //free(stream);
    return 0;
}

int fseek(FILE* stream, long int offset, int type) {
    switch (type) {
        case SEEK_SET:
            if (stream->stream_end == -1) return -1;
            if ((stream->stream_begin + offset) > stream->stream_end ||
                (stream->stream_begin + offset) < stream->stream_begin) return -1;
            stream->stream_ptr = stream->stream_begin + offset;
            return 0;
        case SEEK_END:
            if (stream->stream_end == -1) return -1;
            if ((stream->stream_end + offset) > stream->stream_end ||
                (stream->stream_end + offset) < stream->stream_begin) return -1;
            stream->stream_ptr = stream->stream_end + offset;
            return 0;
        case SEEK_CUR:
            if (stream->stream_end == -1) return -1;
            if ((stream->stream_ptr + offset) > stream->stream_end ||
                (stream->stream_ptr + offset) < stream->stream_begin) return -1;
            stream->stream_ptr += offset;
            return 0;
        default:
            fputs("libc: fseek runtime error: invalid mode.\n", stderr);
            return -1;
    }
}

int fgetc(FILE* stream) {
    if (!strcmp(stream->mode, "w") || !strcmp(stream->mode, "wb") || !strcmp(stream->mode, "wt") ||
        !strcmp(stream->mode, "a") || !strcmp(stream->mode, "ab") || !strcmp(stream->mode, "at")) return -1;
    if (stream->stream_ptr == stream->stream_end) return EOF;
    int c = OS_vfs_read(stream->path, stream->stream_ptr);
    stream->stream_ptr++;
    return c;
}

int getc(FILE* stream) {
    return fgetc(stream);
}

int fputc(int c, FILE* stream) {
    if (!strcmp(stream->mode, "r") || !strcmp(stream->mode, "rb") || !strcmp(stream->mode, "rt")) return -1;
    int ret = OS_vfs_write(stream->path, stream->stream_ptr, c);
    if (ret == VFS_FAILURE) return -1;
    stream->stream_ptr++;
    if (stream->stream_end < stream->stream_ptr)
        stream->stream_end = stream->stream_ptr;
    return c;
}

int putc(int c, FILE* stream) {
    return fputc(c, stream);
}

long int ftell(FILE* stream) {
    return stream->stream_ptr;
}

void rewind(FILE* stream) {
    stream->stream_ptr = stream->stream_begin;
    return;
}

int fputs(const char* str, FILE* stream) {
    int i;
    
    for (i = 0; str[i]; i++)
        if (fputc(str[i], stream) == -1) return -1;
    
    return i;
}

char stdin_path[] = "/dev/stdin";
char stdout_path[] = "/dev/stdout";
char stderr_path[] = "/dev/stderr";

FILE stdin_struct = {
    stdin_path,
    "r+",
    0,
    0,
    -1
};

FILE stdout_struct = {
    stdout_path,
    "r+",
    0,
    0,
    -1
};

FILE stderr_struct = {
    stderr_path,
    "r+",
    0,
    0,
    -1
};

FILE* stdin = &stdin_struct;
FILE* stdout = &stdout_struct;
FILE* stderr = &stderr_struct;

int putchar(int c) {
    return fputc(c, stdout);
}

int getchar(void) {
    return fgetc(stdin);
}

int puts(const char* str) {
    int len;
    if ((len = fputs(str, stdout)) == -1) return -1;
    if (fputc('\n', stdout) == -1) return -1;
    return len + 1;
}

// internal puts with no newline and no return value
void iputs(const char *str) {
    int i;
    for (i=0; str[i]!=0; i++) {
        putchar(str[i]);
    }
    return;
}

#define MOD_NONE 0
#define MOD_BYTE 1
#define MOD_SHORT 2
#define MOD_LONG 3
#define MOD_LONG_LONG 4

#define PAD_RIGHT 1
#define PAD_ZERO 2

#define PRINTF_BUF_MAX 4096

static char printf_buf[PRINTF_BUF_MAX];

int printf(const char* format, ...) {
    va_list args;
    int ret;
    
    va_start(args, format);
    ret = vsnprintf(printf_buf, PRINTF_BUF_MAX, format, args);
    va_end(args);
    
    if (ret == -1) return -1;
    fputs(printf_buf, stdout);
    return ret;
}

int fprintf(FILE* stream, const char* format, ...) {
    va_list args;
    int ret;
    
    va_start(args, format);
    ret = vsnprintf(printf_buf, PRINTF_BUF_MAX, format, args);
    va_end(args);
    
    if (ret == -1) return -1;
    fputs(printf_buf, stream);
    return ret;
}

int vfprintf(FILE* stream, const char* format, va_list args) {
    int ret = vsnprintf(printf_buf, PRINTF_BUF_MAX, format, args);
    
    if (ret == -1) return -1;
    fputs(printf_buf, stream);
    return ret;
}

int sprintf(char* buf, const char* format, ...) {
    va_list args;
    int ret;
    
    va_start(args, format);
    ret = vsnprintf(buf, 0x7fffffff, format, args);
    va_end(args);
    
    return ret;
}

static size_t __PRI_max;
static char* __PRI_buf;
static int __PRI_ptr;

static int __PRI_putc(int c) {
    __PRI_buf[__PRI_ptr++] = c;
    if (__PRI_ptr == __PRI_max) return -1;
    return c;
}

static int __PRI_puts(const char* str) {
    int len = strlen(str);
    if ((__PRI_ptr + len) >= __PRI_max) return -1;
    strcpy(&__PRI_buf[__PRI_ptr], str);
    __PRI_ptr += len;
    return len;
}

int vsnprintf(char* t_buf, size_t t_max, const char* format, va_list args)
{
    int width, pad, fp_width, mod, sign, count = 0;
    __PRI_ptr = 0;
    __PRI_buf = t_buf;
    __PRI_max = t_max;
    
    memset(t_buf, 0, t_max);

    for ( ; *format; format++ )
    {
        if ( *format == '%' )
        {
            ++format;
            width = pad = mod = sign = 0;
            fp_width = -1;

            if ( *format == '\0' ) break;
            if ( *format == '%' ) goto out;
            if ( *format == 'n' )
            {
                *va_arg(args, int *) = count;
                continue;
            }

            if ( *format == '-' )
            {
                format++;
                pad = PAD_RIGHT;

                if ( *format == '+' )
                {
                    format++;
                    sign = 1;
                }
            }

            else if ( *format == '+' )
            {
                format++;
                sign = 1;

                if ( *format == '-' )
                {
                    format++;
                    pad = PAD_RIGHT;
                }
            }

            while ( *format == '0' )
            {
                format++;
                pad |= PAD_ZERO;
            }

            if ( *format == '*' )
            {
                format++;
                width = va_arg(args, int);
                width = width > 0 ? width : 0;
            }

            else
            {
                for ( ; *format >= '0' && *format <= '9'; format++ )
                {
                    width *= 10;
                    width += *format - '0';
                }
            }

            if ( *format == '.' )
            {
                format++;

                if( *format == '*' )
                {
                    fp_width = va_arg(args, int);
                    format++;
                }
                else if ( isdigit(*format) )
                {
                    fp_width = 0;

                    for ( ; *format >= '0' && *format <= '9'; format++ )
                    {
                        fp_width *= 10;
                        fp_width += *format - '0';
                    }
                }
            }

            if ( *format == 'h' && *(format+1) == 'h' )
            {
                int i = 0;

                switch ( tolower(*(format+2)) )
                {
                    case 'o':
                    case 'd':
                    case 'i':
                    case 'x':
                    case 'u':
                        mod = MOD_BYTE;
                        format += 2;
                        break;
                    default:
                        while ( format[--i] != '%' );

                        for( ; i < 3 ; i++ )
                            if (__PRI_putc(format[i]) == -1) return -1;
                            //putchar(format[i]);
//                        text_putstring(format[i], (format + 3) - &format[i]);

                        format += 3;
                        continue;
                }
            }

            else if ( *format == 'h' )
            {
                int i = 0;

                switch ( tolower(*(format+1)) )
                {
                    case 'o':
                    case 'd':
                    case 'i':
                    case 'x':
                    case 'u':
                        mod = MOD_SHORT;
                        format++;
                        break;
                    default:
                        while ( format[--i] != '%' );

                        for( ; i < 2 ; i++ )
                            if (__PRI_putc(format[i]) == -1) return -1;
                            //putchar(format[i]);
//                        text_putstring(format[i], (format + 2) - &format[i]);

                        format += 2;
                        continue;
                }
            }
/*            else if ( strncmp(format, "ll", 2) == 0 )
            {
                int i = 0;

                switch ( tolower(*(format+1)) )
                {
                    case 'd':
                    case 'i':
                    case 'x':
                    case 'u':
                        mod = MOD_LONG_LONG;
                        format++;
                        break;
                    default:
                        while ( format[--i] != '%' );

                        for( ; i < 3 ; i++ )
                            putchar(format[i]);
//                        text_putstring(format[i], (format + 3) - &format[i]);

                        format += 3;
                        continue;
                }
            }*/
            else if ( *format == 'l' )
            {
                int i = 0;

                switch ( tolower(*(format+1)) )
                {
                    case 'o':
                    case 'd':
                    case 'i':
                    case 'x':
                    case 'u':
                    case 'f':
                        mod = MOD_LONG;
                        format++;
                        break;
                    default:
                        while ( format[--i] != '%' );

                        for( ; i < 3 ; i++ )
                            if (__PRI_putc(format[i]) == -1) return -1;
                            //putchar(format[i]);
//                        text_putstring(format[i], (format + 2) - &format[i]);

                        format += 2;
                        continue;
                }
            }

            //done
            if ( *format == 's' )
            {
                char *s = va_arg(args, char *);
                int length = strlen(s);

                if ( width > 0 )
                {
                    count += width;
                    if ( !(pad & PAD_RIGHT) )
                    {
                        if ( length > width )
                        {
                            for ( int i = 0; i < width; i++ )
                                if (__PRI_putc(s[i]) == -1) return -1;
                                //putchar(s[i]);
                        }
                        else
                        {
                            for ( int i = width - length; i; i-- )
                                if (__PRI_putc(' ') == -1) return -1;
                                //putchar(' ');

                            if (__PRI_puts(s) == -1) return -1;
                            //iputs(s);
                        }
                    }
                    else
                    {
                        for ( ; width--;  )
                        {
                            if (*s)
                                if (__PRI_putc(*s++) == -1) return -1;
                                //putchar(*s++)
                            else
                                if (__PRI_putc(' ') == -1) return -1;
                                //putchar(' ');
                        }
                    }
                }
                else
                {
                    if (__PRI_puts(s) == -1) return -1;
                    //iputs(s);
                    count += length;
                }

                continue;
            }

            if ( *format == 'o' )
            {
                char buf[23];
                char padchar = pad & PAD_ZERO ? '0' : ' ';
                uint64_t n;
                int _len;

                switch(mod)
                {
                    case MOD_BYTE:
                        //n = va_arg(args, unsigned char);
                        n = va_arg(args, unsigned int);
                        break;

                    case MOD_SHORT:
                        //n = va_arg(args, unsigned short);
                        n = va_arg(args, unsigned int);
                        break;

                    case MOD_NONE:
                        n = va_arg(args, unsigned int);
                        break;

                    case MOD_LONG:
                        n = va_arg(args, unsigned long);
                        break;

//                    case MOD_LONG_LONG:
//                        n = va_arg(args, unsigned long long);
                }

                _len = strlen(ltoa(n, buf, 8));
                width -= _len;

                count += _len + (width > 0 ? width : 0);

                if ( (width > 0) && !(pad & PAD_RIGHT) )
                    for ( ; width > 0; width-- ) {
                        if (__PRI_putc(padchar) == -1) return -1;
                        count++;
                        //putchar(padchar), count++;
                    }

                if (__PRI_puts(buf) == -1) return -1;
                //iputs(buf);

                if ( pad & PAD_RIGHT )
                    for ( ; width > 0; width-- )
                        if (__PRI_putc(' ') == -1) return -1;
                        //putchar(' ');

                continue;
            }

            //done
            if ( *format == 'd' || *format == 'i' )
            {
                char buf[21];
                char padchar = pad & PAD_ZERO ? '0' : ' ';
                uint64_t n;
                int _len;

                switch(mod)
                {
                    case MOD_BYTE:
                        //n = va_arg(args, unsigned char);
                        n = va_arg(args, unsigned int);
                        break;

                    case MOD_SHORT:
                        //n = va_arg(args, unsigned short);
                        n = va_arg(args, unsigned int);
                        break;

                    case MOD_NONE:
                        n = va_arg(args, unsigned int);
                        break;

                    case MOD_LONG:
                        n = va_arg(args, unsigned long);
                        break;

//                    case MOD_LONG_LONG:
//                        n = va_arg(args, unsigned long long);
                }

                _len = strlen(ltoa(n, buf, 10));
                width -= _len;
                int i = 0;

                if ( sign && buf[0] != '-' ) {
                    if (__PRI_putc('+') == -1) return -1;
                    //putchar('+');
                    width--;
                    count++;
                }

                else if ( buf[0] == '-' )
                    if (__PRI_putc(i++) == -1) return -1;
                    //putchar(buf[i++]);


                count += _len + (width > 0 ? width : 0);

                if ( (width > 0) && !(pad & PAD_RIGHT) )
                {
                    for ( ; width > 0; width-- ) {
                        if (__PRI_putc(padchar) == -1) return -1;
                        //putchar(padchar);
                        count++;
                    }

//                    if ( sign && buf[0] != '-' && padchar == ' ')
//                        putchar('+');
                }

                if (__PRI_puts(&buf[i]) == -1) return -1;
                //iputs(&buf[i]);

                if ( pad & PAD_RIGHT )
                    for ( ; width > 0; width-- )
                        if (__PRI_putc(' ') == -1) return -1;
                        //putchar(' ');

                continue;
            }

            //done
            if ( *format == 'x' )
            {
                char buf[16];
                char padchar = pad & PAD_ZERO ? '0' : ' ';
                uint64_t n;
                int _len;

                switch(mod)
                {
                    case MOD_BYTE:
                        //n = (uint8_t)va_arg(args, unsigned char);
                        n = (uint8_t)va_arg(args, unsigned int);
                        break;

                    case MOD_SHORT:
                        //n = (uint16_t)va_arg(args, unsigned short);
                        n = (uint16_t)va_arg(args, unsigned int);
                        break;

                    case MOD_NONE:
                        n = va_arg(args, unsigned int);
                        break;

                    case MOD_LONG:
                        n = va_arg(args, unsigned long);
                        break;

//                    case MOD_LONG_LONG:
//                        n = va_arg(args, unsigned long long);
                }

                _len = strlen(ltoa(n, buf, 16));
                width -= _len;

                count += _len + (width > 0 ? width : 0);

                if ( (width > 0) && !(pad & PAD_RIGHT) )
                    for ( ; width > 0; width-- ) {
                        if (__PRI_putc(padchar) == -1) return -1;
                        //putchar(padchar);
                        count++;
                    }

                if (__PRI_puts(buf) == -1) return -1;
                //iputs(buf);

                if ( pad & PAD_RIGHT )
                    for ( ; width > 0; width-- )
                        if (__PRI_putc(' ') == -1) return -1;
                        //putchar(' ');

                continue;
            }

            //done
            if ( *format == 'X' )
            {
                char buf[16];
                char padchar = pad & PAD_ZERO ? '0' : ' ';
                uint64_t n;
                int _len;

                switch(mod)
                {
                    case MOD_BYTE:
                        //n = va_arg(args, unsigned char);
                        n = va_arg(args, unsigned int);
                        break;

                    case MOD_SHORT:
                        //n = va_arg(args, unsigned short);
                        n = va_arg(args, unsigned int);
                        break;

                    case MOD_NONE:
                        n = va_arg(args, unsigned int);
                        break;

                    case MOD_LONG:
                        n = va_arg(args, unsigned long);
                        break;

//                    case MOD_LONG_LONG:
//                        n = va_arg(args, unsigned long long);
                }

                _len = strlen(ltoa(n, buf, 16));
                width -= _len;

                count += _len + (width > 0 ? width : 0);

                if ( (width > 0) && !(pad & PAD_RIGHT) )
                    for ( ; width > 0; width-- ) 
                        if (__PRI_putc(padchar) == -1) return -1;
                        //putchar(padchar);

                for ( int i = 0; buf[i] = toupper(buf[i]); i++ );

                if (__PRI_puts(buf) == -1) return -1;
                //iputs(buf);

                if ( pad & PAD_RIGHT )
                    for ( ; width > 0; width-- )
                        if (__PRI_putc(' ') == -1) return -1;
                        //putchar(' ');
                continue;
            }

            //done
            if ( *format == 'u' )
            {
                char buf[21], padchar;
                uint64_t n;

                switch ( mod )
                {
                    case MOD_BYTE:
                        //n = va_arg(args, unsigned char);
                        n = va_arg(args, unsigned int);
                        break;

                    case MOD_SHORT:
                        //n = va_arg(args, short);
                        n = va_arg(args, int);
                        break;

                    case MOD_NONE:
                        n = va_arg(args, unsigned int);
                        break;

                    case MOD_LONG:
                        n = va_arg(args, unsigned long);
                        break;

//                    case MOD_LONG_LONG:
//                        n = va_arg(args, unsigned long long);
                }

                int _len = strlen(_ltoa(n, buf, 10, 0));

                count += _len;
                if ( width > 0 )
                {
                    width -= _len;
                    count += width > 0 ? width : 0;
                    padchar = pad & PAD_ZERO ? '0' : ' ';

                    if ( !(pad & PAD_RIGHT) )
                    {
                        for ( ; width > 0; width-- )
                            if (__PRI_putc(padchar) == -1) return -1;
                            //putchar(padchar);
                    }
                }

                if (__PRI_puts(buf) == -1) return -1;
                //iputs(buf);

                if ( pad & PAD_RIGHT )
                    for ( ; width > 0; width-- )
                        if (__PRI_putc(' ') == -1) return -1;
                        //putchar(' ');

                continue;
            }

            //done
            if ( *format == 'c' )
            {
                /* char are converted to int when pushed on the stack */
                if (__PRI_putc((char)va_arg(args, int)) == -1) return -1;
                //putchar((char)va_arg(args, int));
                count++;
                continue;
            }
            
            //in progress
            if ( tolower(*format) == 'f' )
            {
                double n;
                int len;
                char padchar;

                if ( mod & MOD_LONG )
                {
                    n = va_arg(args, double);
                    fp_width = fp_width > -1 ? fp_width : 16;
                }
                else
                {
                    //n = va_arg(args, float);
                    n = va_arg(args, double);
                    fp_width = fp_width > -1 ? fp_width : 8;
                }

                if ( n < 1e+30  && n > -1e+29 )
                    len = 50;
                else if ( n < 1e+80 && n > -1e+79 )
                    len = 100;
                else if ( n < 1e+130 && n > -1e+129 )
                    len = 150;
                else if ( n < 1e+180 && n > -1e+179 )
                    len = 200;
                else if ( n < 1e+230 && n > -1e+229 )
                    len = 250;
                else
                    len = 328;

                char buf[len];

                int _len = strlen(dtoa(n, buf, fp_width));
                width -= _len;
                count += _len;

                if ( sign && buf[0] != '-' ) {
                    if (__PRI_putc('+') == -1) return -1;
                    //putchar('+');
                    width--;
                    count++;
                }

                count += width;

                padchar = pad & PAD_ZERO ? '0' : ' ';

                if ( width > 0 )
                {
                    int i = 0;

                    // put sign if negative
                    if ( buf[0] == '-' )
                        if (__PRI_putc(buf[i++]) == -1) return -1;
                        //putchar(buf[i++]);

                    // pad left
                    if ( !(pad & PAD_RIGHT) )
                        while ( width-- )
                            if (__PRI_putc(padchar) == -1) return -1;
                            //putchar(padchar);

                    // print float
                    if (__PRI_puts(&buf[i]) == -1) return -1;
                    //iputs(&buf[i]);

                    // pad right
                    if ( pad & PAD_RIGHT )
                        while ( width-- )
                            if (__PRI_putc(' ') == -1) return -1;
                            //putchar(' ');

                }
                else
                    if (__PRI_puts(buf) == -1) return -1;
                    //iputs(buf);
            }

/*            if ( tolower(*format) == 'g' )
            {
                //
            }

            if ( tolower(*format) == 'a' )
            {
                //
            }*/

        }

        else
        {
        out:
            if (__PRI_putc(*format) == -1) return -1;
            //putchar(*format);
            count++;
        }
    }

    //va_end(args);
    return count;
}
