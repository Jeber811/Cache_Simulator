#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        printf("Usage: ./SIM <CACHE_SIZE> <ASSOC> <REPLACEMENT> <WB> <TRACE_FILE>\n");
        return 1;
    }
    long long int cache_size = atoll(argv[1]);
    int assoc = atoi(argv[2]);
    int replacement = atoi(argv[3]);
    int wb = atoi(argv[4]);
    char *trace_file = argv[5];

    printf("Cache Size: %lld\n", cache_size);
    printf("Associativity: %d\n", assoc);
    return 0;
}