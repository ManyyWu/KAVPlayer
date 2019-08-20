#ifndef _AVPLAYERWIDGET_GLOBAL_H_
#define _AVPLAYERWIDGET_GLOBAL_H_

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(AVPLAYERWIDGET_LIB)
#  define AVPLAYERWIDGET_EXPORT Q_DECL_EXPORT
# else
#  define AVPLAYERWIDGET_EXPORT Q_DECL_IMPORT
# endif
#else
# define AVPLAYERWIDGET_EXPORT
#endif

#endif /* _AVPLAYERWIDGET_GLOBAL_H_ */
