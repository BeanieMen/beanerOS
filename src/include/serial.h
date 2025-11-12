#ifndef SERIAL_H
#define SERIAL_H

void serial_init(void);
void serial_putchar(char c);
void serial_write(const char* data);
void serial_write_buf(const char* data, unsigned int size);

#endif /* SERIAL_H */
