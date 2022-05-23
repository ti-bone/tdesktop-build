/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "settings/settings_premium.h"

#include "core/application.h"
#include "info/info_wrap_widget.h" // Info::Wrap.
#include "info/settings/info_settings_widget.h" // SectionCustomTopBarData.
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "settings/settings_common.h"
#include "settings/settings_premium.h"
#include "ui/abstract_button.h"
#include "ui/basic_click_handlers.h"
#include "ui/effects/gradient.h"
#include "ui/effects/premium_graphics.h"
#include "ui/text/text_utilities.h"
#include "ui/widgets/gradient_round_button.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/fade_wrap.h"
#include "ui/wrap/padding_wrap.h"
#include "ui/wrap/slide_wrap.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_controller.h"
#include "main/main_session.h"
#include "main/main_account.h"
#include "main/main_app_config.h"
#include "window/window_session_controller.h"
#include "styles/style_boxes.h"
#include "styles/style_chat_helpers.h"
#include "styles/style_info.h"
#include "styles/style_intro.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"

#include <QSvgRenderer>

namespace Settings {
namespace {

using SectionCustomTopBarData = Info::Settings::SectionCustomTopBarData;

constexpr auto kBodyAnimationPart = 0.90;
constexpr auto kTitleAnimationPart = 0.15;

struct Entry {
	const style::icon *icon;
	rpl::producer<QString> title;
	rpl::producer<QString> description;
};

[[nodiscard]] base::flat_map<QString, Entry> EntryMap() {
	return base::flat_map<QString, Entry>{
		{
			QString("double_limits"),
			Entry{
				&st::settingsPremiumIconDouble,
				tr::lng_premium_summary_subtitle_double_limits(),
				tr::lng_premium_summary_about_double_limits(),
			},
		},
		{
			QString("more_upload"),
			Entry{
				&st::settingsPremiumIconFiles,
				tr::lng_premium_summary_subtitle_more_upload(),
				tr::lng_premium_summary_about_more_upload(),
			},
		},
		{
			QString("faster_download"),
			Entry{
				&st::settingsPremiumIconSpeed,
				tr::lng_premium_summary_subtitle_faster_download(),
				tr::lng_premium_summary_about_faster_download(),
			},
		},
		{
			QString("voice_to_text"),
			Entry{
				&st::settingsPremiumIconVoice,
				tr::lng_premium_summary_subtitle_voice_to_text(),
				tr::lng_premium_summary_about_voice_to_text(),
			},
		},
		{
			QString("no_ads"),
			Entry{
				&st::settingsPremiumIconChannelsOff,
				tr::lng_premium_summary_subtitle_no_ads(),
				tr::lng_premium_summary_about_no_ads(),
			},
		},
		{
			QString("unique_reactions"),
			Entry{
				&st::settingsPremiumIconLike,
				tr::lng_premium_summary_subtitle_unique_reactions(),
				tr::lng_premium_summary_about_unique_reactions(),
			},
		},
		{
			QString("premium_stickers"),
			Entry{
				&st::settingsIconStickers,
				tr::lng_premium_summary_subtitle_premium_stickers(),
				tr::lng_premium_summary_about_premium_stickers(),
			},
		},
		{
			QString("advanced_chat_management"),
			Entry{
				&st::settingsIconChat,
				tr::lng_premium_summary_subtitle_advanced_chat_management(),
				tr::lng_premium_summary_about_advanced_chat_management(),
			},
		},
		{
			QString("profile_badge"),
			Entry{
				&st::settingsPremiumIconStar,
				tr::lng_premium_summary_subtitle_profile_badge(),
				tr::lng_premium_summary_about_profile_badge(),
			},
		},
		{
			QString("animated_userpics"),
			Entry{
				&st::settingsPremiumIconPlay,
				tr::lng_premium_summary_subtitle_animated_userpics(),
				tr::lng_premium_summary_about_animated_userpics(),
			},
		},
	};
}

class TopBar final : public Ui::RpWidget {
public:
	TopBar(not_null<QWidget*> parent);

	void setRoundEdges(bool value);
	void setTextPosition(int x, int y);

protected:
	void paintEvent(QPaintEvent *e) override;

private:
	QSvgRenderer _star;
	Ui::Text::String _title;
	Ui::Text::String _about;

