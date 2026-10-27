#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QVariantList>

class Config;

// Weather from Open-Meteo (free, no API key), like Noctalia. Location comes from config:
//   "location": { "address": "Joinville" }  or  { "auto_locate": true }  (by IP)
// QML: Weather.available, temperatureText, glyph, description, locationName, daily [...]
class WeatherService : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool enabled READ enabled NOTIFY changed)
  Q_PROPERTY(bool available READ available NOTIFY changed)
  Q_PROPERTY(bool loading READ loading NOTIFY changed)
  Q_PROPERTY(QString error READ error NOTIFY changed)
  Q_PROPERTY(QString locationName READ locationName NOTIFY changed)
  Q_PROPERTY(double temperature READ temperature NOTIFY changed)
  Q_PROPERTY(double apparent READ apparent NOTIFY changed)
  Q_PROPERTY(int humidity READ humidity NOTIFY changed)
  Q_PROPERTY(double wind READ wind NOTIFY changed)
  Q_PROPERTY(QString glyph READ glyph NOTIFY changed)
  Q_PROPERTY(QString description READ description NOTIFY changed)
  Q_PROPERTY(QString temperatureText READ temperatureText NOTIFY changed)
  Q_PROPERTY(QString unit READ unit NOTIFY changed)
  Q_PROPERTY(QVariantList daily READ daily NOTIFY changed)

public:
  explicit WeatherService(Config* config, QObject* parent = nullptr);

  bool enabled() const;
  bool available() const { return m_hasData; }
  bool loading() const { return m_loading; }
  QString error() const { return m_error; }
  QString locationName() const { return m_locationName; }
  double temperature() const { return convert(m_temp); }
  double apparent() const { return convert(m_apparent); }
  int humidity() const { return m_humidity; }
  double wind() const { return m_wind; }
  QString glyph() const { return glyphFor(m_code, m_isDay); }
  QString description() const { return describe(m_code); }
  QString temperatureText() const;
  QString unit() const;
  QVariantList daily() const;

  Q_INVOKABLE void refresh();

  static QString glyphFor(int code, bool isDay);
  static QString describe(int code);

signals:
  void changed();

private:
  void onConfigChanged();
  void fetchForecast(double lat, double lon);
  double convert(double celsius) const;
  void fail(const QString& message);

  Config* m_config = nullptr;
  QNetworkAccessManager m_net;
  QTimer m_timer;
  QString m_locationKey; // address / auto, to notice config changes
  bool m_hasData = false;
  bool m_loading = false;
  QString m_error;
  QString m_locationName;
  double m_lat = 0, m_lon = 0;
  bool m_hasCoords = false;
  double m_temp = 0, m_apparent = 0, m_wind = 0;
  int m_humidity = 0;
  int m_code = -1;
  bool m_isDay = true;
  QVariantList m_dailyRaw; // celsius
};
