#ifndef _AVPLAYERWIDGET_CLOCK_H_
#define _AVPLAYERWIDGET_CLOCK_H_

/* clock */
class Clock {
private:
    double time;

public:
    Clock      ();
	~Clock     ();
	void   set (double time);
	double get () const;
};

#endif /* _AVPLAYERWIDGET_CLOCK_H_ */