	QPoint _titlePosition;
	bool _roundEdges = true;

};

TopBar::TopBar(not_null<QWidget*> parent)
: Ui::RpWidget(parent)
, _star(u":/gui/icons/settings/star.svg"_q)
, _title(st::boxTitle.style, tr::lng_premium_summary_title(tr::now)) {
	_about.setMarkedText(
		st::aboutLabel.style,
		tr::lng_premium_summary_top_about(tr::now, Ui::Text::RichLangValue));
}

void TopBar::setRoundEdges(bool value) {
	_roundEdges = value;
	update();
}

void TopBar::setTextPosition(int x, int y) {
	_titlePosition = { x, y };
}

void TopBar::paintEvent(QPaintEvent *e) {
	Painter p(this);

	p.fillRect(e->rect(), Qt::transparent);

	const auto progress = (height() - minimumHeight())
		/ float64(maximumHeight() - minimumHeight());
	const auto topProgress = 1. -
		std::clamp(
			(1. - progress) / kBodyAnimationPart,
			0.,
			1.);
	const auto bodyProgress = topProgress;

	const auto r = rect();
	auto pathTop = QPainterPath();
	if (_roundEdges) {
		pathTop.addRoundedRect(r, st::boxRadius, st::boxRadius);
	} else {
		pathTop.addRect(r);
	}
	auto pathBottom = QPainterPath();
	pathBottom.addRect(
		QRect(
			QPoint(r.x(), r.y() + r.height() - st::boxRadius),
			QSize(r.width(), st::boxRadius)));

	const auto gradientPointTop = r.height() / 3. * 2.;
	auto gradient = QLinearGradient(
		QPointF(0, gradientPointTop),
		QPointF(r.width(), r.height() - gradientPointTop));
	gradient.setColorAt(0., st::premiumButtonBg1->c);
	gradient.setColorAt(.6, st::premiumButtonBg2->c);
	gradient.setColorAt(1., st::premiumButtonBg3->c);

	PainterHighQualityEnabler hq(p);
	p.fillPath(pathTop + pathBottom, gradient);

	p.setOpacity(bodyProgress);

	const auto &starSize = st::settingsPremiumStarSize;
	const auto starRect = QRectF(
		QPointF(
			(width() - starSize.width()) / 2,
			st::settingsPremiumStarTopSkip * topProgress),
		st::settingsPremiumStarSize);
	_star.render(&p, starRect);

	p.setPen(st::premiumButtonFg);

	const auto &padding = st::boxRowPadding;
	const auto availableWidth = width() - padding.left() - padding.right();
	const auto titleTop = starRect.top()
		+ starRect.height()
		+ st::changePhoneTitlePadding.top();
	const auto aboutTop = titleTop
		+ _title.countHeight(availableWidth)
		+ st::changePhoneTitlePadding.bottom();

	p.setFont(st::aboutLabel.style.font);
	_about.draw(p, padding.left(), aboutTop, availableWidth, style::al_top);

	// Subtitle.
	p.setFont(st::boxTitle.style.font);
	_title.draw(p, padding.left(), titleTop, availableWidth, style::al_top);

	// Title.
	const auto titleProgress =
		std::clamp(
			(kTitleAnimationPart - progress) / kTitleAnimationPart,
			0.,
			1.);
	if (titleProgress > 0.) {
		p.setOpacity(titleProgress);
		const auto availableWidth = width() - _titlePosition.x() * 2;
		_title.drawElided(
			p,
			_titlePosition.x(),
			_titlePosition.y(),
			availableWidth);
	}
}

class Premium : public Section<Premium> {
public:
	Premium(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

	[[nodiscard]] QPointer<Ui::RpWidget> createPinnedToTop(
		not_null<QWidget*> parent) override;
	[[nodiscard]] QPointer<Ui::RpWidget> createPinnedToBottom(
		not_null<Ui::RpWidget*> parent) override;

	[[nodiscard]] bool hasFlexibleTopBar() const override;

	void setStepDataReference(std::any &data) override;

	[[nodiscard]] rpl::producer<> sectionShowBack() override final;

private:
	void setupContent();

	const not_null<Window::SessionController*> _controller;

	base::unique_qptr<Ui::FadeWrap<Ui::IconButton>> _back;
	base::unique_qptr<Ui::IconButton> _close;
	rpl::variable<bool> _backToggles;
	rpl::variable<Info::Wrap> _wrap;

	rpl::event_stream<> _showBack;

};

Premium::Premium(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent)
, _controller(controller) {
	setupContent();
}

rpl::producer<QString> Premium::title() {
	return tr::lng_premium_summary_title();
}

bool Premium::hasFlexibleTopBar() const {
	return true;
}

rpl::producer<> Premium::sectionShowBack() {
	return _showBack.events();
}

void Premium::setStepDataReference(std::any &data) {
	const auto my = std::any_cast<SectionCustomTopBarData>(&data);
	if (my) {
		_backToggles = std::move(
			my->backButtonEnables
		) | rpl::map_to(true);
		_wrap = std::move(my->wrapValue);
	}
}

void Premium::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	AddSkip(content);

