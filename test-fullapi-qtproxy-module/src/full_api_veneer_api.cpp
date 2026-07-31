#include "full_api_veneer_api.h"

#include <QDebug>
#include <nlohmann/json.hpp>
#include "logos_qt_lp_bridge.h"

FullApiVeneer::FullApiVeneer(LogosAPI* api, const QString& moduleName)
    : m_api(api), m_client(api->getClient(moduleName)), m_moduleName(moduleName),
      m_bridge(logos::qt::LpBridge::forTarget(api, moduleName)) {}

bool FullApiVeneer::on(const QString& eventName, RawEventCallback callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << eventName;
        return false;
    }
    const QString _name = eventName;
    return logos::qt::subscribe(m_bridge, eventName.toStdString(),
        [callback, _name](nlohmann::json _a) {
            callback(_name, logos::nlohmannArgsToQVariantList(_a));
        });
}

bool FullApiVeneer::on(const QString& eventName, EventCallback callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << eventName;
        return false;
    }
    return on(eventName, [callback](const QString&, const QVariantList& data) {
        callback(data);
    });
}

void FullApiVeneer::setEventSource(LogosObject* source) {
    m_eventSource = source;
}

LogosObject* FullApiVeneer::eventSource() const {
    return m_eventSource;
}

void FullApiVeneer::trigger(const QString& eventName) {
    trigger(eventName, QVariantList{});
}

void FullApiVeneer::trigger(const QString& eventName, const QVariantList& data) {
    if (!m_eventSource) {
        qWarning() << "FullApiVeneer: no event source set for trigger" << eventName;
        return;
    }
    m_client->onEventResponse(m_eventSource, eventName, data);
}

void FullApiVeneer::trigger(const QString& eventName, LogosObject* source, const QVariantList& data) {
    if (!source) {
        qWarning() << "FullApiVeneer: cannot trigger" << eventName << "with null source";
        return;
    }
    m_client->onEventResponse(source, eventName, data);
}

bool FullApiVeneer::onStringEvent(std::function<void(const QString& v)> callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << QStringLiteral("stringEvent");
        return false;
    }
    return logos::qt::subscribe(m_bridge, "stringEvent", [callback](nlohmann::json _a) {
        if (!_a.is_array() || _a.size() < 1) return;
        callback(logos::qt::fromWire<QString>(_a.at(0)));
    });
}

bool FullApiVeneer::onBytesEvent(std::function<void(QByteArray v)> callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << QStringLiteral("bytesEvent");
        return false;
    }
    return logos::qt::subscribe(m_bridge, "bytesEvent", [callback](nlohmann::json _a) {
        if (!_a.is_array() || _a.size() < 1) return;
        callback(logos::qt::fromWire<QByteArray>(_a.at(0)));
    });
}

bool FullApiVeneer::onIntEvent(std::function<void(qlonglong v)> callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << QStringLiteral("intEvent");
        return false;
    }
    return logos::qt::subscribe(m_bridge, "intEvent", [callback](nlohmann::json _a) {
        if (!_a.is_array() || _a.size() < 1) return;
        callback(logos::qt::fromWire<qlonglong>(_a.at(0)));
    });
}

bool FullApiVeneer::onUintEvent(std::function<void(qulonglong v)> callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << QStringLiteral("uintEvent");
        return false;
    }
    return logos::qt::subscribe(m_bridge, "uintEvent", [callback](nlohmann::json _a) {
        if (!_a.is_array() || _a.size() < 1) return;
        callback(logos::qt::fromWire<qulonglong>(_a.at(0)));
    });
}

bool FullApiVeneer::onDoubleEvent(std::function<void(double v)> callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << QStringLiteral("doubleEvent");
        return false;
    }
    return logos::qt::subscribe(m_bridge, "doubleEvent", [callback](nlohmann::json _a) {
        if (!_a.is_array() || _a.size() < 1) return;
        callback(logos::qt::fromWire<double>(_a.at(0)));
    });
}

