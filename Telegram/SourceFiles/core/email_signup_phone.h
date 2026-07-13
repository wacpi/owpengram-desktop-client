/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace Core {

// Email-as-identity signup mode: reversibly encodes an email address into a
// synthetic "888"-prefixed all-digit phone number, so it can be carried end
// to end through the existing phone-based auth.sendCode/signIn/signUp/
// account.changePhone flow unchanged. Must match the server implementation
// bit for bit: see internal/domain/emailphone.go (EncodeEmailPhone) in the
// gramsrv repo. Algorithm: lowercase+trim the email, treat its UTF-8 bytes as
// a big-endian unsigned integer, print in decimal, prefix with "888".

// Returns an empty string if email is empty or has no '@'.
[[nodiscard]] QString EncodeEmailSignupPhone(const QString &email);

// Reverses EncodeEmailSignupPhone. Returns an empty string if phone does not
// carry the "888" prefix or does not decode to a plausible email address.
[[nodiscard]] QString DecodeEmailSignupPhone(const QString &phone);

// True if phone carries the synthetic "888" prefix (without decoding it).
[[nodiscard]] bool IsEmailSignupPhone(const QString &phone);

} // namespace Core
