// Based on https://gist.github.com/tibordp/6909880 
// Great tutorial at http://www.ojdip.net/2013/10/implementing-a-variant-type-in-cpp/
// Extensions/Changes: Casting, value Constructors, no invalid state, better set method, better memory (aligned_union)


#include <iostream>
#include <utility>
#include <typeinfo>
#include <type_traits>
#include <string>
#include <algorithm>

// A stat max template
template <size_t arg1, size_t ... others>
struct static_max;

template <size_t arg>
struct static_max < arg >
{
	static const size_t value = arg;
};

template <size_t arg1, size_t arg2, size_t ... others>
struct static_max < arg1, arg2, others... >
{
	static const size_t value = arg1 >= arg2 ? static_max<arg1, others...>::value :
		static_max<arg2, others...>::value;
};

// Check if a given type T is contained within the following types.
template<typename T, typename... Rest>
struct is_any : std::false_type {};

template<typename T, typename First>
struct is_any<T, First> : std::is_same < T, First > {};

template<typename T, typename First, typename... Rest>
struct is_any<T, First, Rest...> : std::integral_constant < bool, std::is_same<T, First>::value || is_any<T, Rest...>::value > {};



namespace Details
{
	template<typename... Ts>
	struct VariantHelper;

	template<typename F, typename... Ts>
	struct VariantHelper < F, Ts... >
	{
		inline static void Destroy(size_t id, void * data)
		{
			if (id == typeid(F).hash_code())
				reinterpret_cast<F*>(data)->~F();
			else
				VariantHelper<Ts...>::Destroy(id, data);
		}

		inline static void Move(size_t _oldType, void * old_v, void * new_v)
		{
			if (_oldType == typeid(F).hash_code())
				new (new_v)F(std::move(*reinterpret_cast<F*>(old_v)));
			else
				VariantHelper<Ts...>::Move(_oldType, old_v, new_v);
		}

		inline static void Copy(size_t _oldType, const void * old_v, void * new_v)
		{
			if (_oldType == typeid(F).hash_code())
				new (new_v)F(*reinterpret_cast<const F*>(old_v));
			else
				VariantHelper<Ts...>::Copy(_oldType, old_v, new_v);
		}

		template<typename TargetType>
		inline static TargetType Cast(size_t _type, const void* _data)
		{
			if (_type == typeid(F).hash_code())
				return std::is_same<F, TargetType>::value ? *reinterpret_cast<const TargetType*>(_data) : VariantCast<F, TargetType>::Cast(*reinterpret_cast<const F*>(_data));
			else
				return VariantHelper<Ts...>::Cast<TargetType>(_type, _data);
		}
	};

	template<> struct VariantHelper < >
	{
		inline static void Destroy(size_t id, void * data) { }
		inline static void Move(size_t _oldType, void * old_v, void * new_v) { }
		inline static void Copy(size_t _oldType, const void * old_v, void * new_v) { }

		template<typename TargetType>
		inline static TargetType Cast(size_t _type, const void* _data) { throw std::bad_cast(); }
	};


	// Defines casting behavior of Variants.
	template<typename From, typename To>
	struct VariantCast
	{
		static To Cast(const From& from) { return static_cast<To>(from); }
	};

	// To string specialization.
	template<typename From> struct VariantCast<From, std::string>
	{ static std::string Cast(const From& from) { return std::to_string(from); } };
	template<> struct VariantCast<std::string, std::string>
	{ static std::string Cast(const std::string& from) { return from; } };

	// To bool specializations.
	template<typename From> struct VariantCast<From, bool>
	{ static bool Cast(const From& from) { return from > 0; } };
	template<> struct VariantCast<bool, bool>
	{ static bool Cast(const bool& from) { return from; } };

	// From string specializations.
	template<> struct VariantCast<std::string, unsigned int>
	{ static unsigned int Cast(const std::string& from) { return static_cast<unsigned int>(std::stoi(from)); } };
	template<> struct VariantCast<std::string, int>
	{ static int Cast(const std::string& from) { return std::stoi(from); } };
	template<> struct VariantCast<std::string, float>
	{ static float Cast(const std::string& from) { return std::stof(from); } };
	template<> struct VariantCast<std::string, double>
	{ static double Cast(const std::string& from) { return std::stod(from); } };
	template<> struct VariantCast<std::string, bool>
	{
		static bool Cast(const std::string& from)
		{
			std::string str = from;
			std::remove_if(str.begin(), str.end(), ::isspace);
			std::transform(str.begin(), str.end(), str.begin(), ::tolower);
			return str == "true" || VariantCast<std::string, int>::Cast(str) > 0;
		}
	};
}

template<typename... Ts>
struct Variant
{
public:
/*	template<typename T>
	Variant(T&& _value) : m_typeId(typeid(T).hash_code())
	{
		static_assert(is_any<T, Ts...>::value, "Given type is not a valid initial value for this variant type!");
		new (&m_data) T(std::forward<T>(_value));
	} */
	template<typename T>
	Variant(const T& _value) : m_typeId(typeid(T).hash_code())
	{
		static_assert(is_any<T, Ts...>::value, "Given type is not a valid initial value for this variant type!");
		new (&m_data) T(_value);
	}

	Variant(const Variant<Ts...>& old) : m_typeId(old.m_typeId)
	{
		Helper::Copy(old.m_typeId, &old.m_data, &m_data);
	}

	Variant(Variant<Ts...>&& old) : m_typeId(old.m_typeId)
	{
		Helper::Move(old.m_typeId, &old.m_data, &m_data);
	}

	// Serves as both the Move and the Copy assignment operator.
	Variant<Ts...>& operator= (Variant<Ts...> old)
	{
		std::swap(m_typeId, old.m_typeId);
		std::swap(m_data, old.m_data);

		return *this;
	}

	template<typename T>
	void Is()
	{
		return (m_typeId == typeid(T).hash_code());
	}

	/// Sets a new value. May change the internal value type.
	template<typename T>
	void Set(T&& newValue)
	{
		static_assert(is_any<T, Ts...>(), "Given type is not a valid value for this variant type!");

		// First we Destroy the current contents
		Helper::Destroy(m_typeId, &m_data);
		new (&m_data) T(std::forward<T>(newValue));
		m_typeId = typeid(T).hash_code();
	}
	/// Sets a new value. May change the internal value type.
	template<typename T>
	void Set(const T& newValue)
	{
		static_assert(is_any<T, Ts...>(), "Given type is not a valid value for this variant type!");

		// First we Destroy the current contents
		Helper::Destroy(m_typeId, &m_data);
		new (&m_data) T(newValue);
		m_typeId = typeid(T).hash_code();
	}

	/// Returns a casted copy.
	template<typename TargetType>
	TargetType As() const
	{
		if (m_typeId == typeid(TargetType).hash_code())
			return *reinterpret_cast<const TargetType*>(&m_data);
		else
			return Helper::Cast<TargetType>(m_typeId, &m_data);
	}

	/// Returns a reference to the inner type.
	template<typename NativeType>
	NativeType& Get()
	{
		if (m_typeId == typeid(NativeType).hash_code())
			return *reinterpret_cast<NativeType*>(&m_data);
		else
			throw std::bad_cast();
	}

	~Variant()
	{
		Helper::Destroy(m_typeId, &m_data);
	}

private:
	using data_t = typename std::aligned_union<sizeof(unsigned int), Ts...>::type;

	using Helper = Details::VariantHelper < Ts... > ;

	size_t m_typeId;
	data_t m_data;
};