bool FullApiVeneer::onBoolEvent(std::function<void(bool v)> callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << QStringLiteral("boolEvent");
        return false;
    }
    return logos::qt::subscribe(m_bridge, "boolEvent", [callback](nlohmann::json _a) {
        if (!_a.is_array() || _a.size() < 1) return;
        callback(logos::qt::fromWire<bool>(_a.at(0)));
    });
}

bool FullApiVeneer::onAnyEvent(std::function<void(QVariant v)> callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << QStringLiteral("anyEvent");
        return false;
    }
    return logos::qt::subscribe(m_bridge, "anyEvent", [callback](nlohmann::json _a) {
        if (!_a.is_array() || _a.size() < 1) return;
        callback(logos::qt::fromWire<QVariant>(_a.at(0)));
    });
}

bool FullApiVeneer::onStringListEvent(std::function<void(const QStringList& v)> callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << QStringLiteral("stringListEvent");
        return false;
    }
    return logos::qt::subscribe(m_bridge, "stringListEvent", [callback](nlohmann::json _a) {
        if (!_a.is_array() || _a.size() < 1) return;
        callback(logos::qt::fromWire<QStringList>(_a.at(0)));
    });
}

bool FullApiVeneer::onIntListEvent(std::function<void(const QVariantList& v)> callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << QStringLiteral("intListEvent");
        return false;
    }
    return logos::qt::subscribe(m_bridge, "intListEvent", [callback](nlohmann::json _a) {
        if (!_a.is_array() || _a.size() < 1) return;
        callback(logos::qt::fromWire<QVariantList>(_a.at(0)));
    });
}

bool FullApiVeneer::onUintListEvent(std::function<void(const QVariantList& v)> callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << QStringLiteral("uintListEvent");
        return false;
    }
    return logos::qt::subscribe(m_bridge, "uintListEvent", [callback](nlohmann::json _a) {
        if (!_a.is_array() || _a.size() < 1) return;
        callback(logos::qt::fromWire<QVariantList>(_a.at(0)));
    });
}

bool FullApiVeneer::onDoubleListEvent(std::function<void(const QVariantList& v)> callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << QStringLiteral("doubleListEvent");
        return false;
    }
    return logos::qt::subscribe(m_bridge, "doubleListEvent", [callback](nlohmann::json _a) {
        if (!_a.is_array() || _a.size() < 1) return;
        callback(logos::qt::fromWire<QVariantList>(_a.at(0)));
    });
}

bool FullApiVeneer::onBoolListEvent(std::function<void(const QVariantList& v)> callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << QStringLiteral("boolListEvent");
        return false;
    }
    return logos::qt::subscribe(m_bridge, "boolListEvent", [callback](nlohmann::json _a) {
        if (!_a.is_array() || _a.size() < 1) return;
        callback(logos::qt::fromWire<QVariantList>(_a.at(0)));
    });
}

bool FullApiVeneer::onListEvent(std::function<void(const QVariantList& v)> callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << QStringLiteral("listEvent");
        return false;
    }
    return logos::qt::subscribe(m_bridge, "listEvent", [callback](nlohmann::json _a) {
        if (!_a.is_array() || _a.size() < 1) return;
        callback(logos::qt::fromWire<QVariantList>(_a.at(0)));
    });
}

bool FullApiVeneer::onMapEvent(std::function<void(const QVariantMap& v)> callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << QStringLiteral("mapEvent");
        return false;
    }
    return logos::qt::subscribe(m_bridge, "mapEvent", [callback](nlohmann::json _a) {
        if (!_a.is_array() || _a.size() < 1) return;
        callback(logos::qt::fromWire<QVariantMap>(_a.at(0)));
    });
}

bool FullApiVeneer::onTripleEvent(std::function<void(qlonglong i, const QString& s, QByteArray b)> callback) {
    if (!callback) {
        qWarning() << "FullApiVeneer: ignoring empty event callback for" << QStringLiteral("tripleEvent");
        return false;
    }
    return logos::qt::subscribe(m_bridge, "tripleEvent", [callback](nlohmann::json _a) {
        if (!_a.is_array() || _a.size() < 3) return;
        callback(logos::qt::fromWire<qlonglong>(_a.at(0)), logos::qt::fromWire<QString>(_a.at(1)), logos::qt::fromWire<QByteArray>(_a.at(2)));
    });
}

