// Noctalia for Windows — an unofficial port of the Noctalia desktop shell (https://noctalia.dev).

#include "core/Config.h"
#include "core/Glyphs.h"
#include "core/Theme.h"
#include "platform/Platform.h"
#include "services/AudioService.h"
#include "services/ClipboardService.h"
#include "services/DeviceServices.h"
#include "services/WallpaperService.h"
#include "services/WeatherService.h"
#include "services/IconProvider.h"
#include "services/LauncherService.h"
#include "services/PanelService.h"
#include "services/ShellService.h"
#include "services/SystemServices.h"

#include <QDir>
#include <QEventLoop>
#include <QGuiApplication>
#include <QLocalSocket>
#include <QLockFile>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QMouseEvent>
#include <QTimer>
#include <QtQml>

namespace {

  // Debug helper: NOCTALIA_SCREENSHOT=bar.png saves the first bar to a PNG and quits.
  // NOCTALIA_SCREENSHOT_WINDOW=launcher grabs that window instead (objectName).
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
                           if (qw != nullptr && qw->objectName() == qEnvironmentVariable("NOCTALIA_SCREENSHOT_WINDOW", "bar") && qw->isVisible()) {
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
  // A second `noctalia.exe toggle launcher` just forwards its arguments to the running one.
  const QStringList args = QCoreApplication::arguments().mid(1);
  QLockFile lock(QDir::temp().filePath("noctalia-win.lock"));
  if (!lock.tryLock(100)) {
    QLocalSocket socket;
    socket.connectToServer(PanelService::kServerName);
    if (socket.waitForConnected(1000)) {
      socket.write(args.join('\n').toUtf8());
      socket.waitForBytesWritten(1000);
      socket.waitForDisconnected(1000);
    }
    return 0;
  }

  platform::initialize();

  Config config;
  WallpaperService wallpaper(&config);
  Theme theme(&config, &wallpaper);
  Glyphs glyphs;
  ShellService shell(&config);
  AudioService audio;
  BatteryService battery;
  NetworkService network;
  WorkspaceService workspaces;
  SystemMonitorService systemMonitor;
  MediaService media;
  BluetoothService bluetooth;
  WifiService wifi;
  BrightnessService brightness;
  WeatherService weather(&config);
  ClipboardService clipboard(&config);
  PanelService panels(&config);
  LauncherService launcher(&config);

  // Everything QML needs lives in the "Noctalia" module.
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Config", &config);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Theme", &theme);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Glyphs", &glyphs);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Shell", &shell);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Audio", &audio);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Battery", &battery);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Network", &network);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Workspaces", &workspaces);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "SystemMonitor", &systemMonitor);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Media", &media);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Bluetooth", &bluetooth);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Wifi", &wifi);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Brightness", &brightness);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Weather", &weather);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Wallpaper", &wallpaper);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Clipboard", &clipboard);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Panels", &panels);
  qmlRegisterSingletonInstance("Noctalia", 1, 0, "Launcher", &launcher);
  qmlRegisterType<AppBarItem>("Noctalia", 1, 0, "AppBar");

  QQmlApplicationEngine engine;
  engine.addImageProvider("icons", new IconProvider); // the engine takes ownership
  engine.addImageProvider("media", new MediaArtProvider(&media));
  // NOCTALIA_QML_DIR=path/to/qml loads QML from disk (live editing without rebuilding).
  const QString qmlDir = qEnvironmentVariable("NOCTALIA_QML_DIR");
  const QUrl mainUrl = qmlDir.isEmpty() ? QUrl(QStringLiteral("qrc:/qml/Main.qml"))
                                        : QUrl::fromLocalFile(QDir(qmlDir).filePath("Main.qml"));
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
      Qt::QueuedConnection);
  engine.load(mainUrl);

  // NOCTALIA_OPEN=launcher opens a panel right away (debugging / screenshots).
  if (!qEnvironmentVariableIsEmpty("NOCTALIA_OPEN")) {
    QTimer::singleShot(300, &panels, [&panels] { panels.open(qEnvironmentVariable("NOCTALIA_OPEN")); });
  }
  if (!args.isEmpty()) {
    QTimer::singleShot(300, &panels, [&panels, args] { panels.runCommand(args); });
  }

  scheduleScreenshot(engine);
  return QGuiApplication::exec();
}
