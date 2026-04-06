#ifndef DUMMY_MODULE_GENERATOR_H
#define DUMMY_MODULE_GENERATOR_H

#include <QString>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QVector>
#include <cstdlib>

struct DummyModule {
    QString name;
    QString path;
};

class DummyModuleGenerator {
public:
    static QVector<DummyModule> generate(int count, const QString& outputDir) {
        QString templatePath = findTemplate();
        if (templatePath.isEmpty()) return {};

        QFile templateFile(templatePath);
        if (!templateFile.open(QIODevice::ReadOnly)) return {};
        QByteArray templateData = templateFile.readAll();
        templateFile.close();

        QDir().mkpath(outputDir);

        QString ext = QFileInfo(templatePath).suffix();
        if (!ext.isEmpty()) ext.prepend('.');

        static const QByteArray kTemplateName = "dummy_module_000000";

        if (!templateData.contains(kTemplateName)) {
            qWarning("DummyModuleGenerator: template binary does not contain marker '%s' — "
                     "binary patching will not work", kTemplateName.constData());
            return {};
        }

        QVector<DummyModule> result;
        result.reserve(count);

        for (int i = 0; i < count; ++i) {
            QString moduleName = QString("dummy_module_%1").arg(i, 6, 10, QChar('0'));
            QByteArray nameBytes = moduleName.toUtf8();

            QByteArray patched = templateData;
            patched.replace(kTemplateName, nameBytes);

            QString filePath = QDir(outputDir).absoluteFilePath(
                QString("lib%1_plugin%2").arg(moduleName, ext));

            QFile out(filePath);
            if (!out.open(QIODevice::WriteOnly)) return {};
            out.write(patched);
            out.close();

            QFile::setPermissions(filePath,
                QFileDevice::ReadOwner  | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                QFileDevice::ReadGroup  | QFileDevice::ExeGroup |
                QFileDevice::ReadOther  | QFileDevice::ExeOther);

            result.append({moduleName, filePath});
        }

        return result;
    }

private:
    static QString findTemplate() {
        const char* env = std::getenv("DUMMY_PLUGIN_TEMPLATE_DIR");
        if (env && env[0]) return findIn(QString::fromUtf8(env));

#ifdef DUMMY_PLUGIN_TEMPLATE_DIR
        QString fromDefine = findIn(QString(DUMMY_PLUGIN_TEMPLATE_DIR));
        if (!fromDefine.isEmpty()) return fromDefine;
#endif
        return {};
    }

    static QString findIn(const QString& dir) {
        QDir d(dir);
        for (const QFileInfo& fi : d.entryInfoList(QDir::Files)) {
            const QString fn = fi.fileName();
            if (fn.startsWith("dummy_module_000000_plugin") ||
                fn.startsWith("libdummy_module_000000_plugin"))
                return fi.absoluteFilePath();
        }
        return {};
    }
};

#endif // DUMMY_MODULE_GENERATOR_H
