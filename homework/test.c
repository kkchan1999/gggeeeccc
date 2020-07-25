#include "pool.h"

int main(int argc, char const* argv[])
{
    time_t t;
    struct tm* tm_local;
    time(&t);
    tm_local = localtime(&t);
    time_t t1 = mktime(tm_local);
    //printf("localtime=%d:%d:%d\n", tm_local->tm_hour, tm_local->tm_min, tm_local->tm_sec);
    //printf("%s", asctime(gmtime(&t)));

    sleep(1);
    time(&t);
    tm_local = localtime(&t);
    time_t t2 = mktime(tm_local);

    printf("%ld\n", t2 - t1);
    return 0;
}