	const auto &st = st::settingsButton;
	const auto &stLabel = st::defaultFlatLabel;
	const auto iconSize = st::settingsPremiumIconDouble.size();

	auto entryMap = EntryMap();
	auto iconContainers = std::vector<Ui::AbstractButton*>();
	iconContainers.reserve(int(entryMap.size()));

	auto titlePadding = st.padding;
	titlePadding.setBottom(0);
	auto descriptionPadding = st.padding;
	descriptionPadding.setTop(0);
	const auto addRow = [&](
			rpl::producer<QString> &&title,
			rpl::producer<QString> &&text) {
		const auto labelAscent = stLabel.style.font->ascent;

		const auto label = content->add(
			object_ptr<Ui::FlatLabel>(
				content,
				std::move(title) | rpl::map(Ui::Text::Bold),
				stLabel),
			titlePadding);
		AddSkip(content, st::settingsPremiumDescriptionSkip);
		content->add(
			object_ptr<Ui::FlatLabel>(
				content,
				std::move(text),
				st::boxDividerLabel),
			descriptionPadding);

		const auto dummy = Ui::CreateChild<Ui::AbstractButton>(content);
		dummy->setAttribute(Qt::WA_TransparentForMouseEvents);

		content->sizeValue(
		) | rpl::start_with_next([=](const QSize &s) {
			dummy->resize(s.width(), iconSize.height());
		}, dummy->lifetime());

		label->geometryValue(
		) | rpl::start_with_next([=](const QRect &r) {
			dummy->moveToLeft(0, r.y() + (r.height() - labelAscent));
		}, dummy->lifetime());

		iconContainers.push_back(dummy);
	};

	auto icons = std::vector<const style::icon *>();
	icons.reserve(int(entryMap.size()));
	{
		using Order = std::vector<QString>;
		const auto &account = _controller->session().account();
		const auto mtpOrder = account.appConfig().get<Order>(
			"premium_promo_order",
			Order());
		const auto processEntry = [&](Entry &entry) {
			icons.push_back(entry.icon);
			addRow(base::take(entry.title), base::take(entry.description));
		};

		if (!mtpOrder.empty()) {
			for (const auto &key : mtpOrder) {
				auto it = entryMap.find(key);
				if (it == end(entryMap)) {
					continue;
				}
				processEntry(it->second);
			}
		} else {
			for (auto &entry : ranges::views::values(entryMap)) {
				processEntry(entry);
			}
		}
	}

	content->resizeToWidth(content->height());

	// Icons.
	Assert(iconContainers.size() > 2);
	const auto from = iconContainers.front()->y();
	const auto to = iconContainers.back()->y() + iconSize.height();
	auto gradient = QLinearGradient(0, 0, 0, to - from);
	gradient.setColorAt(0.0, st::premiumIconBg1->c);
	gradient.setColorAt(.28, st::premiumIconBg2->c);
	gradient.setColorAt(.55, st::premiumButtonBg2->c);
	gradient.setColorAt(1.0, st::premiumButtonBg1->c);
	for (auto i = 0; i < int(icons.size()); i++) {
		const auto &iconContainer = iconContainers[i];

		const auto pointTop = iconContainer->y() - from;
		const auto pointBottom = pointTop + iconContainer->height();
		const auto ratioTop = pointTop / float64(to - from);
		const auto ratioBottom = pointBottom / float64(to - from);

		auto resultGradient = QLinearGradient(
			QPointF(),
			QPointF(0, pointBottom - pointTop));

		resultGradient.setColorAt(
			.0,
			anim::gradient_color_at(gradient, ratioTop));
		resultGradient.setColorAt(
			.1,
			anim::gradient_color_at(gradient, ratioBottom));

		const auto brush = QBrush(resultGradient);
		AddButtonIcon(
			iconContainer,
			st,
			{ .icon = icons[i], .backgroundBrush = brush });
	}

	AddSkip(content);
	AddDivider(content);
	AddSkip(content);

	content->add(
		object_ptr<Ui::FlatLabel>(
			content,
			tr::lng_premium_summary_bottom_subtitle(
			) | rpl::map(Ui::Text::Bold),
			stLabel),
		st::settingsSubsectionTitlePadding);
	content->add(
		object_ptr<Ui::FlatLabel>(
			content,
			tr::lng_premium_summary_bottom_about(Ui::Text::RichLangValue),
			st::aboutLabel),
		st::boxRowPadding);
	AddSkip(content);

