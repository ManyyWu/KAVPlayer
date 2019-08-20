/****************************************************************************
** Meta object code from reading C++ file 'AVPlayerWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/AVPlayerWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AVPlayerWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_AVPlayerWidget_t {
    QByteArrayData data[11];
    char stringdata0[122];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_AVPlayerWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_AVPlayerWidget_t qt_meta_stringdata_AVPlayerWidget = {
    {
QT_MOC_LITERAL(0, 0, 14), // "AVPlayerWidget"
QT_MOC_LITERAL(1, 15, 14), // "player_playing"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 13), // "player_paused"
QT_MOC_LITERAL(4, 45, 14), // "player_stopped"
QT_MOC_LITERAL(5, 60, 13), // "player_seeked"
QT_MOC_LITERAL(6, 74, 11), // "pos_changed"
QT_MOC_LITERAL(7, 86, 11), // "err_occured"
QT_MOC_LITERAL(8, 98, 4), // "stop"
QT_MOC_LITERAL(9, 103, 8), // "err_code"
QT_MOC_LITERAL(10, 112, 9) // "clear_msg"

    },
    "AVPlayerWidget\0player_playing\0\0"
    "player_paused\0player_stopped\0player_seeked\0"
    "pos_changed\0err_occured\0stop\0err_code\0"
    "clear_msg"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AVPlayerWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x06 /* Public */,
       3,    0,   55,    2, 0x06 /* Public */,
       4,    1,   56,    2, 0x06 /* Public */,
       5,    0,   59,    2, 0x06 /* Public */,
       6,    1,   60,    2, 0x06 /* Public */,
       7,    1,   63,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       8,    1,   66,    2, 0x08 /* Private */,
      10,    0,   69,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double,    2,
    QMetaType::Void, QMetaType::Int,    2,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Int,

       0        // eod
};

void AVPlayerWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AVPlayerWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->player_playing(); break;
        case 1: _t->player_paused(); break;
        case 2: _t->player_stopped((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->player_seeked(); break;
        case 4: _t->pos_changed((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 5: _t->err_occured((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->stop((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: { int _r = _t->clear_msg();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AVPlayerWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AVPlayerWidget::player_playing)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AVPlayerWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AVPlayerWidget::player_paused)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (AVPlayerWidget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AVPlayerWidget::player_stopped)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (AVPlayerWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AVPlayerWidget::player_seeked)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (AVPlayerWidget::*)(double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AVPlayerWidget::pos_changed)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (AVPlayerWidget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AVPlayerWidget::err_occured)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject AVPlayerWidget::staticMetaObject = { {
    &QWidget::staticMetaObject,
    qt_meta_stringdata_AVPlayerWidget.data,
    qt_meta_data_AVPlayerWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *AVPlayerWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AVPlayerWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AVPlayerWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int AVPlayerWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void AVPlayerWidget::player_playing()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void AVPlayerWidget::player_paused()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void AVPlayerWidget::player_stopped(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void AVPlayerWidget::player_seeked()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void AVPlayerWidget::pos_changed(double _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void AVPlayerWidget::err_occured(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
