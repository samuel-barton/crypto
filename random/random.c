#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <fcntl.h>    // File control definitions 
#include <errno.h>    // Error number definitions 
#include <termios.h>  // POSIX terminal control definitions 
#include <string.h>   // String function definitions 

/*=============================================================================
 *
 * Program name: random.c
 *
 * Project: COS 498 - random sequence generator
 *
 * Date: January 2017
 * 
 * Created by: Samuel Barton
 *
 * Description: This program will contain the code needed to read, process, and
 *              output a random sequence of numbers whose randomness is
 *              based on the random nature of the sources being polled.
 *
 *              Below are some proposed random sources
 *
 *              Digital sources:
 *                  - discrepencies in system clock √
 *                  - number of network packets in and out per second
 *                  - key latency
 *
 *              Analog sources:
 *                  - temp fluctuation
 *                  - ambient noise in room
 *
 *              Of these sources the one which is currently working is the 
 *              one based on the CPU clock slippage. The largest limitaiton on
 *              this particular system is the time it takes to generate the 
 *              random sequence when using a 100 microsecond wait time. 
 *
 *===========================================================================*/

static int fd = 0;
static float cur_temp = 0;
static int cur_dif = 0;


/*=============================================================================
 *
 * Function name: serialport_init
 *
 * Parameters: const char* serialport - the name of the port we connect to
 *             int baud - connection speed
 *
 * Returns: int - file descriptor representing serial port connection
 *
 * Description: This method establishes the connection to the arduino's serial
 *              port. 
 *
 *              Both this method, and the serialport_read_until() method came
 *              from https://github.com/todbot/arduino-serial.
 *
 *===========================================================================*/
int serialport_init(const char* serialport, int baud)
{
    struct termios toptions;
    int fd;
    
    fd = open(serialport, O_RDWR | O_NONBLOCK );
    
    if (fd == -1)  {
        perror("serialport_init: Unable to open port ");
        return -1;
    }

    if (tcgetattr(fd, &toptions) < 0) {
        perror("serialport_init: Couldn't get term attributes");
        return -1;
    }
    speed_t brate = B9600; // let you override switch below if needed
    cfsetispeed(&toptions, brate);
    cfsetospeed(&toptions, brate);

    // 8N1
    toptions.c_cflag &= ~PARENB;
    toptions.c_cflag &= ~CSTOPB;
    toptions.c_cflag &= ~CSIZE;
    toptions.c_cflag |= CS8;
    // no flow control
    toptions.c_cflag &= ~CRTSCTS;

    toptions.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
    toptions.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

    toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
    toptions.c_oflag &= ~OPOST; // make raw

    // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
    toptions.c_cc[VMIN]  = 0;
    toptions.c_cc[VTIME] = 0;
    
    tcsetattr(fd, TCSANOW, &toptions);
    if( tcsetattr(fd, TCSAFLUSH, &toptions) < 0) {
        perror("init_serialport: Couldn't set term attributes");
        return -1;
    }

    return fd;
}

/*=============================================================================
 *
 * Function name: serialport_read_until
 *
 * Parameters: int fd - the file descriptor of the serial port we read from
 *             char* buf - the buffer we read into
 *             char until - stop character
 *             int buf_max - the size of the buffer
 *             int timeout - the max wait time for a response
 *
 * Returns: int - status
 *
 * Description: This function handles reading in data from the serial console
 *              of the arduino.
 *
 *===========================================================================*/
int serialport_read_until(int fd, char* buf, char until, int buf_max, int timeout)
{
    char b[1];  // read expects an array, so we give it a 1-byte array
    int i=0;
    do {
        int n = read(fd, b, 1);  // read a char at a time
        if( n==-1) return -1;    // couldn't read
        if( n==0 ) {
            usleep(1);  // wait 1 msec try again
            timeout--;
            if( timeout==0 ) return -2;
            continue;
        }
#ifdef SERIALPORTDEBUG  
        printf("serialport_read_until: i=%d, n=%d b='%c'\n",i,n,b[0]); // debug
#endif
        buf[i] = b[0];
        i++;
    } while( b[0] != until && i < buf_max && timeout>0 );

    buf[i] = 0;  // null terminate the string
    return 0;
}

