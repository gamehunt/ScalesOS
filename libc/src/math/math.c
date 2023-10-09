#include "math.h"
#include "float.h"
#include "stdlib.h"
#include <stdio.h>

#define UNIMPLEMENTED printf("Unimplemented math function called: %s", __func__)

#define __MATH_ARCH_SQRT
#define __MATH_ARCH_ATAN2
#define __MATH_ARCH_POW

int abs(int a) {
	return a < 0 ? -a : a;
}

double fabs(double a) {
	return a < DBL_EPSILON ? a : -a;
}

double cos(double x) {
	return sin(x + M_PI_2);
}

#define _SIN_TAYLOR_1 -0.16666666666666666
#define _SIN_TAYLOR_2 0.008333333333333333
#define _SIN_TAYLOR_3 -0.0001984126984126984
#define _SIN_TAYLOR_4 2.7557319223985893e-06

static const double _sin_taylor_table[] = {
	_SIN_TAYLOR_1,
	_SIN_TAYLOR_2,
	_SIN_TAYLOR_3,
	_SIN_TAYLOR_4,
};

double sin(double x) {
	if(x < DBL_EPSILON) {
		return -sin(-x);
	}

	if(x > M_2_PI) {
		x /= M_2_PI;
	}

	if(x > M_PI_2) {
		return sin(M_PI - x);
	}

	// TODO use something faster than this
	double r = x;
	for(unsigned int i = 0; i < sizeof(_sin_taylor_table) / sizeof(double); i++) {
		x *= x * x;
		r += _sin_taylor_table[i] * x;
	}

	return r;
}

double tan(double x) {
	return sin(x) / cos(x);
}

double cosh(double x) {
	return (exp(x) + exp(-x)) / 2;
}

double sinh(double x) {
	return (exp(x) - exp(-x)) / 2;
}

double tanh(double x) {
	return sinh(x) / cosh(x);
}

double acos(double x) {

	UNIMPLEMENTED;
	abort();
	__builtin_unreachable();
}

double asin(double x) {

	UNIMPLEMENTED;
	abort();
	__builtin_unreachable();
}

#ifdef __MATH_ARCH_ATAN2
extern double __arch_math_atan2(double y, double x);
#endif

double atan2(double y, double x) {
#ifdef __MATH_ARCH_ATAN2
	return __arch_math_atan2(y, x);
#endif
	UNIMPLEMENTED;
	abort();
	__builtin_unreachable();
}

double atan(double x) {
	return atan2(x, 1.0);
}

double log(double x) {
	UNIMPLEMENTED;
	abort();
	__builtin_unreachable();
}

long double logl(long double x){
	return log(x);
}

double exp(double x) {
	return pow(M_E, x);
}

long double expl(long double x) {
	return powl(M_E, x);
}

double floor(double x) {
	UNIMPLEMENTED;
	abort();
	__builtin_unreachable();
}

long double floorl(long double x) {
	return floor(x);
}

double ceil(double x) {
	UNIMPLEMENTED;
	abort();
	__builtin_unreachable();
}

long double ceill(long double x) {
	return ceil(x);
}

#ifdef __MATH_ARCH_POW
extern double __arch_math_pow(double b, double x);
#endif
double pow(double base, double exponent) {
#ifdef __MATH_ARCH_POW
	return __arch_math_pow(base, exponent);
#endif
	UNIMPLEMENTED;
	abort();
	__builtin_unreachable();
}

long double powl(long double base, long double exponent) {
	return pow(base, exponent);
}


#ifdef __MATH_ARCH_SQRT
extern double __arch_math_sqrt(double x);
#endif

double sqrt(double x) {
#ifdef __MATH_ARCH_SQRT
	return __arch_math_sqrt(x);
#endif
// Newton method
  double r = 1.0;
  for(int i = 1; i <= 10; i++) {
    r -= (r*r - x) / (2*r);
  }
  return r;
}

long double sqrtl(long double x) {
	return sqrt(x);
}
