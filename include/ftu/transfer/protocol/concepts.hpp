#pragma once

#include <climits>
#include <concepts>
#include <cstdint>
#include <limits>
#include <ranges>
#include <span>
#include <type_traits>

namespace ftu::transfer::protocol
{
	template <class T>
	concept UnsignedInteger = std::unsigned_integral<T> && (!std::same_as<std::remove_cv_t<T>, bool>) &&
							  (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8) &&
							  (std::numeric_limits<T>::digits == sizeof(T) * CHAR_BIT);

	template <class R>
	concept BorrowedBuffer =
		std::ranges::contiguous_range<R> && std::ranges::sized_range<R> && std::ranges::borrowed_range<R> &&
		std::same_as<std::ranges::range_value_t<R>, std::uint8_t> &&
		std::constructible_from<std::span<const std::uint8_t>, R>;

} // namespace ftu::transfer::protocol
