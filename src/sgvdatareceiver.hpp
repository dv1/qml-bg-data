#ifndef SGVDATARECEIVER_HPP
#define SGVDATARECEIVER_HPP

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QDateTime>
#include <QVariant>

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

class SGVDataReceiver
	: public QObject
{
public:
	Q_OBJECT

	Q_PROPERTY(QString unit READ unit NOTIFY unitChanged)
	Q_PROPERTY(int sgv READ sgv NOTIFY sgvChanged)
	Q_PROPERTY(int delta READ delta NOTIFY deltaChanged)
	Q_PROPERTY(InsulinOnBoard insulinOnBoard READ insulinOnBoard NOTIFY insulinOnBoardChanged)
	Q_PROPERTY(CarbsOnBoard carbsOnBoard READ carbsOnBoard NOTIFY carbsOnBoardChanged)
	Q_PROPERTY(QDateTime lastSgvTime READ lastSgvTime NOTIFY lastSgvTimeChanged)
	Q_PROPERTY(QDateTime lastLoopRunTime READ lastLoopRunTime NOTIFY lastLoopRunTimeChanged)
	Q_PROPERTY(BasalRate basalRate READ basalRate NOTIFY basalRateChanged)
	Q_PROPERTY(QVariantList graph READ graph NOTIFY graphChanged)

public:
	explicit SGVDataReceiver(QObject *parent = nullptr);

	QString unit() const;
	int sgv() const;
	int delta() const;
	InsulinOnBoard const & insulinOnBoard() const;
	CarbsOnBoard const & carbsOnBoard() const;
	QDateTime const & lastSgvTime() const;
	QDateTime const & lastLoopRunTime() const;
	BasalRate const & basalRate() const;
	QVariantList const & graph() const;

signals:
	void updateStarted();
	void updateEnded();

	void unitChanged();
	void sgvChanged();
	void deltaChanged();
	void insulinOnBoardChanged();
	void carbsOnBoardChanged();
	void lastSgvTimeChanged();
	void lastLoopRunTimeChanged();
	void basalRateChanged();
	void graphChanged();

public slots:
	// This slot is invoked by the DBus ExternalAppMessages adaptor
	// that is generated out of externalappmessages.xml.
	void pushMessage(QString sender, QString ID, QString body);

private:
	void update(QJsonObject const &json);

	bool m_unitIsMGDL = false;
	int m_sgv = 0;
	int m_delta = 0;
	InsulinOnBoard m_iob;
	CarbsOnBoard m_cob;
	QDateTime m_lastSgvTime;
	QDateTime m_lastLoopRunTime;
	BasalRate m_basalRate;
	QVariantList m_graph;
};

#endif // SGVDATARECEIVER_HPP
