#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

// Constants for the cache configuration
int NUM_SETS;         /* Configure number of sets */
int ASSOC;            /* Configure cache associativity */
int REPLACEMENT;      /* 0 for LRU, 1 for FIFO */
int WB;               /* 0 for write-through, 1 for write-back */
#define BLOCK_SIZE 64 // 64 bytes as block size

// Simulation statistics
int Hit = 0;
int Miss = 0;
int Reads = 0;
int Writes = 0;

// Cache data structures
long long int **tag_array;    // 2D array for tags
long long int **lru_position; // 2D array for LRU positions
bool **dirty;                 // 2D array for dirty bits (if write-back is used)

// Forward declarations
void Update_lru(char op, long long int add);
void Update_fifo(char op, long long int add);
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
    WB = atoi(argv[4]);
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

    // ---------- Free allocated memory ----------
    for (int i = 0; i < NUM_SETS; i++)
    {
        free(tag_array[i]);
        free(lru_position[i]);
        free(dirty[i]);
    }
    free(tag_array);
    free(lru_position);
    free(dirty);

    // Print statistics;
    printf("Miss ratio: %.6f\n", (float)Miss / (Hit + Miss) * 100);
    printf("write: %d\n", Writes);
    printf("read: %d\n", Reads);

    return 0;
}

// Cache access simulation
void Simulate_access(char op, long long int add)
{
    int set = (add / BLOCK_SIZE) % NUM_SETS;
    long long int tag = (add / BLOCK_SIZE) / NUM_SETS;

    int hit_flag = 0;

    // ---------- CHECK FOR HIT ----------
    for (int i = 0; i < ASSOC; i++)
    {
        if (tag == tag_array[set][i])
        {
            // Cache hit
            Hit++;
            hit_flag = 1;

            // Write handling
            if (op == 'W')
            {
                if (WB == 1)
                    dirty[set][i] = true; // write-back: mark dirty
                else
                    Writes++; // write-through: write to memory immediately
            }

            // Update replacement policy metadata
            if (REPLACEMENT == 0)
                Update_lru(op, add); // LRU
            else
                Update_fifo(op, add); // FIFO

            break; // Stop after hit
        }
    }

    // ---------- CACHE MISS ----------
    if (!hit_flag)
    {
        Miss++;

        if (REPLACEMENT == 0)
            Update_lru(op, add); // LRU handles inserting new block, dirty bit, reads/writes
        else
            Update_fifo(op, add); // FIFO handles inserting new block, dirty bit, reads/writes
    }
}

void Update_lru(char op, long long int add)
{
    int set = (add / BLOCK_SIZE) % NUM_SETS;
    long long int tag = (add / BLOCK_SIZE) / NUM_SETS;

    int hit_way = -1;

    // Search for a hit
    for (int i = 0; i < ASSOC; i++)
    {
        if (tag_array[set][i] == tag)
        {
            hit_way = i;
            break;
        }
    }

    // ---------- CASE 1: CACHE HIT ----------
    if (hit_way != -1)
    {
        if (op == 'W')
        {
            if (WB == 1)
            {
                // Write-back: mark block dirty, defer memory write
                dirty[set][hit_way] = true;
            }
            else
            {
                // Write-through: write immediately to memory
                Writes++;
            }
        }

        // Update LRU order
        int old_lru = lru_position[set][hit_way];
        for (int i = 0; i < ASSOC; i++)
            if (lru_position[set][i] < old_lru)
                lru_position[set][i]++;
        lru_position[set][hit_way] = 0;
        return;
    }

    // ---------- CASE 2: CACHE MISS ----------
    Miss++;

    // Find LRU victim (the one with largest LRU value)
    int victim = 0;
    for (int i = 0; i < ASSOC; i++)
    {
        if (lru_position[set][i] == ASSOC - 1)
        {
            victim = i;
            break;
        }
    }

    // If victim is dirty and we're using write-back, write it back first
    if (WB == 1 && dirty[set][victim])
    {
        Writes++;
        dirty[set][victim] = false;
    }

    // Fetch new block from memory
    Reads++;

    // Install new block
    tag_array[set][victim] = tag;

    if (op == 'W')
    {
        if (WB == 1)
        {
            // Write-back: mark dirty, no immediate memory write
            dirty[set][victim] = true;
        }
        else
        {
            // Write-through: write new data to memory right away
            Writes++;
            dirty[set][victim] = false; // stays clean
        }
    }
    else
    {
        dirty[set][victim] = false; // read miss always brings in clean block
    }

    // Update LRU positions
    for (int i = 0; i < ASSOC; i++)
        if (lru_position[set][i] < ASSOC - 1)
            lru_position[set][i]++;
    lru_position[set][victim] = 0;
}

void Update_fifo(char op, long long int add)
{
    int set = (add / BLOCK_SIZE) % NUM_SETS;
    long long int tag = (add / BLOCK_SIZE) / NUM_SETS;

    int hit_way = -1;

    // ---------- SEARCH FOR HIT ----------
    for (int i = 0; i < ASSOC; i++)
    {
        if (tag_array[set][i] == tag)
        {
            hit_way = i;
            break;
        }
    }

    // ---------- CASE 1: CACHE HIT ----------
    if (hit_way != -1)
    {
        if (op == 'W')
        {
            if (WB == 1)
            {
                // Write-back: mark dirty
                dirty[set][hit_way] = true;
            }
            else
            {
                // Write-through: write directly to memory
                Writes++;
            }
        }

        // FIFO: On hit, insertion order does NOT change
        return;
    }

    // ---------- CASE 2: CACHE MISS ----------
    Miss++;

    // Find the FIFO victim (block with highest insertion age)
    int victim = 0;
    int max_pos = -1;
    for (int i = 0; i < ASSOC; i++)
    {
        if (lru_position[set][i] > max_pos)
        {
            max_pos = lru_position[set][i];
            victim = i;
        }
    }

    // ---------- HANDLE EVICTION ----------
    if (WB == 1 && dirty[set][victim])
    {
        // Dirty victim in write-back → write to memory
        Writes++;
        dirty[set][victim] = false;
    }

    // Always need to read new block from memory
    Reads++;

    // ---------- REPLACE THE VICTIM BLOCK ----------
    tag_array[set][victim] = tag;

    if (op == 'W')
    {
        if (WB == 1)
        {
            // Write-back: mark dirty
            dirty[set][victim] = true;
        }
        else
        {
            // Write-through: immediately update memory
            Writes++;
            dirty[set][victim] = false;
        }
    }
    else
    {
        // Read miss: block starts clean
        dirty[set][victim] = false;
    }

    // ---------- UPDATE FIFO ORDER ----------
    // Increment all existing positions (blocks get "older")
    for (int i = 0; i < ASSOC; i++)
    {
        if (i != victim)
            lru_position[set][i]++;
    }

    // New block is the "youngest" in FIFO order
    lru_position[set][victim] = 0;
}
