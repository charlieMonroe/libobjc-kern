
#ifndef modular_runtime_testing_common_h
#define modular_runtime_testing_common_h

#define TEST_ITERATIONS 128
#define WARMUP_ITERATIONS 4

clock_t test_times[TEST_ITERATIONS];

static clock_t min_time(void){
	int i;
	clock_t min = (clock_t)(-1);
	for (i = 0; i < TEST_ITERATIONS; ++i){
		if (test_times[i] < min){
			min = test_times[i];
		}
	}
	return min;
}

static clock_t max_time(void){
	int i;
	clock_t max = 0.0;
	for (i = 0; i < TEST_ITERATIONS; ++i){
		if (test_times[i] > max){
			max = test_times[i];
		}
	}
	return max;
}

static clock_t avarage_time(void){
	int i;
	clock_t sum = 0.0;
	for (i = 0; i < TEST_ITERATIONS; ++i){
		sum += test_times[i];
	}
	return sum / (clock_t)TEST_ITERATIONS;
}

static clock_t median_time(void){
	/**
	 * No intelligent sorting mechanism
	 * is used since it is just 128 items.
	 */
	int i;
	
	clock_t test_times_copy[TEST_ITERATIONS];
	clock_t test_times_sorted[TEST_ITERATIONS];
	
	for (i = 0; i < TEST_ITERATIONS; ++i){
		test_times_copy[i] = test_times[i];
	}
	
	for (i = 0; i < TEST_ITERATIONS; ++i){
		int o;
		int min_index;
		clock_t min = (clock_t)(-1);
		for (o = 0; o < TEST_ITERATIONS; ++o){
			if (test_times_copy[o] < min){
				min = test_times_copy[o];
				min_index = o;
			}
		}
		
		test_times_copy[min_index] = (clock_t)(-1);
		
		test_times_sorted[i] = min;
	}
	
	if (TEST_ITERATIONS % 2 == 0){
		/* Even number */
		return (test_times_sorted[TEST_ITERATIONS / 2] + test_times_sorted[(TEST_ITERATIONS / 2) + 1]) / 2;
	}
	
	return test_times_sorted[TEST_ITERATIONS / 2];
}

static void perform_tests(clock_t(*testing_function)(void)){
	int i;
	
	/** Warm-up */
	for (i = 0; i < WARMUP_ITERATIONS; ++i){
		testing_function();
	}
	
	for (i = 0; i < TEST_ITERATIONS; ++i){
		test_times[i] = testing_function();
	}
	
	printf("Min: %li\n", min_time());
	printf("Max: %li\n", max_time());
	printf("Average: %li\n", avarage_time());
	printf("Median: %li\n", median_time());
}


#endif
