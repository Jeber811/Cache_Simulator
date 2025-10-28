#include <stdio.h>
#include <stdbool.h>

// Constants for the cache configuration
int NUM_SETS;         /* Configure number of sets */
int ASSOC;            /* Configure cache associativity */
int REPLACEMENT;      /* 0 for LRU, 1 for FIFO */
#define BLOCK_SIZE 64 // 64 bytes as block size

// Simulation statistics
int Hit = 0;
int Miss = 0;

// Cache data structures
long long int **tag_array;    // 2D array for tags
long long int **lru_position; // 2D array for LRU positions
bool **dirty;                 // 2D array for dirty bits (if write-back is used)

// Forward declarations
void Update_lru(long long int add);
void Update_fifo(long long int add);
void Simulate_access(char op, long long int add);

// Main function
int main(int argc, char *argv[])
{

    if (argc != 6)
    {
        printf("Usage: ./SIM <CACHE_SIZE> <ASSOC> <REPLACEMENT> <WB> <TRACE_FILE>\n");
        return 1;
    }

    long long int cache_size = atoll(argv[1]);
    ASSOC = atoi(argv[2]);
    REPLACEMENT = atoi(argv[3]);
    int WB = atoi(argv[4]);
    char *trace_file = argv[5];

    NUM_SETS = cache_size / (BLOCK_SIZE * ASSOC);

    char op;
    long long int add;

    FILE *file = fopen(trace_file, "r");
    if (!file)
    {
        printf("Error: Could not open the trace file.\n");
        return 1;
    }

    // Read until end of file
    while (!feof(file))
    {
        if (fscanf(file, " %c %llx", &op, &add))
        {
            Simulate_access(op, add);
        }
    }

    fclose(file);
    // Free allocated memory (not shown here for brevity)

    // Print statistics
    printf("Hits   : %d\n", Hit);
    printf("Misses : %d\n", Miss);
    printf("Hit Rate: %.2f%%\n", (float)Hit / (Hit + Miss) * 100);
    printf("Miss Rate: %.2f%%\n", (float)Miss / (Hit + Miss) * 100);

    // Print the # writes to memory
    // Print the # reads from memory

    return 0;
}

// Cache access simulation
void Simulate_access(char op, long long int add)
{
    int set = (add / BLOCK_SIZE) % NUM_SETS;
    long long int tag = add / BLOCK_SIZE;

    int hit_flag = 0;
    for (int i = 0; i < ASSOC; i++)
    {
        if (tag == tag_array[set][i])
        {
            // Cache hit
            Hit++;
            hit_flag = 1;

            // Choose policy (LRU or FIFO)
            if (REPLACEMENT == 0)
            { // LRU policy is chosen
                Update_lru(add);
            }
            else
            {
                Update_fifo(add);
            }
            break; // Stop searching after a hit
        }
    }

    if (!hit_flag)
    {
        // Cache miss
        Miss++;
        // TODO: Handle miss (bring block into cache, update dirty bit if write-back, etc.)
    }
}

// Policy update functions
void Update_lru(long long int add)
{
    // TODO: Implement LRU update logic
}

void Update_fifo(long long int add)
{
    // TODO: Implement FIFO update logic
}