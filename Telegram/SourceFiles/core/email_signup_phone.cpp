/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "core/email_signup_phone.h"

namespace Core {
namespace {

// Keep in sync with internal/domain.EmailPhonePrefix and the escape table in
// internal/domain/emailphone.go on the server: letters/digits pass through
// unchanged, a handful of punctuation characters become a 2-character
// 'q' + digit escape. This keeps the encoded length close to the email's own
// length and needs no big-integer arithmetic on either side.
const auto kEmailPhonePrefix = QString::fromLatin1("888");
constexpr auto kEmailPhoneEscape = QChar('q');

[[nodiscard]] bool EscapeDigitFor(QChar ch, QChar &digit) {
	switch (ch.unicode()) {
	case '@': digit = QChar('0'); return true;
	case '.': digit = QChar('1'); return true;
	case '-': digit = QChar('2'); return true;
	case '_': digit = QChar('3'); return true;
	case '+': digit = QChar('4'); return true;
	case 'q': digit = QChar('5'); return true;
	}
	return false;
}

[[nodiscard]] bool CharForEscapeDigit(QChar digit, QChar &ch) {
	switch (digit.unicode()) {
	case '0': ch = QChar('@'); return true;
	case '1': ch = QChar('.'); return true;
	case '2': ch = QChar('-'); return true;
	case '3': ch = QChar('_'); return true;
	case '4': ch = QChar('+'); return true;
	case '5': ch = QChar('q'); return true;
	}
	return false;
}

} // namespace

QString EncodeEmailSignupPhone(const QString &email) {
	const auto normalized = email.trimmed().toLower();
	if (normalized.isEmpty() || !normalized.contains(QChar('@'))) {
		return QString();
	}
	auto body = QString();
	body.reserve(normalized.size() * 2);
	for (const auto &ch : normalized) {
		if ((ch >= QChar('a') && ch <= QChar('z') && ch != kEmailPhoneEscape)
			|| (ch >= QChar('0') && ch <= QChar('9'))) {
			body.append(ch);
			continue;
		}
		auto digit = QChar();
		if (!EscapeDigitFor(ch, digit)) {
			return QString();
		}
		body.append(kEmailPhoneEscape);
		body.append(digit);
	}
	return kEmailPhonePrefix + body;
}

QString DecodeEmailSignupPhone(const QString &phone) {
	const auto lower = phone.trimmed().toLower();
	if (!lower.startsWith(kEmailPhonePrefix)) {
		return QString();
	}
	const auto body = lower.mid(kEmailPhonePrefix.size());
	if (body.isEmpty()) {
		return QString();
	}
	auto email = QString();
	email.reserve(body.size());
	for (auto i = 0; i < body.size(); ++i) {
		const auto ch = body.at(i);
		if (ch != kEmailPhoneEscape) {
			email.append(ch);
			continue;
		}
		++i;
		if (i >= body.size()) {
			return QString();
		}
		auto decoded = QChar();
		if (!CharForEscapeDigit(body.at(i), decoded)) {
			return QString();
		}
		email.append(decoded);
	}
	if (email.isEmpty() || !email.contains(QChar('@'))) {
		return QString();
	}
	return email;
}

bool IsEmailSignupPhone(const QString &phone) {
	const auto lower = phone.trimmed().toLower();
	if (!lower.startsWith(kEmailPhonePrefix)) {
		return false;
	}
	for (const auto &ch : lower) {
		if (ch >= QChar('a') && ch <= QChar('z')) {
			return true;
		}
	}
	return false;
}

} // namespace Core
