#include <QDebug>
#include <QDBusConnection>
#include <QLoggingCategory>

#include "bgdatareceiver.hpp"
#include "jsonhelper.hpp"
#include "extappmsgreceiverifaceadaptor.h"


Q_DECLARE_LOGGING_CATEGORY(lcQmlBgData)


namespace {

QString const DBUS_SERVICE_NAME = "org.asteroidos.externalappmessages.BGDataReceiver";
QString const DBUS_OBJECT_PATH = "/org/asteroidos/externalappmessages/BGDataReceiver";

enum class MessageID
{
	NEW_BG_DATA,
	BG_DATA_UPDATE
};

template<typename T>
QVariant toQVariant(std::optional<T> const &optValue)
{
	return optValue.has_value() ? QVariant::fromValue(*optValue) : QVariant();
}

} // unnamed namespace end


BGDataReceiver::BGDataReceiver(QObject *parent)
	: QObject(parent)
{
	// The adaptor is automatically destroyed by the QObject destructor.
	// For more, see: https://doc.qt.io/qt-5/objecttrees.html
	new ReceiverAdaptor(this);

	QDBusConnection systemBus = QDBusConnection::sessionBus();

	if (!systemBus.registerService(DBUS_SERVICE_NAME))
	{
		qCWarning(lcQmlBgData)
			<< "Unable to register D-Bus service" << DBUS_SERVICE_NAME << ":"
			<< systemBus.lastError().message();
	}

	if (!systemBus.registerObject(DBUS_OBJECT_PATH, this))
	{
		qCWarning(lcQmlBgData)
			<< "Unable to register object at path " << DBUS_OBJECT_PATH << ":"
			<< systemBus.lastError().message();
	}
}


QVariant BGDataReceiver::unit() const
{
	return m_unitIsMGDL.has_value() ? QVariant((*m_unitIsMGDL) ? "mg/dL" : "mmol/L") : QVariant();
}


QVariant BGDataReceiver::bgStatus() const
{
	return toQVariant(m_bgStatus);
}


QVariant BGDataReceiver::insulinOnBoard() const
{
	return toQVariant(m_iob);
}


QVariant BGDataReceiver::carbsOnBoard() const
{
	return toQVariant(m_cob);
}


QDateTime const & BGDataReceiver::lastLoopRunTime() const
{
	return m_lastLoopRunTime;
}


QVariant BGDataReceiver::basalRate() const
{
	return toQVariant(m_basalRate);
}


QVariantList const & BGDataReceiver::bgTimeSeries() const
{
	return m_bgTimeSeries;
}


void BGDataReceiver::pushMessage(QString source, QString ID, QString body)
{
	MessageID messageID;

	if (ID == "NewBGData")
		messageID = MessageID::NEW_BG_DATA;
	else if (ID == "BGDataUpdate")
		messageID = MessageID::BG_DATA_UPDATE;
	else
	{
		qCDebug(lcQmlBgData) << "Got message from source"
			<< source << "with unsupported ID" << ID
			<< "; we do not handle such messages; ignoring";
		return;
	}

	qCDebug(lcQmlBgData) << "Got message with source" << source << "ID" << ID << "body" << body;

	QJsonParseError parseError;
	auto document = QJsonDocument::fromJson(body.toUtf8(), &parseError);
	if (document.isNull())
	{
		qCWarning(lcQmlBgData) << "Could not parse incoming JSON:" << parseError.errorString();
		return;
	}

	switch (messageID)
	{
		case MessageID::NEW_BG_DATA:
		{
			resetQuantities();

			QJsonObject rootObject = document.object();
			if (rootObject.isEmpty())
				emit allQuantitiesCleared();
			else
				update(rootObject);

			break;
		}

		case MessageID::BG_DATA_UPDATE:
			update(document.object());
			break;

		default:
			qCCritical(lcQmlBgData) << "Missing code for handling message with ID" << ID;
			break;
	}
}


void BGDataReceiver::resetQuantities()
{
	m_unitIsMGDL = std::nullopt;
	m_bgStatus = std::nullopt;
	m_iob = std::nullopt;
	m_cob = std::nullopt;
	m_lastLoopRunTime = QDateTime();
	m_basalRate = std::nullopt;
	m_bgTimeSeries = QVariantList();
}


