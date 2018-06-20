#include <stdlib.h>
#include <stdio.h>

int main(int argc, char const *argv[])
{
    if (putenv("ABC=123") < 0) {
        perror("Error put env");
    }else {
        printf("Success put env\n");
    }

    if (getenv("ABC")) {
        printf("a env is just add: %s\n", getenv("ABC"));
    }else {
        printf("Error getenv\n");
    }

    return 0;
}