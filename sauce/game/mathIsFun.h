#ifndef MATHISFUN_H_
#define MATHISFUN_H_

float fmax(float a, float b);
float fmin(float a, float b);
int ceil(float a);
int floor(float a);
double fabs(double a);
int round(float a);
float remapToMax(float a, float maxValue);
int random(int min, int max);
void mathIsFun_shuffleIndices(int *indices, int totalCount);
#endif