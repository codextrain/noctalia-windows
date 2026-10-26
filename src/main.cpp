// Noctalia for Windows — an unofficial port of the Noctalia desktop shell (https://noctalia.dev).

#include "core/Config.h"
#include "core/Glyphs.h"
#include "core/Theme.h"
#include "platform/Platform.h"
#include "services/AudioService.h"
#include "services/ShellService.h"
#include "services/SystemServices.h"

#include <QDir>
#include <QEventLoop>
#include <QGuiApplication>
#include <QLockFile>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QMouseEvent>
#include <QTimer>
#include <QtQml>

namespace {

  // Debug helper: NOCTALIA_SCREENSHOT=bar.png saves the first bar to a PNG and quits.
  void scheduleScreenshot(QQmlApplicationEngine& engine) {
    const QString out = qEnvironmentVariable("NOCTALIA_SCREENSHOT");
    if (out.isEmpty()) {
      return;
    }
    QTimer::singleShot(qEnvironmentVariableIntValue("NOCTALIA_SCREENSHOT_DELAY") > 0
                           ? qEnvironmentVariableIntValue("NOCTALIA_SCREENSHOT_DELAY")
                           : 1500,
                       [&engine, out] {
                         for (QWindow* w : QGuiApplication::topLevelWindows()) {
                           auto* qw = qobject_cast<QQuickWindow*>(w);
                           if (qw != nullptr && qw->objectName() == QLatin1String("bar") && qw->isVisible()) {
                             // NOCTALIA_SCREENSHOT_HOVER=x,y moves the mouse there first (to check hover states).
                             const QStringList xy = qEnvironmentVariable("NOCTALIA_SCREENSHOT_HOVER").split(',');
                             if (xy.size() == 2) {
                               const QPointF p(xy[0].toDouble(), xy[1].toDouble());
                               QMouseEvent move(QEvent::MouseMove, p, qw->mapToGlobal(p), Qt::NoButton, Qt::NoButton,
                                                Qt::NoModifier);
                               QCoreApplication::sendEvent(qw, &move);
                               QEventLoop loop;
                               QTimer::singleShot(400, &loop, &QEventLoop::quit);
                               loop.exec();
                             }
                             qw->grabWindow().save(out);
                             break;
                           }
                         }
                         QCoreApplication::quit();
                       });
  }

} // namespace

int main(int argc, char* argv[]) {
  QGuiApplication app(argc, argv);
  QGuiApplication::setApplicationName("noctalia-win");
  QGuiApplication::setApplicationDisplayName("Noctalia");
  QGuiApplication::setApplicationVersion(NOCTALIA_WIN_VERSION);
  QGuiApplication::setQuitOnLastWindowClosed(false);

  // Only one shell at a time (two bars would fight over the reserved space).
  QLockFile lock(QDir::temp().filePath("noctalia-win.lock"));
  if (!lock.tryLock(100)) {
    qWarning("noctalia-win is already running.");
    return 0;
  }

  platform::initialize();

  Config config;
  Theme theme(&config);
  Glyphs glyphs;
  ShellService shell(&config);
  AudioService audio;
  BatteryService battery;
  NetworkService network;
  WorkspaceService workspaces;

  // Everything QML needs lives in the "Noctalia" module.
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Config", &config);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Theme", &theme);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Glyphs", &glyphs);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Shell", &shell);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Audio", &audio);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Battery", &battery);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Network", &network);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Workspaces", &workspaces);
  qmlRegisterType<AppBarItem>("Noctalia", 1, 0, "AppBar");

  QQmlApplicationEngine engine;
  // NOCTALIA_QML_DIR=path/to/qml loads QML from disk (live editing without rebuilding).
  const QString qmlDir = qEnvironmentVariable("NOCTALIA_QML_DIR");
  const QUrl mainUrl = qmlDir.isEmpty() ? QUrl(QStringLiteral("qrc:/qml/Main.qml"))
                                        : QUrl::fromLocalFile(QDir(qmlDir).filePath("Main.qml"));
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
      Qt::QueuedConnection);
  engine.load(mainUrl);

  scheduleScreenshot(engine);
  return QGuiApplication::exec();
}
