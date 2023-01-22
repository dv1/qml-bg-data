#ifndef TIMESERIESVIEW_HPP
#define TIMESERIESVIEW_HPP

#include <mutex>
#include <QQuickItem>
#include <QVariant>


class QSGTexture;


class TimeSeriesView
	: public QQuickItem
{
	Q_OBJECT

	Q_PROPERTY(QVariantList timeSeries READ timeSeries WRITE setTimeSeries)

public:
	explicit TimeSeriesView(QQuickItem *parent = nullptr);
	~TimeSeriesView() override;

	QVariantList const & timeSeries() const;
	void setTimeSeries(QVariantList newTimeSeries);

protected:
	QSGNode* updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData);

private:
	std::mutex m_dataMutex;
	QVariantList m_timeSeries;
	std::vector<QPointF> m_simplifiedTimeSeries;
	bool m_mustRecreateNodeGeometry;
};


#endif // TIMESERIESVIEW_HPP
