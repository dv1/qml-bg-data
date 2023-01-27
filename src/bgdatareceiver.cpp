#include <QDebug>
#include <QDBusConnection>
#include <QLoggingCategory>
#include <cassert>
#include <cmath>
#include <random>

#include "bgdatareceiver.hpp"
#include "extappmsgreceiverifaceadaptor.h"


Q_DECLARE_LOGGING_CATEGORY(lcQmlBgData)


// In here, we parse incoming BG datasets. The format specification
// for that data can be found in the docs/bg-data-binary-format-spec.txt file.


namespace {

QString const DBUS_SERVICE_NAME = "org.asteroidos.externalappmessages.BGDataReceiver";
QString const DBUS_OBJECT_PATH = "/org/asteroidos/externalappmessages/BGDataReceiver";

unsigned int const FLAG_UNIT_IS_MG_DL                   = (1u << 0);
unsigned int const FLAG_BG_VALUE_IS_VALID               = (1u << 1);
unsigned int const FLAG_BG_STATUS_PRESENT               = (1u << 2);
unsigned int const FLAG_LAST_LOOP_RUN_TIMESTAMP_PRESENT = (1u << 3);
unsigned int const FLAG_MUST_CLEAR_ALL_DATA             = (1u << 4);

template<typename T>
QVariant toQVariant(std::optional<T> const &optValue)
{
	return optValue.has_value() ? QVariant::fromValue(*optValue) : QVariant();
}

// NOTE: These bytes -> numeric converters assume
// that the values are stored in little-endian order.

float floatFrom(QByteArray const &bytes, int &offset)
{
	if (int(offset + sizeof(float)) > bytes.size())
		throw std::out_of_range(QString("attempted to read float at offset %1").arg(offset).toStdString());

	// TODO: Endianness
	float f;
	memcpy(&f, bytes.data() + offset, sizeof(float));
	offset += sizeof(float);
	return f;
}

qint8 int8From(QByteArray const &bytes, int &offset)
{
	if (int(offset + sizeof(qint8)) > bytes.size())
		throw std::out_of_range(QString("attempted to read int8 at offset %1").arg(offset).toStdString());

	auto ret = qint8(bytes[offset]);
	offset += sizeof(qint8);
	return ret;
}

qint16 int16From(QByteArray const &bytes, int &offset)
{
	if (int(offset + sizeof(qint16)) > bytes.size())
		throw std::out_of_range(QString("attempted to read int16 at offset %1").arg(offset).toStdString());

	auto ret = (qint16(bytes[offset + 0]) << 0)
	         | (qint16(bytes[offset + 1]) << 8);
	offset += sizeof(qint16);
	return ret;
}

qint64 int64From(QByteArray const &bytes, int &offset)
{
	if (int(offset + sizeof(qint64)) > bytes.size())
		throw std::out_of_range(QString("attempted to read int64 at offset %1").arg(offset).toStdString());

	auto ret = (qint64(bytes[offset + 0]) << 0)
	         | (qint64(bytes[offset + 1]) << 8)
	         | (qint64(bytes[offset + 2]) << 16)
	         | (qint64(bytes[offset + 3]) << 24)
	         | (qint64(bytes[offset + 4]) << 32)
	         | (qint64(bytes[offset + 5]) << 40)
	         | (qint64(bytes[offset + 6]) << 48)
	         | (qint64(bytes[offset + 7]) << 56);
	offset += sizeof(qint64);
	return ret;
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

	clearAllQuantities();
}


QVariant BGDataReceiver::unit() const
{
	return toQVariant(m_unit);
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


QDateTime const & BGDataReceiver::lastLoopRunTimestamp() const
{
	return m_lastLoopRunTimestamp;
}


QVariant BGDataReceiver::basalRate() const
{
	return toQVariant(m_basalRate);
}


QVariantList const & BGDataReceiver::bgTimeSeries() const
{
	return m_bgTimeSeries;
}


QVariantList const & BGDataReceiver::basalTimeSeries() const
{
	return m_basalTimeSeries;
}


QVariantList const & BGDataReceiver::baseBasalTimeSeries() const
{
	return m_baseBasalTimeSeries;
}


void BGDataReceiver::generateTestQuantities()
{
	std::random_device randomDevice;
	std::mt19937 randomNumberGenerator(randomDevice());
	std::bernoulli_distribution glucoseUnitDistribution(0.5);
	std::uniform_real_distribution<float> bgValueDistribution(40.0f, 300.0f);
	std::uniform_real_distribution<float> deltaDistribution(-30.0f, +30.0f);
	std::bernoulli_distribution isValidBgDistribution(0.5);
	std::uniform_real_distribution<float> basalInsulinDistributtion(-10.0f, +10.0f);
	std::uniform_real_distribution<float> bolusInsulinDistributtion(0.0f, +10.0f);
	std::uniform_real_distribution<float> basalRateDistribution(0.0f, 2.0f);
	std::uniform_int_distribution<int> carbsDistribution(0, 140);
	std::uniform_int_distribution<int> tbrDistribution(0, 400);
	std::uniform_int_distribution<int> bgTimeSeriesStartDistribution(0, 32767);
	std::uniform_int_distribution<int> bgTimeSeriesChangeDistribution(-4000, 4000);

	clearAllQuantities();

	float delta = deltaDistribution(randomNumberGenerator);

	BGStatus bgStatus;
	bgStatus.m_bgValue = bgValueDistribution(randomNumberGenerator);
	bgStatus.m_delta = std::isnan(delta) ? QVariant() : QVariant::fromValue(delta);
	bgStatus.m_isValid = isValidBgDistribution(randomNumberGenerator);
	bgStatus.m_timestamp = QDateTime::currentDateTime();
	if (std::isnan(delta))
		bgStatus.m_trendArrow = BGStatus::TrendArrow::NONE;
	else if (delta < -25)
		bgStatus.m_trendArrow = BGStatus::TrendArrow::TRIPLE_DOWN;
	else if (delta < -18)
		bgStatus.m_trendArrow = BGStatus::TrendArrow::DOUBLE_DOWN;
	else if (delta < -10)
		bgStatus.m_trendArrow = BGStatus::TrendArrow::SINGLE_DOWN;
	else if (delta < -5)
		bgStatus.m_trendArrow = BGStatus::TrendArrow::FORTY_FIVE_DOWN;
	else if (delta > +25)
		bgStatus.m_trendArrow = BGStatus::TrendArrow::TRIPLE_UP;
	else if (delta > +18)
		bgStatus.m_trendArrow = BGStatus::TrendArrow::DOUBLE_UP;
	else if (delta > +10)
		bgStatus.m_trendArrow = BGStatus::TrendArrow::SINGLE_UP;
	else if (delta > +5)
		bgStatus.m_trendArrow = BGStatus::TrendArrow::FORTY_FIVE_UP;
	else
		bgStatus.m_trendArrow = BGStatus::TrendArrow::FLAT;

	QVariantList bgTimeSeries;
	int bgTimeSeriesValue = bgTimeSeriesStartDistribution(randomNumberGenerator);
	for (int i = 0; i < 100; ++i)
	{
		bgTimeSeries.append(QPointF(
			float(i) / 99.0f,
			bgTimeSeriesValue / 32767.0f
		));

		bgTimeSeriesValue += bgTimeSeriesChangeDistribution(randomNumberGenerator);
		bgTimeSeriesValue = std::min(std::max(bgTimeSeriesValue, 0), 32767);
	}

	InsulinOnBoard iob;
	iob.m_basal = basalInsulinDistributtion(randomNumberGenerator);
	iob.m_bolus = bolusInsulinDistributtion(randomNumberGenerator);

	CarbsOnBoard cob;
	cob.m_current = carbsDistribution(randomNumberGenerator);
	cob.m_future = carbsDistribution(randomNumberGenerator);

	BasalRate basalRate;
	basalRate.m_baseRate = basalRateDistribution(randomNumberGenerator);
	basalRate.m_currentRate = basalRateDistribution(randomNumberGenerator);
	basalRate.m_tbrPercentage = tbrDistribution(randomNumberGenerator);

	QDateTime lastLoopRunTimestamp = QDateTime::currentDateTime();

	m_unit = glucoseUnitDistribution(randomNumberGenerator) ? Unit::MG_DL : Unit::MMOL_L;
	m_bgStatus = std::move(bgStatus);
	m_iob = std::move(iob);
	m_cob = std::move(cob);
	m_lastLoopRunTimestamp = std::move(lastLoopRunTimestamp);
	m_basalRate = std::move(basalRate);
	m_bgTimeSeries = std::move(bgTimeSeries);

	emit unitChanged();
	emit bgStatusChanged();
	emit insulinOnBoardChanged();
	emit carbsOnBoardChanged();
	emit lastLoopRunTimestampChanged();
	emit basalRateChanged();

	emit newDataReceived();
}


QVariant BGDataReceiver::getTimespansSince(QDateTime now)
{
	if (!now.isValid())
	{
		qCWarning(lcQmlBgData) << "getTimespansSince() called with invalid datetime; returning invalid values";
		return QVariant::fromValue(Timespans());
	}

	Timespans timespans;

	auto nowInSecsSinceEpoch = now.toSecsSinceEpoch();

	if (m_bgStatus.has_value() && (m_bgStatus->m_timestamp.isValid()))
		timespans.m_bgStatusUpdate = nowInSecsSinceEpoch - m_bgStatus->m_timestamp.toSecsSinceEpoch();

	if (m_lastLoopRunTimestamp.isValid())
		timespans.m_lastLoopRun = nowInSecsSinceEpoch - m_lastLoopRunTimestamp.toSecsSinceEpoch();

	return QVariant::fromValue(timespans);
}


void BGDataReceiver::pushMessage(QString source, QByteArray payload)
{
	qCDebug(lcQmlBgData).nospace().noquote() << "Got message; source: " << source;

	if (payload.isEmpty())
	{
		qCWarning(lcQmlBgData) << "Got message with zero bytes in payload";
		return;
	}

	// Using an epsilon of 0.005 for basal change checks. This
	// is sufficient, because basal quantities are pretty much
	// never given with any granularity smaller than 0.01 IU.
	float const basalEpsilon = 0.005f;

	// Using an epsilon of 0.01 for BG value changes. When using
	// mg/dL values, we never get fractional quantities, only whole
	// numbers. (Exception: When the delta is between 0 and 1 - then
	// 1 fractional delta digit may be used with mg/dL.)
	// And when mmol/L are used, anything more fine grained than
	// 0.05 mmol/L is never used.
	// This also applies to the delta.
	float const bgValueEpsilon = 0.01f;

	try
	{
		int offset = 0;

		// At minimum, a message contains these blocks (because they are not optional):
		qsizetype const minValidSize =
			// The version byte
			1 +
			// The flags byte
			1 +
			// The base and current rate floats
			4+4 +
			// The 16-bit integer that contains the number of BG time series data points (which is 0 in the minimum case)
			2 +
			// The 16-bit integer that contains the number of basal time series data points (which is 0 in the minimum case)
			2 +
			// The 16-bit integer that contains the number of base basal time series data points (which is 0 in the minimum case)
			2 +
			// The basal and bolus IOB floats
			4+4 +
			// The current and future COB
			2+2;

		if (payload.size() < minValidSize)
		{
			qCWarning(lcQmlBgData)
				<< "Got invalid data - insufficient bytes:"
				<< "expected:" << minValidSize
				<< "actual:" << payload.size();
		}

		// Format version number
		qint8 version = int8From(payload, offset);
		if (version != 1)
		{
			qCCritical(lcQmlBgData) << "This receiver can only handle version 1 message data";
			return;
		}

		// Flags
		qint8 flags = int8From(payload, offset);
		if (flags & FLAG_MUST_CLEAR_ALL_DATA)
		{
			qCDebug(lcQmlBgData) << "Clearing all quantities";

			clearAllQuantities();

			emit unitChanged();
			emit bgStatusChanged();
			emit insulinOnBoardChanged();
			emit carbsOnBoardChanged();
			emit lastLoopRunTimestampChanged();

			emit newDataReceived();

			return;
		}

		// Unit
		auto newUnit = (flags & FLAG_UNIT_IS_MG_DL) ? Unit::MG_DL : Unit::MMOL_L;
		if (!m_unit.has_value() || (m_unit != newUnit))
		{
			m_unit = newUnit;
			emit unitChanged();
		}

		// Basal rate
		{
			bool changed = false;

			// Create new BasalRate instance on demand.
			if (!m_basalRate.has_value())
			{
				changed = true;
				m_basalRate = BasalRate();
			}

			float baseRate = floatFrom(payload, offset);
			changed = changed || (std::abs(m_basalRate->m_baseRate - baseRate) >= basalEpsilon);
			m_basalRate->m_baseRate = baseRate;

			float currentRate = floatFrom(payload, offset);
			changed = changed || (std::abs(m_basalRate->m_currentRate - currentRate) >= basalEpsilon);
			m_basalRate->m_currentRate = currentRate;

			qint16 tbrPercentage = int16From(payload, offset);
			changed = changed || (m_basalRate->m_tbrPercentage != tbrPercentage);
			m_basalRate->m_tbrPercentage = tbrPercentage;

			if (changed)
			{
				qCDebug(lcQmlBgData) << "Basal rate changed:"
				                     << "baseRate" << baseRate
				                     << "currentRate" << currentRate
				                     << "TBR percentage" << tbrPercentage;
				emit basalRateChanged();
			}
		}

		// BG status
		if (flags & FLAG_BG_STATUS_PRESENT)
		{
			bool changed = false;

			m_bgStatus->m_isValid = !!(flags & FLAG_BG_VALUE_IS_VALID);

			// Create new BGStatus instance on demand.
			if (!m_bgStatus.has_value())
			{
				changed = true;
				m_bgStatus = BGStatus();
			}

			float bgValue = floatFrom(payload, offset);
			changed = changed || (std::abs(m_bgStatus->m_bgValue - bgValue) >= bgValueEpsilon);
			m_bgStatus->m_bgValue = bgValue;
			qCDebug(lcQmlBgData) << "bgValue:" << bgValue;

			float delta = floatFrom(payload, offset);
			if (std::isnan(delta))
			{
				changed = changed || m_bgStatus->m_delta.isValid();
				m_bgStatus->m_delta = QVariant();
				qCDebug(lcQmlBgData) << "Got NaN as delta; no delta value available";
			}
			else
			{
				changed = changed || (std::abs(m_bgStatus->m_delta.toFloat() - delta) >= bgValueEpsilon);
				m_bgStatus->m_delta = delta;
				qCDebug(lcQmlBgData) << "delta:" << delta;
			}

			QDateTime timestamp = QDateTime::fromSecsSinceEpoch(int64From(payload, offset), Qt::UTC);
			changed = changed || (m_bgStatus->m_timestamp != timestamp);
			m_bgStatus->m_timestamp = timestamp;
			qCDebug(lcQmlBgData) << "timestamp:" << timestamp;

			BGStatus::TrendArrow trendArrow;
			qint8 trendArrowIndex = int8From(payload, offset);
			qCDebug(lcQmlBgData) << "trendArrowIndex:" << trendArrowIndex;
			switch (trendArrowIndex)
			{
				case 0: trendArrow = BGStatus::TrendArrow::NONE; break;
				case 1: trendArrow = BGStatus::TrendArrow::TRIPLE_UP; break;
				case 2: trendArrow = BGStatus::TrendArrow::DOUBLE_UP; break;
				case 3: trendArrow = BGStatus::TrendArrow::SINGLE_UP; break;
				case 4: trendArrow = BGStatus::TrendArrow::FORTY_FIVE_UP; break;
				case 5: trendArrow = BGStatus::TrendArrow::FLAT; break;
				case 6: trendArrow = BGStatus::TrendArrow::FORTY_FIVE_DOWN; break;
				case 7: trendArrow = BGStatus::TrendArrow::SINGLE_DOWN; break;
				case 8: trendArrow = BGStatus::TrendArrow::DOUBLE_DOWN; break;
				case 9: trendArrow = BGStatus::TrendArrow::TRIPLE_DOWN; break;
				default:
					qCWarning(lcQmlBgData).nospace()
						<< "Invalid trendArrow (raw index: " << int(trendArrowIndex) << "); interpreting as \"none\" (= index 0)";
					trendArrow = BGStatus::TrendArrow::NONE;
					break;
			}

			changed = changed || (m_bgStatus->m_trendArrow != trendArrow);
			m_bgStatus->m_trendArrow = trendArrow;

			if (changed)
			{
				qCDebug(lcQmlBgData) << "BG status changed";
				emit bgStatusChanged();
			}
		}

		// BG time series
		{
			qint16 numDataPoints = int16From(payload, offset);
			qCDebug(lcQmlBgData) << "BG time series contains" << numDataPoints << "point(s)";

			m_bgTimeSeries = QVariantList();

			for (int dataPointIndex = 0; dataPointIndex < numDataPoints; ++dataPointIndex)
			{
				qint16 timestamp = int16From(payload, offset);
				qint16 bgValue = int16From(payload, offset);

				m_bgTimeSeries.append(
					QPointF(
						double(timestamp) / 32767.0,
						double(bgValue) / 32767.0
					)
				);
			}
		}

		// Basal time series
		{
			qint16 numDataPoints = int16From(payload, offset);
			qCDebug(lcQmlBgData) << "Basal time series contains" << numDataPoints << "point(s)";

			m_basalTimeSeries = QVariantList();

			for (int dataPointIndex = 0; dataPointIndex < numDataPoints; ++dataPointIndex)
			{
				qint16 timestamp = int16From(payload, offset);
				qint16 basalValue = int16From(payload, offset);

				m_basalTimeSeries.append(
					QPointF(
						double(timestamp) / 32767.0,
						double(basalValue) / 32767.0
					)
				);
			}
		}

		// Base basal time series
		{
			qint16 numDataPoints = int16From(payload, offset);
			qCDebug(lcQmlBgData) << "Base basal time series contains" << numDataPoints << "point(s)";

			m_baseBasalTimeSeries = QVariantList();

			for (int dataPointIndex = 0; dataPointIndex < numDataPoints; ++dataPointIndex)
			{
				qint16 timestamp = int16From(payload, offset);
				qint16 baseBasalValue = int16From(payload, offset);

				m_baseBasalTimeSeries.append(
					QPointF(
						double(timestamp) / 32767.0,
						double(baseBasalValue) / 32767.0
					)
				);
			}
		}

		// Insulin On Board (IOB)
		{
			bool changed = false;

			// Create new InsulinOnBoard instance on demand.
			if (!m_iob.has_value())
			{
				changed = true;
				m_iob = InsulinOnBoard();
			}

			float basal = floatFrom(payload, offset);
			changed = changed || (std::abs(m_iob->m_basal - basal) >= basalEpsilon);
			m_iob->m_basal = basal;

			float bolus = floatFrom(payload, offset);
			changed = changed || (std::abs(m_iob->m_bolus - bolus) >= basalEpsilon);
			m_iob->m_bolus = bolus;

			qCDebug(lcQmlBgData).nospace() << "basal/bolus IOB: " << basal << "/" << bolus;
		}

		// Carbs On Board (COB)
		{
			bool changed = false;

			// Create new CarbsOnBoard instance on demand.
			if (!m_cob.has_value())
			{
				changed = true;
				m_cob = CarbsOnBoard();
			}

			int current = int16From(payload, offset);
			changed = changed || (m_cob->m_current != current);
			m_cob->m_current = current;

			int future = int16From(payload, offset);
			changed = changed || (m_cob->m_future != future);
			m_cob->m_future = future;

			qCDebug(lcQmlBgData).nospace() << "current/future IOB: " << current << "/" << future;
		}

		// Last loop run timestamp
		if (flags & FLAG_LAST_LOOP_RUN_TIMESTAMP_PRESENT)
		{
			bool changed = false;

			QDateTime lastLoopRunTimestamp = QDateTime::fromSecsSinceEpoch(int64From(payload, offset), Qt::UTC);
			changed = changed || (m_lastLoopRunTimestamp != lastLoopRunTimestamp);
			m_lastLoopRunTimestamp = lastLoopRunTimestamp;

			qCDebug(lcQmlBgData) << "lastLoopRunTimestamp:" << lastLoopRunTimestamp;
		}

		emit newDataReceived();
	}
	catch (std::out_of_range const &e)
	{
		qCWarning(lcQmlBgData).noquote() << "Got out-of-range error while parsing data:" << e.what();
	}
}


void BGDataReceiver::clearAllQuantities()
{
	m_unit = std::nullopt;
	m_bgStatus = std::nullopt;
	m_iob = std::nullopt;
	m_cob = std::nullopt;
	m_lastLoopRunTimestamp = QDateTime();
	m_basalRate = std::nullopt;
	m_bgTimeSeries = QVariantList();
}