QString FullApiVeneer::whoAmI(logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "whoAmI", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::whoAmI: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<QString>(_r);
}

void FullApiVeneer::whoAmIAsync(std::function<void(QString)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    logos::qt::invokeAsync(m_bridge, "whoAmI", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<QString>(_r));
        }, timeout.ms);
}

QString FullApiVeneer::echoString(const QString& v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "echoString", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::echoString: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<QString>(_r);
}

void FullApiVeneer::echoStringAsync(const QString& v, std::function<void(QString)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "echoString", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<QString>(_r));
        }, timeout.ms);
}

QByteArray FullApiVeneer::echoBytes(QByteArray v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "echoBytes", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::echoBytes: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<QByteArray>(_r);
}

void FullApiVeneer::echoBytesAsync(QByteArray v, std::function<void(QByteArray)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "echoBytes", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<QByteArray>(_r));
        }, timeout.ms);
}

qlonglong FullApiVeneer::echoInt(qlonglong v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "echoInt", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::echoInt: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<qlonglong>(_r);
}

void FullApiVeneer::echoIntAsync(qlonglong v, std::function<void(qlonglong)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "echoInt", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<qlonglong>(_r));
        }, timeout.ms);
}

qulonglong FullApiVeneer::echoUint(qulonglong v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "echoUint", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::echoUint: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<qulonglong>(_r);
}

void FullApiVeneer::echoUintAsync(qulonglong v, std::function<void(qulonglong)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "echoUint", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<qulonglong>(_r));
        }, timeout.ms);
}

double FullApiVeneer::echoDouble(double v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "echoDouble", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::echoDouble: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<double>(_r);
}

void FullApiVeneer::echoDoubleAsync(double v, std::function<void(double)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "echoDouble", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<double>(_r));
        }, timeout.ms);
}

bool FullApiVeneer::echoBool(bool v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "echoBool", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::echoBool: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<bool>(_r);
}

void FullApiVeneer::echoBoolAsync(bool v, std::function<void(bool)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "echoBool", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<bool>(_r));
        }, timeout.ms);
}

QVariant FullApiVeneer::echoAny(QVariant v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "echoAny", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::echoAny: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<QVariant>(_r);
}

void FullApiVeneer::echoAnyAsync(QVariant v, std::function<void(QVariant)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "echoAny", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<QVariant>(_r));
        }, timeout.ms);
}

QStringList FullApiVeneer::echoStringList(const QStringList& v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "echoStringList", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::echoStringList: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<QStringList>(_r);
}

void FullApiVeneer::echoStringListAsync(const QStringList& v, std::function<void(QStringList)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "echoStringList", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<QStringList>(_r));
        }, timeout.ms);
}

QVariantList FullApiVeneer::echoIntList(const QVariantList& v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "echoIntList", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::echoIntList: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<QVariantList>(_r);
}

void FullApiVeneer::echoIntListAsync(const QVariantList& v, std::function<void(QVariantList)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "echoIntList", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<QVariantList>(_r));
        }, timeout.ms);
}

QVariantList FullApiVeneer::echoUintList(const QVariantList& v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "echoUintList", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::echoUintList: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<QVariantList>(_r);
}

void FullApiVeneer::echoUintListAsync(const QVariantList& v, std::function<void(QVariantList)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "echoUintList", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<QVariantList>(_r));
        }, timeout.ms);
}

QVariantList FullApiVeneer::echoDoubleList(const QVariantList& v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "echoDoubleList", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::echoDoubleList: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<QVariantList>(_r);
}

void FullApiVeneer::echoDoubleListAsync(const QVariantList& v, std::function<void(QVariantList)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "echoDoubleList", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<QVariantList>(_r));
        }, timeout.ms);
}

QVariantList FullApiVeneer::echoBoolList(const QVariantList& v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "echoBoolList", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::echoBoolList: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<QVariantList>(_r);
}

void FullApiVeneer::echoBoolListAsync(const QVariantList& v, std::function<void(QVariantList)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "echoBoolList", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<QVariantList>(_r));
        }, timeout.ms);
}

