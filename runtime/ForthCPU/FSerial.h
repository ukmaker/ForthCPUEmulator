#ifndef UKMAKER_FSERIAL_H
#define UKMAKER_FSERIAL_H
#include <stdio.h>
#include <sys/ioctl.h>
#ifndef ARDUINO


class FSerial {

    public:
    
    int getc() {
        int c = getchar();
        return c;
    }

    int putc(char c) {
        return putchar(c);
    }
    
    void printf(const char *format, int value) {
        ::printf(format, value);
    }

    void println(const char *str) {
        ::printf("%s\n", str);
    }

    void flush() {
        // no op
    }

    void print(char c) {
        putchar(c);
    }

    void print(const char* s) {
        ::printf(s, 0);
    }

    int read() {
        return getchar();
    }

    bool available() {
        int n;
        return (ioctl(0, FIONREAD, &n) == 0 && n > 0);
    }

    int readBytesUntil(char separator, char *buf, size_t len) {
        size_t num_read = 0;
        while (num_read < len) {
            int c = getc();
            if (c < 0 || c == separator) break;
            *buf++ = (char)c;
            num_read++;
        }
        return num_read;
    }
};

FSerial Serial;

#endif
#endif