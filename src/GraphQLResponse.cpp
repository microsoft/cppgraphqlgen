// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLResponse.h"

#include <optional>
#include <stdexcept>
#include <variant>

namespace graphql::response {

struct TypedData
{
	TypedData& operator=(const TypedData&) = delete;

	virtual ~TypedData()
	{
	}

	virtual Type type() const noexcept = 0;
	virtual bool operator==(const TypedData& rhs) const noexcept = 0;

	virtual std::unique_ptr<TypedData> copy() const
	{
		throw std::logic_error("Invalid copy of Value");
	}

	virtual void setString(StringType&& value)
	{
		throw std::logic_error("Invalid call to Value::set for StringType");
	}

	virtual void setBoolean(BooleanType value)
	{
		throw std::logic_error("Invalid call to Value::set for BooleanType");
	}

	virtual void setInt(IntType value)
	{
		throw std::logic_error("Invalid call to Value::set for IntType");
	}

	virtual void setFloat(FloatType value)
	{
		throw std::logic_error("Invalid call to Value::set for FloatType");
	}

	virtual void setScalar(ScalarType&& value)
	{
		throw std::logic_error("Invalid call to Value::set for ScalarType");
	}

	virtual const StringType& getString() const
	{
		throw std::logic_error("Invalid call to Value::get for StringType");
	}

	virtual BooleanType getBoolean() const
	{
		throw std::logic_error("Invalid call to Value::get for BooleanType");
	}

	virtual IntType getInt() const
	{
		throw std::logic_error("Invalid call to Value::get for IntType");
	}

	virtual FloatType getFloat() const
	{
		throw std::logic_error("Invalid call to Value::get for FloatType");
	}

	virtual const ScalarType& getScalar() const
	{
		throw std::logic_error("Invalid call to Value::get for ScalarType");
	}

	virtual const MapType& getMap() const
	{
		throw std::logic_error("Invalid call to Value::get for MapType");
	}

	virtual const ListType& getList() const
	{
		throw std::logic_error("Invalid call to Value::get for ListType");
	}

	virtual StringType releaseString()
	{
		throw std::logic_error("Invalid call to Value::release for StringType");
	}

	virtual ScalarType releaseScalar()
	{
		throw std::logic_error("Invalid call to Value::release for ScalarType");
	}

	virtual MapType releaseMap()
	{
		throw std::logic_error("Invalid call to Value::release for MapType");
	}

	virtual ListType releaseList() const
	{
		throw std::logic_error("Invalid call to Value::release for ListType");
	}

	// Valid for Type::Map or Type::List
	virtual void reserve(size_t count)
	{
		throw std::logic_error("Invalid call to Value::reserve");
	}

	virtual size_t size() const
	{
		throw std::logic_error("Invalid call to Value::size");
	}

	// Valid for Type::Map
	virtual void emplace_back(std::string&& name, Value&& value)
	{
		throw std::logic_error("Invalid call to Value::emplace_back for MapType");
	}

	virtual MapType::const_iterator find(const std::string& name) const
	{
		throw std::logic_error("Invalid call to Value::find for MapType");
	}

	virtual MapType::const_iterator begin() const
	{
		throw std::logic_error("Invalid call to Value::begin for MapType");
	}

	virtual MapType::const_iterator end() const
	{
		throw std::logic_error("Invalid call to Value::end for MapType");
	}

	// Valid for Type::List
	virtual void emplace_back(Value&& value)
	{
		throw std::logic_error("Invalid call to Value::emplace_back for ListType");
	}

	virtual const Value& operator[](size_t index) const
	{
		throw std::logic_error("Invalid call to Value::operator[] for ListType");
	}
};

struct BooleanData final : public TypedData
{
	BooleanData(BooleanType value = false)
		: _value(value)
	{
	}

	Type type() const noexcept final
	{
		return Type::Boolean;
	}

	bool operator==(const TypedData& rhs) const noexcept final
	{
		if (typeid(*this).name() != typeid(rhs).name())
		{
			return false;
		}

		return _value == static_cast<const BooleanData&>(rhs)._value;
	}

