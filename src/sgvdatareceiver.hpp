#ifndef SGVDATARECEIVER_HPP
#define SGVDATARECEIVER_HPP

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

struct SGV
{
	Q_GADGET

	Q_ENUM(TrendArrow)

	Q_PROPERTY(int sgv MEMBER m_sgv)
	Q_PROPERTY(bool isValid MEMBER m_isValid)
	Q_PROPERTY(TrendArrow trendArrow MEMBER m_trendArrow)
	Q_PROPERTY(int delta MEMBER m_delta)
	Q_PROPERTY(QDateTime lastTime MEMBER m_lastTime)

public:
	int m_sgv = 0;
	bool m_isValid = false;
	TrendArrow m_trendArrow = TrendArrow::NONE;
	int m_delta = 0;
	QDateTime m_lastTime;
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
	Q_PROPERTY(int percentage MEMBER m_percentage)

public:
	float m_baseRate = 0.0f;
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

class SGVDataReceiver
	: public QObject
{
public:
	Q_OBJECT

	Q_PROPERTY(QVariant unit READ unit NOTIFY unitChanged)
	Q_PROPERTY(QVariant sgv READ sgv NOTIFY sgvChanged)
	Q_PROPERTY(QVariant insulinOnBoard READ insulinOnBoard NOTIFY insulinOnBoardChanged)
	Q_PROPERTY(QVariant carbsOnBoard READ carbsOnBoard NOTIFY carbsOnBoardChanged)
	Q_PROPERTY(QVariant lastLoopRunTime READ lastLoopRunTime NOTIFY lastLoopRunTimeChanged)
	Q_PROPERTY(QVariant basalRate READ basalRate NOTIFY basalRateChanged)
	Q_PROPERTY(Graph const & graph READ graph NOTIFY graphChanged)

public:
	explicit SGVDataReceiver(QObject *parent = nullptr);

	QVariant unit() const;
	QVariant sgv() const;
	QVariant insulinOnBoard() const;
	QVariant carbsOnBoard() const;
	QDateTime const & lastLoopRunTime() const;
	QVariant basalRate() const;
	Graph const & graph() const;

signals:
	void updateStarted();
	void updateEnded();

	void unitChanged();
	void sgvChanged();
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
	std::optional<SGV> m_sgv;
	std::optional<InsulinOnBoard> m_iob;
	std::optional<CarbsOnBoard> m_cob;
	QDateTime m_lastLoopRunTime;
	std::optional<BasalRate> m_basalRate;
	Graph m_graph;
};

#endif // SGVDATARECEIVER_HPP
