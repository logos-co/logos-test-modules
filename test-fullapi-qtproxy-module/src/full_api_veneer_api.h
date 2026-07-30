#pragma once
#include <QString>
#include <QVariant>
#include <QStringList>
#include <QJsonArray>
#include <QVariantList>
#include <QVariantMap>
#include <functional>
#include <utility>
#include "logos_types.h"
#include "logos_api.h"
#include "logos_api_client.h"
#include "logos_call_error.h"
#include "logos_object.h"

namespace logos { namespace qt { class LpBridge; } }

class FullApiVeneer {
public:
    explicit FullApiVeneer(LogosAPI* api, const QString& moduleName);

    using RawEventCallback = std::function<void(const QString&, const QVariantList&)>;
    using EventCallback = std::function<void(const QVariantList&)>;

    bool on(const QString& eventName, RawEventCallback callback);
    bool on(const QString& eventName, EventCallback callback);
    void setEventSource(LogosObject* source);
    LogosObject* eventSource() const;
    void trigger(const QString& eventName);
    void trigger(const QString& eventName, const QVariantList& data);
    template<typename... Args>
    void trigger(const QString& eventName, Args&&... args) {
        trigger(eventName, packVariantList(std::forward<Args>(args)...));
    }
    void trigger(const QString& eventName, LogosObject* source, const QVariantList& data);
    template<typename... Args>
    void trigger(const QString& eventName, LogosObject* source, Args&&... args) {
        trigger(eventName, source, packVariantList(std::forward<Args>(args)...));
    }

    bool onStringEvent(std::function<void(const QString& v)> callback);
    bool onBytesEvent(std::function<void(QByteArray v)> callback);
    bool onIntEvent(std::function<void(qlonglong v)> callback);
    bool onUintEvent(std::function<void(qulonglong v)> callback);
    bool onDoubleEvent(std::function<void(double v)> callback);
    bool onBoolEvent(std::function<void(bool v)> callback);
    bool onAnyEvent(std::function<void(QVariant v)> callback);
    bool onStringListEvent(std::function<void(const QStringList& v)> callback);
    bool onIntListEvent(std::function<void(const QVariantList& v)> callback);
    bool onUintListEvent(std::function<void(const QVariantList& v)> callback);
    bool onDoubleListEvent(std::function<void(const QVariantList& v)> callback);
    bool onBoolListEvent(std::function<void(const QVariantList& v)> callback);
    bool onListEvent(std::function<void(const QVariantList& v)> callback);
    bool onMapEvent(std::function<void(const QVariantMap& v)> callback);
    bool onTripleEvent(std::function<void(qlonglong i, const QString& s, QByteArray b)> callback);

