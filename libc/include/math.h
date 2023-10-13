#ifndef __MATH_H
#define __MATH_H

#define M_E		    2.71828182845904523536
#define M_LOG2E		1.44269504088896340736
#define M_LOG10E	0.434294481903251827651
#define M_LN2		0.693147180559945309417
#define M_LN10		2.30258509299404568402
#define M_PI		3.14159265358979323846
#define M_PI_2		1.57079632679489661923
#define M_PI_4		0.785398163397448309616
#define M_1_PI		0.318309886183790671538
#define M_2_PI		0.636619772367581343076
#define M_2_SQRTPI	1.12837916709551257390
#define M_SQRT2		1.41421356237309504880
#define M_SQRT1_2	0.707106781186547524401

double fabs(double);
double cos(double);
double sin(double);
double tan(double);

double cosh(double);
double sinh(double);
double tanh(double);

double acos(double);
double asin(double);
double atan(double);
double atan2(double, double);

double      log(double);
long double logl(long double);

double      exp(double);
long double expl(long double);

double      floor(double);
long double floorl(long double);

double      ceil(double);
long double ceill(long double);

double      pow(double, double);
long double powl(long double, long double);

double      sqrt(double);
long double sqrtl(long double);

#endif
