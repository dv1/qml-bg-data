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

	Timer {
		id: minutesAgoUpdatesTimer

		interval: 30 * 1000
		running: false
		repeat: true
		triggeredOnStart: true

		onTriggered: {
			var timespans = bgDataReceiver.getTimespansSince(new Date());

			if ((timespans.bgStatusUpdate == null) && (timespans.lastLoopRun == null))
				stop();

			minutesAgoText.text = "";

			if (timespans.bgStatusUpdate != null)
				minutesAgoText.text += "lBG: " + parseInt(timespans.bgStatusUpdate / 60) + " ";
			if (timespans.lastLoopRun != null)
				minutesAgoText.text += "lLoop: " + parseInt(timespans.lastLoopRun / 60);
		}
	}

	BGDataReceiver {
		id: bgDataReceiver

		onNewDataReceived: {
			bgTimeSeriesView.bgTimeSeries = bgTimeSeries;
			minutesAgoUpdatesTimer.restart();
		}

		onUnitChanged: {
			bgValueAndTrendArrowText.unit = unit;
		}

		onBgStatusChanged: {
			bgValueAndTrendArrowText.bgStatus = bgStatus;
		}

		onInsulinOnBoardChanged: {
			iobAndCobText.iob = insulinOnBoard;
		}

		onCarbsOnBoardChanged: {
			iobAndCobText.cob = carbsOnBoard;
		}

		onBasalRateChanged: {
			basalAndTbrText.basalRate = basalRate;
		}

		Component.onCompleted: {
			bgDataReceiver.generateTestQuantities()
		}
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

			property var bgStatus: null
			property var unit: null

			x: 0
			y: parent.height * 0.2
			width: parent.width
			height: parent.height * 0.2

			font.pixelSize: parent.height * 0.06

			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter

			text: {
				if (bgStatus == null)
					return "";

				var unitLabel = (unit == BGDataReceiver.Unit.MG_DL) ? "mg/dL" : "mmol/L";
				var s = roundTo1Digit(bgStatus.bgValue) + " " + unitLabel + " " + trendArrowCharacter(bgStatus.trendArrow);

				if (bgStatus.delta != null)
					s += " Δ: " + roundTo1Digit(bgStatus.delta);
				else
					s += " Δ: N/A";

				return s;
			}
			font.strikeout: (bgStatus != null) ? !(bgStatus.isValid) : false
		}

		Text {
			id: basalAndTbrText

			property var basalRate: null

			x: 0
			y: parent.height * 0.4
			width: parent.width
			height: parent.height * 0.2

			font.pixelSize: parent.height * 0.06

			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter

			text: {
				if (basalRate != null)
					return "basal: base " + roundTo2Digits(basalRate.baseRate) + " cur " + roundTo2Digits(basalRate.currentRate) + " " + basalRate.tbrPercentage + "%";
				else
					return "";
			}
		}

		Text {
			id: iobAndCobText

			property var iob: null
			property var cob: null

			x: 0
			y: parent.height * 0.6
			width: parent.width
			height: parent.height * 0.2

			font.pixelSize: parent.height * 0.06

			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter

			text: {
				var s = "";

				if (iob != null)
					s += "IOB: " + roundTo2Digits(iob.basal + iob.bolus) + "(" + roundTo2Digits(iob.basal) + "|" + roundTo2Digits(iob.bolus) + ") ";
				if (cob != null)
					s += "COB: " + cob.current + "|" + cob.future;

				return s;
			}
		}

		Rectangle {
			x: 0
			y: parent.height * 0.8
			width: parent.width * 0.5
			height: parent.height * 0.2

			border.color: "black"
			border.width: 1
			color: "transparent"

			BGTimeSeriesView {
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
