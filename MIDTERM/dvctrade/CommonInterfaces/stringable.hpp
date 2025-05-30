/*
 * Stringable.hpp
 *
 *  Created on: Aug 13, 2015
 *      Author: archit
 */

#ifndef DVCCODE_COMMONINTERFACES_STRINGABLE_HPP_
#define DVCCODE_COMMONINTERFACES_STRINGABLE_HPP_

namespace HFSAT {

class Stringable {
 public:
  virtual std::string ToString() const = 0;
  virtual ~Stringable(){};
};

} /* namespace HFSAT */
#endif /* DVCCODE_COMMONINTERFACES_STRINGABLE_HPP_ */
