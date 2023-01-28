#ifndef BGDATARECEIVER_HPP
#define BGDATARECEIVER_HPP

#include <optional>
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QDateTime>
#include <QVariant>

/*!
	\class BGStatus
	\brief Structure containing information about the current BG status.

	This structure contains the following quantities:

	\list
	\li bgValue : The numeric BG value. Unit is either mg/dL or mmol/L,
		depending on the unit specified in \c BGDataReceiver.
	\li delta : The BG value delta (that is, by how much the BG value
		changed since the last BG measurement). Just like \c bgValue,
		its unit depends on the unit specified in \c BGDataReceiver.
		This is a \c QVariant, because sometimes, the delta value may be
		absent, in which case the QVariant will be empty. In QML, such
		an empty QVariant maps to a null value. Thus, in QML scripts,
		if delta is not present, then that property's value is null.
	\li isValid : True if the \c bgValue is valid. This does not refer
		to the _presence_ of the bgValue. A bgValue can exist and yet
		be invalid. If it is invalid, and if the QML UI displays the
		bgValue, it must then do that with the BG value written on
		the UI with a strikethrough style. If isValid is true, then
		the BG value can be written normally on the UI.
	\li timestamp : \c QDateTime object with the timestamp (in UTC) of
		when this BG status was updated. This can be invalid (in QML,
		this means a null value) if no BG status update timestamp is
		known. Typically, this property is not used. Instead, it is
		recommended to use \c {BGDataReceiver.getTimespansSince()}.
	\li trendArrow : Enum indicating the trend of the BG value's
		change. If no trend is currently known, this is set to
		\c {TrendArrow.NONE}. Ideally, this should be visualized by
		an icon, and that icon should always occupy the same space,
		no matter if the trend arrow is set to \c {TrendArrow.FLAT},
		\c {TrendArrow.SINGLE_UP}, \c {TrendArrow.TRIPLE_UP} etc. Simple
		test UIs might just write something like "↑↑↑" or
		"↑" on the UI, but that is not ideal.
		\c {TrendArrow.FLAT} is an arrow that points to the right, like
		"→". \c {TrendArrow.FORTY_FIVE_UP} and \c {TrendArrow.FORTY_FIVE_DOWN}
		point upwards/downwards and to the right, like "↗" and
		"↘", respectively.
	\endlist

	For bgValue, the recommended number of fractional digits shown
	on the UI depends on the unit. If the unit is mg/dL, no fractional
	digits should be shown. If the unit is mmol/L, one fractional digit
	should be shown. For the delta, the same applies, except when the
	unit is mg/dL and the delta is between 0 and 1. Ine fractional
	digit can be shown then.

	\c BGDataReceiver contains a \c bgStatus property that is an instance
	of this structure. That property can be null, however, in which
	case any and all quantities on the UI that come from this
	structure must be reset to their initial state. That's because
	a null \c {BGDataReceiver.bgValue} indicates that all previously
	displayed \c BGStatus state is now invalid and has to be erased.
*/
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
	Q_PROPERTY(QVariant delta MEMBER m_delta)
	Q_PROPERTY(bool isValid MEMBER m_isValid)
	Q_PROPERTY(QDateTime timestamp MEMBER m_timestamp)
	Q_PROPERTY(TrendArrow trendArrow MEMBER m_trendArrow)

public:
	float m_bgValue = 0;
	QVariant m_delta;
	bool m_isValid = false;
	QDateTime m_timestamp;
	TrendArrow m_trendArrow = TrendArrow::NONE;
};

/*!
	\class InsulinOnBoard
	\brief Structure containing information about the current insulin on board (IOB) quantities.

	This structure contains the following quantities:

	\list
		\li basal : Amount of basal insulin on board, in IU units.
		\li bolus : Amount of bolus insulin on board, in IU units.
	\endlist

	The basal and bolus values in this structure are always valid. It is
	recommended to limit the number of fractional digits shown on screen to 2.

	The basal can be negative. Bolus is never negative, however.

	\c BGDataReceiver contains an iob property that is an instance
	of this structure. That property can be null, however, in which
	case any and all quantities on the UI that come from this
	structure must be reset to their initial state. That's because
	a null \c {BGDataReceiver.insulinOnBoard} indicates that all previously
	displayed InsulinOnBoard state is now invalid and has to be erased.
*/
struct InsulinOnBoard
{
	Q_GADGET

	Q_PROPERTY(float basal MEMBER m_basal)
	Q_PROPERTY(float bolus MEMBER m_bolus)

public:
	float m_basal = 0.0f;
	float m_bolus = 0.0f;
};

