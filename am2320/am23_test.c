
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

 short *dodata(char *buff)
 {
     char tmp[2] = {0};
     short *ptmp = NULL;
     int i;
     for(i=0; i<2; i++)
        memcpy(tmp+1-i, buff+i, 1);
    ptmp = (short *)tmp;
    return ptmp;
 }

int main (int argc, char **argv)
{
    int fd;
    int i = 0;
    short data_temp = 0,data_hum = 0;
     char result[5];
    float humidity = 0,temperature = 0;

    printf("will open fd... \n");
    if((fd = open("/dev/am2320",O_RDWR|O_NONBLOCK)) < 0 )
    {
        perror("open device fail.\n");
        return -1;
    }
    else printf("Open Device Ds18b20 Successful!!\n");

    while(1)
    {
        int ret;

        printf("\nWill read temperature...\n");
        usleep(100);
        ret = read(fd, result, sizeof(result));
        if(ret != 5)
        {
            printf("read wrong\n");
            exit(0);
        }
        else printf("read success !\n");

        if((buff[0] & buff[1] & buff[2] & buff[3])  != buff [4])
        {
            printf("data error!\n");
            continue;
        }
        data_temp = *dodata(result);
        humidity = data_hum * 0.1;
        temperature = data_temp * 0.1;
        printf("humidity = %.2f ?\n",humidity);
        printf("Temperature = %.2f ?\n",temperature);
        fflush(stdout);
        sleep(1);
    }
    close(fd);
    return 0;
} /* ----- End of main() ----- */
