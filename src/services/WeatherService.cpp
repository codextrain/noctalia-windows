#include "services/WeatherService.h"

#include "core/Config.h"

#include <QDate>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QNetworkReply>
#include <QUrlQuery>

#include <cmath>

WeatherService::WeatherService(Config* config, QObject* parent) : QObject(parent), m_config(config) {
  connect(m_config, &Config::changed, this, &WeatherService::onConfigChanged);
  connect(&m_timer, &QTimer::timeout, this, &WeatherService::refresh);
  onConfigChanged();
}

bool WeatherService::enabled() const { return m_config->weather().value("enabled", true).toBool(); }

QString WeatherService::unit() const {
  return m_config->weather().value("unit", "celsius").toString() == "fahrenheit" ? QStringLiteral("°F") : QStringLiteral("°C");
}

double WeatherService::convert(double celsius) const {
  return unit() == QStringLiteral("°F") ? 32.0 + celsius * 1.8 : celsius;
}

QString WeatherService::temperatureText() const {
  if (!m_hasData) {
    return QStringLiteral("--");
  }
  return QStringLiteral("%1%2").arg(std::lround(temperature())).arg(unit());
}

QVariantList WeatherService::daily() const {
  QVariantList out;
  const QLocale locale;
  for (const QVariant& v : m_dailyRaw) {
    QVariantMap d = v.toMap();
    const QDate date = QDate::fromString(d.value("date").toString(), Qt::ISODate);
    QString name = date == QDate::currentDate() ? QStringLiteral("Hoje") : locale.toString(date, "ddd");
    name = name.left(1).toUpper() + name.mid(1);
    name.remove('.');
    d["dayName"] = name;
    d["min"] = static_cast<int>(std::lround(convert(d.value("minC").toDouble())));
    d["max"] = static_cast<int>(std::lround(convert(d.value("maxC").toDouble())));
    d["glyph"] = glyphFor(d.value("code").toInt(), true);
    d["description"] = describe(d.value("code").toInt());
    out.append(d);
  }
  return out;
}

// Same mapping as noctalia/src/system/weather_service.cpp (WMO weather codes).
QString WeatherService::glyphFor(int code, bool isDay) {
  if (code < 0) return "weather-cloud";
  if (code == 0) return isDay ? "weather-sun" : "weather-moon";
  if (code == 1 || code == 2) return isDay ? "weather-cloud-sun" : "weather-moon-stars";
  if (code == 3) return "weather-cloud";
  if (code >= 45 && code <= 48) return "weather-cloud-haze";
  if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) return "weather-cloud-rain";
  if ((code >= 71 && code <= 77) || (code >= 85 && code <= 86)) return "weather-cloud-snow";
  if (code >= 95 && code <= 99) return "weather-cloud-lightning";
  return "weather-cloud";
}

QString WeatherService::describe(int code) {
  if (code < 0) return QStringLiteral("Clima");
  if (code == 0) return QStringLiteral("Céu limpo");
  if (code == 1) return QStringLiteral("Poucas nuvens");
  if (code == 2) return QStringLiteral("Parcialmente nublado");
  if (code == 3) return QStringLiteral("Encoberto");
  if (code == 45 || code == 48) return QStringLiteral("Nevoeiro");
  if (code >= 51 && code <= 57) return QStringLiteral("Garoa");
  if (code >= 61 && code <= 67) return QStringLiteral("Chuva");
  if (code >= 71 && code <= 77) return QStringLiteral("Neve");
  if (code >= 80 && code <= 82) return QStringLiteral("Pancadas de chuva");
  if (code >= 85 && code <= 86) return QStringLiteral("Pancadas de neve");
  if (code >= 95) return QStringLiteral("Tempestade com raios");
  return QStringLiteral("Desconhecido");
}

void WeatherService::onConfigChanged() {
  const QVariantMap loc = m_config->location();
  const QString key = loc.value("auto_locate", false).toBool() ? QStringLiteral("auto")
                                                               : loc.value("address").toString().trimmed();
  const int minutes = std::max(5, m_config->weather().value("refresh_minutes", 30).toInt());
  m_timer.setInterval(minutes * 60 * 1000);
  if (key != m_locationKey) {
    m_locationKey = key;
    m_hasCoords = false;
    refresh();
  } else {
    emit changed(); // unit may have changed
  }
  if (enabled()) {
    m_timer.start();
  } else {
    m_timer.stop();
  }
}

void WeatherService::fail(const QString& message) {
  m_loading = false;
  m_error = message;
  emit changed();
}

