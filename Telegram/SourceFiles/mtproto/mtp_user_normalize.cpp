/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "mtproto/mtp_user_normalize.h"

namespace MTP {
namespace {

[[nodiscard]] MTPUser UserFromLayer216(const MTPUser &user) {
	const auto &data = user.c_user_layer216();
	using Flag = MTPDuser::Flag;
	const auto flags = MTP_flags<Flag>(
		(data.is_self() ? Flag::f_self : Flag())
		| (data.is_contact() ? Flag::f_contact : Flag())
		| (data.is_mutual_contact() ? Flag::f_mutual_contact : Flag())
		| (data.is_deleted() ? Flag::f_deleted : Flag())
		| (data.is_bot() ? Flag::f_bot : Flag())
		| (data.is_bot_chat_history() ? Flag::f_bot_chat_history : Flag())
		| (data.is_bot_nochats() ? Flag::f_bot_nochats : Flag())
		| (data.is_verified() ? Flag::f_verified : Flag())
		| (data.is_restricted() ? Flag::f_restricted : Flag())
		| (data.is_min() ? Flag::f_min : Flag())
		| (data.is_bot_inline_geo() ? Flag::f_bot_inline_geo : Flag())
		| (data.is_support() ? Flag::f_support : Flag())
		| (data.is_scam() ? Flag::f_scam : Flag())
		| (data.is_apply_min_photo() ? Flag::f_apply_min_photo : Flag())
		| (data.is_fake() ? Flag::f_fake : Flag())
		| (data.is_bot_attach_menu() ? Flag::f_bot_attach_menu : Flag())
		| (data.is_premium() ? Flag::f_premium : Flag())
		| (data.is_attach_menu_enabled() ? Flag::f_attach_menu_enabled : Flag())
		| (data.is_bot_can_edit() ? Flag::f_bot_can_edit : Flag())
		| (data.is_close_friend() ? Flag::f_close_friend : Flag())
		| (data.is_stories_hidden() ? Flag::f_stories_hidden : Flag())
		| (data.is_stories_unavailable() ? Flag::f_stories_unavailable : Flag())
		| (data.is_contact_require_premium()
			? Flag::f_contact_require_premium
			: Flag())
		| (data.is_bot_business() ? Flag::f_bot_business : Flag())
		| (data.is_bot_has_main_app() ? Flag::f_bot_has_main_app : Flag())
		| (data.vaccess_hash() ? Flag::f_access_hash : Flag())
		| (data.vfirst_name() ? Flag::f_first_name : Flag())
		| (data.vlast_name() ? Flag::f_last_name : Flag())
		| (data.vusername() ? Flag::f_username : Flag())
		| (data.vphone() ? Flag::f_phone : Flag())
		| (data.vphoto() ? Flag::f_photo : Flag())
		| (data.vstatus() ? Flag::f_status : Flag())
		| (data.vbot_info_version() ? Flag::f_bot_info_version : Flag())
		| (data.vrestriction_reason() ? Flag::f_restriction_reason : Flag())
		| (data.vbot_inline_placeholder()
			? Flag::f_bot_inline_placeholder
			: Flag())
		| (data.vlang_code() ? Flag::f_lang_code : Flag())
		| (data.vemoji_status() ? Flag::f_emoji_status : Flag())
		| (data.vusernames() ? Flag::f_usernames : Flag())
		| (data.vstories_max_id() ? Flag::f_stories_max_id : Flag())
		| (data.vcolor() ? Flag::f_color : Flag())
		| (data.vprofile_color() ? Flag::f_profile_color : Flag())
		| (data.vbot_active_users() ? Flag::f_bot_active_users : Flag())
		| (data.vbot_verification_icon()
			? Flag::f_bot_verification_icon
			: Flag())
		| (data.vsend_paid_messages_stars()
			? Flag::f_send_paid_messages_stars
			: Flag()));
	const auto stories = data.vstories_max_id()
		? MTPRecentStory(MTP_recentStory(
			MTP_flags(MTPDrecentStory::Flag::f_max_id),
			*data.vstories_max_id()))
		: MTPRecentStory();
	return MTP_user(
		flags,
		data.vid(),
		data.vaccess_hash() ? *data.vaccess_hash() : MTPlong(),
		data.vfirst_name() ? *data.vfirst_name() : MTPstring(),
		data.vlast_name() ? *data.vlast_name() : MTPstring(),
		data.vusername() ? *data.vusername() : MTPstring(),
		data.vphone() ? *data.vphone() : MTPstring(),
		data.vphoto() ? *data.vphoto() : MTPUserProfilePhoto(),
		data.vstatus() ? *data.vstatus() : MTPUserStatus(),
		data.vbot_info_version() ? *data.vbot_info_version() : MTPint(),
		data.vrestriction_reason()
			? *data.vrestriction_reason()
			: MTPVector<MTPRestrictionReason>(),
		data.vbot_inline_placeholder()
			? *data.vbot_inline_placeholder()
			: MTPstring(),
		data.vlang_code() ? *data.vlang_code() : MTPstring(),
		data.vemoji_status() ? *data.vemoji_status() : MTPEmojiStatus(),
		data.vusernames() ? *data.vusernames() : MTPVector<MTPUsername>(),
		stories,
		data.vcolor() ? *data.vcolor() : MTPPeerColor(),
		data.vprofile_color() ? *data.vprofile_color() : MTPPeerColor(),
		data.vbot_active_users() ? *data.vbot_active_users() : MTPint(),
		data.vbot_verification_icon()
			? *data.vbot_verification_icon()
			: MTPlong(),
		data.vsend_paid_messages_stars()
			? *data.vsend_paid_messages_stars()
			: MTPlong());
}

} // namespace

MTPUser NormalizeUser(const MTPUser &user) {
	if (user.type() == mtpc_user_layer216) {
		return UserFromLayer216(user);
	}
	return user;
}

} // namespace MTP
