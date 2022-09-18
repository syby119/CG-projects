#pragma once

#include <cassert>
#include <cmath>
#include <limits>

/* function qualifier for CUDA support */
#ifdef ENABLE_CUDA
#include <cuda_runtime.h>
#define FUNC_QUALIFIER __host__ __device__
#else
#define FUNC_QUALIFIER
#endif

namespace gtm {
/* constant values */
template <typename T>
constexpr T pi() {
	return static_cast<T>(3.14159265358979323846);
}

template <typename T>
constexpr T e() {
	return static_cast<T>(2.71828182845904523536);
}

/* round error */
template <typename T>
constexpr T epsilon() {
	return static_cast<T>(10) * std::numeric_limits<T>::epsilon();
}

/* convertion between degrees and radians */
template <typename T>
T radian_cast(T deg) {
	return deg * pi<T>() / static_cast<T>(180);
}

template <typename T>
T degree_cast(T rad) {
	return rad * static_cast<T>(180) / pi<T>();
}

} // namespace gtm