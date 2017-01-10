#include <stdio.h>
#include <stdlib.h>

int main(int ac, char** av)
{
    int i;
    for (i = 0; i < 100; i++)
    {
        int j;
        for (j = 0; j < 80; j++)
            printf("%d", ((int)random()&01));
        printf("\n");
    }
}
