#include "services/HardwareService.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcessEnvironment>

HardwareService::HardwareService(QObject* parent) : QObject(parent) {
  m_timeout.setSingleShot(true);
  m_timeout.setInterval(30000);
  connect(&m_timeout, &QTimer::timeout, this, [this] {
    fail(QStringLiteral("A consulta demorou demais. Tente atualizar."));
    m_process.kill();
  });
  connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
    if (m_busy) fail(QStringLiteral("Não foi possível consultar os componentes do Windows."));
  });
  connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
          [this](int code, QProcess::ExitStatus status) {
    if (!m_busy) return;
    m_timeout.stop();
    const auto doc = QJsonDocument::fromJson(m_process.readAllStandardOutput());
    if (code != 0 || status != QProcess::NormalExit || !doc.isArray() || doc.array().isEmpty()) {
      fail(QStringLiteral("Não foi possível ler os componentes. Tente atualizar."));
      return;
    }
    m_components = doc.array().toVariantList();
    m_busy = false;
    m_loaded = true;
    emit changed();
  });
}

void HardwareService::fail(const QString& message) {
  m_timeout.stop();
  m_busy = false;
  m_error = message;
  emit changed();
}

void HardwareService::ensureLoaded() { if (!m_loaded) refresh(); }

void HardwareService::refresh() {
  if (m_busy || m_process.state() != QProcess::NotRunning) return;
  m_error.clear();
  m_busy = true;
  emit changed();
#ifdef Q_OS_WIN
  QFile script(QStringLiteral(":/assets/system-info.ps1"));
  if (!script.open(QIODevice::ReadOnly)) {
    fail(QStringLiteral("O leitor de componentes não foi encontrado."));
    return;
  }
  const auto windows = QProcessEnvironment::systemEnvironment().value(QStringLiteral("SystemRoot"), QStringLiteral("C:/Windows"));
  m_process.start(windows + QStringLiteral("/System32/WindowsPowerShell/v1.0/powershell.exe"),
                  {QStringLiteral("-NoLogo"), QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                   QStringLiteral("-WindowStyle"), QStringLiteral("Hidden"), QStringLiteral("-Command"),
                   QString::fromUtf8(script.readAll()).remove(QChar(0xfeff))});
  m_timeout.start();
#else
  fail(QStringLiteral("As informações de hardware estão disponíveis no Windows."));
#endif
}
