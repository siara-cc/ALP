#include <string>
#include <fstream>
#include <stdlib.h>
#include <iostream>

#include "include/alp.hpp"

class alp_test {
public:
	double* intput_buf {};
	double* exception_buf {};
	double* decoded_buf {};
	double* sample_buf {};
	double* glue_buf {};

	uint16_t* rd_exc_arr {};
	uint16_t* pos_arr {};
	uint16_t* exc_c_arr {};
	int64_t*  ffor_buf {};
	int64_t*  unffor_arr {};
	int64_t*  base_buf {};
	int64_t*  encoded_buf {};

	uint64_t* ffor_right_buf {};
	uint16_t* ffor_left_arr {};
	uint64_t* right_buf {};
	uint16_t* left_arr {};
	uint64_t* unffor_right_buf {};
	uint16_t* unffor_left_arr {};

	alp::bw_t bit_width {};

	void init(size_t tuples_count) {
		intput_buf    = new double[tuples_count];
		sample_buf    = new double[tuples_count];
		exception_buf = new double[tuples_count];
		decoded_buf   = new double[tuples_count];
		glue_buf      = new double[tuples_count];
		//

		right_buf        = new uint64_t[tuples_count];
		ffor_right_buf   = new uint64_t[tuples_count];
		unffor_right_buf = new uint64_t[tuples_count];

		//
		encoded_buf = new int64_t[tuples_count];
		base_buf    = new int64_t[tuples_count];
		ffor_buf    = new int64_t[tuples_count];

		//
		rd_exc_arr      = new uint16_t[tuples_count];
		pos_arr         = new uint16_t[tuples_count];
		exc_c_arr       = new uint16_t[tuples_count];
		unffor_arr      = new int64_t[tuples_count];
		left_arr        = new uint16_t[tuples_count];
		ffor_left_arr   = new uint16_t[tuples_count];
		unffor_left_arr = new uint16_t[tuples_count];
	}

	~alp_test() {
		delete[] intput_buf;
		delete[] sample_buf;
		delete[] exception_buf;
		delete[] rd_exc_arr;
		delete[] pos_arr;
		delete[] encoded_buf;
		delete[] decoded_buf;
		delete[] exc_c_arr;
		delete[] ffor_buf;
		delete[] unffor_arr;
		delete[] base_buf;
		delete[] right_buf;
		delete[] left_arr;
		delete[] unffor_right_buf;
		delete[] unffor_left_arr;
	}

