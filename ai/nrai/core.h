/*************************************************************************
  > File Name:    core.h
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: Thu 12 Oct 2017 02:44:00 PM CST
 ************************************************************************/

#ifndef CORE_H_
#define CORE_H_

#include <iostream>

namespace nrai {

enum BALANCE_TYPE {
  BT_BASE = 0,
  BT_SUM = 1,
  BT_PROD = 2,
  BT_CALC = 3,
  BT_MIX = 128,
  BT_UNKOWN = 255,
};

template <typename T>
class Balance {
 public:
  Balance() {
    type_ = BT_SUM;
    coeff_ = 1;
    x_ = -1;;
    y_ = 1;
  };

  Balance(int coeff, int type) : coeff_(coeff), type_(type) {
    if (type_ == BT_UNKOWN) {
      printf("Not support unkown type, change to sum type!\n");
      type_ = BT_SUM;
      x_ = -1;
      y_ = 1;
    } else if  (BT_SUM == type_) {
      x_ = -1;
      y_ = 1;
    } else if  (BT_PROD == type_) {
      x_ = 1;
      y_ = 1;
    } else if  (BT_CALC == type_) {
      x_ = 1;
      y_ = 1;
    } else {
      type_ = BT_SUM;
      x_ = -1;
      y_ = 1;
    }
  }

  void set_coeff(int coeff) {
    coeff_ = coeff;
  }

  void swap() {
    T temp= x_;
    x_ = y_;
    y_ = temp;
  }

  void born() {
    // initialization
    Balance<T> temp = Balance(coeff_, type_);
    x_ = temp.x();
    y_ = temp.y();
  }

  void die() {
    // reinit
    born();
  }

  void grow() {
    if  (BT_SUM == type_) {
      x_ = x_ - coeff_;
      y_ = y_ + coeff_;
    } else if  (BT_PROD == type_) {
      x_ = x_ / coeff_;
      y_ = y_ * coeff_;
    } else if  (BT_CALC == type_) {
      // calculus of x_, y_
    }
  }

  void shrink() {
    if  (BT_SUM == type_) {
      x_ = x_ + coeff_;
      y_ = y_ - coeff_;
    } else if  (BT_PROD == type_) {
      x_ = x_ * coeff_;
      y_ = y_ / coeff_;
    } else if  (BT_CALC == type_) {
      // calculus of x_, y_
    }
  }

  const T& x() {
    return x_;
  }

  const T& y() {
    return y_;
  }

 private:
  T   x_;
  T   y_;
  int type_;
  int coeff_; // the coefficient of pair paras x_, y_
};

class Core {
};

}// namespace nrai
#endif
