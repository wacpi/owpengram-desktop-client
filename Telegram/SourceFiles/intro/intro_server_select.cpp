/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.
For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "base/weak_ptr.h"
#include "intro/intro_server_select.h"
#include "intro/intro_start.h"
#include "intro/intro_qr.h"
#include "intro/intro_widget.h"
#include "boxes/abstract_box.h"
#include "boxes/owpengram_add_server_box.h"
#include "lang/lang_keys.h"
#include "main/main_account.h"
#include "owpengram/owpengram_servers.h"
#include "ui/boxes/confirm_box.h"
#include "ui/painter.h"
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
		Fn<void(const Owpengram::Server&)> join);
	void setOnline(bool online);
protected:
	void paintEvent(QPaintEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;
private:
	void updateGeometryInner();
	Owpengram::Server _server;
	Fn<void(const Owpengram::Server&)> _join;
	object_ptr<Ui::RpWidget> _logo;
	object_ptr<Ui::FlatLabel> _name;
	object_ptr<Ui::FlatLabel> _endpoint;
	object_ptr<Ui::FlatLabel> _description;
	object_ptr<Ui::FlatLabel> _status;
	object_ptr<Ui::RoundButton> _joinButton;
	std::optional<bool> _online;
};
ServerCard::ServerCard(
	QWidget *parent,
	const Owpengram::Server &server,
	Fn<void(const Owpengram::Server&)> join)
: RippleButton(parent, st::introServerCardRipple)
, _server(server)
, _join(std::move(join))
, _logo(this)
, _name(this, server.name, st::introServerCardName)
, _endpoint(this, tr::lng_owpengram_server_endpoint(
	tr::now,
	lt_host,
	server.host,
	lt_port,
	QString::number(server.port)), st::introServerCardEndpoint)
