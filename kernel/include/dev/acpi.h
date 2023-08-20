#ifndef __K_DEV_ACPI_H
#define __K_DEV_ACPI_H

#include "kernel.h"

void     k_dev_acpi_set_addr(void* buff);
void     k_dev_acpi_reboot();
K_STATUS k_dev_acpi_init();

#endif
