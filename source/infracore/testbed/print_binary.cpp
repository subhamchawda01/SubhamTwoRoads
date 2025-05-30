#include <iostream>

void print_in_binary_form(const char given_byte) {
  unsigned char bit_mask = 0x80;
  unsigned char one_bit_in_given_byte;

  for (int bit_counter = 0; bit_counter < 8; bit_counter++) {
    one_bit_in_given_byte = given_byte & bit_mask;

    if (one_bit_in_given_byte == 0) {
      std::cout << '0';
    } else {
      std::cout << '1';
    }

    bit_mask = bit_mask >> 1;
  }
}

void print_in_binary_form(const double& test_number) {
  char* byte_in_test_number = (char*)&test_number;

  for (int byte_counter = 0; byte_counter < sizeof(double); byte_counter++) {
    print_in_binary_form(*byte_in_test_number);
    byte_in_test_number++;
  }
}

void print_in_binary_form(const int& _int_) {
  unsigned int bit_mask = 0x80000000;
  unsigned int one_bit_in__int_;

  for (int bit_counter = 0; bit_counter < 32; bit_counter++) {
    one_bit_in__int_ = _int_ & bit_mask;

    if (one_bit_in__int_ == 0) {
      std::cout << '0';
    } else {
      std::cout << '1';
    }

    bit_mask = bit_mask >> 1;
  }
}

void print_in_binary_form(const unsigned int& _unsigned_int_) {
  unsigned int bit_mask = 0x80000000;
  unsigned int one_bit_in__unsigned_int_;

  for (int bit_counter = 0; bit_counter < 32; bit_counter++) {
    one_bit_in__unsigned_int_ = _unsigned_int_ & bit_mask;

    if (one_bit_in__unsigned_int_ == 0) {
      std::cout << '0';
    } else {
      std::cout << '1';
    }

    bit_mask = bit_mask >> 1;
  }
}

int main() {
  double test_double = 2395;
  print_in_binary_form(test_double);
  std::cout << "\n";

  test_double = 2394.99999;
  print_in_binary_form(test_double);
  std::cout << "\n";
  test_double = 2395.00001;
  print_in_binary_form(test_double);
  std::cout << "\n";
  std::cout << "\n";

  int test_int = 12345;
  print_in_binary_form(test_int);
  std::cout << "\n";

  unsigned int test_uint = 12345;
  print_in_binary_form(test_uint);
}
