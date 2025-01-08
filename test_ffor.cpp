#include <time.h>
#include <string>
#include <fstream>
#include <stdlib.h>
#include <iostream>
#include <inttypes.h>

#include "include/alp.hpp"

double time_taken_in_secs(struct timespec t) {
  struct timespec t_end;
  clock_gettime(CLOCK_REALTIME, &t_end);
  return (t_end.tv_sec - t.tv_sec) + (t_end.tv_nsec - t.tv_nsec) / 1e9;
}

struct timespec print_time_taken(struct timespec t, const char *msg) {
  double time_taken = time_taken_in_secs(t); // in seconds
  printf("%s %lf\n", msg, time_taken);
  clock_gettime(CLOCK_REALTIME, &t);
  return t;
}

class ffor_test {
public:

  template <typename PT>
	size_t encode_block(PT *input_arr, PT *encoded_arr, PT *base_arr) {

		// Init
		// alp::encoder<PT>::init(input_arr, rowgroup_offset, tuples_count, sample_arr, stt);
    // alp::encoder<PT>::analyze_ffor(input_arr, bit_width, base_arr);
		int32_t min = INT32_MAX;
		for (size_t i = 0; i < alp::config::VECTOR_SIZE; ++i) {
			if (input_arr[i] < min)
				min = input_arr[i];
		}
		size_t bit_width = 0;
		for (size_t i = 0; i < alp::config::VECTOR_SIZE; ++i) {
			size_t cur_bw = 1;
			if (input_arr[i] - min > 0)
				cur_bw = 32 - __builtin_clz(input_arr[i] - min);
			//printf("cur_bw: %lu\n", cur_bw);
			if (cur_bw > bit_width)
				bit_width = cur_bw;
		}
		if (bit_width > 32)
			bit_width = 32;
		if (bit_width == 0)
			bit_width = 32;
		// printf("Bit width: %lu\n", bit_width);
		*base_arr = min;
		ffor::ffor(input_arr, encoded_arr, bit_width, base_arr);

		// size_t encoded_size = bit_width * 1024 / 64;
		return bit_width;
	}
};

int main(int argc, char *argv[]) {

	FILE *fp = fopen(argv[1], "rb");
	if (!fp) { throw std::runtime_error(std::string(argv[1]) + " : " + strerror(errno)); }

	char val_str[50];
	size_t tuples_count {0};
	while (fgets(val_str, sizeof(val_str), fp) != nullptr) {
		tuples_count++;
	}
	while (tuples_count % 1024)
		tuples_count++;
	rewind(fp);

	int32_t *input_arr = new int32_t[tuples_count];
	int32_t *base_arr = new int32_t[tuples_count];
	size_t *bit_widths = new size_t[tuples_count];
	int32_t *encoded_arr = new int32_t[tuples_count];
	int32_t *decoded_arr = new int32_t[tuples_count];
	int32_t value_to_encode;
	char *end_ptr;
	// keep storing values from the text file so long as data exists:
	size_t row_idx {0};
	bool is_neg = false;
	while (fgets(val_str, sizeof(val_str), fp) != nullptr) {
		//printf("row: %lu, %s -> ", row_idx, val_str);
		value_to_encode = atoll(val_str);
		if (value_to_encode < 0)
			is_neg = true;
		input_arr[row_idx] = value_to_encode;
		// printf("%" PRId64 "\n", input_arr[row_idx]);
		row_idx += 1;
	}
	while (row_idx % 1024)
		input_arr[row_idx++] = 0;
	if (is_neg) {
		printf("negative\n");
		for (size_t i = 0; i < tuples_count; ++i) {
			if (input_arr[i] > 0) {
				input_arr[i] <<= 1;
			} else {
				input_arr[i] = -input_arr[i];
				input_arr[i] <<= 1;
				input_arr[i]++;
			}
			// input_arr[i] = (input_arr[i] << 1) | (input_arr[i] >> 31);
			//printf("%" PRId64 "\n", input_arr[i]);
		}
	}

  ffor_test ft;
	size_t block_count = tuples_count / 1024;

  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);

	for (size_t i = 0; i < block_count; i++) {
	  bit_widths[i] = ft.encode_block<int32_t>(input_arr + i * 1024, encoded_arr + i * 1024, base_arr + i * 1024);
	}
  printf("\nNumbers per sec: %lf\n", tuples_count / time_taken_in_secs(t) / 1000);
  t = print_time_taken(t, "Time taken for encode: ");

	size_t encoded_size = 0;
	for (size_t i = 0; i < block_count; i++) {
		unffor::unffor(encoded_arr + i * 1024, decoded_arr + i * 1024, bit_widths[i], base_arr + i * 1024);
	}
  printf("\nNumbers per sec: %lf\n", tuples_count / time_taken_in_secs(t) / 1000);
  t = print_time_taken(t, "Time taken for decode: ");

	// verification
	for (size_t i = 0; i < tuples_count; i++) {
		encoded_size += bit_widths[i] * 1024 / 8;
		if (input_arr[i] != decoded_arr[i]) {
			printf("Not matching : %lu, %lld, %lld\n", i, input_arr[i], decoded_arr[i]);
		}
	}

  printf("Completed ALP: %lu\n", encoded_size);

  fclose(fp);

}
