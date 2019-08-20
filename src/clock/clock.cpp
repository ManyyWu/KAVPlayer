#include "clock.h"

Clock::Clock ()
{
    time = 0.0;
}

Clock::~Clock ()
{
}

void Clock::set (double time)
{
    this->time = time;
}

double Clock::get () const
{
	return time;
}