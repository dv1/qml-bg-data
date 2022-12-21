#ifndef JSONHELPER_HPP
#define JSONHELPER_HPP

#include <QString>
#include <QJsonArray>
#include <QJsonObject>


namespace detail
{

template <typename T, typename Func>
struct ReadFromJSONHelper;

template <typename Func>
struct ReadFromJSONHelper<int, Func>
{
	void operator()(QJsonObject const &json, QString const &valueName, Func const &func) const
	{
		QJsonValue value = json[valueName];
		if (value.type() == QJsonValue::Double)
			func(int(value.toDouble())); // using this instead of toInt() in case due to rounding errors the value is not a whole number
	}
};

template <typename Func>
struct ReadFromJSONHelper<qint64, Func>
{
	void operator()(QJsonObject const &json, QString const &valueName, Func const &func) const
	{
		QJsonValue value = json[valueName];
		if (value.type() == QJsonValue::Double)
			func(qint64(value.toDouble())); // using this instead of toInt() in case due to rounding errors the value is not a whole number
	}
};

template <typename Func>
struct ReadFromJSONHelper<float, Func>
{
	void operator()(QJsonObject const &json, QString const &valueName, Func const &func) const
	{
		QJsonValue value = json[valueName];
		if (value.type() == QJsonValue::Double)
			func(float(value.toDouble()));
	}
};

template <typename Func>
struct ReadFromJSONHelper<QString, Func>
{
	void operator()(QJsonObject const &json, QString const &valueName, Func const &func) const
	{
		QJsonValue value = json[valueName];
		if (value.type() == QJsonValue::String)
			func(value.toString());
	}
};

template <typename Func>
struct ReadFromJSONHelper<QJsonArray, Func>
{
	void operator()(QJsonObject const &json, QString const &valueName, Func const &func) const
	{
		QJsonValue value = json[valueName];
		if (value.type() == QJsonValue::Array)
			func(value.toArray());
	}
};

template <typename Func>
struct ReadFromJSONHelper<QJsonObject, Func>
{
	void operator()(QJsonObject const &json, QString const &valueName, Func const &func) const
	{
		QJsonValue value = json[valueName];
		if (value.type() == QJsonValue::Object)
			func(value.toObject());
	}
};

} // namespace detail end

template <typename T, typename Func>
void readFromJSON(QJsonObject const &json, QString const &valueName, Func const &func)
{
	detail::ReadFromJSONHelper<T, Func>()(json, valueName, func);
}


#endif // JSONHELPER_HPP
