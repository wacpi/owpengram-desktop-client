/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.
For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "base/weak_ptr.h"
#include "intro/intro_server_select.h"
#include "intro/intro_qr.h"
#include "intro/intro_widget.h"
#include "boxes/abstract_box.h"
#include "boxes/owpengram_add_server_box.h"
#include "boxes/owpengram_server_details_box.h"
#include "lang/lang_keys.h"
#include "main/main_account.h"
#include "owpengram/owpengram_servers.h"
#include "ui/boxes/confirm_box.h"
#include "ui/painter.h"
#include "ui/toast/toast.h"
#include "ui/ui_utility.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/scroll_area.h"
#include "styles/style_intro.h"
#include "styles/style_layers.h"
namespace Intro {
namespace details {
namespace {
class ServerCard final : public Ui::RippleButton {
public:
	ServerCard(
		QWidget *parent,
		const Owpengram::Server &server,
		Fn<void(const Owpengram::Server&)> join,
		Fn<void(const Owpengram::Server&)> details);
	void setOnline(bool online, int latencyMs);
protected:
	void paintEvent(QPaintEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;
private:
	void updateGeometryInner();
	Owpengram::Server _server;
	Fn<void(const Owpengram::Server&)> _join;
	Fn<void(const Owpengram::Server&)> _details;
	object_ptr<Ui::RpWidget> _logo;
	object_ptr<Ui::FlatLabel> _name;
	object_ptr<Ui::FlatLabel> _endpoint;
	object_ptr<Ui::FlatLabel> _description;
	object_ptr<Ui::FlatLabel> _status;
	object_ptr<Ui::FlatLabel> _latency;
	object_ptr<Ui::RoundButton> _joinButton;
	std::optional<bool> _online;
	int _latencyMs = -1;
};
ServerCard::ServerCard(
	QWidget *parent,
	const Owpengram::Server &server,
	Fn<void(const Owpengram::Server&)> join,
	Fn<void(const Owpengram::Server&)> details)
: RippleButton(parent, st::introServerCardRipple)
, _server(server)
, _join(std::move(join))
, _details(std::move(details))
, _logo(this)
, _name(this, server.name, st::introServerCardName)
, _endpoint(this, Owpengram::FormatEndpoint(server), st::introServerCardEndpoint)
, _description(this, server.description, st::introServerCardDescription)
, _status(this, tr::lng_owpengram_server_checking(tr::now), st::introServerCardStatusOnline)
, _latency(this, QString(), st::introServerCardStatusLatency)
, _joinButton(this, tr::lng_owpengram_server_join(), st::introServerCardJoinButton) {
	setPointerCursor(true);
	_logo->resize(st::introServerCardLogo, st::introServerCardLogo);
	_logo->setAttribute(Qt::WA_TransparentForMouseEvents);
	_logo->paintRequest(
	) | rpl::on_next([=] {
		auto p = QPainter(_logo.data());
		PainterHighQualityEnabler hq(p);
		const auto image = QPixmap(_server.logoPath).scaled(
			st::introServerCardLogo,
			st::introServerCardLogo,
			Qt::KeepAspectRatio,
			Qt::SmoothTransformation);
		const auto left = (st::introServerCardLogo - image.width()) / 2;
		const auto top = (st::introServerCardLogo - image.height()) / 2;
		p.drawPixmap(left, top, image);
	}, _logo->lifetime());
	for (const auto label : { _name.data(), _endpoint.data(), _description.data(), _status.data(), _latency.data() }) {
		label->setAttribute(Qt::WA_TransparentForMouseEvents);
	}
	setClickedCallback([=] {
		if (_details) {
			_details(_server);
		}
	});
	_joinButton->setClickedCallback([=] {
		if (_join) {
			_join(_server);
		}
	});
	resize(st::introServerCardMinWidth, st::introServerCardMinWidth);
	updateGeometryInner();
}
void ServerCard::setOnline(bool online, int latencyMs) {
	_online = online;
	_latencyMs = latencyMs;
	_status->setText(online
		? tr::lng_owpengram_server_online(tr::now)
		: tr::lng_owpengram_server_offline(tr::now));
	_status->setTextColorOverride(online
		? st::windowActiveTextFg->c
		: st::attentionButtonFg->c);
	_latency->setText(online && latencyMs >= 0
		? tr::lng_owpengram_server_latency(
			tr::now,
			lt_latency,
			QString::number(latencyMs))
		: QString());
	update();
}
void ServerCard::paintEvent(QPaintEvent *e) {
	Painter p(this);
	p.setClipRect(e->rect());
	const auto inner = rect().marginsRemoved(st::introServerCardPadding);
	const auto radius = st::introServerCardRadius;
	p.setPen(Qt::NoPen);
	p.setBrush(st::boxBg);
	p.drawRoundedRect(inner, radius, radius);
	p.setPen(st::boxDividerFg);
	p.setBrush(Qt::NoBrush);
	p.drawRoundedRect(inner, radius, radius);
	if (_online.has_value()) {
		const auto dotSize = st::introServerCardStatusDot;
		const auto dotLeft = inner.x();
		const auto dotTop = _status->y() + (_status->height() - dotSize) / 2;
		p.setPen(Qt::NoPen);
		p.setBrush(*_online ? st::windowActiveTextFg : st::attentionButtonFg);
		p.drawEllipse(QRectF(dotLeft, dotTop, dotSize, dotSize));
	}
	paintRipple(p, 0, 0);
}
void ServerCard::resizeEvent(QResizeEvent *e) {
	RippleButton::resizeEvent(e);
	updateGeometryInner();
}
void ServerCard::updateGeometryInner() {
	const auto padding = st::introServerCardPadding;
	const auto inner = rect().marginsRemoved(padding);
	const auto logoSize = st::introServerCardLogo;
	const auto headerSkip = st::introServerCardHeaderSkip;
	const auto footerSkip = st::introServerCardFooterSkip;
	const auto joinHeight = _joinButton->height();
	const auto joinWidth = st::introServerCardJoinButton.width;
	_logo->move(inner.x(), inner.y());
	const auto textLeft = inner.x() + logoSize + headerSkip;
	const auto textWidth = std::max(
		inner.width() - logoSize - headerSkip,
		1);
	_name->resizeToWidth(textWidth);
	_name->moveToLeft(textLeft, inner.y());
	_endpoint->resizeToWidth(textWidth);
	_endpoint->moveToLeft(
		textLeft,
		_name->y() + _name->height() + padding.top() / 4);
	const auto headerBottom = std::max(
		_logo->y() + logoSize,
		_endpoint->y() + _endpoint->height());
	const auto footerHeight = joinHeight + footerSkip;
	const auto descriptionTop = headerBottom + headerSkip;
	const auto descriptionHeight = std::max(
		inner.height() - (descriptionTop - inner.y()) - footerHeight - footerSkip,
		st::introServerCardDescription.style.font->height * 2);
	_description->resizeToWidth(inner.width());
	if (_description->height() > descriptionHeight) {
		_description->resize(inner.width(), descriptionHeight);
	}
	_description->moveToLeft(inner.x(), descriptionTop);
	_joinButton->resize(joinWidth, joinHeight);
	_joinButton->moveToLeft(
		inner.x() + inner.width() - joinWidth,
		inner.y() + inner.height() - joinHeight);
	const auto statusLeft = inner.x() + st::introServerCardStatusDot + headerSkip / 2;
	const auto statusWidth = std::max(
		inner.width() - joinWidth - headerSkip - st::introServerCardStatusDot,
		1);
	_status->resizeToWidth(statusWidth);
	_status->moveToLeft(
		statusLeft,
		inner.y() + inner.height() - joinHeight - _status->height() - padding.top() / 4);
	_latency->resizeToWidth(statusWidth);
	_latency->moveToLeft(statusLeft, _status->y() + _status->height());
}
} // namespace
ServerSelectWidget::ServerSelectWidget(
	QWidget *parent,
	not_null<Main::Account*> account,
	not_null<Data*> data)
: Step(parent, account, data, true)
, _addServer(
	this,
	tr::lng_owpengram_server_add(),
	st::introServerAddButton)
, _scroll(this, st::boxScroll)
, _grid(_scroll->setOwnedWidget(object_ptr<Ui::RpWidget>(_scroll))) {
	setTitleText(tr::lng_owpengram_server_selection_title());
	setDescriptionText(tr::lng_owpengram_server_selection_subtitle());
	_addServer->setClickedCallback([=] {
		const auto weak = base::make_weak(this);
		Ui::show(Box<AddServerBox>([=](Owpengram::Server server) {
			Ui::PostponeCall(weak, [=] {
				if (weak) {
					rebuildCards();
				}
			});
		}));
	});
	_statusTimer.setCallback([=] { rebuildCards(); });
	show();
}
void ServerSelectWidget::activate() {
	Step::activate();
	_scroll->show();
	_addServer->show();
	_statusTimer.cancel();
	_statusTimer.callEach(30000);
	Ui::PostponeCall(this, [=] {
		updateScrollGeometry();
		rebuildCards();
	});
}
void ServerSelectWidget::showEvent(QShowEvent *e) {
	Step::showEvent(e);
	Ui::PostponeCall(this, [=] {
		updateScrollGeometry();
		updateCardsGeometry();
	});
}
void ServerSelectWidget::submit() {
}
rpl::producer<QString> ServerSelectWidget::nextButtonText() const {
	return rpl::single(QString());
}
int ServerSelectWidget::scrollWidth() const {
	return std::min(
		width() - 2 * st::introSettingsSkip,
		st::introServerGridWidth);
}
int ServerSelectWidget::effectiveScrollWidth() const {
	return std::max(_scroll->width(), scrollWidth());
}
int ServerSelectWidget::columnCount() const {
	const auto width = effectiveScrollWidth();
	const auto minWidth = st::introServerCardMinWidth;
	const auto spacing = st::introServerGridSpacing;
	const auto maxColumns = st::introServerGridMaxColumns;
	if (width <= minWidth) {
		return 1;
	}
	const auto columns = (width + spacing) / (minWidth + spacing);
	return std::clamp(columns, 1, maxColumns);
}
void ServerSelectWidget::rebuildCards() {
	for (const auto card : _cards) {
		delete card;
	}
	_cards.clear();
	const auto join = [=](const Owpengram::Server &server) {
		joinServer(server);
	};
	const auto details = [=](const Owpengram::Server &server) {
		Ui::show(Box<ServerDetailsBox>(server, join));
	};
	for (const auto &server : Owpengram::ListServers()) {
		if (!server.valid()) {
			continue;
		}
		const auto card = Ui::CreateChild<ServerCard>(
			_grid,
			server,
			join,
			details);
		card->show();
		_cards.push_back(card);
		Owpengram::CheckServerOnline(server, crl::guard(card, [=](
				bool online,
				int latencyMs) {
			if (card) {
				card->setOnline(online, latencyMs);
			}
		}));
	}
	updateCardsGeometry();
}
void ServerSelectWidget::joinServer(const Owpengram::Server &server) {
	if (!server.valid()) {
		Ui::Toast::Show(tr::lng_owpengram_server_invalid(tr::now));
		return;
	}
	const auto weak = base::make_weak(this);
	Owpengram::CheckServerOnline(server, crl::guard(weak, [=](
			bool online,
			int latencyMs) {
		if (!weak) {
			return;
		}
		if (!online) {
			Ui::show(Ui::MakeConfirmBox({
				.text = tr::lng_owpengram_server_offline_confirm(),
				.confirmed = crl::guard(weak, [=] {
					proceedJoin(server);
				}),
				.confirmText = tr::lng_owpengram_server_join(),
			}));
			return;
		}
		proceedJoin(server);
	}));
}
void ServerSelectWidget::proceedJoin(const Owpengram::Server &server) {
	if (!server.valid()) {
		Ui::Toast::Show(tr::lng_owpengram_server_invalid(tr::now));
		return;
	}
	const auto weak = base::make_weak(this);
	Owpengram::ApplyServerToAccount(&account(), server);
	Owpengram::WaitForServerConnection(&account(), server, crl::guard(weak, [=] {
		if (weak) {
			goNext<QrWidget>();
		}
	}));
}
void ServerSelectWidget::resizeEvent(QResizeEvent *e) {
	Step::resizeEvent(e);
	updateAddButtonGeometry();
	updateScrollGeometry();
}
void ServerSelectWidget::updateScrollGeometry() {
	const auto scrollTop = coverDescriptionBottom() + st::introServerGridTop;
	const auto scrollHeight = height() - scrollTop - st::introSettingsSkip;
	const auto width = scrollWidth();
	const auto cardSize = [&] {
		if (_cards.empty()) {
			return st::introServerCardMinWidth;
		}
		const auto columns = columnCount();
		const auto spacing = st::introServerGridSpacing;
		return (width - spacing * (columns - 1)) / columns;
	}();
	_scroll->setGeometry(
		(this->width() - width) / 2,
		scrollTop,
		width,
		std::max(
			scrollHeight,
			cardSize + st::introServerGridSpacing));
	updateCardsGeometry();
}
void ServerSelectWidget::updateAddButtonGeometry() {
	const auto skip = st::introSettingsSkip;
	_addServer->moveToRight(
		skip,
		st::introCoverHeight - _addServer->height() - skip);
}
void ServerSelectWidget::updateCardsGeometry() {
	if (_cards.empty()) {
		return;
	}
	const auto columns = columnCount();
	const auto spacing = st::introServerGridSpacing;
	const auto scrollAreaWidth = effectiveScrollWidth();
	const auto cardWidth = (scrollAreaWidth - spacing * (columns - 1))
		/ columns;
	const auto cardHeight = cardWidth;
	const auto rows = (_cards.size() + columns - 1) / columns;
	for (auto i = 0; i != _cards.size(); ++i) {
		const auto card = _cards[i];
		const auto row = i / columns;
		const auto column = i % columns;
		const auto cardsInRow = std::min<int>(
			columns,
			int(_cards.size()) - row * columns);
		const auto rowWidth = cardsInRow * cardWidth
			+ (cardsInRow - 1) * spacing;
		const auto rowOffset = (scrollAreaWidth - rowWidth) / 2;
		card->resize(cardWidth, cardHeight);
		card->moveToLeft(
			rowOffset + column * (cardWidth + spacing),
			row * (cardHeight + spacing));
	}
	const auto maxHeight = rows * cardHeight + (rows - 1) * spacing;
	_grid->resize(scrollAreaWidth, maxHeight);
}
} // namespace details
} // namespace Intro
