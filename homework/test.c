#include <stdio.h>

int main(int argc, char const* argv[])
{
    FILE* f = fopen("./a.txt", "a+");

    fprintf(f, "haha\n");

    fclose(f);
    return 0;
}
