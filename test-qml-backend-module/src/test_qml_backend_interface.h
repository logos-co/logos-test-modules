#pragma once
#include <QObject>
#include <QString>
#include "interface.h"

class TestQmlBackendInterface : public PluginInterface {
public:
    virtual ~TestQmlBackendInterface() = default;
};

#define TestQmlBackendInterface_iid "logos.test.qml_backend/1.0"
Q_DECLARE_INTERFACE(TestQmlBackendInterface, TestQmlBackendInterface_iid)
