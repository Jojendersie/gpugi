#pragma once

#include <cstdint>

/// \file flagoperators.hpp
/// \brief This header adds the operators | ^ ~ & to all enumeration types.
/// \details After including this header all enumeration types have the
///		mentioned operators defined.
///		
///		The operators are guaranteed to be generated for enums only. This
///		is done by generating invalid operators otherwise.

/// \brief This construct guarantees that the operator is defined for
///		enumeration types only.
///	\details The Idea: In case of anything else generate an invalid signature
///		for the operator. Therefore the second argument of each operator is
///		dependent on the fact if the type deduced from the first argument is
///		an enumeration type.
#define ASSERTED_ENUM_TYPE	typename std::conditional<std::is_enum<EnumT>::value, EnumT, struct NoEmumType>::type
/// \brief The INT_TYPE macro determines an integer type which has at least the
///		precision of the enum such that all enum-base types are supported
#define INT_TYPE			typename std::conditional<sizeof(EnumT) == 1, uint_fast8_t, \
									 std::conditional<sizeof(EnumT) == 2, uint_fast16_t, \
									 std::conditional<sizeof(EnumT) == 4, uint_fast32_t, uint_fast64_t>::type>::type>::type

/// \brief Operator to combine flags: Bitwise OR
template<typename EnumT>
inline EnumT operator | (EnumT _lhs, ASSERTED_ENUM_TYPE _rhs)
{
	return static_cast<EnumT>(static_cast<INT_TYPE>(_lhs) | static_cast<INT_TYPE>(_rhs));
}

/// \brief Bitwise XOR for enumeration flags
template<typename EnumT>
inline EnumT operator ^ (EnumT _lhs, ASSERTED_ENUM_TYPE _rhs)
{
	return static_cast<EnumT>(static_cast<INT_TYPE>(_lhs) ^ static_cast<INT_TYPE>(_rhs));
}

/// \brief Bitwise AND for enumeration flags
template<typename EnumT>
inline EnumT operator & (EnumT _lhs, ASSERTED_ENUM_TYPE _rhs)
{
	return static_cast<EnumT>(static_cast<INT_TYPE>(_lhs)& static_cast<INT_TYPE>(_rhs));
}

/// \brief Bitwise NOT for enumeration flags
template<typename EnumT>
inline ASSERTED_ENUM_TYPE operator ~ (EnumT _lhs)
{
	// If you get an error here you are trying to call ~ for a type which does
	// not define the ~ operator. This may happen because an include is missing
	// or there is no such operator. The operator in this header is designed
	// for enum-types only.
	static_assert(std::is_enum<EnumT>::value, "Some type is trying to use the operator ~ but does not define it.");
	return static_cast<ASSERTED_ENUM_TYPE>(~static_cast<INT_TYPE>(_lhs));
}

/// \brief Check if any bit is set.
/// \details It is not possible to implement casting for enums because these
///		operators can only be declared as member.
///		The best work around is a short check function.
/// \return true if != 0
template<typename EnumT>
inline typename std::enable_if<std::is_enum<EnumT>::value, bool>::type any(EnumT _arg)
{
	return static_cast<INT_TYPE>(_arg) != 0;
}

#undef INT_TYPE
#undef ASSERTED_ENUM_TYPE