    QString whoAmI(logos::CallError* err = nullptr);
    void whoAmIAsync(std::function<void(QString)> callback, Timeout timeout = Timeout());
    QString echoString(const QString& v, logos::CallError* err = nullptr);
    void echoStringAsync(const QString& v, std::function<void(QString)> callback, Timeout timeout = Timeout());
    QByteArray echoBytes(QByteArray v, logos::CallError* err = nullptr);
    void echoBytesAsync(QByteArray v, std::function<void(QByteArray)> callback, Timeout timeout = Timeout());
    qlonglong echoInt(qlonglong v, logos::CallError* err = nullptr);
    void echoIntAsync(qlonglong v, std::function<void(qlonglong)> callback, Timeout timeout = Timeout());
    qulonglong echoUint(qulonglong v, logos::CallError* err = nullptr);
    void echoUintAsync(qulonglong v, std::function<void(qulonglong)> callback, Timeout timeout = Timeout());
    double echoDouble(double v, logos::CallError* err = nullptr);
    void echoDoubleAsync(double v, std::function<void(double)> callback, Timeout timeout = Timeout());
    bool echoBool(bool v, logos::CallError* err = nullptr);
    void echoBoolAsync(bool v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    QVariant echoAny(QVariant v, logos::CallError* err = nullptr);
    void echoAnyAsync(QVariant v, std::function<void(QVariant)> callback, Timeout timeout = Timeout());
    QStringList echoStringList(const QStringList& v, logos::CallError* err = nullptr);
    void echoStringListAsync(const QStringList& v, std::function<void(QStringList)> callback, Timeout timeout = Timeout());
    QVariantList echoIntList(const QVariantList& v, logos::CallError* err = nullptr);
    void echoIntListAsync(const QVariantList& v, std::function<void(QVariantList)> callback, Timeout timeout = Timeout());
    QVariantList echoUintList(const QVariantList& v, logos::CallError* err = nullptr);
    void echoUintListAsync(const QVariantList& v, std::function<void(QVariantList)> callback, Timeout timeout = Timeout());
    QVariantList echoDoubleList(const QVariantList& v, logos::CallError* err = nullptr);
    void echoDoubleListAsync(const QVariantList& v, std::function<void(QVariantList)> callback, Timeout timeout = Timeout());
    QVariantList echoBoolList(const QVariantList& v, logos::CallError* err = nullptr);
    void echoBoolListAsync(const QVariantList& v, std::function<void(QVariantList)> callback, Timeout timeout = Timeout());
    QVariantList echoList(const QVariantList& v, logos::CallError* err = nullptr);
    void echoListAsync(const QVariantList& v, std::function<void(QVariantList)> callback, Timeout timeout = Timeout());
    QVariantMap echoMap(const QVariantMap& v, logos::CallError* err = nullptr);
    void echoMapAsync(const QVariantMap& v, std::function<void(QVariantMap)> callback, Timeout timeout = Timeout());
    void doVoid(logos::CallError* err = nullptr);
    void doVoidAsync(std::function<void()> callback, Timeout timeout = Timeout());
    QString echoTriple(qlonglong i, const QString& s, QByteArray b, logos::CallError* err = nullptr);
    void echoTripleAsync(qlonglong i, const QString& s, QByteArray b, std::function<void(QString)> callback, Timeout timeout = Timeout());
    LogosResult makeResult(bool ok, logos::CallError* err = nullptr);
    void makeResultAsync(bool ok, std::function<void(LogosResult)> callback, Timeout timeout = Timeout());
    bool fireStringEvent(const QString& v, logos::CallError* err = nullptr);
    void fireStringEventAsync(const QString& v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireBytesEvent(QByteArray v, logos::CallError* err = nullptr);
    void fireBytesEventAsync(QByteArray v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireIntEvent(qlonglong v, logos::CallError* err = nullptr);
    void fireIntEventAsync(qlonglong v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireUintEvent(qulonglong v, logos::CallError* err = nullptr);
    void fireUintEventAsync(qulonglong v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireDoubleEvent(double v, logos::CallError* err = nullptr);
    void fireDoubleEventAsync(double v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireBoolEvent(bool v, logos::CallError* err = nullptr);
    void fireBoolEventAsync(bool v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireAnyEvent(QVariant v, logos::CallError* err = nullptr);
    void fireAnyEventAsync(QVariant v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireStringListEvent(const QStringList& v, logos::CallError* err = nullptr);
    void fireStringListEventAsync(const QStringList& v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireIntListEvent(const QVariantList& v, logos::CallError* err = nullptr);
    void fireIntListEventAsync(const QVariantList& v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireUintListEvent(const QVariantList& v, logos::CallError* err = nullptr);
    void fireUintListEventAsync(const QVariantList& v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireDoubleListEvent(const QVariantList& v, logos::CallError* err = nullptr);
    void fireDoubleListEventAsync(const QVariantList& v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireBoolListEvent(const QVariantList& v, logos::CallError* err = nullptr);
    void fireBoolListEventAsync(const QVariantList& v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireListEvent(const QVariantList& v, logos::CallError* err = nullptr);
    void fireListEventAsync(const QVariantList& v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireMapEvent(const QVariantMap& v, logos::CallError* err = nullptr);
    void fireMapEventAsync(const QVariantMap& v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireTripleEvent(qlonglong i, const QString& s, QByteArray b, logos::CallError* err = nullptr);
    void fireTripleEventAsync(qlonglong i, const QString& s, QByteArray b, std::function<void(bool)> callback, Timeout timeout = Timeout());

private:
    template<typename... Args>
    static QVariantList packVariantList(Args&&... args) {
        QVariantList list;
        list.reserve(sizeof...(Args));
        using Expander = int[];
        (void)Expander{0, (list.append(QVariant::fromValue(std::forward<Args>(args))), 0)...};
        return list;
    }
    LogosAPI* m_api;
    LogosAPIClient* m_client;
    QString m_moduleName;
    LogosObject* m_eventSource = nullptr;
    logos::qt::LpBridge* m_bridge;
};
