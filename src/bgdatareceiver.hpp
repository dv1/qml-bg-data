#ifndef BGDATARECEIVER_HPP
#define BGDATARECEIVER_HPP

#include <optional>
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QDateTime>
#include <QVariant>

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

struct BGStatus
{
	Q_GADGET

	Q_ENUM(TrendArrow)

	Q_PROPERTY(int bgValue MEMBER m_bgValue)
	Q_PROPERTY(float delta MEMBER m_delta)
	Q_PROPERTY(bool isValid MEMBER m_isValid)
	Q_PROPERTY(QDateTime timestamp MEMBER m_timestamp)
	Q_PROPERTY(TrendArrow trendArrow MEMBER m_trendArrow)

public:
	int m_bgValue = 0;
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
	Q_PROPERTY(int percentage MEMBER m_percentage)

public:
	float m_baseRate = 0.0f;
	float m_currentRate = 0.0f;
	int m_percentage = 100;
};

struct Graph
{
	Q_GADGET

	Q_PROPERTY(QVariantList bgValues MEMBER m_bgValues)
	Q_PROPERTY(QVariantList bgTimestamps MEMBER m_bgTimestamps)

public:
	QVariantList m_bgValues;
	QVariantList m_bgTimestamps;
};

class BGDataReceiver
	: public QObject
{
public:
	Q_OBJECT

	Q_PROPERTY(QVariant unit READ unit NOTIFY unitChanged)
	Q_PROPERTY(QVariant bgStatus READ bgStatus NOTIFY bgStatusChanged)
	Q_PROPERTY(QVariant insulinOnBoard READ insulinOnBoard NOTIFY insulinOnBoardChanged)
	Q_PROPERTY(QVariant carbsOnBoard READ carbsOnBoard NOTIFY carbsOnBoardChanged)
	Q_PROPERTY(QVariant lastLoopRunTime READ lastLoopRunTime NOTIFY lastLoopRunTimeChanged)
	Q_PROPERTY(QVariant basalRate READ basalRate NOTIFY basalRateChanged)
	Q_PROPERTY(Graph const & graph READ graph NOTIFY graphChanged)

public:
	explicit BGDataReceiver(QObject *parent = nullptr);

	QVariant unit() const;
	QVariant bgStatus() const;
	QVariant insulinOnBoard() const;
	QVariant carbsOnBoard() const;
	QDateTime const & lastLoopRunTime() const;
	QVariant basalRate() const;
	Graph const & graph() const;

signals:
	void updateStarted();
	void updateEnded();

	void allQuantitiesCleared();

	void unitChanged();
	void bgStatusChanged();
	void insulinOnBoardChanged();
	void carbsOnBoardChanged();
	void lastLoopRunTimeChanged();
	void basalRateChanged();
	void graphChanged();

public slots:
	// This slot is invoked by the DBus ExternalAppMessages adaptor
	// that is generated out of externalappmessages.xml.
	void pushMessage(QString sender, QString ID, QString body);

private:
	void resetQuantities();
	void update(QJsonObject const &json);

	std::optional<bool> m_unitIsMGDL;
	std::optional<BGStatus> m_bgStatus;
	std::optional<InsulinOnBoard> m_iob;
	std::optional<CarbsOnBoard> m_cob;
	QDateTime m_lastLoopRunTime;
	std::optional<BasalRate> m_basalRate;
	Graph m_graph;
};

#endif // BGDATARECEIVER_HPP
