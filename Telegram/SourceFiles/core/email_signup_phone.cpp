/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "core/email_signup_phone.h"

#include "base/openssl_help.h"

namespace Core {
namespace {

// Keep in sync with internal/domain.EmailPhonePrefix on the server.
const auto kEmailPhonePrefix = QString::fromLatin1("888");

} // namespace

QString EncodeEmailSignupPhone(const QString &email) {
	const auto normalized = email.trimmed().toLower();
	if (normalized.isEmpty() || !normalized.contains(QChar('@'))) {
		return QString();
	}
	const auto utf8 = normalized.toUtf8();
	auto number = openssl::BigNum(bytes::make_span(utf8));
	if (number.failed()) {
		return QString();
	}
	const auto decimal = BN_bn2dec(number.raw());
	if (!decimal) {
		return QString();
	}
	const auto result = kEmailPhonePrefix + QString::fromLatin1(decimal);
	OPENSSL_free(decimal);
	return result;
}

QString DecodeEmailSignupPhone(const QString &phone) {
	if (!phone.startsWith(kEmailPhonePrefix)) {
		return QString();
	}
	const auto digits = phone.mid(kEmailPhonePrefix.size());
	if (digits.isEmpty()) {
		return QString();
	}
	const auto digitsUtf8 = digits.toUtf8();
	BIGNUM *raw = nullptr;
	if (!BN_dec2bn(&raw, digitsUtf8.constData()) || !raw) {
		return QString();
	}
	const auto length = BN_num_bytes(raw);
	auto buffer = QByteArray(length, char(0));
	const auto written = BN_bn2bin(
		raw,
		reinterpret_cast<unsigned char*>(buffer.data()));
	BN_free(raw);
	if (written != length) {
		return QString();
	}
	const auto email = QString::fromUtf8(buffer);
	if (email.isEmpty() || !email.contains(QChar('@'))) {
		return QString();
	}
	return email;
}

bool IsEmailSignupPhone(const QString &phone) {
	return phone.startsWith(kEmailPhonePrefix);
}

} // namespace Core