QVariantList FullApiVeneer::echoList(const QVariantList& v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "echoList", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::echoList: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<QVariantList>(_r);
}

void FullApiVeneer::echoListAsync(const QVariantList& v, std::function<void(QVariantList)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "echoList", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<QVariantList>(_r));
        }, timeout.ms);
}

QVariantMap FullApiVeneer::echoMap(const QVariantMap& v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "echoMap", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::echoMap: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<QVariantMap>(_r);
}

void FullApiVeneer::echoMapAsync(const QVariantMap& v, std::function<void(QVariantMap)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "echoMap", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<QVariantMap>(_r));
        }, timeout.ms);
}

void FullApiVeneer::doVoid(logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "doVoid", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::doVoid: remote call failed:" << QString::fromStdString(_err.message);
    (void)_r;
}

void FullApiVeneer::doVoidAsync(std::function<void()> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    logos::qt::invokeAsync(m_bridge, "doVoid", _args,
        [callback](nlohmann::json _r) {
            (void)_r; callback();
        }, timeout.ms);
}

QString FullApiVeneer::echoTriple(qlonglong i, const QString& s, QByteArray b, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(i)));
    _args.push_back(logos::qt::toWire(QVariant::fromValue(s)));
    _args.push_back(logos::qt::toWire(QVariant::fromValue(b)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "echoTriple", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::echoTriple: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<QString>(_r);
}

void FullApiVeneer::echoTripleAsync(qlonglong i, const QString& s, QByteArray b, std::function<void(QString)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(i)));
    _args.push_back(logos::qt::toWire(QVariant::fromValue(s)));
    _args.push_back(logos::qt::toWire(QVariant::fromValue(b)));
    logos::qt::invokeAsync(m_bridge, "echoTriple", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<QString>(_r));
        }, timeout.ms);
}

LogosResult FullApiVeneer::makeResult(bool ok, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(ok)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "makeResult", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::makeResult: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<LogosResult>(_r);
}

void FullApiVeneer::makeResultAsync(bool ok, std::function<void(LogosResult)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(ok)));
    logos::qt::invokeAsync(m_bridge, "makeResult", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<LogosResult>(_r));
        }, timeout.ms);
}

bool FullApiVeneer::fireStringEvent(const QString& v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "fireStringEvent", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::fireStringEvent: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<bool>(_r);
}

void FullApiVeneer::fireStringEventAsync(const QString& v, std::function<void(bool)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "fireStringEvent", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<bool>(_r));
        }, timeout.ms);
}

bool FullApiVeneer::fireBytesEvent(QByteArray v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "fireBytesEvent", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::fireBytesEvent: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<bool>(_r);
}

void FullApiVeneer::fireBytesEventAsync(QByteArray v, std::function<void(bool)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "fireBytesEvent", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<bool>(_r));
        }, timeout.ms);
}

bool FullApiVeneer::fireIntEvent(qlonglong v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "fireIntEvent", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::fireIntEvent: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<bool>(_r);
}

void FullApiVeneer::fireIntEventAsync(qlonglong v, std::function<void(bool)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "fireIntEvent", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<bool>(_r));
        }, timeout.ms);
}

bool FullApiVeneer::fireUintEvent(qulonglong v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "fireUintEvent", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::fireUintEvent: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<bool>(_r);
}

void FullApiVeneer::fireUintEventAsync(qulonglong v, std::function<void(bool)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "fireUintEvent", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<bool>(_r));
        }, timeout.ms);
}

bool FullApiVeneer::fireDoubleEvent(double v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "fireDoubleEvent", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::fireDoubleEvent: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<bool>(_r);
}

void FullApiVeneer::fireDoubleEventAsync(double v, std::function<void(bool)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "fireDoubleEvent", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<bool>(_r));
        }, timeout.ms);
}

bool FullApiVeneer::fireBoolEvent(bool v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "fireBoolEvent", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::fireBoolEvent: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<bool>(_r);
}

void FullApiVeneer::fireBoolEventAsync(bool v, std::function<void(bool)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "fireBoolEvent", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<bool>(_r));
        }, timeout.ms);
}