/*!
	\class CarbsOnBoard
	\brief Structure containing information about the current carbs on board (IOB) quantities.

	This structure contains the following quantities:

	\list
		\li current : Amount of carbohydrates that are being actively absorbed right now, in grams (g).
		\li future : Amount of carbohydrates that will be absorbed in the future, in grams (g).
	\endlist

	The current and future values in this structure are always valid.
	These are always integers; no fractional digits may be shown.

	The quantities are never negative.

	\c BGDataReceiver contains a cob property that is an instance
	of this structure. That property can be null, however, in which
	case any and all quantities on the UI that come from this
	structure must be reset to their initial state. That's because
	a null \c {BGDataReceiver.carbsOnBoard} indicates that all previously
	displayed CarbsOnBoard state is now invalid and has to be erased.
*/
struct CarbsOnBoard
{
	Q_GADGET

	Q_PROPERTY(int current MEMBER m_current)
	Q_PROPERTY(int future MEMBER m_future)

public:
	int m_current = 0;
	int m_future = 0;
};

/*!
	\class BasalRate
	\brief Structure containing information about the currently active basal rate.

	This structure contains the following quantities:

	\list
		\li baseRate : The current base basal rate, in IU units.
		    "Base" means that this is the basal rate _without_ the TBR percentage
		    applied; this quantity stays the same no matter what TBR percentage
		    is currentĺy being used. The value only depends on the actual basal
		    rate that is programmed in the insulin pump.
		\li currentRate : The currently active basal rate. This is the
		    baseRate _with_ the TBR percentage applied. For example, if baseRate
		    is 0.5, and TBR percentage is 400%, then currentRate is 0.5 * 400% = 2.
		\li tbrPercentage : The Temporary Basal Rate (TBR) percentage that is
		    currently being used. If no TBR is running, this value is set to 100.
	\endlist

	The quantities are never negative.

	\c BGDataReceiver contains a basalRate property that is an instance
	of this structure. That property can be null, however, in which
	case any and all quantities on the UI that come from this
	structure must be reset to their initial state. That's because
	a null \c {BGDataReceiver.basalRate} indicates that all previously
	displayed BasalRate state is now invalid and has to be erased.
*/
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

/*!
	\class Timespans
	\brief Structure containing functionality to determine how long ago certain actions were performed.

	This is useful for showing information like "action X happened Y minutes ago" on the UI.
	Two such actions are currently supported:

	\list
		\li bgStatusUpdate : How many seconds ago the BGDataReceiver.bgStatus was updated.
		\li lastLoopRun : How many seconds ago the closed-loop system was run.
	\endlist

	The quantities are integers, and in seconds. Even though typically, UIs show minutes,
	as in "X minutes ago", seconds are provided, because some UIs might require that accuracy.

	The quantities are never negative.

	If an action was not performed yet, or was performed at some unknwon time, then the
	corresponding value will be set to an empty \c QVariant (corresponding ot a null value in QML).

	There is no property in \c BGDataReceiver whose type is this structure. Instead, use
	\c {BGDataReceiver.getTimespansSince()} to get an instance of this structure.
*/
struct Timespans
{
	Q_GADGET

	Q_PROPERTY(QVariant bgStatusUpdate MEMBER m_bgStatusUpdate)
	Q_PROPERTY(QVariant lastLoopRun MEMBER m_lastLoopRun)

public:
	QVariant m_bgStatusUpdate;
	QVariant m_lastLoopRun;
};

