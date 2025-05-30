#define NUM_WORK_SECONDS 2
#define NUM_FLOPS 20000000
#define NUM_MISSES 2000000
#define NUM_READS 20000
#define SUCCESS 1
#define FAILURE 0
#define MAX_THREADS 256
#define NUM_THREADS 4
#define NUM_ITERS 1000000
#define THRESHOLD 1000000
#define L1_MISS_BUFFER_SIZE_INTS 128 * 1024
#define CACHE_FLUSH_BUFFER_SIZE_INTS 16 * 1024 * 1024
#define TOLERANCE .2
#define OVR_TOLERANCE .75
#define MPX_TOLERANCE .20
#define TIME_LIMIT_IN_US 60 * 1000000 /* Run for about 1 minute or 60000000 us */
