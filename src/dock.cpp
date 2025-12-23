// dock.cpp
#define LOG_TAG "[" PLUGIN_NAME "][dock]"
#include "dock.hpp"

#include "core.hpp"
#include "settings.hpp"
#include "widget.hpp" // use your existing carousel

#include <obs-frontend-api.h>
#include <obs.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QCheckBox>
#include <QLineEdit>
#include <QFileDialog>
#include <QStyle>
#include <QFrame>
#include <QEvent>
#include <QShortcut>
#include <QMouseEvent>
#include <QDir>
#include <QPixmap>
#include <QVariant>
#include <QSizePolicy>

static QWidget *g_dockWidget = nullptr;

namespace smart_lt::ui {

LowerThirdDock::LowerThirdDock(QWidget *parent) : QWidget(parent)
{
	setObjectName(QStringLiteral("LowerThirdDock"));
	setAttribute(Qt::WA_StyledBackground, true);

	// Keep your blue overlay effect + hidden checkbox
	setStyleSheet(R"(
#LowerThirdDock { background: rgba(39, 42, 51, 1.0); }
QFrame#sltRowFrame {
  border: 1px solid rgba(255,255,255,40);
  border-radius: 4px;
  padding: 4px 6px;
  background: transparent;
}
QFrame#sltRowFrame:hover { background: rgba(255,255,255,0.04); }
QFrame#sltRowFrame[sltActive="true"] {
  background: rgba(88,166,255,0.16);
  border: 1px solid rgba(88,166,255,0.9);
}
QLabel#sltRowLabel { color: #f0f6fc; font-weight: 500; }
QLabel#sltRowThumbnail { border-radius: 16px; background: rgba(0,0,0,0.35); }
QScrollArea#LowerThirdContent QPushButton { border: none; background: transparent; padding: 2px; }
QScrollArea#LowerThirdContent QPushButton:hover { background: rgba(255,255,255,0.06); border-radius: 3px; }
)");

	auto *rootLayout = new QVBoxLayout(this);
	rootLayout->setContentsMargins(8, 8, 8, 8);
	rootLayout->setSpacing(6);

	auto *st = style();

	// Top row
	{
		auto *row = new QHBoxLayout();
		row->setSpacing(6);

		auto *lbl = new QLabel(tr("Resources:"), this);

		outputPathEdit = new QLineEdit(this);
		outputPathEdit->setReadOnly(true);

		outputBrowseBtn = new QPushButton(this);
		outputBrowseBtn->setCursor(Qt::PointingHandCursor);
		outputBrowseBtn->setToolTip(tr("Select output folder"));
		outputBrowseBtn->setFlat(true);
		outputBrowseBtn->setIcon(st->standardIcon(QStyle::SP_DirOpenIcon));

		ensureSourceBtn = new QPushButton(this);
		ensureSourceBtn->setCursor(Qt::PointingHandCursor);
		ensureSourceBtn->setToolTip(tr("Add/Ensure Browser Source in current scene"));
		ensureSourceBtn->setFlat(true);

		QIcon globe = QIcon::fromTheme(QStringLiteral("internet-web-browser"));
		if (globe.isNull())
			globe = st->standardIcon(QStyle::SP_BrowserReload);
		ensureSourceBtn->setIcon(globe);

		row->addWidget(lbl);
		row->addWidget(outputPathEdit, 1);
		row->addWidget(outputBrowseBtn);
		row->addWidget(ensureSourceBtn);

		rootLayout->addLayout(row);

		connect(outputBrowseBtn, &QPushButton::clicked, this, &LowerThirdDock::onBrowseOutputFolder);
		connect(ensureSourceBtn, &QPushButton::clicked, this, &LowerThirdDock::onEnsureBrowserSourceClicked);
	}

	// List
	scrollArea = new QScrollArea(this);
	scrollArea->setObjectName(QStringLiteral("LowerThirdContent"));
	scrollArea->setWidgetResizable(true);

	listContainer = new QWidget(scrollArea);
	listLayout = new QVBoxLayout(listContainer);
	listLayout->setContentsMargins(8, 8, 8, 8);
	listLayout->setSpacing(6);
	listLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

	scrollArea->setWidget(listContainer);
	rootLayout->addWidget(scrollArea, 1);

	// Add button
	{
		auto *row = new QHBoxLayout();
		row->setSpacing(6);
		row->addStretch(1);

		addBtn = new QPushButton(this);
		addBtn->setCursor(Qt::PointingHandCursor);
		addBtn->setToolTip(tr("Add new lower third"));
		addBtn->setFlat(true);

		QIcon plus = QIcon::fromTheme(QStringLiteral("list-add"));
		if (plus.isNull())
			plus = st->standardIcon(QStyle::SP_DialogYesButton);
		addBtn->setIcon(plus);

		row->addWidget(addBtn);
		rootLayout->addLayout(row);

		connect(addBtn, &QPushButton::clicked, this, &LowerThirdDock::onAddLowerThird);
	}

	// Footer: your existing widget carousel
	rootLayout->addWidget(create_widget_carousel(this));

	// initial enablement
	const bool hasDir = smart_lt::has_output_dir();
	addBtn->setEnabled(hasDir);
	ensureSourceBtn->setEnabled(hasDir);
}

