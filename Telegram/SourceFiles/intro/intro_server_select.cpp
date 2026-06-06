/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "intro/intro_server_select.h"

#include "intro/intro_qr.h"
#include "intro/intro_widget.h"

#include "boxes/abstract_box.h"
#include "boxes/owpengram_add_server_box.h"
#include "boxes/owpengram_server_details_box.h"
#include "lang/lang_keys.h"
#include "main/main_account.h"
#include "owpengram/owpengram_servers.h"
#include "settings/settings_common.h"
#include "ui/boxes/confirm_box.h"
#include "ui/painter.h"
#include "ui/toast/toast.h"
#include "ui/ui_utility.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "styles/style_intro.h"

#include <QRegion>

namespace Intro {
namespace details {
namespace {

[[nodiscard]] QString FormatStatusText(bool online) {
	return online
		? tr::lng_owpengram_server_online(tr::now)
		: tr::lng_owpengram_server_offline(tr::now);
}

void AddServerLogo(
		not_null<Ui::SettingsButton*> button,
		const QString &logoPath) {
	const auto icon = Ui::CreateChild<Ui::RpWidget>(button.get());
	const auto size = st::introServerRowLogo;
	icon->resize(size, size);
	icon->setAttribute(Qt::WA_TransparentForMouseEvents);
	button->sizeValue(
	) | rpl::on_next([=](const QSize &widgetSize) {
		icon->moveToLeft(
			st::introServerListButton.iconLeft,
			(widgetSize.height() - size) / 2,
			widgetSize.width());
	}, icon->lifetime());
	icon->paintRequest(
	) | rpl::on_next([=] {
		auto p = QPainter(icon);
		PainterHighQualityEnabler hq(p);
		p.setPen(Qt::NoPen);
		p.setBrush(st::windowBg);
		p.drawEllipse(0, 0, size, size);
		const auto image = QPixmap(logoPath).scaled(
			size,
			size,
			Qt::KeepAspectRatioByExpanding,
			Qt::SmoothTransformation);
		const auto left = (size - image.width()) / 2;
		const auto top = (size - image.height()) / 2;
		p.setClipRect(0, 0, size, size);
		p.setClipRegion(QRegion(0, 0, size, size, QRegion::Ellipse));
		p.drawPixmap(left, top, image);
	}, icon->lifetime());
	icon->show();
}

void AddThinSeparator(not_null<Ui::VerticalLayout*> container) {
	const auto separator = container->add(object_ptr<Ui::RpWidget>(container));
	separator->resize(0, st::lineWidth);
	separator->paintRequest(
	) | rpl::on_next([=] {
		auto p = QPainter(separator);
		p.fillRect(
			st::introServerListSeparatorLeft,
			0,
			separator->width() - st::introServerListSeparatorLeft,
			st::lineWidth,
			st::inputBorderFg);
	}, separator->lifetime());
}

} // namespace

ServerSelectWidget::ServerSelectWidget(
	QWidget *parent,
	not_null<Main::Account*> account,
	not_null<Data*> data)
: Step(parent, account, data, false)
, _panel(Ui::CreateChild<Ui::RpWidget>(this))
, _list(Ui::CreateChild<Ui::VerticalLayout>(_panel.get()))
, _statusTimer([=] { rebuildList(); }) {
	setTitleText(tr::lng_owpengram_server_selection_title());
	setDescriptionText(tr::lng_owpengram_server_selection_subtitle());

	_panel->paintRequest(
	) | rpl::on_next([=] {
		auto p = QPainter(_panel);
		PainterHighQualityEnabler hq(p);
		const auto radius = st::introServerListRadius;
		p.setPen(st::inputBorderFg);
		p.setBrush(st::windowBg);
		p.drawRoundedRect(_panel->rect(), radius, radius);
	}, _panel->lifetime());

	_addLink = Ui::CreateChild<Ui::LinkButton>(
		this,
		tr::lng_owpengram_server_add(tr::now),
		st::introLink);
	_addLink->setClickedCallback([=] {
		const auto weak = base::make_weak(this);
		Ui::show(Box<AddServerBox>([=](Owpengram::Server server) {
			Ui::PostponeCall(weak, [=] {
				if (weak) {
					rebuildList();
				}
			});
		}));
	});
	_addLink->show();
}

void ServerSelectWidget::finishInit() {
	updateListGeometry();
	rebuildList();
}

void ServerSelectWidget::activate() {
	Step::activate();
	updateListGeometry();
	if (!_list->count()) {
		rebuildList();
	}
	showChildren();
	_panel->show();
	_list->show();
	_list->showChildren();
	_addLink->show();
	_statusTimer.cancel();
	_statusTimer.callEach(30000);
}

void ServerSelectWidget::submit() {
}

rpl::producer<QString> ServerSelectWidget::nextButtonText() const {
	return rpl::single(QString());
}

int ServerSelectWidget::listWidth() const {
	return st::introNextButton.width;
}

int ServerSelectWidget::listTop() const {
	return contentTop() + st::introStepFieldTop;
}

void ServerSelectWidget::rebuildList() {
	_list->clear();

	const auto join = [=](const Owpengram::Server &server) {
		joinServer(server);
	};
	const auto details = [=](const Owpengram::Server &server) {
		Ui::show(Box<ServerDetailsBox>(server, join));
	};

	auto servers = std::vector<Owpengram::Server>();
	for (const auto &server : Owpengram::ListServers()) {
		if (server.valid()) {
			servers.push_back(server);
		}
	}

	for (auto i = 0; i != int(servers.size()); ++i) {
		const auto &server = servers[i];
		const auto button = Settings::AddButtonWithIcon(
			_list,
			rpl::single(server.name),
			st::introServerListButton);
		const auto status = button->lifetime().make_state<rpl::variable<QString>>(
			tr::lng_owpengram_server_checking(tr::now));
		Settings::CreateRightLabel(
			button,
			status->value(),
			st::introServerListButton,
			rpl::single(server.name));
		AddServerLogo(button, server.logoPath);
		button->addClickHandler([=] {
			details(server);
		});

		const auto weak = base::make_weak(button);
		Owpengram::CheckServerOnline(server, crl::guard(button, [=
				](bool online, int) {
			if (weak) {
				*status = FormatStatusText(online);
			}
		}));

		if (i + 1 != int(servers.size())) {
			AddThinSeparator(_list);
		}
	}

	_list->resizeToWidth(listWidth());
	updateListGeometry();
	_list->show();
	_list->showChildren();
}

void ServerSelectWidget::joinServer(const Owpengram::Server &server) {
	if (!server.valid()) {
		Ui::Toast::Show(tr::lng_owpengram_server_invalid(tr::now));
		return;
	}
	const auto weak = base::make_weak(this);
	Owpengram::CheckServerOnline(server, crl::guard(weak, [=](bool online, int) {
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
	updateListGeometry();
}

void ServerSelectWidget::updateListGeometry() {
	const auto width = listWidth();
	_list->resizeToWidth(width);
	_panel->resize(width, _list->height());
	_panel->moveToLeft(contentLeft(), listTop());
	_list->moveToLeft(0, 0);
	_addLink->moveToLeft(
		contentLeft(),
		_panel->y() + _panel->height() + st::introServerListAddTop);
}

} // namespace details
} // namespace Intro
