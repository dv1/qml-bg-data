// NOTE: Even though the qmlRegisterType() documentation lists QQmlEngine
// as the header, it alone is not enough, since the QML registration
// functions are defined elsewhere. Including QtQml makes sure the necessary
// heades are included.
#include <QtQml>
#include <QLoggingCategory>

#include "qmlbgdataplugin.hpp"
#include "bgdatareceiver.hpp"


Q_LOGGING_CATEGORY(lcQmlBgData, "qmlbgdata")


QmlBgDataPlugin::QmlBgDataPlugin(QObject *parent)
	: QQmlExtensionPlugin(parent)
{
	// Make sure our debug logs are disabled by default
	// to avoid flooding the output. This can be overridden
	// by setting the QT_LOGGING_RULES environment variable.
	// Also see http://doc.qt.io/qt-5/qloggingcategory.html#logging-rules
	QLoggingCategory::setFilterRules(QStringLiteral("qmlbgdata*.debug=true"));
}


void QmlBgDataPlugin::registerTypes(char const *uri)
{
	qmlRegisterType<BGDataReceiver>(uri, 1, 0, "BGDataReceiver");
}