/*=============================================================================
 *
 * Function name: time_dif
 *
 * Parameters: none
 *
 * Returns: int (0 if same time as before, 1 otherwise)
 *
 * Description: This method generates a random sequence of bits by calculating
 *              the actual time elapsed when the CPU waits for 10 microseconds.
 *              If the time passed on the current call to time_dif() is 
 *              different from the last call, then return 1, otherwise return 0
 *
 *===========================================================================*/
int time_dif()
{
    struct timeval start, stop;

    gettimeofday(&start, NULL);
    // sleep for half a second
    usleep(10);
    gettimeofday(&stop, NULL);

    /* the "average" time taken by the system to complete the sleep operation
     * is 145 microseconds. So comparing the sleep time to the 
     * average gives us a somewhat even and seemingly random stream of ones and
     * zeroes. */
    int tmp = stop.tv_usec - start.tv_usec;
    if (cur_dif == tmp)
        return 0;
    else
    {
        cur_dif = tmp;
        return 1;
    }
}


/*=============================================================================
 *
 * Function name: gen_rand
 *
 * Parameters: none
 *
 * Returns: int - random 0 or 1
 *
 * Description: This function generates a random sequence of bits by taking
 *              the exclusive or on the time slippage in the cpu and the 
 *              pseudorandom sequence generator provided by POSIX. This 
 *              sequence is truly random as even though one of the functions
 *              is periodic, the other is not and thus predetermining the 
 *              result of the XOR of the two is impossible. This can generate
 *              100,000 bits per second, or 100KB/s.
 *
 *===========================================================================*/
int gen_rand()
{
    return time_dif() ^ (random()&01);
}

/*=============================================================================
 *
 * Function name: temp
 *
 * Parameters: none
 *
 * Returns: int (0 if same temp as before, 1 otherwise)
 *
 * Description: This method is used to create a random sequence of bits. It 
 * gets the current temperature from an arduino, and compares it to the last 
 * temperature it received. If the temps are the same, then the function will 
 * return a zero. Otherwise, it will return a 1. Since there is no way to 
 * predetermine the temperature fluctuation, there is no way to get a seed for
 * the sequence.
 *
 * This function uses library code for accessing the arduino's serial console.
 * see the function headers for the attribution.
 *
 *===========================================================================*/
int temp()
{
    if (fd == 0)   
        fd = serialport_init("/dev/cu.usbmodemFD121", 9600);

    char buffer[32];

    int n = serialport_read_until(fd, buffer, '\n', sizeof(buffer), 5e6);

    if (n < 0)
        fputs("read failed!\n", stderr);

    float tmp = atof(buffer);
    if (cur_temp == tmp)
        return 0;
    else
    {
        cur_temp = tmp;
        return 1;
    } 
}

int main(int ac, char** av)
{
    printf("clock slip in microseconds.\n");

    int i;
   
    int num_zero = 0;
    int num_one = 0;
 
    for (i = 0; i < 10000; i++)
    {
        int j;
        for (j = 0; j < 80; j++)
        {
            int dif = gen_rand();
            if (dif > 0)
                num_one++;
            else
                num_zero++;
            printf("%d", dif);
        }

        printf("\n");
    }
    printf("number of ones: %d\tnumber of zeros: %d\n", num_one, num_zero);
/*
    printf("Temp\n");

    int num_zero = 0;
    int num_one = 0;
    int i,j;
    for (i = 0; i < 100; i++)
    {
        for (j = 0; j < 80; j++) 
        {
            int t = temp();
            if (t > 0)
                num_one++;
            else
                num_zero++;
            printf("%d", t);
        }
    
        printf("\n");
    }
    printf("number of ones: %d\tnumber of zeros: %d\n", num_one, num_zero);
*/
}
