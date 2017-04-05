static int cur_dif = 0;

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

int gen_rand()
{
    return time_dif() ^ (random()&01);
}
