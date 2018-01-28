#ifndef _USB2GPIO_H_
#define _USB2GPIO_H_

#include "usb2gpio.h"
#include "usb_device.h"


class usb2gpio
{
public:
    usb2gpio()
    {

    }

    int init()
    {
        int ret = USB_ScanDevice(DevHandle);
        if(ret <= 0){
            printf("No device connected!\n");
            return 1;
        }
        else{
		    printf("Found Devices:");
            for(int i=0;i<ret;i++){
                printf("%08X ",DevHandle[i]);
		    }
            printf("\n");
            }

        int state = USB_OpenDevice(DevHandle[0]);
        if(!state){
            printf("Open device error!\n");
            return 1;
        }

        GPIO_SetOutput(DevHandle[0],0xFFFF,0);
        GPIO_Write(DevHandle[0], 0xFFFF, 0xFFFF);

        return 0;
    }

    int trigger()
    {
        int ret;
        ret = GPIO_Write(DevHandle[0], 0xFFFF, 0);
        printf("ret1:%d\n", ret);
        usleep(1000*500*2);
        ret = GPIO_Write(DevHandle[0], 0xFFFF, 0xFFFF);
        printf("ret2:%d\n", ret);

        return 0;   
    }


private:
    int DevHandle[10];
};

#endif