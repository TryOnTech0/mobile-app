#include <QtQml/qqmlprivate.h>
#include <QtCore/qdir.h>
#include <QtCore/qurl.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>

namespace QmlCacheGeneratedCode {
namespace _0x5f_ARClothTryOn_qml_Main_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_ARClothTryOn_qml_MainPage_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_ARClothTryOn_qml_CameraPage_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_ARClothTryOn_qml_ScanResultPage_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_ARClothTryOn_qml_GarmentSelectionPage_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_ARClothTryOn_qml_GarmentPreviewPage_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_ARClothTryOn_qml_AuthorizationPage_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_ARClothTryOn_qml_ARCamera_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_ARClothTryOn_qml_Style_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}

}
namespace {
struct Registry {
    Registry();
    ~Registry();
    QHash<QString, const QQmlPrivate::CachedQmlUnit*> resourcePathToCachedUnit;
    static const QQmlPrivate::CachedQmlUnit *lookupCachedUnit(const QUrl &url);
};

Q_GLOBAL_STATIC(Registry, unitRegistry)


Registry::Registry() {
    resourcePathToCachedUnit.insert(QStringLiteral("/ARClothTryOn/qml/Main.qml"), &QmlCacheGeneratedCode::_0x5f_ARClothTryOn_qml_Main_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/ARClothTryOn/qml/MainPage.qml"), &QmlCacheGeneratedCode::_0x5f_ARClothTryOn_qml_MainPage_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/ARClothTryOn/qml/CameraPage.qml"), &QmlCacheGeneratedCode::_0x5f_ARClothTryOn_qml_CameraPage_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/ARClothTryOn/qml/ScanResultPage.qml"), &QmlCacheGeneratedCode::_0x5f_ARClothTryOn_qml_ScanResultPage_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/ARClothTryOn/qml/GarmentSelectionPage.qml"), &QmlCacheGeneratedCode::_0x5f_ARClothTryOn_qml_GarmentSelectionPage_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/ARClothTryOn/qml/GarmentPreviewPage.qml"), &QmlCacheGeneratedCode::_0x5f_ARClothTryOn_qml_GarmentPreviewPage_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/ARClothTryOn/qml/AuthorizationPage.qml"), &QmlCacheGeneratedCode::_0x5f_ARClothTryOn_qml_AuthorizationPage_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/ARClothTryOn/qml/ARCamera.qml"), &QmlCacheGeneratedCode::_0x5f_ARClothTryOn_qml_ARCamera_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/ARClothTryOn/qml/Style.qml"), &QmlCacheGeneratedCode::_0x5f_ARClothTryOn_qml_Style_qml::unit);
    QQmlPrivate::RegisterQmlUnitCacheHook registration;
    registration.structVersion = 0;
    registration.lookupCachedQmlUnit = &lookupCachedUnit;
    QQmlPrivate::qmlregister(QQmlPrivate::QmlUnitCacheHookRegistration, &registration);
}

Registry::~Registry() {
    QQmlPrivate::qmlunregister(QQmlPrivate::QmlUnitCacheHookRegistration, quintptr(&lookupCachedUnit));
}

const QQmlPrivate::CachedQmlUnit *Registry::lookupCachedUnit(const QUrl &url) {
    if (url.scheme() != QLatin1String("qrc"))
        return nullptr;
    QString resourcePath = QDir::cleanPath(url.path());
    if (resourcePath.isEmpty())
        return nullptr;
    if (!resourcePath.startsWith(QLatin1Char('/')))
        resourcePath.prepend(QLatin1Char('/'));
    return unitRegistry()->resourcePathToCachedUnit.value(resourcePath, nullptr);
}
}
int QT_MANGLE_NAMESPACE(qInitResources_qmlcache_ARClothTryOn)() {
    ::unitRegistry();
    return 1;
}
Q_CONSTRUCTOR_FUNCTION(QT_MANGLE_NAMESPACE(qInitResources_qmlcache_ARClothTryOn))
int QT_MANGLE_NAMESPACE(qCleanupResources_qmlcache_ARClothTryOn)() {
    return 1;
}
