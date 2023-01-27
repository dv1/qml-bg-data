#ifndef BGDATARECEIVER_HPP
#define BGDATARECEIVER_HPP

#include <optional>
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QDateTime>
#include <QVariant>

struct BGStatus
{
	Q_GADGET

public:
	enum class TrendArrow
	{
		NONE,
		TRIPLE_UP,
		DOUBLE_UP,
		SINGLE_UP,
		FORTY_FIVE_UP,
		FLAT,
		FORTY_FIVE_DOWN,
		SINGLE_DOWN,
		DOUBLE_DOWN,
		TRIPLE_DOWN
	};

	Q_ENUM(TrendArrow)

private:
	Q_PROPERTY(float bgValue MEMBER m_bgValue)
	Q_PROPERTY(float delta MEMBER m_delta)
	Q_PROPERTY(bool isValid MEMBER m_isValid)
	Q_PROPERTY(QDateTime timestamp MEMBER m_timestamp)
	Q_PROPERTY(TrendArrow trendArrow MEMBER m_trendArrow)

public:
	float m_bgValue = 0;
	float m_delta = 0;
	bool m_isValid = false;
	QDateTime m_timestamp;
	TrendArrow m_trendArrow = TrendArrow::NONE;
};

struct InsulinOnBoard
{
	Q_GADGET

	Q_PROPERTY(float basal MEMBER m_basal)
	Q_PROPERTY(float bolus MEMBER m_bolus)

public:
	float m_basal = 0.0f;
	float m_bolus = 0.0f;
};

struct CarbsOnBoard
{
	Q_GADGET

	Q_PROPERTY(int current MEMBER m_current)
	Q_PROPERTY(int future MEMBER m_future)

public:
	int m_current = 0;
	int m_future = 0;
};

struct BasalRate
{
	Q_GADGET

	Q_PROPERTY(float baseRate MEMBER m_baseRate)
	Q_PROPERTY(float currentRate MEMBER m_currentRate)
	Q_PROPERTY(int tbrPercentage MEMBER m_tbrPercentage)

public:
	float m_baseRate = 0.0f;
	float m_currentRate = 0.0f;
	int m_tbrPercentage = 100;
};

class BGDataReceiver
	: public QObject
{
	Q_OBJECT

public:
	enum class Unit
	{
		MG_DL,
		MMOL_L
	};
	Q_ENUM(Unit)

private:
	Q_PROPERTY(QVariant unit READ unit NOTIFY unitChanged)
	Q_PROPERTY(QVariant bgStatus READ bgStatus NOTIFY bgStatusChanged)
	Q_PROPERTY(QVariant insulinOnBoard READ insulinOnBoard NOTIFY insulinOnBoardChanged)
	Q_PROPERTY(QVariant carbsOnBoard READ carbsOnBoard NOTIFY carbsOnBoardChanged)
	Q_PROPERTY(QVariant lastLoopRunTimestamp READ lastLoopRunTimestamp NOTIFY lastLoopRunTimestampChanged)
	Q_PROPERTY(QVariant basalRate READ basalRate NOTIFY basalRateChanged)
	Q_PROPERTY(QVariantList const & bgTimeSeries READ bgTimeSeries)
	Q_PROPERTY(QVariantList const & basalTimeSeries READ basalTimeSeries)
	Q_PROPERTY(QVariantList const & baseBasalTimeSeries READ baseBasalTimeSeries)

public:
	explicit BGDataReceiver(QObject *parent = nullptr);

	QVariant unit() const;
	QVariant bgStatus() const;
	QVariant insulinOnBoard() const;
	QVariant carbsOnBoard() const;
	QDateTime const & lastLoopRunTimestamp() const;
	QVariant basalRate() const;
	QVariantList const & bgTimeSeries() const;
	QVariantList const & basalTimeSeries() const;
	QVariantList const & baseBasalTimeSeries() const;

	Q_INVOKABLE void generateTestQuantities();

signals:
	void quantitiesChanged();

	void unitChanged();
	void bgStatusChanged();
	void insulinOnBoardChanged();
	void carbsOnBoardChanged();
	void lastLoopRunTimestampChanged();
	void basalRateChanged();

public slots:
	// This slot is invoked by the DBus ExternalAppMessages adaptor
	// that is generated out of externalappmessages.xml.
	void pushMessage(QString sender, QByteArray payload);

private:
	void clearAllQuantities();

	std::optional<Unit> m_unit;
	std::optional<BGStatus> m_bgStatus;
	std::optional<InsulinOnBoard> m_iob;
	std::optional<CarbsOnBoard> m_cob;
	QDateTime m_lastLoopRunTimestamp;
	std::optional<BasalRate> m_basalRate;
	QVariantList m_bgTimeSeries;
	QVariantList m_basalTimeSeries;
	QVariantList m_baseBasalTimeSeries;
};

#endif // BGDATARECEIVER_HPP
