#pragma once	
#include "framework.h"

struct Vector2
{
    static Vector2 One;
    static Vector2 Zero;

    float x;
    float y;

    Vector2()
        : x(0.0f)
        , y(0.0f)
    {
    }

    Vector2(float _x, float _y)
        : x(_x)
        , y(_y)
    {
    }

    // 덧셈 연산자
    Vector2 operator+(Vector2 other) const
    {
        return Vector2(x + other.x, y + other.y);
    }

    // 뺄셈 연산자
    Vector2 operator-(Vector2 other) const
    {
        return Vector2(x - other.x, y - other.y);
    }

    // 스칼라 나눗셈 연산자
    Vector2 operator/(float value) const
    {
        return Vector2(x / value, y / value);
    }

    // 스칼라 곱셈 연산자 (카메라 zoom 적용)
    Vector2 operator*(float value) const
    {
        return Vector2(x * value, y * value);
    }

    // 벡터 간 좌표별 곱셈 (component-wise multiplication)
    Vector2 operator*(Vector2 other) const
    {
        return Vector2(x * other.x, y * other.y);
    }

    // 동등 비교 연산자
    bool operator==(Vector2 other) const
    {
        return x == other.x && y == other.y;
    }

    // 비동등 비교 연산자
    bool operator!=(Vector2 other) const
    {
        return !(*this == other);
    }

    // 대입 연산자: 덧셈
    Vector2& operator+=(Vector2 other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    // 대입 연산자: 뺄셈
    Vector2& operator-=(Vector2 other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    // 벡터 크기 (Length) 계산
    float Length() const
    {
        return sqrtf(x * x + y * y);
    }

    // 벡터 정규화 (단위 벡터 반환)
    Vector2 Normalize() const
    {
        float len = Length();
        if (len == 0.0f)
            return Vector2(0.0f, 0.0f);
        return Vector2(x / len, y / len);
    }

    // 내적 (Dot Product)
    float Dot(Vector2 other) const
    {
        return x * other.x + y * other.y;
    }
};