#include <math.h>
#include <vector>

class abc {
public:
  double val1;
  double val2;
};

int main ( )
{
  std::vector<abc> vec12;
  vec12.resize(100);

  auto factor_ = 1.3;
  auto sum_ = 0.0;
  auto i_ = 0;
  for ( auto & val12 : vec12 ) {
    val12.val1 = pow ( factor_, i_ );
    sum_ += val12.val1;
    i_ ++;
    val12.val2 = sum_;
  }
}
 
