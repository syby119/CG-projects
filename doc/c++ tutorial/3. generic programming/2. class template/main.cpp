#include <iostream>
#include "vector3.hpp"

using namespace gtm;

template <typename T> FUNC_QUALIFIER
void print(const Vector3<T>& v) {
    printf("(%f, %f, %f)\n", v.x, v.y, v.z);
}

int main() {
	printf("========= Vector3 Unit Test =========");

	printf("\n****************** class member function *****************\n");

	/* default constructor */ {
		printf("\ndefault constructor: zero vector3\n");
		Vector3<float> v;
		print(v);
	}

	/* constructor with 3 values*/ {
		printf("\nconstructor with 3 values\n");
		Vector3<float> v(1.0f, 2.0f, 3.0f);
		print(v);
	}

	/* copy constructor */ {
		printf("\ncopy constructor\n");
		Vector3<float> u(1.0f, 2.0f, 3.0f);
		Vector3<float> v = u;
		print(v);
	}
	
	/* move constructor */ {
		printf("\nno explicitly defined move constructor \n");
	}

	/* operator = */ {
		printf("\noperator = \n");
		Vector3<float> v;
		Vector3<float> u(1.0f, 2.0f, 3.0f);
		v = u;
		print(v);
	}

	/* operator[] */ {
		printf("\nv[i] = c\n");
		Vector3<float> v;
		v[0] = 1.0f;
		v[1] = 2.0f;
		v[2] = 3.0f;
		print(v);
	}

	/* const operator[] */ {
		printf("\nv[i]\n");
		const Vector3<float> v(1.0f, 2.0f, 3.0f);
		print(v);
	}

	/* operator += */ {
		printf("\noperator += \n");
		Vector3<float> v(1.0f, 2.0f, 3.0f);
		Vector3<float> u(1.0f, 2.0f, 3.0f);
		v += u;
		print(v);
	}

	/* operator -= */ {
		printf("\noperator -= \n");
		Vector3<float> v(1.0f, 2.0f, 3.0f);
		Vector3<float> u(1.0f, 2.0f, 3.0f);
		v -= u;
		print(v);
	}

	/* operator *= */ {
		printf("\noperator *= \n");
		Vector3<float> v(1.0f, 2.0f, 3.0f);
		v *= 2.0f;
		print(v);
	}

	/* operator /= */ {
		printf("\noperator /= \n");
		Vector3<float> v(1.0f, 2.0f, 3.0f);
		v /= 2.0f;
		print(v);
	}

	/* v.length() */ {
		printf("\nv.length()\n");
		Vector3<float> v(1.0f, 2.0f, 3.0f);
		printf("%f\n", v.length());
	}

	/* v.normalize() */ {
		printf("\nv.normalize()\n");
		Vector3<float> v(1.0f, 2.0f, 3.0f);
		v.normalize();
		print(v);
	}

	/* v.dot(u) */ {
		printf("\nv.dot(u)\n");
		Vector3<float> v(1.0f, 2.0f, 3.0f);
		Vector3<float> u(3.0f, 2.0f, 1.0f);
		printf("%f\n", v.dot(u));
	}

	/* v.cross(u) */ {
		printf("\nv.cross(u)\n");
		Vector3<float> v(1.0f, 2.0f, 3.0f);
		Vector3<float> u(3.0f, 2.0f, 1.0f);
		print(v.cross(u));
	}

	printf("\n********************* global function *************************\n");

	/* operator+ (unary)*/ {
		printf("\n+v\n");
		Vector3<float> v(1.0f, 2.0f, 3.0f);
		print(+v);
	}


	/* operator- (unary)*/ {
		printf("\n-v\n");
		Vector3<float> v(1.0f, 2.0f, 3.0f);
		print(-v);
	}

	/* operator+ */ {
		printf("\nu + v\n");
		Vector3<float> u(1.0f, 2.0f, 3.0f);
		Vector3<float> v(3.0f, 2.0f, 1.0f);
		print(u + v);
	}

	/* operator- */ {
		printf("\nu - v\n");
		Vector3<float> u(1.0f, 2.0f, 3.0f);
		Vector3<float> v(3.0f, 2.0f, 1.0f);
		print(u - v);
	}

	/* operator*(scaler * vector3) */ {
		printf("\nc * v\n");
		Vector3<float> v(1.0f, 2.0f, 3.0f);
		print(2.0f * v);
	}

	/* operator*(vector3 * scaler) */ {
		printf("\nv * c\n");
		Vector3<float> v(1.0f, 2.0f, 3.0f);
		print(v * 2.0f);
	}

	/* operator/ */ {
		printf("\nv / c\n");
		Vector3<float> v(1.0f, 2.0f, 3.0f);
		print(v / 2.0f);
	}

	/* normalize(v) */ {
		printf("\nnormalize(v)\n");
		Vector3<float> v(1.0f, 2.0f, 3.0f);
		print(normalize(v));
	}

	/* dot(u, v) */ {
		printf("\ndot(u, v)\n");
		Vector3<float> u(1.0f, 2.0f, 3.0f);
		Vector3<float> v(3.0f, 2.0f, 1.0f);
		printf("%f\n", dot(u, v));
	}

	/* cross(u, v) */ {
		printf("\ncross(u, v)\n");
		Vector3<float> u(1.0f, 2.0f, 3.0f);
		Vector3<float> v(3.0f, 2.0f, 1.0f);
		print(cross(u, v));
	}

    return 0;
}