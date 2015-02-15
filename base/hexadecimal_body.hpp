#pragma once

#include <string.h>

#include "base/hexadecimal.hpp"
#include "glog/logging.h"

namespace principia {
namespace base {

static char const kByteToHexadecimalDigits[] = 
    "000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F2021222324"
    "25262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F40414243444546474849"
    "4A4B4C4D4E4F505152535455565758595A5B5C5D5E5F606162636465666768696A6B6C6D6E"
    "6F707172737475767778797A7B7C7D7E7F808182838485868788898A8B8C8D8E8F90919293"
    "9495969798999A9B9C9D9E9FA0A1A2A3A4A5A6A7A8A9AAABACADAEAFB0B1B2B3B4B5B6B7B8"
    "B9BABBBCBDBEBFC0C1C2C3C4C5C6C7C8C9CACBCCCDCECFD0D1D2D3D4D5D6D7D8D9DADBDCDD"
    "DEDFE0E1E2E3E4E5E6E7E8E9EAEBECEDEEEFF0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF";

#if defined(SKIP_48) || defined(SKIP_26) || defined(SKIP_7)
#error SKIP_* already defined
#else
#define SKIP_7 0, 0, 0, 0, 0, 0, 0
#define SKIP_26 SKIP_7, SKIP_7, SKIP_7, 0, 0, 0, 0, 0
#define SKIP_48 SKIP_26, SKIP_7, SKIP_7, SKIP_7, 0

static uint8_t const kHexadecimalDigitsToNibble[256] = {
    SKIP_48, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    SKIP_7, '\xa', '\xb', '\xc', '\xd', '\xe', '\xf',
    SKIP_26, '\xa', '\xb', '\xc', '\xd', '\xe', '\xf'};

#undef SKIP_7
#undef SKIP_26
#undef SKIP_48
#endif


inline void HexadecimalEncode(uint8_t const* input, int64_t const input_size,
                              uint8_t* output, int64_t const output_size) {
  // We iterate backward.
  // |input <= &output[1]| is still valid because we write two bytes of output
  // from reading one byte of input, so output[1] and output[0] are written
  // after reading input[0]. Greater values of |output| would
  // overwrite input data before it is read, unless there is no overlap, i.e.,
  // |&output[input_size << 1] <= input|.
  CHECK(input <= &output[1] || &output[input_size << 1] <= input) << "bad overlap";
  CHECK_GE(output_size, input_size << 1) << "output too small";
  // We want the result to start at |output[0]|;
  output = output + ((input_size - 1) << 1);
  input = input + input_size - 1;
  for (uint8_t const* const input_rend = input - input_size;
       input != input_rend;
       --input, output -= 2) {
    memcpy(output, &kByteToHexadecimalDigits[*input << 1], 2);
  }
}

inline void HexadecimalDecode(uint8_t const* input, int64_t input_size,
                              uint8_t* output, int64_t const output_size) {
  input_size &= ~1;
  // |output <= &input[1]| is still valid because we write one byte of output
  // from reading two bytes of input, so output[0] is written after reading
  // input[0] and input[1]. Greater values of |output| would overwrite input
  // data before it is read, unless there is no overlap, i.e.,
  // |&input[input_size] <= output|.
  CHECK(output <= &input[1] || &input[input_size] <= output) << "bad overlap";
  CHECK_GE(output_size, input_size / 2) << "output too small";
  for (uint8_t const* const input_end = input + input_size; 
       input != input_end;
       input += 2, ++output) {
    *output = (kHexadecimalDigitsToNibble[*input] << 4) |
              kHexadecimalDigitsToNibble[*(input + 1)];
  }
}

}  // namespace base
}  // namespace principia
