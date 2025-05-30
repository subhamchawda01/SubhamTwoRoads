/*
 * book_interface.hpp
 *
 *  Created on: 05-Jun-2014
 *      Author: archit
 */

#ifndef BOOKINTERFACE_HPP_
#define BOOKINTERFACE_HPP_

#include "baseinfra/MarketAdapter/market_defines.hpp"

namespace HFSAT {

class BookInterface {
 public:
  virtual double bestbid_price() const = 0;
  virtual double bestask_price() const = 0;

  virtual double bestbid_size() const = 0;
  virtual double bestask_size() const = 0;

  virtual double price_from_type(const std::string pricetype_) const = 0;

  virtual inline std::string PriceType_t_To_String_Extended_(const PriceType_t t_price_type_) const {
    return PriceType_t_To_String(t_price_type_);
  }

  virtual inline double price_from_type(const PriceType_t t_price_type_) const {
    return price_from_type(PriceType_t_To_String_Extended_(t_price_type_));
  }

  virtual void ShowBook() const = 0;

  virtual void PrintBook() const {
    // to print on screen, can be changed by derived class
    ShowBook();
  }

  virtual ~BookInterface() {}
};

} /* namespace HFSAT */

#endif /* BOOKINTERFACE_HPP_ */
