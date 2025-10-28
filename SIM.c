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

    // Allocate 2D arrays
    tag_array = malloc(NUM_SETS * sizeof(long long int *));
    lru_position = malloc(NUM_SETS * sizeof(long long int *));
    dirty = malloc(NUM_SETS * sizeof(bool *));
    for (int i = 0; i < NUM_SETS; i++)
    {
        tag_array[i] = malloc(ASSOC * sizeof(long long int));
        lru_position[i] = malloc(ASSOC * sizeof(long long int));
        dirty[i] = malloc(ASSOC * sizeof(bool));

        // Initialize arrays
        for (int j = 0; j < ASSOC; j++)
        {
            tag_array[i][j] = -1;   // empty slot
            lru_position[i][j] = j; // initial LRU order
            dirty[i][j] = false;    // clean block
        }
    }

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
                Update_lru(op, add);
            }
            else
            {
                Update_fifo(op, add);
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

void Update_lru(char op, long long int add)
{
    int set = (add / BLOCK_SIZE) % NUM_SETS;
    long long int tag = add / BLOCK_SIZE;

    int hit_way = -1;
    // Find which way was hit
    for (int i = 0; i < ASSOC; i++)
    {
        if (tag_array[set][i] == tag)
        {
            hit_way = i;
            break;
        }
    }

    if (hit_way == -1)
        return; // shouldn't happen on a hit

    // If this is a write, mark the block as dirty
    if (op == 'W')
        dirty[set][hit_way] = true;

    int old_lru = lru_position[set][hit_way];

    // Increment LRU value of all blocks more recently used than this one
    for (int i = 0; i < ASSOC; i++)
    {
        if (lru_position[set][i] < old_lru)
        {
            lru_position[set][i]++;
        }
    }

    // Set this block as most recently used
    lru_position[set][hit_way] = 0;
}

void Update_fifo(char op, long long int add)
{
    int set = (add / BLOCK_SIZE) % NUM_SETS;
    long long int tag = add / BLOCK_SIZE;

    int hit_way = -1;
    // Check if this block is already in cache (hit)
    for (int i = 0; i < ASSOC; i++)
    {
        if (tag_array[set][i] == tag)
        {
            hit_way = i;
            break;
        }
    }

    if (hit_way != -1)
    {
        // FIFO: on a hit, do nothing
        return;
    }

    // Miss: find the block to replace (FIFO = largest lru_position)
    int victim_way = 0;
    int max_pos = -1;
    for (int i = 0; i < ASSOC; i++)
    {
        if (lru_position[set][i] > max_pos)
        {
            max_pos = lru_position[set][i];
            victim_way = i;
        }
    }

    // Insert the new block into victim way
    tag_array[set][victim_way] = tag;
    dirty[set][victim_way] = false; // or true if it's a write

    // Increment all positions to maintain FIFO order
    for (int i = 0; i < ASSOC; i++)
    {
        if (i != victim_way)
            lru_position[set][i]++;
    }

    // Set victim to the "youngest" in FIFO
    lru_position[set][victim_way] = 0;
}