void BGDataReceiver::update(QJsonObject const &json)
{
	emit updateStarted();

	// In here, parse the JSON object to extract the updated values.
	// Only emit signals if the values really did change.

	try
	{
		parse<QJsonObject>(json, true, "basalRate", [this](auto const &basalRateJson) {
			bool changed = false;

			// Create new BasalRate instance on demand.
			if (!m_basalRate.has_value())
			{
				changed = true;
				m_basalRate = BasalRate();
			}

			parse<float>(basalRateJson, true, "baseRate", [this, &changed](auto baseRate) {
				changed = changed || (m_basalRate->m_baseRate != baseRate);
				m_basalRate->m_baseRate = baseRate;
			});

			parse<int>(basalRateJson, true, "percentage", [this, &changed](auto percentage) {
				changed = changed || (m_basalRate->m_percentage != percentage);
				m_basalRate->m_percentage = percentage;
			});

			if (changed)
			{
				qCDebug(lcQmlBgData) << "Basal rate changed: baseRate" << m_basalRate->m_baseRate << "percentage" << m_basalRate->m_percentage;
				emit basalRateChanged();
			}
		});

		parse<QJsonObject>(json, false, "bgStatus", [this](auto const &bgStatusJson) {
			bool changed = false;

			// Create new BGStatus instance on demand.
			if (!m_bgStatus.has_value())
			{
				changed = true;
				m_bgStatus = BGStatus();
			}

			parse<float>(bgStatusJson, true, "bgValue", [this, &changed](auto bgValue) {
				changed = changed || (m_bgStatus->m_bgValue != bgValue);
				m_bgStatus->m_bgValue = bgValue;
			});

			parse<float>(bgStatusJson, true, "delta", [this, &changed](auto delta) {
				changed = changed || (m_bgStatus->m_delta != delta); // TODO epsilon
				m_bgStatus->m_delta = delta;
			});

			parse<bool>(bgStatusJson, true, "isValid", [this, &changed](auto isValid) {
				changed = changed || (m_bgStatus->m_isValid != isValid);
				m_bgStatus->m_isValid = isValid;
			});

			parse<qint64>(bgStatusJson, true, "timestamp", [this, &changed](auto timestampInt) {
				auto timestamp = QDateTime::fromSecsSinceEpoch(timestampInt, Qt::UTC);
				changed = changed || (m_bgStatus->m_timestamp != timestamp);
				m_bgStatus->m_timestamp = timestamp;
			});

			parse<QString>(bgStatusJson, true, "trendArrow", [this, &changed](auto trendArrowStr) {
				TrendArrow trendArrow;
				bool validValue = true;

				if      (trendArrowStr == "none")          trendArrow = TrendArrow::NONE;
				else if (trendArrowStr == "tripleUp")      trendArrow = TrendArrow::TRIPLE_UP;
				else if (trendArrowStr == "doubleUp")      trendArrow = TrendArrow::DOUBLE_UP;
				else if (trendArrowStr == "singleUp")      trendArrow = TrendArrow::SINGLE_UP;
				else if (trendArrowStr == "fortyFiveUp")   trendArrow = TrendArrow::FORTY_FIVE_UP;
				else if (trendArrowStr == "flat")          trendArrow = TrendArrow::FLAT;
				else if (trendArrowStr == "fortyFiveDown") trendArrow = TrendArrow::FORTY_FIVE_DOWN;
				else if (trendArrowStr == "singleDown")    trendArrow = TrendArrow::SINGLE_DOWN;
				else if (trendArrowStr == "doubleDown")    trendArrow = TrendArrow::DOUBLE_DOWN;
				else if (trendArrowStr == "tripleDown")    trendArrow = TrendArrow::TRIPLE_DOWN;
				else validValue = false;

				if (validValue)
				{
					changed = changed || (m_bgStatus->m_trendArrow != trendArrow);
					m_bgStatus->m_trendArrow = trendArrow;
				}
				else
					qCWarning(lcQmlBgData) << "Skipping invalid trendArrow value" << trendArrowStr;
			});

			if (changed)
			{
				qCDebug(lcQmlBgData) << "BG status changed";
				emit bgStatusChanged();
			}
		});

		parse<QJsonObject>(json, false, "bgTimeSeries", [this](auto const &bgTimeSeriesJson) {
			bool valid = true;

			auto valuesJson = bgTimeSeriesJson["values"];
			if (!valuesJson.isArray())
			{
				qCWarning(lcQmlBgData) << "values field in bgTimeSeries JSON invalid or missing";
				valid = false;
			}

			auto timestampsJson = bgTimeSeriesJson["timestamps"];
			if (!timestampsJson.isArray())
			{
				qCWarning(lcQmlBgData) << "timestamps field in bgTimeSeries JSON invalid or missing";
				valid = false;
			}

			QJsonArray values = valuesJson.toArray();
			QJsonArray timestamps = timestampsJson.toArray();

			if (values.count() != timestamps.count())
			{
				qCWarning(lcQmlBgData).nospace().noquote()
				                       << "values and timestamps arrays have different number of items "
				                       << "(" << values.count() << " vs. " << timestamps.count() << ")";
				valid = false;
			}

			m_bgTimeSeries = QVariantList();

			if (valid)
			{
				for (int i = 0; i < values.count(); ++i)
				{
					auto timestampJson = timestamps[i];
					if (!timestampJson.isDouble())
					{
						qCWarning(lcQmlBgData).nospace().noquote() << "BG timestamp #" << i << " is not a number";
						valid = false;
						break;
					}

					auto valueJson = values[i];
					if (!valueJson.isDouble())
					{
						qCWarning(lcQmlBgData).nospace().noquote() << "BG value #" << i << " is not a number";
						valid = false;
						break;
					}

					m_bgTimeSeries.append(QPointF(timestampJson.toDouble() / 1000.0, valueJson.toDouble() / 1000.0));
				}
			}

			qCDebug(lcQmlBgData) << "BG time series changed";

			emit bgTimeSeriesChanged();
		});

		parse<QJsonObject>(json, true, "iob", [this](auto const &iobJson) {
			bool changed = false;

			// Create new InsulinOnBoard instance on demand.
			if (!m_iob.has_value())
			{
				changed = true;
				m_iob = InsulinOnBoard();
			}

			parse<float>(iobJson, true, "basal", [this, &changed](auto basal) {
				changed = changed || (m_iob->m_basal != basal);
				m_iob->m_basal = basal;
			});

			parse<float>(iobJson, true, "bolus", [this, &changed](auto bolus) {
				changed = changed || (m_iob->m_bolus != bolus);
				m_iob->m_bolus = bolus;
			});

			if (changed)
			{
				qCDebug(lcQmlBgData) << "IOB changed: basal" << m_iob->m_basal << "bolus" << m_iob->m_bolus;
				emit insulinOnBoardChanged();
			}
		});

		parse<QJsonObject>(json, true, "cob", [this](QJsonObject const &cobJson) {
			bool changed = false;

			// Create new CarbsOnBoard instance on demand.
			if (!m_iob.has_value())
			{
				changed = true;
				m_cob = CarbsOnBoard();
			}

			parse<int>(cobJson, true, "current", [this, &changed](int current) {
				changed = changed || (m_cob->m_current != current);
				m_cob->m_current = current;
			});

			parse<int>(cobJson, true, "future", [this, &changed](int future) {
				changed = changed || (m_cob->m_future != future);
				m_cob->m_future = future;
			});

			if (changed)
			{
				qCDebug(lcQmlBgData) << "COB changed: current" << m_cob->m_current << "future" << m_cob->m_future;
				emit carbsOnBoardChanged();
			}
		});

		parse<qint64>(json, false, "lastLoopRunTime", [this](auto lastLoopRunTimeInt) {
			auto lastLoopRunTime = QDateTime::fromSecsSinceEpoch(lastLoopRunTimeInt, Qt::UTC);
			if (m_lastLoopRunTime == lastLoopRunTime)
				return;

			m_lastLoopRunTime = lastLoopRunTime;
			qCDebug(lcQmlBgData) << "lastLoopRunTime changed:" << lastLoopRunTime;
			emit lastLoopRunTimeChanged();
		});

		parse<QString>(json, true, "unit", [this](QString unit) {
			bool unitIsMGDL = (unit == "mgdl");
			if (m_unitIsMGDL == unitIsMGDL)
				return;

			m_unitIsMGDL = unitIsMGDL;
			qCDebug(lcQmlBgData) << "Unit changed:" << unit;
			emit unitChanged();
		});
	}
	catch (JSONError const &e)
	{
		qCWarning(lcQmlBgData) << "Error while parsing JSON: " << e.what();
	}

	emit updateEnded();
}