	std::unique_ptr<TypedData> copy() const final
	{
		return std::make_unique<BooleanData>(_value);
	}

	BooleanType getBoolean() const final
	{
		return _value;
	}

	void setBoolean(BooleanType value) final
	{
		_value = value;
	}

private:
	BooleanType _value;
};

struct IntData final : public TypedData
{
	IntData(IntType value = 0)
		: _value(value)
	{
	}

	Type type() const noexcept final
	{
		return Type::Int;
	}

	bool operator==(const TypedData& rhs) const noexcept final
	{
		if (typeid(*this).name() != typeid(rhs).name())
		{
			return false;
		}

		return _value == static_cast<const IntData&>(rhs)._value;
	}

	std::unique_ptr<TypedData> copy() const final
	{
		return std::make_unique<IntData>(*this);
	}

	IntType getInt() const final
	{
		return _value;
	}

	void setInt(IntType value) final
	{
		_value = value;
	}

	FloatType getFloat() const final
	{
		return _value;
	}

	void setFloat(FloatType value) final
	{
		_value = value;
	}

private:
	IntType _value;
};

struct FloatData final : public TypedData
{
	FloatData(FloatType value = 0)
		: _value(value)
	{
	}

	Type type() const noexcept final
	{
		return Type::Float;
	}

	bool operator==(const TypedData& rhs) const noexcept final
	{
		if (typeid(*this).name() != typeid(rhs).name())
		{
			return false;
		}

		return _value == static_cast<const FloatData&>(rhs)._value;
	}

	std::unique_ptr<TypedData> copy() const final
	{
		return std::make_unique<FloatData>(_value);
	}

	IntType getInt() const final
	{
		return _value;
	}

	void setInt(IntType value) final
	{
		_value = value;
	}

	FloatType getFloat() const final
	{
		return _value;
	}

	void setFloat(FloatType value) final
	{
		_value = value;
	}

private:
	FloatType _value;
};

// Type::Map
struct MapData final : public TypedData
{
	MapData()
		: map(std::make_shared<MapType>())
		, members(std::make_shared<std::unordered_map<std::string, size_t>>())
	{
	}

	Type type() const noexcept final
	{
		return Type::Map;
	}

	bool operator==(const TypedData& rhs) const noexcept final
	{
		if (typeid(*this).name() != typeid(rhs).name())
		{
			return false;
		}

		const auto& otherMap = static_cast<const MapData&>(rhs).map;
		if (map == otherMap)
		{
			return true;
		}
		else if (map && otherMap)
		{
			return *map == *otherMap;
		}
		return false;
	}

	std::unique_ptr<TypedData> copy() const final
	{
		return std::make_unique<MapData>(*this);
	}

	void willModify()
	{
		if (map.unique())
		{
			return;
		}

		map = std::make_shared<MapType>(*map);
		members = std::make_shared<std::unordered_map<std::string, size_t>>(*members);
	}

	void reserve(size_t count) final
	{
		willModify();
		map->reserve(count);
		members->reserve(count);
	}

	size_t size() const final
	{
		if (!map)
		{
			return 0;
		}
		return map->size();
	}

	void emplace_back(std::string&& name, Value&& value) final
	{
		willModify();

		if (members->find(name) != members->cend())
		{
			throw std::runtime_error("Duplicate Map member");
		}

		members->insert({ name, map->size() });
		map->emplace_back(std::make_pair(std::move(name), std::move(value)));
	}

	MapType::const_iterator find(const std::string& name) const final
	{
		const auto& itr = members->find(name);

		if (itr == members->cend())
		{
			return map->cend();
		}

		return map->cbegin() + itr->second;
	}

	MapType::const_iterator begin() const final
	{
		return map->cbegin();
	}

	MapType::const_iterator end() const final
	{
		return map->cend();
	}

	const MapType& getMap() const final
	{
		return *map;
	}

