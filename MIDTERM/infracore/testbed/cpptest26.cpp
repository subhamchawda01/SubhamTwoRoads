#include <stdio.h>
#include <iostream>
#include <string>

class ABC {
 private:
  std::string str_;

 protected:
  ABC(std::string& _str_) : str_(_str_) {}

 public:
  static ABC& GetUniqueInstance(std::string& _str_) {
    static ABC uniqueinstance_(_str_);
    return uniqueinstance_;
  }

  const std::string& str() const { return str_; }
};

int main() {
  std::string s1 = "hello";
  ABC& abc1 = ABC::GetUniqueInstance(s1);
  std::cout << "abc1 " << abc1.str() << std::endl;

  std::string s2 = "world";
  ABC& abc2 = ABC::GetUniqueInstance(s2);
  std::cout << "abc2 " << abc2.str() << std::endl;

  return 0;
}
