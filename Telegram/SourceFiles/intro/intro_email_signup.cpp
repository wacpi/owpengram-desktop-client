/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "intro/intro_email_signup.h"

#include "config.h"
#include "core/email_signup_phone.h"
#include "intro/intro_code.h"
#include "intro/intro_widget.h"
#include "lang/lang_keys.h"
#include "ui/boxes/confirm_box.h"
#include "ui/widgets/fields/input_field.h"
#include "boxes/abstract_box.h"
#include "styles/style_intro.h"

namespace Intro {
namespace details {

EmailSignupWidget::EmailSignupWidget(
	QWidget *parent,
	not_null<Main::Account*> account,
	not_null<Data*> data)
: Step(parent, account, data)
, _email(
	this,
	st::introName,
	tr::lng_settings_cloud_login_email_placeholder()) {
	setTitleText(tr::lng_intro_email_setup_title());
	setDescriptionText(tr::lng_settings_cloud_login_email_about());
	setErrorCentered(true);

	_email->setText(getData()->email);
	_email->changes() | rpl::on_next([=] {
		hideEmailError();
	}, _email->lifetime());
	_email->submits() | rpl::on_next([=] {
		submit();
	}, _email->lifetime());
}

void EmailSignupWidget::resizeEvent(QResizeEvent *e) {
	Step::resizeEvent(e);
	_email->moveToLeft(contentLeft(), contentTop() + st::introStepFieldTop);
}

void EmailSignupWidget::showEmailError(rpl::producer<QString> text) {
	_email->showError();
	showError(std::move(text));
}

void EmailSignupWidget::hideEmailError() {
	hideError();
}

void EmailSignupWidget::setInnerFocus() {
	_email->setFocusFast();
}

void EmailSignupWidget::activate() {
	Step::activate();
	_email->show();
	setInnerFocus();
}

void EmailSignupWidget::finished() {
	Step::finished();
	apiClear();
	cancelled();
}

void EmailSignupWidget::cancelled() {
	api().request(base::take(_sentRequest)).cancel();
}

void EmailSignupWidget::submit() {
	if (_sentRequest) {
		return;
	}
	const auto email = _email->getLastText().trimmed();
	const auto phone = Core::EncodeEmailSignupPhone(email);
	if (phone.isEmpty()) {
		showEmailError(tr::lng_cloud_password_bad_email());
		_email->setFocus();
		return;
	}
	hideEmailError();

	getData()->email = email;
	_sentPhone = phone;
	_sentRequest = api().request(MTPauth_SendCode(
		MTP_string(_sentPhone),
		MTP_int(ApiId),
		MTP_string(ApiHash),
		MTP_codeSettings(
			MTP_flags(0),
			MTPVector<MTPbytes>(),
			MTPstring(),
			MTPBool())
	)).done([=](const MTPauth_SentCode &result) {
		sendCodeDone(result);
	}).fail([=](const MTP::Error &error) {
		sendCodeFail(error);
	}).handleFloodErrors().send();
}

void EmailSignupWidget::sendCodeDone(const MTPauth_SentCode &result) {
	_sentRequest = 0;
	result.match([&](const MTPDauth_sentCode &data) {
		fillSentCodeData(data);
		getData()->phone = _sentPhone;
		getData()->phoneHash = qba(data.vphone_code_hash());
		getData()->callStatus = CallStatus::Disabled;
		getData()->callTimeout = 0;
		if (getData()->emailStatus == EmailStatus::SetupRequired) {
			// The server did not recognize our synthetic phone as an
			// email-signup number and fell back to the unrelated
			// "set up a login email" flow (auth.sentCodeTypeSetUpEmailRequired),
			// which expects a completely different next step and never
			// actually sent a code anywhere. Surface this as an error
			// instead of silently entering a broken CodeWidget state.
			showEmailError(rpl::single(u"Server did not accept this as an email-signup account (SetUpEmailRequired). Check TELESRV_EMAIL_SIGNUP_ENABLE on the server."_q));
			return;
		}
		goNext<CodeWidget>();
	}, [&](const MTPDauth_sentCodeSuccess &data) {
		finish(data.vauthorization());
	}, [](const MTPDauth_sentCodePaymentRequired &) {
		LOG(("API Error: Unexpected auth.sentCodePaymentRequired "
			"(EmailSignupWidget::sendCodeDone)."));
	});
}

void EmailSignupWidget::sendCodeFail(const MTP::Error &error) {
	if (MTP::IsFloodError(error)) {
		_sentRequest = 0;
		showEmailError(tr::lng_flood_error());
		return;
	}

	_sentRequest = 0;
	auto &err = error.type();
	if (err == u"PHONE_NUMBER_FLOOD"_q) {
		Ui::show(Ui::MakeInformBox(tr::lng_error_phone_flood()));
	} else if (err == u"PHONE_NUMBER_INVALID"_q
		|| err == u"PHONE_NUMBER_BANNED"_q) {
		showEmailError(tr::lng_cloud_password_bad_email());
	} else if (!MTP::IgnoreError(error)) {
		showEmailError(rpl::single(err));
	}
}

} // namespace details
} // namespace Intro