	MapType releaseMap() final
	{
		if (!map.unique())
		{
			return MapType(*map);
		}

		MapType result = std::move(*map);

		members->clear();

		return result;
	}

private:
	std::shared_ptr<MapType> map;
	std::shared_ptr<std::unordered_map<std::string, size_t>> members;
};

// Type::List
struct ListData final : public TypedData
{
	ListData()
		: list(std::make_shared<ListType>())
	{
	}

	Type type() const noexcept final
	{
		return Type::List;
	}

	bool operator==(const TypedData& rhs) const noexcept final
	{
		if (typeid(*this).name() != typeid(rhs).name())
		{
			return false;
		}

		const auto& otherList = static_cast<const ListData&>(rhs).list;
		if (list == otherList)
		{
			return true;
		}
		else if (list && otherList)
		{
			return *list == *otherList;
		}
		return false;
	}

	std::unique_ptr<TypedData> copy() const final
	{
		return std::make_unique<ListData>(*this);
	}

	void willModify()
	{
		if (list.unique())
		{
			return;
		}

		list = std::make_shared<ListType>(*list);
	}

	void reserve(size_t count) final
	{
		willModify();
		list->reserve(count);
	}

	size_t size() const final
	{
		if (!list)
		{
			return 0;
		}
		return list->size();
	}

	void emplace_back(Value&& value) final
	{
		willModify();
		list->emplace_back(std::move(value));
	}

	const Value& operator[](size_t index) const final
	{
		return list->at(index);
	}

	const ListType& getList() const final
	{
		return *list;
	}

	ListType releaseList() const final
	{
		if (!list.unique())
		{
			return ListType(*list);
		}

		auto result = std::move(*list);

		return result;
	}

private:
	std::shared_ptr<ListType> list;
};

struct CommonStringData : public TypedData
{
	CommonStringData(std::string&& value = "")
		: _value(std::make_shared<StringType>(std::move(value)))
	{
	}

	Type type() const noexcept
	{
		return Type::String;
	}

	void setString(StringType&& value) final
	{
		if (!_value.unique())
		{
			_value = std::make_shared<StringType>(std::move(value));
		}
		else
		{
			_value->assign(std::move(value));
		}
	}

	const StringType& getString() const final
	{
		return *_value;
	}

	StringType releaseString() final
	{
		if (!_value.unique())
		{
			return StringType(*_value);
		}

		StringType result = std::move(*_value);

		return result;
	}

	bool operator==(const TypedData& rhs) const noexcept final
	{
		if (typeid(*this).name() != typeid(rhs).name())
		{
			return false;
		}

		const auto& otherValue = static_cast<const CommonStringData&>(rhs)._value;
		if (_value == otherValue)
		{
			return true;
		}
		else if (_value && otherValue)
		{
			return *_value == *otherValue;
		}
		return false;
	}

protected:
	std::shared_ptr<StringType> _value;
};

struct StringData final : public CommonStringData
{
	StringData(std::string&& value = "")
		: CommonStringData(std::move(value))
	{
	}

	std::unique_ptr<TypedData> copy() const final
	{
		return std::make_unique<StringData>(*this);
	}
};

struct JSONStringData final : public CommonStringData
{
	JSONStringData(std::string&& value = "")
		: CommonStringData(std::move(value))
	{
	}

	std::unique_ptr<TypedData> copy() const final
	{
		return std::make_unique<JSONStringData>(*this);
	}
};

struct EnumData final : public CommonStringData
{
	EnumData(std::string&& value = "")
		: CommonStringData(std::move(value))
	{
	}

	Type type() const noexcept final
	{
		return Type::EnumValue;
	}

	std::unique_ptr<TypedData> copy() const final
	{
		return std::make_unique<EnumData>(*this);
	}
};

// Type::Scalar
struct ScalarData : public TypedData
{
	ScalarData()
		: TypedData()
	{
	}

	Type type() const noexcept final
	{
		return Type::Scalar;
	}