bool FullApiVeneer::fireAnyEvent(QVariant v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "fireAnyEvent", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::fireAnyEvent: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<bool>(_r);
}

void FullApiVeneer::fireAnyEventAsync(QVariant v, std::function<void(bool)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "fireAnyEvent", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<bool>(_r));
        }, timeout.ms);
}

bool FullApiVeneer::fireStringListEvent(const QStringList& v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "fireStringListEvent", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::fireStringListEvent: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<bool>(_r);
}

void FullApiVeneer::fireStringListEventAsync(const QStringList& v, std::function<void(bool)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "fireStringListEvent", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<bool>(_r));
        }, timeout.ms);
}

bool FullApiVeneer::fireIntListEvent(const QVariantList& v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "fireIntListEvent", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::fireIntListEvent: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<bool>(_r);
}

void FullApiVeneer::fireIntListEventAsync(const QVariantList& v, std::function<void(bool)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "fireIntListEvent", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<bool>(_r));
        }, timeout.ms);
}

bool FullApiVeneer::fireUintListEvent(const QVariantList& v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "fireUintListEvent", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::fireUintListEvent: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<bool>(_r);
}

void FullApiVeneer::fireUintListEventAsync(const QVariantList& v, std::function<void(bool)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "fireUintListEvent", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<bool>(_r));
        }, timeout.ms);
}

bool FullApiVeneer::fireDoubleListEvent(const QVariantList& v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "fireDoubleListEvent", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::fireDoubleListEvent: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<bool>(_r);
}

void FullApiVeneer::fireDoubleListEventAsync(const QVariantList& v, std::function<void(bool)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "fireDoubleListEvent", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<bool>(_r));
        }, timeout.ms);
}

bool FullApiVeneer::fireBoolListEvent(const QVariantList& v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "fireBoolListEvent", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::fireBoolListEvent: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<bool>(_r);
}

void FullApiVeneer::fireBoolListEventAsync(const QVariantList& v, std::function<void(bool)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "fireBoolListEvent", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<bool>(_r));
        }, timeout.ms);
}

bool FullApiVeneer::fireListEvent(const QVariantList& v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "fireListEvent", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::fireListEvent: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<bool>(_r);
}

void FullApiVeneer::fireListEventAsync(const QVariantList& v, std::function<void(bool)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "fireListEvent", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<bool>(_r));
        }, timeout.ms);
}

bool FullApiVeneer::fireMapEvent(const QVariantMap& v, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "fireMapEvent", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::fireMapEvent: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<bool>(_r);
}

void FullApiVeneer::fireMapEventAsync(const QVariantMap& v, std::function<void(bool)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(v)));
    logos::qt::invokeAsync(m_bridge, "fireMapEvent", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<bool>(_r));
        }, timeout.ms);
}

bool FullApiVeneer::fireTripleEvent(qlonglong i, const QString& s, QByteArray b, logos::CallError* err) {
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(i)));
    _args.push_back(logos::qt::toWire(QVariant::fromValue(s)));
    _args.push_back(logos::qt::toWire(QVariant::fromValue(b)));
    logos::CallError _err;
    nlohmann::json _r = logos::qt::invoke(m_bridge, "fireTripleEvent", _args, &_err);
    if (err) *err = _err;
    else if (!_err.ok()) qWarning() << "FullApiVeneer::fireTripleEvent: remote call failed:" << QString::fromStdString(_err.message);
    return logos::qt::fromWire<bool>(_r);
}

void FullApiVeneer::fireTripleEventAsync(qlonglong i, const QString& s, QByteArray b, std::function<void(bool)> callback, Timeout timeout) {
    if (!callback) return;
    nlohmann::json _args = nlohmann::json::array();
    _args.push_back(logos::qt::toWire(QVariant::fromValue(i)));
    _args.push_back(logos::qt::toWire(QVariant::fromValue(s)));
    _args.push_back(logos::qt::toWire(QVariant::fromValue(b)));
    logos::qt::invokeAsync(m_bridge, "fireTripleEvent", _args,
        [callback](nlohmann::json _r) {
            callback(logos::qt::fromWire<bool>(_r));
        }, timeout.ms);
}

