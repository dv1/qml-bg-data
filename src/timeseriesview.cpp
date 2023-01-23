#include <QDebug>
#include <QLoggingCategory>
#include <QImage>
#include <QSGFlatColorMaterial>
#include <QPointF>
#include <QPainter>
#include <QQuickWindow>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include "timeseriesview.hpp"


Q_DECLARE_LOGGING_CATEGORY(lcQmlBgData)


namespace
{


void simplifyTimeSeries(QVariantList const &sourceSeries, std::vector<QPointF> &destSeries, int minBucketWidth, int viewWidth)
{
	// This implements Sveinn Steinarsson’s Largest-Triangle-Three-Buckets (LTTB) algorithm
	// for downsampling time series data. Source: https://github.com/sveinn-steinarsson/flot-downsample
	// The MSc thesis with a description of LTTB is found at: http://hdl.handle.net/1946/15343
	//
	// The number of buckets is chosen based on the width of the QML item and the
	// desired minimum bucket with in pixels.
	// TODO: Currently, the "dynamic" variant of LTTB is not implemented. It could yield
	// better visual results and is worth investigating.

	// Calculate the number of buckets by rounding the viewWidth/minBucketWidth result.
	int numBuckets = ((viewWidth + (minBucketWidth -1)) / minBucketWidth);

	// Check for the special case where the view width is large enough to accomodate
	// for all source series data points. If so, then just return all source points.
	if (numBuckets > sourceSeries.size())
	{
		destSeries.resize(sourceSeries.size());
		for (int i = 0; i < sourceSeries.size(); ++i)
			destSeries[i] = sourceSeries[i].toPointF();
		return;
	}

	struct Bucket
	{
		std::vector<QPointF> m_points;
		int m_selectedPointIndex;
	};

	std::vector<Bucket> buckets;

	// Step 1: Initialize the buckets and assign each source series
	// point to an appropriate bucket. As per the definiton of LTTB,
	// the first and last source data points are placed in the first
	// and last buckets, respectively. Those buckets only contain
	// those single points. The remaining source series data points
	// are assigned to the remaining buckets based on the index
	// of the source data point in the source series to produce
	// buckets with approximately the same number of points in them.

	buckets.resize(numBuckets);

	buckets.front().m_points.push_back(sourceSeries.front().toPointF());
	buckets.back().m_points.push_back(sourceSeries.back().toPointF());

	// For each bucket, one of its points becomes "selected" (more
	// on that below). In the first and last buckets, since there is
	// exactly one point, only that single point can be the selected
	// point of those buckets.
	buckets.front().m_selectedPointIndex = 0;
	buckets.back().m_selectedPointIndex = 0;

	for (int sourcePointIndex = 1; sourcePointIndex < (sourceSeries.size() - 1); ++sourcePointIndex)
	{
		QPointF sourcePoint = sourceSeries[sourcePointIndex].toPointF();
		int bucketIndex = (sourcePointIndex - 1) * (numBuckets - 2) / (sourceSeries.size() - 2) + 1;

		buckets[bucketIndex].m_points.push_back(sourcePoint);
	}

	// Step 2: Rank the points in each bucket by going over each of
	// them and calculating the are of the triangle that is described
	// by the previous bucket's selected point, the current point
	// that is being evaluated, and the average of all of the next
	// bucket's points. The current bucket's point with the largest
	// triangle area gets the highest rank and thus "wins", becoming
	// the bucket's selected point.

	for (int bucketIndex = 1; bucketIndex < (numBuckets - 1); ++bucketIndex)
	{
		Bucket const &previousBucket = buckets[bucketIndex - 1];
		Bucket &currentBucket = buckets[bucketIndex];
		Bucket const &nextBucket = buckets[bucketIndex + 1];

		QPointF const &prevSelectedPoint = previousBucket.m_points[previousBucket.m_selectedPointIndex];

		QPointF nextAveragePoint(0.0f, 0.0f);
		for (auto const &nextBucketPoint : nextBucket.m_points)
			nextAveragePoint += nextBucketPoint;
		nextAveragePoint /= nextBucket.m_points.size();

		float currentBestRank = 0;
		int currentBestPointIndex = 0;

		float x1 = prevSelectedPoint.x();
		float y1 = prevSelectedPoint.y();
		float x3 = nextAveragePoint.x();
		float y3 = nextAveragePoint.y();

		for (std::size_t bucketPointIndex = 0; bucketPointIndex < currentBucket.m_points.size(); ++bucketPointIndex)
		{
			float x2 = currentBucket.m_points[bucketPointIndex].x();
			float y2 = currentBucket.m_points[bucketPointIndex].y();

			// The correct triangle area formulat is:
			//
			//   (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2)) * 0.5
			//
			// However, since we only need the area values for comparison purposes
			// to find the "best" point, we omit the "* 0.5" as a small optimization,
			// hence getting the "doubleTriangleArea" instead.
			float doubleTriangleArea = (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));

			if (doubleTriangleArea > currentBestRank)
			{
				currentBestRank = doubleTriangleArea;
				currentBestPointIndex = bucketPointIndex;
			}
		}

		currentBucket.m_selectedPointIndex = currentBestPointIndex;
	}

	// Step 3: Produce a new time series out of the selected points of each bucket.

