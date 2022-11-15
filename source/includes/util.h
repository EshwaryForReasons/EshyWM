#pragma once

#include <X11/Xlib.h>

typedef unsigned long Color;

template <typename T>
class Vector2D
{
public:
	T x;
	T y;

	Vector2D<T> operator+(const Vector2D<T>& a)
	{
		return Vector2D<T>(x + a.x, y + a.y);
	}

	Vector2D<T> operator-(const Vector2D<T>& a)
	{
		return Vector2D<T>(x - a.x, y - a.y);
	}

	void operator+=(const Vector2D<T>& a)
	{
		x += a.x;
		y += a.y;
	}

	void operator-=(const Vector2D<T>& a)
	{
		x -= a.x;
		y -= a.y;
	}

	Vector2D() = default;
	Vector2D(T _x, T _y)
		: x(_x)
		, y(_y)
	{}
};

struct rect
{
	int x;
    int y;
	unsigned int width;
	unsigned int height;

	rect() = default;

	rect(int _x, int _y, unsigned int _width, unsigned int _height)
      : x(_x)
      , y(_y)
	  , width(_width)
	  , height(_height)
    {}
};