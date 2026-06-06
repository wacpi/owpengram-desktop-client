/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "boxes/owpengram_server_details_box.h"

#include "base/weak_ptr.h"
#include "lang/lang_keys.h"
#include "ui/painter.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "styles/style_intro.h"
#include "styles/style_layers.h"

ServerDetailsBox::ServerDetailsBox(
	QWidget*,
	Owpengram::Server server,
	Fn<void(const Owpengram::Server&)> connect)
: _server(std::move(server))
, _connect(std::move(connect))
, _content(this) {
	const auto logoWrap = _content->add(
		object_ptr<Ui::RpWidget>(_content));
	logoWrap->resize(st::introServerCardLogo, st::introServerCardLogo);
	logoWrap->paintRequest(
	) | rpl::on_next([=] {
		auto p = QPainter(logoWrap);
		PainterHighQualityEnabler hq(p);
		const auto image = QPixmap(_server.logoPath).scaled(
			st::introServerCardLogo,
			st::introServerCardLogo,
			Qt::KeepAspectRatio,
			Qt::SmoothTransformation);
		const auto left = (st::introServerCardLogo - image.width()) / 2;
		const auto top = (st::introServerCardLogo - image.height()) / 2;
		p.drawPixmap(left, top, image);
	}, logoWrap->lifetime());
	_content->add(
		object_ptr<Ui::FlatLabel>(
			_content,
			_server.name,
			st::introServerCardName),
		st::boxRowPadding + QMargins(0, st::boxRowPadding.top(), 0, 0));
	addField(
		tr::lng_owpengram_server_details_host(tr::now),
		_server.host);
	addField(
		tr::lng_owpengram_server_details_port(tr::now),
		_server.port > 0 ? QString::number(_server.port) : u"—"_q);
	if (!_server.description.isEmpty()) {
		addField(
			tr::lng_owpengram_server_details_description(tr::now),
			_server.description);
	}
	_content->add(
		object_ptr<Ui::FlatLabel>(
			_content,
			tr::lng_owpengram_server_details_status(tr::now),
			st::boxLabel),
		st::boxRowPadding + QMargins(0, st::boxRowPadding.top(), 0, 0));
	_status = _content->add(
		object_ptr<Ui::FlatLabel>(
			_content,
			tr::lng_owpengram_server_checking(tr::now),
			st::introServerCardStatusOnline),
		st::boxRowPadding);
	_latency = _content->add(
		object_ptr<Ui::FlatLabel>(
			_content,
			QString(),
			st::introServerCardStatusLatency),
		st::boxRowPadding + QMargins(st::introServerCardStatusDot + 8, 0, 0, 0));
}

void ServerDetailsBox::prepare() {
	setTitle(rpl::single(_server.name));
	addButton(tr::lng_owpengram_server_join(), [=] {
		if (_connect) {
			_connect(_server);
		}
		closeBox();
	});
	addButton(tr::lng_cancel(), [=] { closeBox(); });
	setDimensionsToContent(st::boxWidth, _content);
	Owpengram::CheckServerOnline(_server, crl::guard(this, [=](
			bool online,
			int latencyMs) {
		updateStatus(online, latencyMs);
	}));
}

void ServerDetailsBox::addField(const QString &label, const QString &value) {
	_content->add(
		object_ptr<Ui::FlatLabel>(
			_content,
			label,
			st::boxLabel),
		st::boxRowPadding);
	_content->add(
		object_ptr<Ui::FlatLabel>(
			_content,
			value,
			st::defaultFlatLabel),
		st::boxRowPadding);
}

void ServerDetailsBox::updateStatus(bool online, int latencyMs) {
	if (!_status) {
		return;
	}
	_status->setText(online
		? tr::lng_owpengram_server_online(tr::now)
		: tr::lng_owpengram_server_offline(tr::now));
	_status->setTextColorOverride(online
		? st::windowActiveTextFg->c
		: st::attentionButtonFg->c);
	if (_latency) {
		_latency->setText(online && latencyMs >= 0
			? tr::lng_owpengram_server_latency(
				tr::now,
				lt_latency,
				QString::number(latencyMs))
			: QString());
	}
}
