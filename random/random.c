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
static int cur_dif = 0;

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
    // sleep for 10 microseconds
    usleep(10);
    gettimeofday(&stop, NULL);

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
 * Function name: print
 *
 * Parameters: N - the number of line to print
 *
 * Returns: void 
 *
 * Description: print out N binary digits to the screen
 *
 *===========================================================================*/
void print(int N)
{
    int i;
   
    int num_zero = 0;
    int num_one = 0;
 
    for (i = 0; i < N; i++)
    {
        int dif = gen_rand();
        if (dif > 0)
            num_one++;
        else
            num_zero++;
        printf("%d", dif);

        if (i % 80 == 0)
            printf("\n");
    }

    printf("number of ones: %d\tnumber of zeros: %d\n", num_one, num_zero);
}


/*=============================================================================
 *
 * Function: itoa
 *
 * Parameters: int n
 *
 * Returns: char* - the string representation of the integer i passed in.
 *
 * description: this function generates the string representation of the int
 *              it is passed.
 *
 *===========================================================================*/
char* itoa(int n)
{
    int num_chars = 0;
    if (n > 0)
        num_chars = ceil(log(n));
    char* result = malloc(num_chars + 1);

    sprintf(result, "%d", n);

    return result;
}

unsigned long factorial(int n)
{
    unsigned long result = 1L;
    while (n > 0)
    {
        result *= (n--);
    }

    return result;
}

unsigned long choose(int n, int k)
{
    return factorial(n)/(factorial(k)*factorial(n - k));
}

double binProb(int n, int k)
{
    return choose(n,k)*pow(0.5,n);
}

double calcChiI(int expected, int got)
{
    double res = 0.0;
    if (expected != 0)
        res = pow((expected - got),2)/expected;

    return res;
}

/*=============================================================================
 *
 * Function: genBins
 *
 * Parameters: int N            - the number of runs to do
 *             char* filename   - the file to write out the values to
 *
 * Returns: char* - the string representation of the integer i passed in.
 *
 * description: this function generates the string representation of the int
 *              it is passed.
 *
 *===========================================================================*/
double chiSquared(int N, const char* filename)
{
    // Pick a number of binary digits to test with
    int num_bits = 20;

    // generate the expected values for each bin
    int* expected_vals = malloc(sizeof(int)*num_bits + 1);
    // Setup the bins where we will need (the number of bins will be num_bits)
    int* bins = malloc(sizeof(int)*num_bits + 1);

    int i;
    double sum = 0.0;
    for (i = 0; i <= num_bits; i++)
    {
        expected_vals[i] = (int)round((N*binProb(num_bits,i)));
        bins[i] = 0;
    }

    // Take N passes
    for (i = 0; i < N; i++)
    {
        /* on each pass, sum num_bits randomly generated bits and increment
         * the appropriate bin. */
        int j;
        int sum = 0;
        for (j = 0; j < num_bits; j++)
            sum += gen_rand();

        bins[sum]++;
    }

    double chi_squared = 0;
    // print out the results
    for (i = 0; i <= num_bits; i++)
    {
        double tmp = calcChiI(expected_vals[i],bins[i]);
        //printf("%d, exp %d, got %d\n",i,expected_vals[i],bins[i]);
        chi_squared += tmp;
    }

    return chi_squared;
}

/*=============================================================================
 *
 * Function name: fileWrite
 *
 * Parameters: int N                - the number of random numbers to write out
 *             condt char* filename - the name of the file to write to
 *
 * Returns: void
 *
 * Description: Write out N binary digits to the specified file.
 *
 *===========================================================================*/