	Ui::ResizeFitChild(this, content);

}

QPointer<Ui::RpWidget> Premium::createPinnedToTop(
		not_null<QWidget*> parent) {
	const auto content = Ui::CreateChild<TopBar>(parent.get());

	_wrap.value(
	) | rpl::start_with_next([=](Info::Wrap wrap) {
		content->setRoundEdges(wrap == Info::Wrap::Layer);
	}, content->lifetime());

	content->setMaximumHeight(st::introQrStepsTop);
	content->setMinimumHeight(st::infoLayerTopBarHeight);

	content->resize(content->width(), content->maximumHeight());

	_wrap.value(
	) | rpl::start_with_next([=](Info::Wrap wrap) {
		const auto isLayer = (wrap == Info::Wrap::Layer);
		_back = base::make_unique_q<Ui::FadeWrap<Ui::IconButton>>(
			content,
			object_ptr<Ui::IconButton>(
				content,
				isLayer
					? st::settingsPremiumLayerTopBarBack
					: st::settingsPremiumTopBarBack),
			st::infoTopBarScale);
		_back->setDuration(0);
		_back->toggleOn(_backToggles.value());
		_back->entity()->addClickHandler([=] {
			_showBack.fire({});
		});
		_back->toggledValue(
		) | rpl::start_with_next([=](bool toggled) {
			const auto &st = isLayer ? st::infoLayerTopBar : st::infoTopBar;
			content->setTextPosition(
				toggled ? st.back.width : st.titlePosition.x(),
				st.titlePosition.y());
		}, _back->lifetime());

		if (!isLayer) {
			_close = nullptr;
		} else {
			_close = base::make_unique_q<Ui::IconButton>(
				content,
				st::settingsPremiumTopBarClose);
			_close->addClickHandler([=] {
				_controller->parentController()->hideLayer();
				_controller->parentController()->hideSpecialLayer();
			});
			content->widthValue(
			) | rpl::start_with_next([=] {
				_close->moveToRight(0, 0);
			}, _close->lifetime());
		}
	}, content->lifetime());

	return Ui::MakeWeak(not_null<Ui::RpWidget*>{ content });
}

QPointer<Ui::RpWidget> Premium::createPinnedToBottom(
		not_null<Ui::RpWidget*> parent) {

	const auto content = Ui::CreateChild<Ui::VerticalLayout>(parent.get());

	auto result = object_ptr<Ui::GradientButton>(
		content,
		Ui::Premium::ButtonGradientStops());

	result->setClickedCallback([=] {
		StartPremiumPayment(_controller, "settings");
	});

	const auto &st = st::premiumPreviewBox.button;
	result->resize(content->width(), st.height);

	const auto label = Ui::CreateChild<Ui::FlatLabel>(
		result.data(),
		tr::lng_premium_summary_button(tr::now, lt_cost, "$5"),
		st::premiumPreviewButtonLabel);
	label->setAttribute(Qt::WA_TransparentForMouseEvents);
	rpl::combine(
		result->widthValue(),
		label->widthValue()
	) | rpl::start_with_next([=](int outer, int width) {
		label->moveToLeft(
			(outer - width) / 2,
			st::premiumPreviewBox.button.textTop,
			outer);
	}, label->lifetime());
	auto padding = st::settingsPremiumButtonPadding;
	const auto paddingBottom = padding.bottom();
	padding.setBottom(paddingBottom - st::boxRadius);
	content->add(std::move(result), padding);

	content->add(
		object_ptr<Ui::SlideWrap<Ui::FixedHeightWidget>>(
			content,
			object_ptr<Ui::FixedHeightWidget>(content, st::boxRadius))
	)->setDuration(0)->toggleOn(_wrap.value(
	) | rpl::map(rpl::mappers::_1 != Info::Wrap::Layer));

	return Ui::MakeWeak(not_null<Ui::RpWidget*>{ content });
}

} // namespace

Type PremiumId() {
	return Premium::Id();
}

void ShowPremium(not_null<Main::Session*> session) {
	const auto active = Core::App().activeWindow();
	const auto controller = (active && active->isPrimary())
		? active->sessionController()
		: nullptr;
	if (controller && session == &controller->session()) {
		controller->showSettings(Settings::PremiumId());
	} else {
		for (const auto &controller : session->windows()) {
			if (controller->window().isPrimary()) {
				controller->showSettings(Settings::PremiumId());
			}
		}
	}
}

void StartPremiumPayment(
		not_null<Window::SessionController*> controller,
		const QString &ref) {
	const auto account = &controller->session().account();
	const auto username = account->appConfig().get<QString>(
		"premium_bot_username",
		QString());
	const auto slug = account->appConfig().get<QString>(
		"premium_invoice_slug",
		QString());
	if (!username.isEmpty()) {
		UrlClickHandler::Open("https://t.me/" + username + "?start=" + ref);
	} else if (!slug.isEmpty()) {
		UrlClickHandler::Open("https://t.me/$" + slug);
	}
}

} // namespace Settings