bool LowerThirdDock::init()
{
	// KEY FIX: load saved resources path + state on OBS startup
	smart_lt::init_from_disk();

	if (smart_lt::has_output_dir())
		outputPathEdit->setText(QString::fromStdString(smart_lt::output_dir()));
	else
		outputPathEdit->clear();

	const bool hasDir = smart_lt::has_output_dir();
	addBtn->setEnabled(hasDir);
	ensureSourceBtn->setEnabled(hasDir);

	if (hasDir)
		smart_lt::ensure_output_artifacts_exist();

	rebuildList();
	return true;
}

void LowerThirdDock::onBrowseOutputFolder()
{
	const QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output Folder"));
	if (dir.isEmpty())
		return;

	// Path change = rebuild trigger (core handles rebuild + config save)
	smart_lt::set_output_dir_and_load(dir.toStdString());

	outputPathEdit->setText(dir);

	const bool hasDir = smart_lt::has_output_dir();
	addBtn->setEnabled(hasDir);
	ensureSourceBtn->setEnabled(hasDir);

	rebuildList();
	emit requestSave();
}

void LowerThirdDock::onEnsureBrowserSourceClicked()
{
	if (!smart_lt::has_output_dir()) {
		LOGW("Cannot ensure browser source: output dir not set");
		return;
	}

	smart_lt::ensure_output_artifacts_exist();
	smart_lt::ensure_browser_source_in_current_scene();

	// requirement: NO rebuild here
}

void LowerThirdDock::onAddLowerThird()
{
	if (!smart_lt::has_output_dir())
		return;

	// Add = rebuild trigger (core does save + rebuild)
	const std::string id = smart_lt::add_default_lower_third();
	if (id.empty())
		return;

	rebuildList();
	emit requestSave();
}

