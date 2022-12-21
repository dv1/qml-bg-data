#include <QDebug>
#include <QDBusConnection>
#include <QLoggingCategory>

#include "sgvdatareceiver.hpp"
#include "jsonhelper.hpp"
#include "extappmsgreceiverifaceadaptor.h"


Q_DECLARE_LOGGING_CATEGORY(lcQmlSgvData)


namespace {

QString const DBUS_SERVICE_NAME = "org.asteroidos.externalappmessages.SGVDataReceiver";
QString const DBUS_OBJECT_PATH = "/org/asteroidos/externalappmessages/SGVDataReceiver";

} // unnamed namespace end


SGVDataReceiver::SGVDataReceiver(QObject *parent)
	: QObject(parent)
{
	// The adaptor is automatically destroyed by the QObject destructor.
	// For more, see: https://doc.qt.io/qt-5/objecttrees.html
	new ReceiverAdaptor(this);

	QDBusConnection systemBus = QDBusConnection::sessionBus();

	if (!systemBus.registerService(DBUS_SERVICE_NAME))
	{
		qCWarning(lcQmlSgvData)
			<< "Unable to register D-Bus service" << DBUS_SERVICE_NAME << ":"
			<< systemBus.lastError().message();
	}

	if (!systemBus.registerObject(DBUS_OBJECT_PATH, this))
	{
		qCWarning(lcQmlSgvData)
			<< "Unable to register object at path " << DBUS_OBJECT_PATH << ":"
			<< systemBus.lastError().message();
	}
}


QString SGVDataReceiver::unit() const
{
	return m_unitIsMGDL ? "mg/dL" : "mmol/L";
}


int SGVDataReceiver::sgv() const
{
	return m_sgv;
}


int SGVDataReceiver::delta() const
{
	return m_delta;
}


InsulinOnBoard const & SGVDataReceiver::insulinOnBoard() const
{
	return m_iob;
}


CarbsOnBoard const & SGVDataReceiver::carbsOnBoard() const
{
	return m_cob;
}


QDateTime const & SGVDataReceiver::lastSgvTime() const
{
	return m_lastSgvTime;
}


QDateTime const & SGVDataReceiver::lastLoopRunTime() const
{
	return m_lastLoopRunTime;
}


BasalRate const & SGVDataReceiver::basalRate() const
{
	return m_basalRate;
}


QVariantList const & SGVDataReceiver::graph() const
{
	return m_graph;
}


void SGVDataReceiver::pushMessage(QString source, QString ID, QString body)
{
	if (ID != "SGVData")
	{
		qCDebug(lcQmlSgvData) << "Got message from source"
			<< source << "with unsupported ID" << ID
			<< "; we do not handle such messages; ignoring";
		return;
	}

	QJsonParseError parseError;
	auto document = QJsonDocument::fromJson(body.toUtf8(), &parseError);
	if (document.isNull())
	{
		qCWarning(lcQmlSgvData) << "Could not parse incoming JSON:" << parseError.errorString();
		return;
	}

	update(document.object());
}


void SGVDataReceiver::update(QJsonObject const &json)
{
	emit updateStarted();

	// In here, parse the JSON object to extract the updated values.
	// Only emit signals if the values really did change.

	readFromJSON<QString>(json, "unit", [this](QString unit) {
		bool unitIsMGDL = (unit == "mgdl");
		if (m_unitIsMGDL == unitIsMGDL)
			return;

		m_unitIsMGDL = unitIsMGDL;
		qCDebug(lcQmlSgvData) << "Unit changed:" << unit;
		emit unitChanged();
	});

	readFromJSON<int>(json, "sgv", [this](int sgv) {
		if (m_sgv == sgv)
			return;

		m_sgv = sgv;
		qCDebug(lcQmlSgvData) << "SGV changed:" << sgv;
		emit sgvChanged();
	});

	readFromJSON<int>(json, "delta", [this](int delta) {
		if (m_delta == delta)
			return;

		m_delta = delta;
		qCDebug(lcQmlSgvData) << "Delta changed:" << delta;
		emit deltaChanged();
	});

	readFromJSON<QJsonObject>(json, "iob", [this](QJsonObject const &iobJson) {
		bool changed = false;

		readFromJSON<float>(iobJson, "basal", [this, &changed](float basal) {
			changed = changed || (m_iob.m_basal != basal);
			m_iob.m_basal = basal;
		});

		readFromJSON<float>(iobJson, "bolus", [this, &changed](float bolus) {
			changed = changed || (m_iob.m_bolus != bolus);
			m_iob.m_bolus = bolus;
		});

		if (changed)
		{
			qCDebug(lcQmlSgvData) << "IOB changed: basal" << m_iob.m_basal << "bolus" << m_iob.m_bolus;
			emit insulinOnBoardChanged();
		}
	});

	readFromJSON<QJsonObject>(json, "cob", [this](QJsonObject const &cobJson) {
		bool changed = false;

		readFromJSON<int>(cobJson, "current", [this, &changed](int current) {
			changed = changed || (m_cob.m_current != current);
			m_cob.m_current = current;
		});

		readFromJSON<int>(cobJson, "future", [this, &changed](int future) {
			changed = changed || (m_cob.m_future != future);
			m_cob.m_future = future;
		});

		if (changed)
		{
			qCDebug(lcQmlSgvData) << "COB changed: current" << m_cob.m_current << "future" << m_cob.m_future;
			emit carbsOnBoardChanged();
		}
	});

	readFromJSON<qint64>(json, "lastSgvTime", [this](qint64 lastSgvTimeInt) {
		auto lastSgvTime = QDateTime::fromSecsSinceEpoch(lastSgvTimeInt, Qt::UTC);
		if (m_lastSgvTime == lastSgvTime)
			return;

		m_lastSgvTime = lastSgvTime;
		qCDebug(lcQmlSgvData) << "lastSgvTime changed:" << lastSgvTime;
		emit lastSgvTimeChanged();
	});

	readFromJSON<qint64>(json, "lastLoopRunTime", [this](qint64 lastLoopRunTimeInt) {
		auto lastLoopRunTime = QDateTime::fromSecsSinceEpoch(lastLoopRunTimeInt, Qt::UTC);
		if (m_lastLoopRunTime == lastLoopRunTime)
			return;

		m_lastLoopRunTime = lastLoopRunTime;
		qCDebug(lcQmlSgvData) << "lastLoopRunTime changed:" << lastLoopRunTime;
		emit lastLoopRunTimeChanged();
	});

	readFromJSON<QJsonObject>(json, "basalRate", [this](QJsonObject const &basalRateJson) {
		bool changed = false;

		readFromJSON<float>(basalRateJson, "baseRate", [this, &changed](float baseRate) {
			changed = changed || (m_basalRate.m_baseRate != baseRate);
			m_basalRate.m_baseRate = baseRate;
		});

		readFromJSON<int>(basalRateJson, "percentage", [this, &changed](int percentage) {
			changed = changed || (m_basalRate.m_percentage != percentage);
			m_basalRate.m_percentage = percentage;
		});

		if (changed)
		{
			qCDebug(lcQmlSgvData) << "Basal rate changed: baseRate" << m_basalRate.m_baseRate << "percentage" << m_basalRate.m_percentage;
			emit basalRateChanged();
		}
	});

	readFromJSON<QJsonArray>(json, "graph", [this](QJsonArray const &graphJson) {
		QVariantList newGraph = graphJson.toVariantList();
		if (m_graph == newGraph)
			return;

		m_graph = std::move(newGraph);
		qCDebug(lcQmlSgvData) << "Graph changed";
		emit graphChanged();
	});

	emit updateEnded();
}
