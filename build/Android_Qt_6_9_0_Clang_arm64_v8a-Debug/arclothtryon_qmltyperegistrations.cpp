/****************************************************************************
** Generated QML type registration code
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <QtQml/qqml.h>
#include <QtQml/qqmlmoduleregistration.h>

#if __has_include(<NetworkManager.h>)
#  include <NetworkManager.h>
#endif
#if __has_include(<QMLManager.h>)
#  include <QMLManager.h>
#endif


#if !defined(QT_STATIC)
#define Q_QMLTYPE_EXPORT Q_DECL_EXPORT
#else
#define Q_QMLTYPE_EXPORT
#endif
Q_QMLTYPE_EXPORT void qml_register_types_ARClothTryOn()
{
    QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    qmlRegisterTypesAndRevisions<NetworkManager>("ARClothTryOn", 1);
    qmlRegisterTypesAndRevisions<QMLManager>("ARClothTryOn", 1);
    QT_WARNING_POP
    qmlRegisterModule("ARClothTryOn", 1, 0);
}

static const QQmlModuleRegistration aRClothTryOnRegistration("ARClothTryOn", qml_register_types_ARClothTryOn);