  template <typename PT>
	void test_column(std::string csv_file_path) {
		using UT = typename alp::inner_t<PT>::ut;
		using ST = typename alp::inner_t<PT>::st;

		auto* input_arr        = reinterpret_cast<PT*>(intput_buf);
		auto* sample_arr       = reinterpret_cast<PT*>(sample_buf);
		auto* right_arr        = reinterpret_cast<UT*>(right_buf);
		auto* ffor_right_arr   = reinterpret_cast<UT*>(ffor_right_buf);
		auto* unffor_right_arr = reinterpret_cast<UT*>(unffor_right_buf);
		auto* glue_arr         = reinterpret_cast<PT*>(glue_buf);
		auto* exc_arr          = reinterpret_cast<PT*>(exception_buf);
		auto* dec_dbl_arr      = reinterpret_cast<PT*>(decoded_buf);
		auto* encoded_arr      = reinterpret_cast<ST*>(encoded_buf);
		auto* base_arr         = reinterpret_cast<ST*>(base_buf);
		auto* ffor_arr         = reinterpret_cast<ST*>(ffor_buf);

    FILE *fp = fopen(csv_file_path.c_str(), "rb");
		if (!fp) { throw std::runtime_error(csv_file_path + " : " + strerror(errno)); }

		alp::state<PT> stt;
		size_t         tuples_count {alp::config::VECTOR_SIZE};
		size_t         rowgroup_offset {0};

		double      value_to_encode;
		char val_str[50];
    char *end_ptr;
		// keep storing values from the text file so long as data exists:
		size_t row_idx {0};
		while (fgets(val_str, sizeof(val_str), fp) != nullptr) {
      //printf("row: %lu, %s -> ", row_idx, val_str);
			if constexpr (std::is_same_v<PT, double>) {
				value_to_encode = strtod(val_str, &end_ptr);
			} else {
				value_to_encode = atof(val_str);
			}
      //printf("%lf\n", value_to_encode);

			input_arr[row_idx] = value_to_encode;
			row_idx += 1;
		}
		while (row_idx%1024) {
			input_arr[row_idx] = value_to_encode;
			row_idx++;
		}
		tuples_count = row_idx;

		// Init
		alp::encoder<PT>::init(input_arr, rowgroup_offset, tuples_count, sample_arr, stt);

		switch (stt.scheme) {
		case alp::Scheme::ALP_RD: {
			alp::rd_encoder<PT>::init(input_arr, rowgroup_offset, tuples_count, sample_arr, stt);

			alp::rd_encoder<PT>::encode(input_arr, rd_exc_arr, pos_arr, exc_c_arr, right_arr, left_arr, stt);
			ffor::ffor(right_arr, ffor_right_arr, stt.right_bit_width, &stt.right_for_base);
			ffor::ffor(left_arr, ffor_left_arr, stt.left_bit_width, &stt.left_for_base);

			// Decode
			unffor::unffor(ffor_right_arr, unffor_right_arr, stt.right_bit_width, &stt.right_for_base);
			unffor::unffor(ffor_left_arr, unffor_left_arr, stt.left_bit_width, &stt.left_for_base);
			alp::rd_encoder<PT>::decode(
			    glue_arr, unffor_right_arr, unffor_left_arr, rd_exc_arr, pos_arr, exc_c_arr, stt);

			for (size_t i = 0; i < alp::config::VECTOR_SIZE; ++i) {
				auto l = input_arr[i];
				auto r = glue_arr[i];
				if (l != r) { std::cout << i << " | " << i << " r : " << r << " l : " << l << '\n'; }
				//test::ALP_ASSERT(r, l);
			}
      printf("Completed ALR_RD\n");

			break;
		}
		case alp::Scheme::ALP: {
			// Encode
			alp::encoder<PT>::encode(input_arr, exc_arr, pos_arr, exc_c_arr, encoded_arr, stt);
			alp::encoder<PT>::analyze_ffor(encoded_arr, bit_width, base_arr);
			ffor::ffor(encoded_arr, ffor_arr, bit_width, base_arr);

			size_t encoded_size = bit_width * tuples_count / 64;
			for (size_t i = encoded_size; i < tuples_count / 64; ++i) {
				encoded_arr[i] = 0;
			}

			printf("bw: %d, input size: %lu, output size: %lu\n", bit_width, tuples_count*8, encoded_size * 8);

			// Decode
			generated::falp::fallback::scalar::falp(ffor_arr, dec_dbl_arr, bit_width, base_arr, stt.fac, stt.exp);
			alp::decoder<PT>::patch_exceptions(dec_dbl_arr, exc_arr, pos_arr, exc_c_arr);

			// validation
			auto exceptions_count = exc_c_arr[0];
			for (size_t i = 0; i < tuples_count; ++i) {
				if (input_arr[i] != dec_dbl_arr[i]) {
          printf("Not matching : %lu, %lf, %lf\n", i, input_arr[i], dec_dbl_arr[i]);
        }
			}

      printf("Completed ALP\n");

			// ASSERT_EQ(column.exceptions_count, exceptions_count);
			// ASSERT_EQ(column.bit_width, bit_width);
		}
		default:;
		}

		std::cout << "\033[32m-- " << "column_name" << '\n';

		fclose(fp);
	}
};

int main(int argc, char *argv[]) {
  alp_test at;
	size_t row_idx {0};
  char val_str[50];
  FILE *fp = fopen(argv[1], "rb");
	while (fgets(val_str, sizeof(val_str), fp) != nullptr) {
		row_idx += 1;
	}
	fclose(fp);
	while (row_idx%1024)
		row_idx++;
  at.init(row_idx);
  at.test_column<double>(std::string(argv[1]));
}
