#ifndef BGTIMESERIESVIEW_HPP
#define BGTIMESERIESVIEW_HPP

#include <mutex>
#include <QQuickItem>
#include <QVariant>


/*!
	\class BGTimeSeriesView
	\brief Graphical Quick item for drawing BG time series data coming from \c BGDataReceiver.

	This class implements a Qt Quick item that renders \c BGDataReceiver BG time series
	as a graph. This graph is simplified if the item's with is too small for the amount
	of time series data to improve graph readability.

	To use, assign the value of \c {BGDataReceiver.bgTimeSeries} to this item's
	\c bgTimeSeries property every time new BG time series become available. Typically,
	the way to go is to do this assignment in a \c {BGDataReceiver.newDataReceived}
	signal handler, like this:

	\qml
		BGDataReceiver {
			onNewDataReceived: {
				bgTimeSeriesView.bgTimeSeries = bgTimeSeries;
			}
		}

		BGTimeSeriesView {
			id: bgTimeSeriesView
		}
	\endqml

	The item does not render any background; only the line graph itself is drawn,
	with the color specified by the \c color property.
*/
class BGTimeSeriesView
	: public QQuickItem
{
	Q_OBJECT

	/*!
		\property BGTimeSeriesView::color
		\brief Color of the line graph.

		The default color is black. The alpha channel is suppported,
		so semi-translucent graphs are possible.
	*/
	Q_PROPERTY(QColor color READ color WRITE setColor)

	/*!
		\property BGTimeSeriesView::lineWidth
		\brief Width (or thickness) of the lines that make up the graph, in pixels.

		\note A line width other than 1.0 may not always be supported. This is
		decided by the GPU and GPU driver support. This limitation is being worked on.
	*/
	Q_PROPERTY(float lineWidth READ lineWidth WRITE setLineWidth)

	/*!
		\property BGTimeSeriesView::bgTimeSeries
		\brief The BG time series to render.
	*/
	Q_PROPERTY(QVariantList bgTimeSeries READ bgTimeSeries WRITE setBGTimeSeries)

public:
	explicit BGTimeSeriesView(QQuickItem *parent = nullptr);
	~BGTimeSeriesView() override;

	QColor const & color() const;
	void setColor(QColor newColor);

	float lineWidth() const;
	void setLineWidth(float newLineWidth);

	QVariantList const & bgTimeSeries() const;
	void setBGTimeSeries(QVariantList newBGTimeSeries);

protected:
	QSGNode* updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData);

private:
	// This mutex protects states that are relevant to the QSG node.
	// In particular, it synchronizes write access by the setters
	// above and the code in updatePaintNode(), because the latter
	// runs in a different thread than the getters and setters abvoe.
	std::mutex m_nodeStateMutex;

	QColor m_color;
	float m_lineWidth;
	bool m_mustUpdateMaterial;

	QVariantList m_bgTimeSeries;
	std::vector<QPointF> m_simplifiedBGTimeSeries;
	bool m_mustRecreateNodeGeometry;
};


#endif // BGTIMESERIESVIEW_HPP
