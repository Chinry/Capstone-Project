#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"









#include "fix_fft.h"

#define BUF_SIZE 512


int16_t fr[BUF_SIZE];
int16_t fi[BUF_SIZE];




int main(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
    int i;
    for( i = 0; i < BUF_SIZE; i++)
    {
        fr[i] = (int16_t)(30000 * sin(((2 * M_PI) / 45) * i));
        fi[i] = 0;
    }
    int scale  = fix_fft(fr, fi, 9, 0);
    return 0;
}
