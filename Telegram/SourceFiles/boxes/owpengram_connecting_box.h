/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "ui/layers/box_content.h"
#include "base/timer.h"

namespace Ui {
class FlatLabel;
class VerticalLayout;
} // namespace Ui

// A blocking modal shown while the client connects to a server. While in the
// "connecting" state it cannot be dismissed (no button, no escape, no outside
// click). On timeout call showFailed(): the box becomes closable and invokes
// onFailedClose() when the user closes it.
class ConnectingBox : public Ui::BoxContent {
public:
	ConnectingBox(QWidget*, Fn<void()> onFailedClose = nullptr);

	void showFailed();

protected:
	void prepare() override;

private:
	void updateText();

	Fn<void()> _onFailedClose;
	object_ptr<Ui::VerticalLayout> _content;
	QPointer<Ui::FlatLabel> _label;
	base::Timer _dotsTimer;
	int _dots = 0;
	bool _failed = false;

};
