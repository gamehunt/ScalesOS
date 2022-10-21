#include "dev/serial.h"
void _putchar(char a){
    k_dev_serial_write(a);
}