#ifndef NV_MATH_OPS
#define NV_MATH_OPS

#pragma once

#include <Math/Types.h>

#if NV_USE_DIRECTXMATH

// This code uses the DirectXMath library in abstracted form. 
// Ideally DirectXMath would be replaced with custom definitions, 
// but in interest of time and brevity, we use it as-is for now. 

namespace nv::math
{
#define DEFINE_ALL(macro) \
macro(2)\
macro(3)\
macro(4)

#define DEFINE_ALL_OPS(macro, op) \
macro(op, 2)\
macro(op, 3)\
macro(op, 4)

#define DEFINE_LOAD_FLOAT(count) inline Vector Load(const float##count& value) noexcept { return DirectX::XMLoadFloat##count(&value); }
#define DEFINE_LOAD_MATRIX(countA, countB) inline Matrix Load(const float##countA##x##countB& value) noexcept { return DirectX::XMLoadFloat##countA##x##countB(&value); }

#define DEFINE_STORE_FLOAT(count) inline auto Store(Vector value, float##count& dest) noexcept { DirectX::XMStoreFloat##count(&dest, value); }
#define DEFINE_STORE_MATRIX(countA, countB) inline auto Store(Matrix value, float##countA##x##countB& dest) noexcept { return DirectX::XMStoreFloat##countA##x##countB(&dest, value); }

DEFINE_ALL(DEFINE_LOAD_FLOAT)
DEFINE_ALL(DEFINE_STORE_FLOAT)

DEFINE_LOAD_MATRIX(3, 3)
DEFINE_LOAD_MATRIX(4, 3)
DEFINE_LOAD_MATRIX(4, 4)

DEFINE_STORE_MATRIX(3, 3)
DEFINE_STORE_MATRIX(4, 3)
DEFINE_STORE_MATRIX(4, 4)

#define DEFINE_OP_SINGLE_VECTOR(op, count) inline Vector Vector##count##op##(Vector v) noexcept { return DirectX::XMVector##count##op(v); }
#define DEFINE_OP_SINGLE_MATRIX(op) inline Matrix Matrix##op##(Matrix mat) noexcept { return DirectX::XMMatrix##op(mat); }
#define DEFINE_OP_SINGLE_MATRIX_FROM_VECTOR(op) inline Matrix Matrix##op##(Vector v) noexcept { return DirectX::XMMatrix##op##FromVector(v); }
#define DEFINE_OP_SINGLE_MATRIX_FROM_FLOAT(op) inline Matrix Matrix##op##(float x, float y, float z) noexcept { return DirectX::XMMatrix##op(x, y, z); }
#define DEFINE_OP_SINGLE_VECTOR_AUTO(op, count) inline auto Vector##count##op##(Vector v) noexcept { return DirectX::XMVector##count##op(v); }
#define DEFINE_OP_TWO_VECTORS(op, count) inline Vector Vector##count##op##(Vector lhs, Vector rhs) noexcept { return DirectX::XMVector##count##op(lhs, rhs); }
#define DEFINE_OP_VECTOR_MATRIX(op, count) inline Vector Vector##count##op##(Vector v, Matrix mat) noexcept { return DirectX::XMVector##count##op(v, mat); }
#define DEFINE_OP_TWO_VECTORS_AUTO(op, count) inline auto Vector##count##op##(Vector lhs, Vector rhs) noexcept { return DirectX::XMVector##count##op(lhs, rhs); }
#define DEFINE_OP_THREE_VECTORS(op, count) inline Vector Vector##count##op##(Vector a, Vector b, Vector c) noexcept { return DirectX::XMVector##count##op(a, b, c); }


DEFINE_ALL_OPS(DEFINE_OP_SINGLE_VECTOR, Normalize)
DEFINE_ALL_OPS(DEFINE_OP_SINGLE_VECTOR, Length)
DEFINE_ALL_OPS(DEFINE_OP_SINGLE_VECTOR, LengthEst)

DEFINE_ALL_OPS(DEFINE_OP_TWO_VECTORS, Dot)
DEFINE_ALL_OPS(DEFINE_OP_TWO_VECTORS_AUTO, Equal)
DEFINE_ALL_OPS(DEFINE_OP_TWO_VECTORS, Reflect)
DEFINE_ALL_OPS(DEFINE_OP_VECTOR_MATRIX, Transform)

DEFINE_OP_TWO_VECTORS(Cross, 2)
DEFINE_OP_TWO_VECTORS(Cross, 3)
DEFINE_OP_THREE_VECTORS(Cross, 4)

DEFINE_OP_TWO_VECTORS(Rotate, 3)
DEFINE_OP_SINGLE_MATRIX(Transpose)
DEFINE_OP_SINGLE_MATRIX_FROM_VECTOR(RotationRollPitchYaw)
DEFINE_OP_SINGLE_MATRIX_FROM_VECTOR(Scaling)
DEFINE_OP_SINGLE_MATRIX_FROM_VECTOR(Translation)
DEFINE_OP_SINGLE_MATRIX_FROM_FLOAT(RotationRollPitchYaw)
DEFINE_OP_SINGLE_MATRIX_FROM_FLOAT(Scaling)
DEFINE_OP_SINGLE_MATRIX_FROM_FLOAT(Translation)

inline Matrix MatrixInverse(Matrix matrix, Vector* pDeterminant = nullptr) noexcept
{
    using namespace DirectX;
    return XMMatrixInverse(pDeterminant, matrix);
}

inline Matrix MatrixPerspectiveFovLH(float fovAngleY, float aspectRatio, float nearZ, float farZ) noexcept
{
    return DirectX::XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ);
}

inline Matrix MatrixAffineTransformation(Vector scaling, Vector rotationOrigin, Vector rotationQuaternion, Vector translation) noexcept
{
    return DirectX::XMMatrixAffineTransformation(scaling, rotationOrigin, rotationQuaternion, translation);
}

inline Matrix MatrixLookToLH(Vector eyePos, Vector eyeDir, Vector upDir) noexcept
{
    return DirectX::XMMatrixLookToLH(eyePos, eyeDir, upDir);
}

// TODO: Matrix View and Projection functions + Utilities

}

#endif

#endif // !NV_MATH_OPS