	destSeries.resize(numBuckets);
	for (int bucketIndex = 0; bucketIndex < numBuckets; ++bucketIndex)
	{
		Bucket const &bucket = buckets[bucketIndex];
		destSeries[bucketIndex] = bucket.m_points[bucket.m_selectedPointIndex];
	}
}


} // unnamed namespace end


TimeSeriesView::TimeSeriesView(QQuickItem *parent)
	: QQuickItem(parent)
	, m_color(Qt::black)
	, m_mustUpdateMaterial(false)
	, m_mustRecreateNodeGeometry(false)
{
	setFlag(QQuickItem::ItemHasContents, true);

	connect(this, &QQuickItem::widthChanged, [this](){
		qCDebug(lcQmlBgData).nospace().noquote() << "Width changed to " << width() << "; need to recreate QSG node geometry";

		{
			std::lock_guard<std::mutex> lock(m_nodeStateMutex);
			m_mustRecreateNodeGeometry = true;
		}

		update();
	});

	connect(this, &QQuickItem::heightChanged, [this](){
		qCDebug(lcQmlBgData).nospace().noquote() << "Height changed to " << height() << "; need to recreate QSG node geometry";

		{
			std::lock_guard<std::mutex> lock(m_nodeStateMutex);
			m_mustRecreateNodeGeometry = true;
		}

		update();
	});
}


TimeSeriesView::~TimeSeriesView()
{
}


QColor const & TimeSeriesView::color() const
{
	return m_color;
}


void TimeSeriesView::setColor(QColor newColor)
{
	qCDebug(lcQmlBgData).nospace().noquote()
		<< "Using new color " << newColor;

	{
		std::lock_guard<std::mutex> lock(m_nodeStateMutex);
		m_color = std::move(newColor);
		m_mustUpdateMaterial = true;
	}

	update();
}


QVariantList const & TimeSeriesView::timeSeries() const
{
	return m_timeSeries;
}


void TimeSeriesView::setTimeSeries(QVariantList newTimeSeries)
{
	qCDebug(lcQmlBgData).nospace().noquote()
		<< "Got new time series with " << newTimeSeries.size()
		<< " item(s); will recreate QSG node geometry";

	{
		std::lock_guard<std::mutex> lock(m_nodeStateMutex);
		m_timeSeries = std::move(newTimeSeries);
		m_mustRecreateNodeGeometry = true;
	}

	update();
}


QSGNode* TimeSeriesView::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
	std::lock_guard<std::mutex> lock(m_nodeStateMutex);

	QSGGeometryNode *node;

	if (oldNode == nullptr)
	{
		qCDebug(lcQmlBgData) << "Creating new QSG time series node";

		node = new QSGGeometryNode();
		node->setFlag(QSGNode::OwnsGeometry, true);
		node->setFlag(QSGNode::OwnsMaterial, true);

		QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
		node->setMaterial(material);
		m_mustUpdateMaterial = true;

		QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
		geometry->setDrawingMode(QSGGeometry::DrawLineStrip);
		node->setGeometry(geometry);
	}
	else
	{
		node = static_cast<QSGGeometryNode *>(oldNode);
	}

	if (m_mustUpdateMaterial)
	{
		static_cast<QSGFlatColorMaterial *>(node->material())->setColor(m_color);
		m_mustUpdateMaterial = false;
	}

	if (m_timeSeries.empty())
	{
		qCDebug(lcQmlBgData) << "Clearing QSG time series node since the time series is empty";
		node->geometry()->allocate(0);
		m_simplifiedTimeSeries.clear();
		m_mustRecreateNodeGeometry = false;
	}
	else if (m_mustRecreateNodeGeometry)
	{
		int currentWidth = width();
		int currentHeight = height();

		if ((currentWidth <= 0) || (currentHeight <= 0))
		{
			// This should in theory never happen, but if it does,
			// we risk division by zero errors, so be on the safe side.
			qCWarning(lcQmlBgData).nospace().noquote()
				<< "Need to recreate QSG node geometry, but this currently cannot be done; "
				<< "QML item width and/or height are invalid; "
				<< "width: " << currentWidth << " height: " << currentHeight;
		}
		else
		{
			qCDebug(lcQmlBgData).nospace().noquote() << "Recreating QSG node geometry";

			simplifyTimeSeries(m_timeSeries, m_simplifiedTimeSeries, 3, currentWidth);
			qCDebug(lcQmlBgData).nospace().noquote()
				<< "Simplified original time series with " << m_timeSeries.size() << " item(s)"
				<< " to a time series with " << m_simplifiedTimeSeries.size() << " item(s)";

			QSGGeometry *geometry = node->geometry();

			if (int(m_simplifiedTimeSeries.size()) != geometry->vertexCount())
				geometry->allocate(m_simplifiedTimeSeries.size());

			QSGGeometry::Point2D *vertices = geometry->vertexDataAsPoint2D();

			for (std::size_t i = 0; i < m_simplifiedTimeSeries.size(); ++i)
			{
				QPointF const &timeSeriesPoint = m_simplifiedTimeSeries[i];
				float x = timeSeriesPoint.x() * currentWidth;
				float y = (1.0 - timeSeriesPoint.y()) * currentHeight;

				vertices[i].set(x, y);
			}

			node->markDirty(QSGNode::DirtyGeometry);

			m_mustRecreateNodeGeometry = false;
		}
	}

	return node;
}
