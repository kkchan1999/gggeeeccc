#include "pool.h"
#include <stdio.h>
int main(int argc, char const* argv[])
{
    FILE* f = fopen("./staff.txt", "r");
    if (f == NULL) {
        printf("打开失败!\n");
        return -1;
    }
    while (1) {
        staff_info_t* staff = malloc(sizeof(staff_info_t));
        if (read_staff(f, staff) == false) {
            break;
        }

        // add_staff(pool, staff);
    }

    fclose(f);
    return 0;
}
