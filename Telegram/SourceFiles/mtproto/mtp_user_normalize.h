/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "scheme.h"

namespace MTP {

[[nodiscard]] MTPUser NormalizeUser(const MTPUser &user);

template <typename FilledHandler, typename EmptyHandler>
decltype(auto) MatchNormalizedUser(
		const MTPUser &user,
		FilledHandler &&filled,
		EmptyHandler &&empty) {
	return NormalizeUser(user).match(
		std::forward<FilledHandler>(filled),
		std::forward<EmptyHandler>(empty),
		[](const MTPDuser_layer216 &) -> decltype(auto) {
			Unexpected("user_layer216 after NormalizeUser.");
		});
}

} // namespace MTP
