#pragma once

#include <X11/Xlib.h>
#include <iostream>
#include <sstream>

//Force enable logging
#define __LOGGING_ENABLED

#define ensure(s)             if(!(s)) abort();
#define safe_ensure(s)        if(!(s)) return;

template <typename T>
class Vector2D;

#ifdef __LOGGING_ENABLED
	enum LogSeverity : uint8_t
	{
		LS_Verbose,
		LS_Info,
		LS_Warning,
		LS_Error,
		LS_Fatal
	};

	extern LogSeverity __global_log_severity;

	void __set_global_log_severity(LogSeverity severity);
	void __log_event_info(LogSeverity severity, XEvent event);
	void __log_message(LogSeverity severity, char* message, ...);
	template <typename T>
	void __log_vector(LogSeverity severity, void* vector);

	#define SET_GLOBAL_SEVERITY(severity)		__set_global_log_severity(severity);
	#define LOG(severity, message, ...)			__log_message(severity, message, ...);
	#define LOG_EVENT_INFO(severity, event)		__log_event_info(severity, event);
	#define LOG_VECTOR(severity, vector)		std::cout << "(" << vector.x << ", " << vector.y << ")" << std::endl;
#else
#ifndef __LOGGING_ENABLED
	#define SET_GLOBAL_SEVERITY(severity)
	#define LOG(severity, message)
	#define LOG_EVENT_INFO(severity, event)
	#define LOG_VECTOR(severity, vector)
#endif
#endif

typedef unsigned long Color;

#define CENTER_W(w)       (int)((DISPLAY_WIDTH(0) - w) / 2)
#define CENTER_H(h)       (int)((DISPLAY_HEIGHT(0) - h) / 2)

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
};

struct rect
{
	int x;
    int y;
	uint width;
	uint height;
};