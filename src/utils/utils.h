#ifndef _AVPLAYERWIDGET_CMDUTILS_H_
#define _AVPLAYERWIDGET_CMDUTILS_H_

#ifdef max
#undef max
#endif
#ifdef min 
#undef min
#endif
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

void init_dynload();

#endif /* _AVPLAYERWIDGET_CMDUTILS_H_ */
