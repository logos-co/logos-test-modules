#ifndef DUMMY_MODULE_GENERATOR_H
#define DUMMY_MODULE_GENERATOR_H

#include <QString>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QVector>
#include <cstdlib>
#ifdef Q_OS_MACOS
#include <cstring>
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
#endif

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

#ifdef Q_OS_MACOS
            // The patch invalidates the template's page hashes, and Apple Silicon SIGKILLs any
            // process that maps such a page (CODESIGNING, "Invalid Page") — so re-sign ad hoc.
            adhocSign(filePath);
#endif

            result.append({moduleName, filePath});
        }

        return result;
    }

private:
#ifdef Q_OS_MACOS
    // Fatal rather than an empty result: SetUp reads "no modules" as "no template" and skips.
    static void adhocSign(const QString& path) {
        QByteArray file = QFile::encodeName(path);
        char* argv[] = {const_cast<char*>("codesign"), const_cast<char*>("--force"),
                        const_cast<char*>("--sign"), const_cast<char*>("-"), file.data(), nullptr};
        pid_t pid = 0;
        if (int e = posix_spawnp(&pid, "codesign", nullptr, nullptr, argv, environ))
            qFatal("DummyModuleGenerator: cannot run codesign for %s: %s", file.constData(), strerror(e));
        int status = 0;
        if (waitpid(pid, &status, 0) != pid || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
            qFatal("DummyModuleGenerator: codesign failed for %s (status %d)", file.constData(), status);
    }
#endif

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
