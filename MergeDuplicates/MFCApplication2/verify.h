#define VERIFY_API __declspec(dllexport)
//@param x:feature1;
//@param y:feature2;
//@param n:length of feature;
//@return value:score of verification
VERIFY_API float  verify(float* x, float* y, int n);