/*!
	\class BGDataReceiver
	\brief Class that receives BG data over D-Bus, processes it, and updates its properties with it.

	Even though that this class can be used in non-QML projects as well, it is primarily
	designed for use in QML scripts. Only one instance of this class may exist at the same
	time, since it registers a D-Bus receiver adaptor, and two instances would cause an
	error due to collision.

	To use this for receiving BG data and get notified when new BG data arrives, create
	an instance of this class in the QML script (typically at root level). Then, add
	signal handlers for the \c {*Changed} signals of interest (or for \c newDataReceived).

	All properties may be invalid at some point. If they are, then they are set to their
	default empty/invalid \c QVariant values (which maps to null values in QML). For the
	time series properties, they are never really invalid. Instead, if there are no
	time series, these properties (which are lists) are simply empty.

	For properties that can be null, it is important to clear out any quantities
	that are shown on the UI if the corresponding property is null. If for example the
	\c insulinOnBoard property becomes null, and the UI currently shows quantities for
	the basal and bolus IOB, then those quantities must be erased from the UI.

	All quantities except the time series are of type \c QVariant. This is necessary to
	allow for them to be set to null. If they aren't set to null, these properties
	are set to the types that correspond to the property name. For example, a non-null
	\c insulinOnBoard property contains an instance of \c InsulinOnBoard.

	The time series are lists of variants. Again, this is done that way for integration
	into QML. The items in these lists are \c QVariant instances, but those variants are
	never invalid - they always contain a \c QPointF instance. However, users typically
	do not have to bother with the contents of the time series. All that's necessary
	is to pass them to the corresponding properties in a \c BGTimeSeriesView. The whole
	point of these time series is visualization, which \c BGTimeSeriesView takes care of.

	NOTE: The basalTimeSeries and baseBasalTimeSeries properties are currently not in use.

	When new BG data is received, the class checks which parts of the BG data actually
	changed. If for example a new BG status is contained in the BG data, but it turns
	out that compared to the currently already available BGStatus information, nothing
	changed, then the \c bgStatusChanged signal will not be emitted. That way, UI updates
	can be limited to when they are really necessary. However, for time series, no such
	\c {*Changed} signal exists - those need to be updated every time new BG data is received.
	It is recommended to update them when the \c newDataReceived signal is emitted. That
	signal is emitted every time new BG data arrives.

	In sum:
	\list
		\li Update time series (and _only_ the time series) in a \c newDataReceived signal handler.
		\li Update everything else in their corresponding \c {*Changed} signal handlers.
	\endlist

	For developing watchfaces, it may be useful to be able to use \c BGDataReceiver
	without having to set up an actual BG data source. This then requires a substitute
	for that BG data. \c {generateTestQuantities()} can be used to generate random BG data.
	Doing this will emit all of the \c {*Changed} signals as well as \c newDataReceived.

	\c {getTimespansSince()} is useful for getting a \c Timespans instance that contains
	the times since the BG data was updated and since the closed-loop system was run.
	This is needed for "X min ago" information shown on the UI. See the \c Timespans
	and \c {getTimespansSince()} documentations for details.

	Basic example QML usage:

	\qml
		import QmlBgData 1.0

		BGDataReceiver {
			id: bgDataReceiver

			onNewDataReceived: {
				// Pass the new bgTimeSeries to the time series view
				bgTimeSeriesView.bgTimeSeries = bgTimeSeries;
			}

			onBgStatusChanged: {
				// Update the BG status on screen; this signal handler
				// is only called when the BG status really did change
				bgValueText.bgStatus = bgStatus;
			}

			Component.onCompleted: {
				// Generate some test quantities as soon as the receiver is up
				bgDataReceiver.generateTestQuantities()
			}
		}

		Item {
			id: root
			anchors.fill: parent

			Text {
				id: bgValueText

				property var bgStatus: null

				x: 0
				y: 0
				width: parent.width
				height: parent.height * 0.5

				text: {
					if (bgStatus == null)
						return "";
					else
						return Math.round(bgStatus.bgValue * 10) / 10;
				}
				font.strikeout: (bgStatus != null) ? !(bgStatus.isValid) : false
			}

			BGTimeSeriesView {
				id: bgTimeSeriesView

				x: 0
				y: parent.height * 0.5
				width: parent.width
				height: parent.height * 0.5

				color: "black"
			}
		}
	\endqml
*/
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

	/*!
		\fn BGDataReceiver::generateTestQuantities()

		Fills all of the properties with random test data.

		This is useful for testing out QML UIs that use this
		QML item without having to have a BG source running.
	*/
	Q_INVOKABLE void generateTestQuantities();

	/*!
		\fn BGDataReceiver::getTimespansSince(QDateTime now)

		Returns the times in seconds since certain actions were taken.

		The return value is a \c Timespans instance (wrapped in a \c QVariant)
		that contains the seconds since the BG status was received (if
		any was received) and the seconds since the last time the
		closed-loop system was run. If this cannot be determined (because
		for example no BG status has been received so far), then the
		corresponding field in the returned \c Timespans instance is set
		to an empty \c QVariant value, which maps to null in QML.

		These timespans are calculated based on the \c now timestamp.
		That is, the times from the past actions until the \c now
		timestamp is calculated (in seconds) and stored in the
		corresponding \c Timespans fields. \c now must always
		be a valid \c QDateTime instance.
	*/
	Q_INVOKABLE QVariant getTimespansSince(QDateTime now);

signals:
	/*!
		\fn BGDataReceiver::newDataReceived()
		Emitted when new BG data is received.

		This is useful at least for updating \c BGTimeSeriesView items with the
		new time series from this \c BGDataReceiver. (The update is done by
		passing \c bgTimeSeries to the \c {BGTimeSeriesView.bgTimeSeries} property.)
		However, for other values like \c bgStatus, it is generally better
		to listen to the individual \c {*Changed} signals instead, since
		those are only emitted when the associated value actually changed.
	*/
	void newDataReceived();

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