void WeatherService::refresh() {
  if (!enabled()) {
    return;
  }
  if (m_hasCoords) {
    fetchForecast(m_lat, m_lon);
    return;
  }
  const QVariantMap loc = m_config->location();
  m_loading = true;
  m_error.clear();
  emit changed();

  if (loc.value("auto_locate", false).toBool()) {
    QNetworkReply* reply = m_net.get(QNetworkRequest(QUrl("https://ipwho.is/")));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
      reply->deleteLater();
      const QJsonObject o = QJsonDocument::fromJson(reply->readAll()).object();
      if (reply->error() != QNetworkReply::NoError || !o.value("success").toBool(true)) {
        fail(QStringLiteral("Não foi possível descobrir sua localização pela internet."));
        return;
      }
      m_lat = o.value("latitude").toDouble();
      m_lon = o.value("longitude").toDouble();
      m_locationName = o.value("city").toString();
      m_hasCoords = true;
      fetchForecast(m_lat, m_lon);
    });
    return;
  }

  const QString address = loc.value("address").toString().trimmed();
  if (address.isEmpty()) {
    fail(QStringLiteral("Defina sua cidade no config: \"location\": { \"address\": \"Sua cidade\" }"));
    return;
  }
  QUrl url("https://geocoding-api.open-meteo.com/v1/search");
  QUrlQuery q;
  // "Joinville, SC" -> search the first part, it's what the API understands
  q.addQueryItem("name", address.section(',', 0, 0).trimmed());
  q.addQueryItem("count", "1");
  q.addQueryItem("language", "pt");
  q.addQueryItem("format", "json");
  url.setQuery(q);
  QNetworkReply* reply = m_net.get(QNetworkRequest(url));
  connect(reply, &QNetworkReply::finished, this, [this, reply, address] {
    reply->deleteLater();
    const QJsonArray results = QJsonDocument::fromJson(reply->readAll()).object().value("results").toArray();
    if (reply->error() != QNetworkReply::NoError || results.isEmpty()) {
      fail(QStringLiteral("Cidade não encontrada: %1").arg(address));
      return;
    }
    const QJsonObject r = results.first().toObject();
    m_lat = r.value("latitude").toDouble();
    m_lon = r.value("longitude").toDouble();
    m_locationName = r.value("name").toString();
    const QString region = r.value("admin1").toString();
    if (!region.isEmpty()) {
      m_locationName += ", " + region;
    }
    m_hasCoords = true;
    fetchForecast(m_lat, m_lon);
  });
}

void WeatherService::fetchForecast(double lat, double lon) {
  m_loading = true;
  emit changed();
  QUrl url("https://api.open-meteo.com/v1/forecast");
  QUrlQuery q;
  q.addQueryItem("latitude", QString::number(lat, 'f', 4));
  q.addQueryItem("longitude", QString::number(lon, 'f', 4));
  q.addQueryItem("current", "temperature_2m,apparent_temperature,relative_humidity_2m,wind_speed_10m,weather_code,is_day");
  q.addQueryItem("daily", "weather_code,temperature_2m_max,temperature_2m_min");
  q.addQueryItem("timezone", "auto");
  q.addQueryItem("forecast_days", "6");
  url.setQuery(q);
  QNetworkReply* reply = m_net.get(QNetworkRequest(url));
  connect(reply, &QNetworkReply::finished, this, [this, reply] {
    reply->deleteLater();
    const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
    if (reply->error() != QNetworkReply::NoError || !root.contains("current")) {
      fail(QStringLiteral("Falha ao buscar dados do clima"));
      return;
    }
    const QJsonObject cur = root.value("current").toObject();
    m_temp = cur.value("temperature_2m").toDouble();
    m_apparent = cur.value("apparent_temperature").toDouble();
    m_humidity = cur.value("relative_humidity_2m").toInt();
    m_wind = cur.value("wind_speed_10m").toDouble();
    m_code = cur.value("weather_code").toInt();
    m_isDay = cur.value("is_day").toInt(1) == 1;

    const QJsonObject daily = root.value("daily").toObject();
    const QJsonArray dates = daily.value("time").toArray();
    const QJsonArray codes = daily.value("weather_code").toArray();
    const QJsonArray maxs = daily.value("temperature_2m_max").toArray();
    const QJsonArray mins = daily.value("temperature_2m_min").toArray();
    m_dailyRaw.clear();
    for (int i = 0; i < dates.size(); ++i) {
      m_dailyRaw.append(QVariantMap{{"date", dates[i].toString()},
                                    {"code", codes[i].toInt()},
                                    {"maxC", maxs[i].toDouble()},
                                    {"minC", mins[i].toDouble()}});
    }
    m_hasData = true;
    m_loading = false;
    m_error.clear();
    emit changed();
  });
}
