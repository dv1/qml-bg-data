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


Graph const & BGDataReceiver::graph() const
{
	return m_graph;
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
	m_graph = Graph();
}


void BGDataReceiver::update(QJsonObject const &json)
{
	emit updateStarted();

	// In here, parse the JSON object to extract the updated values.
	// Only emit signals if the values really did change.

	readFromJSON<QString>(json, "unit", [this](QString unit) {
		bool unitIsMGDL = (unit == "mgdl");
		if (m_unitIsMGDL == unitIsMGDL)
			return;

		m_unitIsMGDL = unitIsMGDL;
		qCDebug(lcQmlBgData) << "Unit changed:" << unit;
		emit unitChanged();
	});

	readFromJSON<QJsonObject>(json, "bgStatus", [this](QJsonObject bgStatusJson) {
		bool changed = false;

		// Initialize m_bgStatus if we haven't done so already.
		if (!m_bgStatus.has_value())
		{
			changed = true;
			m_bgStatus = BGStatus();
		}

		readFromJSON<float>(bgStatusJson, "bgValue", [this, &changed](float bgValue) {
			changed = changed || (m_bgStatus->m_bgValue != bgValue);
			m_bgStatus->m_bgValue = bgValue;
		});

		readFromJSON<bool>(bgStatusJson, "isValid", [this, &changed](bool isValid) {
			changed = changed || (m_bgStatus->m_isValid != isValid);
			m_bgStatus->m_isValid = isValid;
		});

		readFromJSON<QString>(bgStatusJson, "trendArrow", [this, &changed](QString trendArrowStr) {
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

		readFromJSON<float>(bgStatusJson, "delta", [this, &changed](float delta) {
			changed = changed || (m_bgStatus->m_delta != delta); // TODO epsilon
			m_bgStatus->m_delta = delta;
		});

		readFromJSON<qint64>(bgStatusJson, "timestamp", [this, &changed](qint64 timestampInt) {
			auto timestamp = QDateTime::fromSecsSinceEpoch(timestampInt, Qt::UTC);
			changed = changed || (m_bgStatus->m_timestamp != timestamp);
			m_bgStatus->m_timestamp = timestamp;
		});

		if (changed)
		{
			qCDebug(lcQmlBgData) << "BG status changed";
			emit bgStatusChanged();
		}
	});

	readFromJSON<QJsonObject>(json, "iob", [this](QJsonObject const &iobJson) {
		bool changed = false;
		if (!m_iob.has_value())
		{
			changed = true;
			m_iob = InsulinOnBoard();
		}

		readFromJSON<float>(iobJson, "basal", [this, &changed](float basal) {
			changed = changed || (m_iob->m_basal != basal);
			m_iob->m_basal = basal;
		});

		readFromJSON<float>(iobJson, "bolus", [this, &changed](float bolus) {
			changed = changed || (m_iob->m_bolus != bolus);
			m_iob->m_bolus = bolus;
		});

		if (changed)
		{
			qCDebug(lcQmlBgData) << "IOB changed: basal" << m_iob->m_basal << "bolus" << m_iob->m_bolus;
			emit insulinOnBoardChanged();
		}
	});

	readFromJSON<QJsonObject>(json, "cob", [this](QJsonObject const &cobJson) {
		bool changed = false;
		if (!m_iob.has_value())
		{
			changed = true;
			m_cob = CarbsOnBoard();
		}

		readFromJSON<int>(cobJson, "current", [this, &changed](int current) {
			changed = changed || (m_cob->m_current != current);
			m_cob->m_current = current;
		});

		readFromJSON<int>(cobJson, "future", [this, &changed](int future) {
			changed = changed || (m_cob->m_future != future);
			m_cob->m_future = future;
		});

		if (changed)
		{
			qCDebug(lcQmlBgData) << "COB changed: current" << m_cob->m_current << "future" << m_cob->m_future;
			emit carbsOnBoardChanged();
		}
	});

	readFromJSON<qint64>(json, "lastLoopRunTime", [this](qint64 lastLoopRunTimeInt) {
		auto lastLoopRunTime = QDateTime::fromSecsSinceEpoch(lastLoopRunTimeInt, Qt::UTC);
		if (m_lastLoopRunTime == lastLoopRunTime)
			return;

		m_lastLoopRunTime = lastLoopRunTime;
		qCDebug(lcQmlBgData) << "lastLoopRunTime changed:" << lastLoopRunTime;
		emit lastLoopRunTimeChanged();
	});

	readFromJSON<QJsonObject>(json, "basalRate", [this](QJsonObject const &basalRateJson) {
		bool changed = false;
		if (!m_basalRate.has_value())
		{
			changed = true;
			m_basalRate = BasalRate();
		}

		readFromJSON<float>(basalRateJson, "baseRate", [this, &changed](float baseRate) {
			changed = changed || (m_basalRate->m_baseRate != baseRate);
			m_basalRate->m_baseRate = baseRate;
		});

		readFromJSON<int>(basalRateJson, "percentage", [this, &changed](int percentage) {
			changed = changed || (m_basalRate->m_percentage != percentage);
			m_basalRate->m_percentage = percentage;
		});

		if (changed)
		{
			qCDebug(lcQmlBgData) << "Basal rate changed: baseRate" << m_basalRate->m_baseRate << "percentage" << m_basalRate->m_percentage;
			emit basalRateChanged();
		}
	});

	readFromJSON<QJsonObject>(json, "graph", [this](QJsonObject const &graphJson) {
		readFromJSON<QJsonArray>(graphJson, "bgValues", [this](QJsonArray const &bgValuesJson) {
			QVariantList bgValues = bgValuesJson.toVariantList();
			m_graph.m_bgValues = std::move(bgValues);
		});

		readFromJSON<QJsonArray>(graphJson, "bgTimestamps", [this](QJsonArray const &bgTimestampsJson) {
			QVariantList bgTimestamps = bgTimestampsJson.toVariantList();
			m_graph.m_bgTimestamps = std::move(bgTimestamps);
		});

		qCDebug(lcQmlBgData) << "Graph changed";
		emit graphChanged();
	});

	emit updateEnded();
}