void fileWrite(int N, const char* filename)
{
    FILE* output = fopen(filename, "w");

    char* buf = malloc(sizeof(char)*4096);

    if (output == NULL)
    {
        printf("Error: %d\n", errno);
        return;
    }

    int i;

    int buf_index = 1;
    for (i = 0; i < N; i++)
    {
        int dif = gen_rand();
        int sum = 0;
        int j;
        for (j = 1; j <= 20; j++)
            sum += gen_rand();
        char* tmp = itoa(sum);

        int length = strlen(tmp);
        int index = 0;
        while (index < length)
        {
            if (buf_index % 4096 == 0)
            {
                fwrite(buf, 4096, 1, output); 
                buf_index = 0;
            }
        
            buf[buf_index++] = tmp[index++];
        }
        buf[buf_index++] = ',';
        free(tmp);
    }

        if (buf_index % 4096 != 0)
        {
           fwrite(buf, (buf_index + 1), 1, output); 
        }

    free(buf);

    fclose(output);
}

void frequency(int N)
{
    int num_1 = 0;
    int num_0 = 0;

    for (int i = 0; i < N; i++)
        (gen_rand() == 1) ? num_1++ : num_0++;

    double freq_1 = 100*(num_1 + 0.0)/N;
    double freq_0 = 100*(num_0 + 0.0)/N;

    printf("Statistics\n");
    printf("-------------------------------------------\n");
    printf("number of 1's: %d\tnumber of 0's: %d\tN: %d\n", num_1,num_0,N);
    printf("frequency of 1's: %f percent \tfrequency of 0's: %f percent\n",
           freq_1,freq_0);
}

void writeBits(int N, char* filename)
{
    FILE* output = fopen(filename, "w");

    // create a buffer to hold 51 lines worth of text
    char* buf = malloc(sizeof(char)*4080);

    int i;
}

void threePer(int N)
{
    // total number of permutations
    int permutations = 8;

    // probability for each possible case
    double probability = 0.125;

    // for a given N, generate the expected counts for each permutation
    double expected_counts[8]; 
    for (int i = 0; i < 8; i++)
        expected_counts[i] = probability*N;

    // provide the possible permutations
    char* three_permutations[8] = {"000",
                                   "001", 
                                   "010", 
                                   "011", 
                                   "100", 
                                   "101",
                                   "110",
                                   "111"};

    // create an array to hold ten runs of the permutation counts for this N
    int perm_counts[10][8];

    // create a string to hold the current permutation
    char* current_per = malloc(4*sizeof(char));

    for (int a = 0; a < 10; a++)
    {
        for (int i = 0; i < N; i++)
        {
            // create the new permutation
            for (int j = 0; j < 3; j++)
            {
                current_per[j] = ((gen_rand() == 0) ? '0' : '1');
            }
            current_per[3] = '\0';
            // update the correct count
            for (int k = 0; k < 8; k++)
            {
                if (strcmp(three_permutations[k], current_per) == 0)
                    perm_counts[a][k]++;
            }
        }
    }
    free(current_per);

    // Run a chi-squared analysis on our data
    double chi_squared_values[10];

    for (int i = 0; i < 10; i++)
    {
        double local_chi_squared = 0.0;
        for (int j = 0; j < 8; j++)
        {
            local_chi_squared += calcChiI(expected_counts[j],
                                          perm_counts[i][j]);
        }
        chi_squared_values[i] = local_chi_squared;
    }

    for (int i = 0; i < 10; i++)
    {
        printf("%d %f\n",N, chi_squared_values[i]);
    }
}

int main(int ac, char** av)
{
    if (ac >= 2)
    {
        int N = atoi(av[1]);
        if (ac == 3)
        {
            char* test = av[2];

            if (strcmp(test, "chi") == 0)
            {
                int i;
                for(i = 0; i < 10; i++)
                {
                    double sum = 0;
                    sum = chiSquared(N, test);
                    printf("Thechi^2 over %d runs is %f\n",N,sum);
                }
            }
            else if (strcmp(test,"freq") == 0)
                frequency(N);
            else if (strcmp(test,"perm") == 0)
                threePer(N);
            else
                fileWrite(N, test);    
        }
        else
            print(N);
    }
    else
        printf("Program usage: ./random N [filename]\n");
}
