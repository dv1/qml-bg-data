import QtQuick 2.15
import QtQuick.Shapes 1.15
import QtGraphicalEffects 1.15

import QmlBgData 1.0

Item {
	id: root

	anchors.fill: parent

	function roundTo1Digit(x) {
		return Math.round(x * 10) / 10;
	}

	function roundTo2Digits(x) {
		return Math.round(x * 100) / 100;
	}

	function trendArrowCharacter(trendArrow) {
		switch (trendArrow) {
			case BGStatus.TrendArrow.TRIPLE_UP: return "↑↑↑";
			case BGStatus.TrendArrow.DOUBLE_UP: return "↑↑";
			case BGStatus.TrendArrow.SINGLE_UP: return "↑";
			case BGStatus.TrendArrow.FORTY_FIVE_UP: return "↗";
			case BGStatus.TrendArrow.FLAT: return "→";
			case BGStatus.TrendArrow.FORTY_FIVE_DOWN: return "↘";
			case BGStatus.TrendArrow.SINGLE_DOWN: return "↓";
			case BGStatus.TrendArrow.DOUBLE_DOWN: return "↓↓";
			case BGStatus.TrendArrow.TRIPLE_DOWN: return "↓↓↓";
			default: return "";
		}
	}

	function calcHowManyMinutesAgo(timestamp, now) {
		return Math.trunc(Math.max((now - timestamp) / 1000 / 60, 0));
	}

	Timer {
		id: minutesAgoUpdatesTimer

		interval: 500
		running: false
		repeat: true
		triggeredOnStart: true

		onTriggered: {
			var now = (new Date()).getTime();
			minutesAgoText.text = "";
			if (lastBgStatusTS != null)
				minutesAgoText.text += "lBG: " + calcHowManyMinutesAgo(lastBgStatusTS.getTime(), now) + " ";
			if (lastLoopRunTS != null)
				minutesAgoText.text += "lLoop: " + calcHowManyMinutesAgo(lastLoopRunTS, now);
		}
	}

	property var lastBgStatusTS: null
	property var lastLoopRunTS: null

	BGDataReceiver {
		id: bgDataReceiver

		onQuantitiesChanged: {
			console.log("Got new BG data")

			if (bgStatus == null) {
				minutesAgoUpdatesTimer.stop();
				lastBgStatusTS = null;
				lastLoopRunTS = null;
				minutesAgoText.text = "";

				bgValueAndTrendArrowText.unit = null;
				bgValueAndTrendArrowText.bgValue = null;
				bgValueAndTrendArrowText.bgValueIsValid = false;
				bgValueAndTrendArrowText.trendArrow = BGStatus.TrendArrow.NONE;
				bgValueAndTrendArrowText.delta = null;

				basalAndTbrText.baseRate = 0;
				basalAndTbrText.currentDate = 0;
				basalAndTbrText.tbrPercentage = 100;
			} else {
				bgValueAndTrendArrowText.unit = unit;
				bgValueAndTrendArrowText.bgValue = bgStatus.bgValue;
				bgValueAndTrendArrowText.bgValueIsValid = bgStatus.isValid;
				bgValueAndTrendArrowText.trendArrow = bgStatus.trendArrow;
				bgValueAndTrendArrowText.delta = bgStatus.delta;

				basalAndTbrText.baseRate = basalRate.baseRate;
				basalAndTbrText.currentRate = basalRate.currentRate;
				basalAndTbrText.tbrPercentage = basalRate.tbrPercentage;

				iobAndCobText.basal = insulinOnBoard.basal;
				iobAndCobText.bolus = insulinOnBoard.bolus;
				iobAndCobText.currentCarbs = carbsOnBoard.current;
				iobAndCobText.futureCarbs = carbsOnBoard.future;

				bgTimeSeriesView.timeSeries = bgTimeSeries;

				lastBgStatusTS = bgStatus.timestamp;
				lastLoopRunTS = lastLoopRunTimestamp;

				minutesAgoUpdatesTimer.restart();
			}
		}

		/* Component.onCompleted: {
			bgDataReceiver.generateTestQuantities()
		} */
	}

	Item {
		id: watchfaceRoot

		anchors.fill: parent

		Text {
			id: datetimeText

			property date currentDate: new Date()

			x: 0
			y: 0
			width: parent.width
			height: parent.height * 0.2

			font.pixelSize: parent.height * 0.06

			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter

			text: currentDate.toLocaleDateString()
		}

		Text {
			id: bgValueAndTrendArrowText

			property var unit: null
			property var bgValue: null
			property bool bgValueIsValid: false
			property int trendArrow: BGStatus.TrendArrow.NONE
			property var delta: null

			x: 0
			y: parent.height * 0.2
			width: parent.width
			height: parent.height * 0.2

			font.pixelSize: parent.height * 0.06

			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter

			text: {
				if (bgValue == null) {
					text = "";
					return;
				}

				var unitLabel = (unit == BGDataReceiver.Unit.MG_DL) ? "mg/dL" : "mmol/L";
				var text = roundTo1Digit(bgValue) + " " + unitLabel + " " + trendArrowCharacter(trendArrow);

				if (delta != null)
					text += " Δ: " + roundTo1Digit(delta);

				return text;
			}
			font.strikeout: !bgValueIsValid
		}

		Text {
			id: basalAndTbrText

			property real baseRate: 0.0
			property real currentRate: 0.0
			property int tbrPercentage: 100

			x: 0
			y: parent.height * 0.4
			width: parent.width
			height: parent.height * 0.2

			font.pixelSize: parent.height * 0.06

			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter

			text: "basal: base " + roundTo2Digits(baseRate) + " cur " + roundTo2Digits(currentRate) + " " + tbrPercentage + "%";
		}

		Text {
			id: iobAndCobText

			property real basal: 0.0
			property real bolus: 0.0
			property int currentCarbs: 0
			property int futureCarbs: 0

			x: 0
			y: parent.height * 0.6
			width: parent.width
			height: parent.height * 0.2

			font.pixelSize: parent.height * 0.06

			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter

			text: "IOB: " + roundTo2Digits(basal + bolus) + "(" + roundTo2Digits(basal) + "|" + roundTo2Digits(bolus) + ") "
			    + "COB: " + currentCarbs + "|" + futureCarbs;
		}

		Rectangle {
			x: 0
			y: parent.height * 0.8
			width: parent.width * 0.5
			height: parent.height * 0.2

			border.color: "black"
			border.width: 1
			color: "transparent"

			TimeSeriesView {
				id: bgTimeSeriesView
				anchors.fill: parent
				color: "black"
			}
		}

		Text {
			id: minutesAgoText

			x: parent.width * 0.5
			y: parent.height * 0.8
			width: parent.width * 0.5
			height: parent.height * 0.2

			font.pixelSize: parent.height * 0.06

			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter

			text: "";
		}
	}

	Component.onCompleted: {
		datetimeText.currentDate = new Date()
	}
}