void LowerThirdDock::rebuildList()
{
	for (auto &row : rows) {
		if (row.row)
			row.row->deleteLater();
	}
	rows.clear();

	const auto &items = smart_lt::all();
	const QString outDir = QString::fromStdString(smart_lt::output_dir());

	for (const auto &cfg : items) {
		LowerThirdRowUi ui;
		ui.id = QString::fromStdString(cfg.id);

		auto *rowFrame = new QFrame(listContainer);
		rowFrame->setObjectName(QStringLiteral("sltRowFrame"));
		rowFrame->setProperty("sltActive", QVariant(smart_lt::is_visible(cfg.id)));
		rowFrame->setCursor(Qt::PointingHandCursor);

		auto *h = new QHBoxLayout(rowFrame);
		h->setContentsMargins(8, 4, 8, 4);
		h->setSpacing(6);

		// Hidden checkbox indicator (mouse transparent)
		auto *visible = new QCheckBox(rowFrame);
		visible->setChecked(smart_lt::is_visible(cfg.id));
		visible->setFocusPolicy(Qt::NoFocus);
		visible->setAttribute(Qt::WA_TransparentForMouseEvents, true);
		visible->setStyleSheet("QCheckBox::indicator { width: 0px; height: 0px; margin: 0; padding: 0; }");
		h->addWidget(visible);
		ui.visibleCheck = visible;

		// Thumbnail
		auto *thumb = new QLabel(rowFrame);
		thumb->setObjectName(QStringLiteral("sltRowThumbnail"));
		thumb->setFixedSize(32, 32);
		thumb->setScaledContents(true);

		bool hasThumb = false;
		if (!cfg.profile_picture.empty() && !outDir.isEmpty()) {
			const QString imgPath = QDir(outDir).filePath(QString::fromStdString(cfg.profile_picture));
			QPixmap px(imgPath);
			if (!px.isNull()) {
				thumb->setPixmap(px.scaled(32, 32, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
				hasThumb = true;
			}
		}
		thumb->setVisible(hasThumb);
		h->addWidget(thumb);
		ui.thumbnailLbl = thumb;

		// Label
		auto *label = new QLabel(QString::fromStdString(cfg.title), rowFrame);
		label->setObjectName(QStringLiteral("sltRowLabel"));
		label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
		label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		h->addWidget(label, 1);
		ui.labelLbl = label;

		// Buttons
		auto *cloneBtn = new QPushButton(rowFrame);
		auto *settingsBtn = new QPushButton(rowFrame);
		auto *removeBtn = new QPushButton(rowFrame);

		auto *st = rowFrame->style();

		cloneBtn->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy"), st->standardIcon(QStyle::SP_DialogOpenButton)));
		cloneBtn->setToolTip(tr("Clone lower third"));
		cloneBtn->setFlat(true);

		settingsBtn->setIcon(QIcon::fromTheme(QStringLiteral("settings-configure"),
						      st->standardIcon(QStyle::SP_FileDialogInfoView)));
		settingsBtn->setToolTip(tr("Open settings"));
		settingsBtn->setFlat(true);

		removeBtn->setIcon(st->standardIcon(QStyle::SP_DialogCloseButton));
		removeBtn->setToolTip(tr("Remove lower third"));
		removeBtn->setFlat(true);

		cloneBtn->setIconSize(QSize(16, 16));
		settingsBtn->setIconSize(QSize(16, 16));
		removeBtn->setIconSize(QSize(16, 16));

		h->addWidget(cloneBtn);
		h->addWidget(settingsBtn);
		h->addWidget(removeBtn);

		listLayout->addWidget(rowFrame);

		ui.row = rowFrame;
		ui.cloneBtn = cloneBtn;
		ui.settingsBtn = settingsBtn;
		ui.removeBtn = removeBtn;

		const QString id = ui.id;

		// Row click toggles visibility
		rowFrame->installEventFilter(this);
		label->installEventFilter(this);
		thumb->installEventFilter(this);

		connect(cloneBtn, &QPushButton::clicked, this, [this, id]() { handleClone(id); });
		connect(settingsBtn, &QPushButton::clicked, this, [this, id]() { handleOpenSettings(id); });
		connect(removeBtn, &QPushButton::clicked, this, [this, id]() { handleRemove(id); });

		rows.push_back(ui);
	}

	rebuildShortcuts();
	updateRowActiveStyles();
}

void LowerThirdDock::updateRowActiveStyles()
{
	for (auto &row : rows) {
		const bool active = smart_lt::is_visible(row.id.toStdString());

		if (row.row) {
			row.row->setProperty("sltActive", QVariant(active));
			row.row->style()->unpolish(row.row);
			row.row->style()->polish(row.row);
			row.row->update();
		}

		if (row.visibleCheck) {
			row.visibleCheck->blockSignals(true);
			row.visibleCheck->setChecked(active);
			row.visibleCheck->blockSignals(false);
		}
	}
}

bool LowerThirdDock::eventFilter(QObject *watched, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonPress) {
		auto *me = static_cast<QMouseEvent *>(event);
		if (me->button() == Qt::LeftButton) {
			for (auto &r : rows) {
				if (watched == r.row || watched == r.labelLbl || watched == r.thumbnailLbl) {
					handleToggleVisible(r.id);
					return true;
				}
			}
		}
	}
	return QWidget::eventFilter(watched, event);
}

void LowerThirdDock::clearShortcuts()
{
	for (auto *sc : shortcuts_) {
		if (sc)
			sc->deleteLater();
	}
	shortcuts_.clear();
}

void LowerThirdDock::rebuildShortcuts()
{
	clearShortcuts();

	const auto &items = smart_lt::all();
	for (const auto &cfg : items) {
		if (cfg.hotkey.empty())
			continue;

		const QString seqStr = QString::fromStdString(cfg.hotkey).trimmed();
		if (seqStr.isEmpty())
			continue;

		QKeySequence seq(seqStr);
		if (seq.isEmpty())
			continue;

		auto *sc = new QShortcut(seq, this);
		sc->setContext(Qt::ApplicationShortcut);
		shortcuts_.push_back(sc);

		const QString id = QString::fromStdString(cfg.id);
		connect(sc, &QShortcut::activated, this, [this, id]() { handleToggleVisible(id); });
	}
}

void LowerThirdDock::handleToggleVisible(const QString &id)
{
	// Requirement: toggle must NOT rebuild files
	smart_lt::toggle_visible(id.toStdString());
	smart_lt::save_visible_json();

	updateRowActiveStyles();
	emit requestSave();
}

void LowerThirdDock::handleClone(const QString &id)
{
	if (!smart_lt::has_output_dir())
		return;

	// Clone = rebuild trigger handled by core helper
	const std::string newId = smart_lt::clone_lower_third(id.toStdString());
	if (newId.empty())
		return;

	rebuildList();
	emit requestSave();
}

void LowerThirdDock::handleOpenSettings(const QString &id)
{
	smart_lt::ui::LowerThirdSettingsDialog dlg(this);
	dlg.setLowerThirdId(id);

	// Save&Apply rebuild is handled inside settings.cpp/core
	dlg.exec();

	rebuildList();
	emit requestSave();
}

void LowerThirdDock::handleRemove(const QString &id)
{
	if (!smart_lt::has_output_dir())
		return;

	// Remove = rebuild trigger handled by core helper
	smart_lt::remove_lower_third(id.toStdString());

	rebuildList();
	emit requestSave();
}

} // namespace smart_lt::ui

void LowerThird_create_dock()
{
	if (g_dockWidget)
		return;

	auto *panel = new smart_lt::ui::LowerThirdDock(nullptr);
	panel->init();

#if defined(HAVE_OBS_DOCK_BY_ID)
	obs_frontend_add_dock_by_id(sltDockId, sltDockTitle, panel);
#else
	obs_frontend_add_dock(panel);
#endif

	g_dockWidget = panel;
	LOGI("Dock created");
}

void LowerThird_destroy_dock()
{
	if (!g_dockWidget)
		return;

#if defined(HAVE_OBS_DOCK_BY_ID)
	obs_frontend_remove_dock(sltDockId);
#else
	obs_frontend_remove_dock(g_dockWidget);
#endif

	g_dockWidget = nullptr;
	LOGI("Dock destroyed");
}

smart_lt::ui::LowerThirdDock *LowerThird_get_dock()
{
	return qobject_cast<smart_lt::ui::LowerThirdDock *>(g_dockWidget);
}
