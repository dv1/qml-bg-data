#ifndef QMLSGVDATAPLUGIN_HPP
#define QMLSGVDATAPLUGIN_HPP

#include <QQmlExtensionPlugin>


/// Main entrypoint class for the QML plugin
class QmlSgvDataPlugin
	: public QQmlExtensionPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
	explicit QmlSgvDataPlugin(QObject *parent = nullptr);
	void registerTypes(char const *uri) override;
};


#endif // QMLSGVDATAPLUGIN_HPP