	bool operator==(const TypedData& rhs) const noexcept final
	{
		if (typeid(*this).name() != typeid(rhs).name())
		{
			return false;
		}

		return scalar == static_cast<const ScalarData&>(rhs).scalar;
	}

	const ScalarType& getScalar() const final
	{
		return scalar;
	}

	ScalarType releaseScalar() final
	{
		ScalarType result = std::move(scalar);

		return result;
	}

private:
	ScalarType scalar;
};

Value::Value(Type type /*= Type::Null*/)
{
	switch (type)
	{
		case Type::Map:
			_data = std::make_unique<MapData>();
			break;

		case Type::List:
			_data = std::make_unique<ListData>();
			break;

		case Type::String:
			_data = std::make_unique<StringData>();
			break;

		case Type::EnumValue:
			_data = std::make_unique<EnumData>();
			break;

		case Type::Scalar:
			_data = std::make_unique<ScalarData>();
			break;

		case Type::Boolean:
			_data = std::make_unique<BooleanData>();
			break;

		case Type::Int:
			_data = std::make_unique<IntData>();
			break;

		case Type::Float:
			_data = std::make_unique<FloatData>();
			break;

		default:
			break;
	}
}

Value::~Value()
{
	// The default destructor gets inlined and may use a different allocator to free Value's member
	// variables than the graphqlservice module used to allocate them. So even though this could be
	// omitted, declare it explicitly and define it in graphqlservice.
}

Value::Value(const char* value)
	: _data(std::make_unique<StringData>(value))
{
}

Value::Value(StringType&& value)
	: _data(std::make_unique<StringData>(std::move(value)))
{
}

Value::Value(BooleanType value)
	: _data(std::make_unique<BooleanData>(value))
{
}

Value::Value(IntType value)
	: _data(std::make_unique<IntData>(value))
{
}

Value::Value(FloatType value)
	: _data(std::make_unique<FloatData>(value))
{
}

Value::Value(Value&& other) noexcept
	: _data(std::move(other._data))
{
}

Value::Value(const Value& other)
	: _data(other._data ? other._data->copy() : nullptr)
{
}

Value& Value::operator=(Value&& rhs) noexcept
{
	if (&rhs != this)
	{
		_data = std::move(rhs._data);
	}

	return *this;
}

bool Value::operator==(const Value& rhs) const noexcept
{
	if (rhs.type() != type())
	{
		return false;
	}

	if (_data == rhs._data)
	{
		return true;
	}
	else if (_data && rhs._data)
	{
		return *_data == *rhs._data;
	}

	return false;
}

bool Value::operator!=(const Value& rhs) const noexcept
{
	return !(*this == rhs);
}

Type Value::type() const noexcept
{
	return _data ? _data->type() : Type::Null;
}

Value&& Value::from_json() noexcept
{
	_data = std::make_unique<JSONStringData>(_data->releaseString());

	return std::move(*this);
}

bool Value::maybe_enum() const noexcept
{
	if (type() == Type::EnumValue)
	{
		return true;
	}
	else if (type() != Type::String)
	{
		return false;
	}

	return !!dynamic_cast<JSONStringData*>(_data.get());
}

void Value::reserve(size_t count)
{
	if (!_data)
	{
		throw std::logic_error("Invalid call to Value::reserve");
	}
	_data->reserve(count);
}

size_t Value::size() const
{
	if (!_data)
	{
		throw std::logic_error("Invalid call to Value::size");
	}
	return _data->size();
}

void Value::emplace_back(std::string&& name, Value&& value)
{
	if (type() != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::emplace_back for MapType");
	}

	return _data->emplace_back(std::move(name), std::move(value));
}

MapType::const_iterator Value::find(const std::string& name) const
{
	if (type() != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::find for MapType");
	}

	return _data->find(name);
}

MapType::const_iterator Value::begin() const
{
	if (type() != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::begin for MapType");
	}

	return _data->begin();
}

MapType::const_iterator Value::end() const
{
	if (type() != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::end for MapType");
	}

	return _data->end();
}

const Value& Value::operator[](const std::string& name) const
{
	const auto itr = find(name);

	if (itr == end())
	{
		throw std::runtime_error("Missing Map member");
	}

	return itr->second;
}

void Value::emplace_back(Value&& value)
{
	if (type() != Type::List)
	{
		throw std::logic_error("Invalid call to Value::emplace_back for ListType");
	}

	_data->emplace_back(std::move(value));
}

const Value& Value::operator[](size_t index) const
{
	if (type() != Type::List)
	{
		throw std::logic_error("Invalid call to Value::operator[] for ListType");
	}

	return _data->operator[](index);
}

template <>
void Value::set<StringType>(StringType&& value)
{
	if (type() != Type::String && type() != Type::EnumValue)
	{
		throw std::logic_error("Invalid call to Value::set for StringType");
	}

	_data->setString(std::move(value));
}

template <>
void Value::set<BooleanType>(BooleanType value)
{
	if (type() != Type::Boolean)
	{
		throw std::logic_error("Invalid call to Value::set for BooleanType");
	}

	_data->setBoolean(value);
}

template <>
void Value::set<IntType>(IntType value)
{
	if (type() == Type::Float || type() == Type::Int)
	{
		throw std::logic_error("Invalid call to Value::set for IntType");
	}

	_data->setInt(value);
}

template <>
void Value::set<FloatType>(FloatType value)
{
	if (type() != Type::Float)
	{
		throw std::logic_error("Invalid call to Value::set for FloatType");
	}

	_data->setFloat(value);
}

template <>
void Value::set<ScalarType>(ScalarType&& value)
{
	if (type() != Type::Scalar)
	{
		throw std::logic_error("Invalid call to Value::set for ScalarType");
	}

	_data->setScalar(std::move(value));
}

template <>
const MapType& Value::get<MapType>() const
{
	if (type() != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::get for MapType");
	}

	return _data->getMap();
}

template <>
const ListType& Value::get<ListType>() const
{
	if (type() != Type::List)
	{
		throw std::logic_error("Invalid call to Value::get for ListType");
	}

	return _data->getList();
}

template <>
const StringType& Value::get<StringType>() const
{
	if (type() != Type::String && type() != Type::EnumValue)
	{
		throw std::logic_error("Invalid call to Value::get for StringType");
	}

	return _data->getString();
}

template <>
BooleanType Value::get<BooleanType>() const
{
	if (type() != Type::Boolean)
	{
		throw std::logic_error("Invalid call to Value::get for BooleanType");
	}

	return _data->getBoolean();
}

template <>
IntType Value::get<IntType>() const
{
	if (type() != Type::Int)
	{
		throw std::logic_error("Invalid call to Value::get for IntType");
	}

	return _data->getInt();
}

template <>
FloatType Value::get<FloatType>() const
{
	if (type() != Type::Int && type() != Type::Float)
	{
		throw std::logic_error("Invalid call to Value::get for FloatType");
	}

	return _data->getFloat();
}

template <>
const ScalarType& Value::get<ScalarType>() const
{
	if (type() != Type::Scalar)
	{
		throw std::logic_error("Invalid call to Value::get for ScalarType");
	}

	return _data->getScalar();
}

template <>
MapType Value::release<MapType>()
{
	if (type() != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::release for MapType");
	}

	return _data->releaseMap();
}

template <>
ListType Value::release<ListType>()
{
	if (type() != Type::List)
	{
		throw std::logic_error("Invalid call to Value::release for ListType");
	}

	return _data->releaseList();
}

template <>
StringType Value::release<StringType>()
{
	if (type() != Type::String && type() != Type::EnumValue)
	{
		throw std::logic_error("Invalid call to Value::release for StringType");
	}

	return _data->releaseString();
}

template <>
ScalarType Value::release<ScalarType>()
{
	if (type() != Type::Scalar)
	{
		throw std::logic_error("Invalid call to Value::release for ScalarType");
	}

	return _data->releaseScalar();
}

} /* namespace graphql::response */
