/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "intro/intro_step.h"

namespace Ui {
class InputField;
} // namespace Ui

namespace Intro {
namespace details {

// Shown instead of PhoneWidget when the server advertises
// email_signup_enabled=true in help.getAppConfig: the user types an email
// address instead of a phone number. The email is locally encoded into a
// synthetic "888"-prefixed phone number (Core::EncodeEmailSignupPhone) and
// fed into the exact same auth.sendCode call PhoneWidget makes, so the rest
// of the intro flow (CodeWidget, SignupWidget, ...) needs no changes at all.
class EmailSignupWidget final : public Step {
public:
	EmailSignupWidget(
		QWidget *parent,
		not_null<Main::Account*> account,
		not_null<Data*> data);

	void setInnerFocus() override;
	void activate() override;
	void finished() override;
	void cancelled() override;
	void submit() override;

	bool hasBack() const override {
		return true;
	}

protected:
	void resizeEvent(QResizeEvent *e) override;

private:
	void sendCodeDone(const MTPauth_SentCode &result);
	void sendCodeFail(const MTP::Error &error);
	void showEmailError(rpl::producer<QString> text);
	void hideEmailError();
	void setupPhoneFallbackLink();
	void confirmPhoneFallback();

	object_ptr<Ui::InputField> _email;

	QString _sentPhone;
	mtpRequestId _sentRequest = 0;

};

} // namespace details
} // namespace Intro