, _description(this, server.description, st::introServerCardDescription)
, _status(this, QString(), st::introServerCardStatusOnline)
, _joinButton(this, tr::lng_owpengram_server_join(), st::introServerCardJoinButton) {
	setPointerCursor(false);
	_logo->resize(st::introServerCardLogo, st::introServerCardLogo);
	_logo->setAttribute(Qt::WA_TransparentForMouseEvents);
	_logo->paintRequest(
	) | rpl::on_next([=] {
		auto p = QPainter(_logo.data());
		PainterHighQualityEnabler hq(p);
		p.setRenderHint(QPainter::Antialiasing);
		const auto image = QPixmap(_server.logoPath).scaled(
			st::introServerCardLogo,
			st::introServerCardLogo,
			Qt::KeepAspectRatio,
			Qt::SmoothTransformation);
		const auto left = (st::introServerCardLogo - image.width()) / 2;
		const auto top = (st::introServerCardLogo - image.height()) / 2;
		p.drawPixmap(left, top, image);
	}, _logo->lifetime());
	_name->setAttribute(Qt::WA_TransparentForMouseEvents);
	_endpoint->setAttribute(Qt::WA_TransparentForMouseEvents);
	_description->setAttribute(Qt::WA_TransparentForMouseEvents);
	_status->setAttribute(Qt::WA_TransparentForMouseEvents);
	_status->hide();
	_joinButton->setClickedCallback([=] {
		if (_join) {
			_join(_server);
		}
	});
	resize(st::introServerGridWidth / st::introServerGridColumns, st::introServerCardHeight);
	updateGeometryInner();
}
void ServerCard::setOnline(bool online) {
	_online = online;
	_status->setText(online
		? tr::lng_owpengram_server_online(tr::now)
		: tr::lng_owpengram_server_offline(tr::now));
	_status->setTextColorOverride(online
		? st::windowActiveTextFg->c
		: st::attentionButtonFg->c);
	_status->show();
	update();
}
void ServerCard::paintEvent(QPaintEvent *e) {
	Painter p(this);
	p.setClipRect(e->rect());
	const auto inner = rect().marginsRemoved(st::introServerCardPadding);
	const auto radius = st::introServerCardRadius;
	p.setPen(Qt::NoPen);
	p.setBrush(st::windowBg);
	p.drawRoundedRect(inner, radius, radius);
	p.setPen(st::boxDividerFg);
	p.setBrush(Qt::NoBrush);
	p.drawRoundedRect(inner, radius, radius);
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
	_logo->move(inner.x(), inner.y());
	const auto textLeft = inner.x() + logoSize + padding.left();
	const auto textWidth = inner.width() - logoSize - padding.left();
	_name->resizeToWidth(textWidth);
	_name->moveToLeft(textLeft, inner.y());
	_endpoint->resizeToWidth(textWidth);
	_endpoint->moveToLeft(textLeft, _name->y() + _name->height() + padding.top() / 2);
	_description->resizeToWidth(inner.width());
	_description->moveToLeft(
		inner.x(),
		_endpoint->y() + _endpoint->height() + padding.top() / 2);
	const auto joinHeight = _joinButton->height();
	const auto joinWidth = std::min(
		_joinButton->width(),
		inner.width() / 3);
	_joinButton->resize(joinWidth, joinHeight);
	_joinButton->move(
		inner.x() + inner.width() - joinWidth,
		inner.y() + inner.height() - joinHeight);
	_status->resizeToWidth(inner.width() - joinWidth - padding.left());
	_status->moveToLeft(
		inner.x(),
		inner.y() + inner.height() - _status->height());
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
			if (weak) {
				rebuildCards();
			}
		}));
	});
	_statusTimer.setCallback([=] { rebuildCards(); });
	show();
}
void ServerSelectWidget::activate() {
	Step::activate();
	rebuildCards();
	_statusTimer.callEach(30000);
}
void ServerSelectWidget::submit() {
}
rpl::producer<QString> ServerSelectWidget::nextButtonText() const {
	return rpl::single(QString());
}
void ServerSelectWidget::rebuildCards() {
	for (const auto card : _cards) {
		delete card;
	}
	_cards.clear();
	const auto join = [=](const Owpengram::Server &server) {
		joinServer(server);
	};
	for (const auto &server : Owpengram::ListServers()) {
		const auto card = Ui::CreateChild<ServerCard>(_grid, server, join);
		_cards.push_back(card);
		Owpengram::CheckServerOnline(server, crl::guard(card, [=](bool online) {
			card->setOnline(online);
		}));
	}
	updateCardsGeometry();
}
void ServerSelectWidget::joinServer(const Owpengram::Server &server) {
	const auto weak = base::make_weak(this);
	Owpengram::CheckServerOnline(server, crl::guard(weak, [=](bool online) {
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
	Owpengram::ApplyServerToAccount(&account(), server);
	if (getData()->enterPoint == EnterPoint::Qr) {
		goNext<QrWidget>();
	} else {
		goNext<StartWidget>();
	}
}
void ServerSelectWidget::resizeEvent(QResizeEvent *e) {
	Step::resizeEvent(e);
	updateAddButtonGeometry();
	const auto scrollTop = coverDescriptionBottom() + st::introServerGridTop;
	const auto scrollHeight = height() - scrollTop - st::introSettingsSkip;
	const auto scrollWidth = std::min(width() - 2 * st::introSettingsSkip, st::introServerGridWidth);
	_scroll->setGeometry(
		(width() - scrollWidth) / 2,
		scrollTop,
		scrollWidth,
		std::max(scrollHeight, st::introServerCardHeight + st::introServerGridSpacing));
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
	const auto columns = st::introServerGridColumns;
	const auto spacing = st::introServerGridSpacing;
	const auto cardWidth = (_scroll->width()
		- spacing * (columns - 1)) / columns;
	const auto cardHeight = st::introServerCardHeight;
	const auto rows = (_cards.size() + columns - 1) / columns;
	for (auto i = 0; i != _cards.size(); ++i) {
		const auto card = _cards[i];
		const auto row = i / columns;
		const auto column = i % columns;
		const auto cardsInRow = std::min<int>(
			columns,
			_cards.size() - row * columns);
		const auto rowWidth = cardsInRow * cardWidth
			+ (cardsInRow - 1) * spacing;
		const auto rowOffset = (_scroll->width() - rowWidth) / 2;
		card->resize(cardWidth, cardHeight);
		card->moveToLeft(
			rowOffset + column * (cardWidth + spacing),
			row * (cardHeight + spacing));
	}
	const auto maxHeight = rows * cardHeight + (rows - 1) * spacing;
	_grid->resize(_scroll->width(), maxHeight);
}
} // namespace details
} // namespace Intro
