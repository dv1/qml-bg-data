#ifndef QMLBGDATAPLUGIN_HPP
#define QMLBGDATAPLUGIN_HPP

#include <QQmlExtensionPlugin>


/// Main entrypoint class for the QML plugin
class QmlBgDataPlugin
	: public QQmlExtensionPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
	explicit QmlBgDataPlugin(QObject *parent = nullptr);
	void registerTypes(char const *uri) override;
};


#endif // QMLBGDATAPLUGIN_HPP
