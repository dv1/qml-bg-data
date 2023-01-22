#ifndef JSONHELPER_HPP
#define JSONHELPER_HPP

#include <QDebug>
#include <QLoggingCategory>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <stdexcept>
#include <cmath>


Q_DECLARE_LOGGING_CATEGORY(lcQmlBgData)


namespace detail
{

inline QString qjsonTypeToString(QJsonValue::Type type)
{
	switch (type)
	{
		case QJsonValue::Null: return "null";
		case QJsonValue::Bool: return "bool";
		case QJsonValue::Double: return "double";
		case QJsonValue::String: return "string";
		case QJsonValue::Array: return "array";
		case QJsonValue::Object: return "object";
		case QJsonValue::Undefined: return "undefined";
		default: return QString("<unknown/invalid type %1>").arg(int(type));
	}
}

} // namespace detail end


class JSONError
	: public std::runtime_error
{
public:
	explicit JSONError(QString what)
		: std::runtime_error(what.toUtf8().constData())
	{
	}
};

class MissingJSONValueError
	: public JSONError
{
public:
	explicit MissingJSONValueError(QString valueName)
		: JSONError(
			QString("Value %1 is missing")
				.arg(valueName)
		)
	{
	}
};

class InvalidJSONTypeError
	: public JSONError
{
public:
	explicit InvalidJSONTypeError(QString valueName, QJsonValue::Type expected, QJsonValue::Type actual)
		: JSONError(
			QString("Expected value %1 to be of JSON type %1, is actually of type %2")
				.arg(valueName)
				.arg(detail::qjsonTypeToString(expected))
				.arg(detail::qjsonTypeToString(actual))
		)
	{
	}
};


namespace detail
{

template <typename Func, typename ConvFunc>
void readFromJSONHelperImpl(QJsonObject const &json, bool required, QJsonValue::Type expectedType, QString const &valueName, Func const &func, ConvFunc const &convFunc)
{
	if (!json.contains(valueName))
	{
		if (required)
			throw MissingJSONValueError(valueName);
		else
			return;
	}

	QJsonValue value = json[valueName];
	if (value.type() == expectedType)
	{
		auto convValue = convFunc(value);
		func(convValue);
	}
	else
		throw InvalidJSONTypeError(valueName, expectedType, value.type());
}

template <typename T, typename Func>
struct ReadFromJSONHelper;

// using toDouble() instead of toInt() for int and qin64. JSON does not
// have dedicated integers, just "numbers", which are interpreted as
// doubles. If during the parsing the number ends up not being perfectly
// integer (for example, something being parsed as 1000555.99999), then
// toInt() might end up returning its default value. We do not want that;
// instead, in such cases, we want the result to be rounded.

template <typename Func>
struct ReadFromJSONHelper<int, Func>
{
	void operator()(QJsonObject const &json, bool required, QString const &valueName, Func const &func) const
	{
		readFromJSONHelperImpl<Func>(json, required, QJsonValue::Double, valueName, func, [](auto value) { return int(std::round(value.toDouble())); });
	}
};

template <typename Func>
struct ReadFromJSONHelper<qint64, Func>
{
	void operator()(QJsonObject const &json, bool required, QString const &valueName, Func const &func) const
	{
		readFromJSONHelperImpl<Func>(json, required, QJsonValue::Double, valueName, func, [](auto value) { return qint64(std::round(value.toDouble())); });
	}
};

template <typename Func>
struct ReadFromJSONHelper<float, Func>
{
	void operator()(QJsonObject const &json, bool required, QString const &valueName, Func const &func) const
	{
		readFromJSONHelperImpl<Func>(json, required, QJsonValue::Double, valueName, func, [](auto value) { return float(value.toDouble()); });
	}
};

template <typename Func>
struct ReadFromJSONHelper<bool, Func>
{
	void operator()(QJsonObject const &json, bool required, QString const &valueName, Func const &func) const
	{
		readFromJSONHelperImpl<Func>(json, required, QJsonValue::Bool, valueName, func, [](auto value) { return value.toBool(); });
	}
};

template <typename Func>
struct ReadFromJSONHelper<QString, Func>
{
	void operator()(QJsonObject const &json, bool required, QString const &valueName, Func const &func) const
	{
		readFromJSONHelperImpl<Func>(json, required, QJsonValue::String, valueName, func, [](auto value) { return value.toString(); });
	}
};

template <typename Func>
struct ReadFromJSONHelper<QJsonArray, Func>
{
	void operator()(QJsonObject const &json, bool required, QString const &valueName, Func const &func) const
	{
		readFromJSONHelperImpl<Func>(json, required, QJsonValue::Array, valueName, func, [](auto value) { return value.toArray(); });
	}
};

template <typename Func>
struct ReadFromJSONHelper<QJsonObject, Func>
{
	void operator()(QJsonObject const &json, bool required, QString const &valueName, Func const &func) const
	{
		readFromJSONHelperImpl<Func>(json, required, QJsonValue::Object, valueName, func, [](auto value) { return value.toObject(); });
	}
};

} // namespace detail end

template <typename T, typename Func>
void parse(QJsonObject const &json, bool required, QString const &valueName, Func const &func)
{
	detail::ReadFromJSONHelper<T, Func>()(json, required, valueName, func);
}


#endif // JSONHELPER_HPP
