#ifndef BGTIMESERIESVIEW_HPP
#define BGTIMESERIESVIEW_HPP

#include <mutex>
#include <QQuickItem>
#include <QVariant>


class QSGTexture;


class BGTimeSeriesView
	: public QQuickItem
{
	Q_OBJECT

	Q_PROPERTY(QColor color READ color WRITE setColor)
	Q_PROPERTY(float lineWidth READ lineWidth WRITE setLineWidth)
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
