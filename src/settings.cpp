#define LOG_TAG "[" PLUGIN_NAME "][settings]"
#include "settings.hpp"
#include "core.hpp"

#include <obs.h>
#include <obs-frontend-api.h>

#include <unzip.h>
#include <zip.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QColor>
#include <QFontComboBox>
#include <QFont>
#include <QKeySequenceEdit>
#include <QKeySequence>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QTemporaryDir>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QStyle>

using namespace smart_lt;

static bool unzip_to_dir(const QString &zipPath,
			 const QString &destDir,
			 QString &htmlPath,
			 QString &cssPath,
			 QString &jsonPath,
			 QString &profilePath)
{
	htmlPath.clear();
	cssPath.clear();
	jsonPath.clear();
	profilePath.clear();

	unzFile zip = unzOpen(zipPath.toUtf8().constData());
	if (!zip)
		return false;

	if (unzGoToFirstFile(zip) != UNZ_OK) {
		unzClose(zip);
		return false;
	}

	do {
		char filename[512] = {0};
		unz_file_info info;

		if (unzGetCurrentFileInfo(zip, &info, filename, sizeof(filename), nullptr, 0, nullptr, 0) != UNZ_OK)
			break;

		const QString name = QString::fromUtf8(filename);
		const QString outPath = destDir + "/" + name;

		const QFileInfo fi(outPath);
		QDir().mkpath(fi.path());

		if (unzOpenCurrentFile(zip) != UNZ_OK)
			break;

		QFile out(outPath);
		if (!out.open(QIODevice::WriteOnly)) {
			unzCloseCurrentFile(zip);
			break;
		}

		constexpr int BUF = 8192;
		char buffer[BUF];

		int bytesRead = 0;
		while ((bytesRead = unzReadCurrentFile(zip, buffer, BUF)) > 0) {
			out.write(buffer, bytesRead);
		}

		out.close();
		unzCloseCurrentFile(zip);

		if (name == "template.html")
			htmlPath = outPath;
		else if (name == "template.css")
			cssPath = outPath;
		else if (name == "template.json")
			jsonPath = outPath;
		else if (name.startsWith("profile.") || name.contains("profile", Qt::CaseInsensitive))
			profilePath = outPath;

	} while (unzGoToNextFile(zip) == UNZ_OK);

	unzClose(zip);

	return (!htmlPath.isEmpty() && !cssPath.isEmpty() && !jsonPath.isEmpty());
}

static bool zip_write_file(zipFile zf, const char *internalName, const QByteArray &data)
{
	zip_fileinfo zi;
	memset(&zi, 0, sizeof(zi));

	const int errOpen = zipOpenNewFileInZip(zf,
					       internalName,
					       &zi,
					       nullptr, 0,
					       nullptr, 0,
					       nullptr,
					       Z_DEFLATED,
					       Z_BEST_COMPRESSION);
	if (errOpen != ZIP_OK)
		return false;

	if (!data.isEmpty()) {
		const int errWrite = zipWriteInFileInZip(zf, data.constData(), data.size());
		if (errWrite != ZIP_OK) {
			zipCloseFileInZip(zf);
			return false;
		}
	}

	zipCloseFileInZip(zf);
	return true;
}

LowerThirdSettingsDialog::LowerThirdSettingsDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(tr("Lower Third Settings"));
	resize(720, 620);

	auto *root = new QVBoxLayout(this);

	{
		auto *contentBox = new QGroupBox(tr("Content && Media"), this);
		auto *contentLayout = new QGridLayout(contentBox);

		auto *titleLabel = new QLabel(tr("Title:"), this);
		titleEdit = new QLineEdit(this);

		auto *subtitleLabel = new QLabel(tr("Subtitle:"), this);
		subtitleEdit = new QLineEdit(this);

		auto *picLabel = new QLabel(tr("Profile picture:"), this);
		auto *picRow = new QHBoxLayout();
		profilePictureEdit = new QLineEdit(this);
		profilePictureEdit->setReadOnly(true);

		browseProfilePictureBtn = new QPushButton(this);
		browseProfilePictureBtn->setCursor(Qt::PointingHandCursor);
		browseProfilePictureBtn->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
		browseProfilePictureBtn->setToolTip(tr("Browse profile picture..."));

		picRow->addWidget(profilePictureEdit, 1);
		picRow->addWidget(browseProfilePictureBtn);

		int row = 0;
		contentLayout->addWidget(titleLabel, row, 0);
		contentLayout->addWidget(titleEdit, row, 1, 1, 3);

		row++;
		contentLayout->addWidget(subtitleLabel, row, 0);
		contentLayout->addWidget(subtitleEdit, row, 1, 1, 3);

		row++;
		contentLayout->addWidget(picLabel, row, 0);
		contentLayout->addLayout(picRow, row, 1, 1, 3);

		contentLayout->setColumnStretch(1, 1);
		root->addWidget(contentBox);

		connect(browseProfilePictureBtn, &QPushButton::clicked, this, &LowerThirdSettingsDialog::onBrowseProfilePicture);
	}

	{
		auto *styleBox = new QGroupBox(tr("Style"), this);
		auto *styleGrid = new QGridLayout(styleBox);

		auto *lblIn = new QLabel(tr("Anim In:"), this);
		animInCombo = new QComboBox(this);
		for (const auto &opt : AnimInOptions)
			animInCombo->addItem(tr(opt.label), QString::fromUtf8(opt.value));

		auto *lblOut = new QLabel(tr("Anim Out:"), this);
		animOutCombo = new QComboBox(this);
		for (const auto &opt : AnimOutOptions)
			animOutCombo->addItem(tr(opt.label), QString::fromUtf8(opt.value));

		auto *lblFont = new QLabel(tr("Font:"), this);
		fontCombo = new QFontComboBox(this);
		fontCombo->setEditable(false);

		int row = 0;
		styleGrid->addWidget(lblIn, row, 0);
		styleGrid->addWidget(animInCombo, row, 1);
		styleGrid->addWidget(lblOut, row, 2);
		styleGrid->addWidget(animOutCombo, row, 3);

		row++;
		styleGrid->addWidget(lblFont, row, 0);
		styleGrid->addWidget(fontCombo, row, 1);

		auto *lblPos = new QLabel(tr("Position:"), this);
		ltPosCombo = new QComboBox(this);
		for (const auto &opt : LtPositionOptions)
			ltPosCombo->addItem(tr(opt.label), QString::fromUtf8(opt.value));

		styleGrid->addWidget(lblPos, row, 2);
		styleGrid->addWidget(ltPosCombo, row, 3);

		row++;
		customAnimInLabel = new QLabel(tr("Custom In class:"), this);
		customAnimInEdit = new QLineEdit(this);
		customAnimInEdit->setPlaceholderText(tr("e.g. myFadeIn"));

		customAnimOutLabel = new QLabel(tr("Custom Out class:"), this);
		customAnimOutEdit = new QLineEdit(this);
		customAnimOutEdit->setPlaceholderText(tr("e.g. myFadeOut"));

		styleGrid->addWidget(customAnimInLabel, row, 0);
		styleGrid->addWidget(customAnimInEdit, row, 1);
		styleGrid->addWidget(customAnimOutLabel, row, 2);
		styleGrid->addWidget(customAnimOutEdit, row, 3);

		row++;
		auto *lblBg = new QLabel(tr("Background:"), this);
		bgColorBtn = new QPushButton(tr("Pick"), this);

		auto *lblText = new QLabel(tr("Text color:"), this);
		textColorBtn = new QPushButton(tr("Pick"), this);

		styleGrid->addWidget(lblBg, row, 0);
		styleGrid->addWidget(bgColorBtn, row, 1);
		styleGrid->addWidget(lblText, row, 2);
		styleGrid->addWidget(textColorBtn, row, 3);

		root->addWidget(styleBox);

		connect(bgColorBtn, &QPushButton::clicked, this, &LowerThirdSettingsDialog::onPickBgColor);
		connect(textColorBtn, &QPushButton::clicked, this, &LowerThirdSettingsDialog::onPickTextColor);
		connect(ltPosCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LowerThirdSettingsDialog::onLtPosChanged);
	}

	{
		auto *behaviorBox = new QGroupBox(tr("Behavior"), this);
		auto *behaviorGrid = new QGridLayout(behaviorBox);

		auto *hotkeyLabel = new QLabel(tr("Hotkey:"), this);
		hotkeyEdit = new QKeySequenceEdit(this);

		clearHotkeyBtn = new QPushButton(this);
		clearHotkeyBtn->setCursor(Qt::PointingHandCursor);
		clearHotkeyBtn->setFlat(true);
		clearHotkeyBtn->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
		clearHotkeyBtn->setToolTip(tr("Clear hotkey"));
		clearHotkeyBtn->setFocusPolicy(Qt::NoFocus);

		auto *hotkeyRowLayout = new QHBoxLayout();
		hotkeyRowLayout->addWidget(hotkeyEdit, 1);
		hotkeyRowLayout->addWidget(clearHotkeyBtn);

		behaviorGrid->addWidget(hotkeyLabel, 0, 0);
		behaviorGrid->addLayout(hotkeyRowLayout, 0, 1);

		root->addWidget(behaviorBox);

		connect(clearHotkeyBtn, &QPushButton::clicked, this, [this]() {
			hotkeyEdit->setKeySequence(QKeySequence());
			onHotkeyChanged(hotkeyEdit->keySequence());
		});
	}

	{
		auto *tplRow = new QHBoxLayout();

		auto *htmlCard = new QGroupBox(tr("HTML Template"), this);
		auto *htmlLayout = new QVBoxLayout(htmlCard);

		auto *htmlHeaderRow = new QHBoxLayout();
		expandHtmlBtn = new QPushButton(this);
		expandHtmlBtn->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
		expandHtmlBtn->setCursor(Qt::PointingHandCursor);
		expandHtmlBtn->setToolTip(tr("Open HTML editor..."));
		expandHtmlBtn->setFlat(true);

		htmlHeaderRow->addStretch(1);
		htmlHeaderRow->addWidget(expandHtmlBtn);
		htmlLayout->addLayout(htmlHeaderRow);

		htmlEdit = new QPlainTextEdit(this);
		htmlLayout->addWidget(htmlEdit, 1);

		connect(expandHtmlBtn, &QPushButton::clicked, this, &LowerThirdSettingsDialog::onOpenHtmlEditorDialog);

		tplRow->addWidget(htmlCard, 1);

		auto *cssCard = new QGroupBox(tr("CSS Template"), this);
		auto *cssLayout = new QVBoxLayout(cssCard);

		auto *cssHeaderRow = new QHBoxLayout();
		expandCssBtn = new QPushButton(this);
		expandCssBtn->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
		expandCssBtn->setCursor(Qt::PointingHandCursor);
		expandCssBtn->setToolTip(tr("Open CSS editor..."));
		expandCssBtn->setFlat(true);

		cssHeaderRow->addStretch(1);
		cssHeaderRow->addWidget(expandCssBtn);
		cssLayout->addLayout(cssHeaderRow);

		cssEdit = new QPlainTextEdit(this);
		cssLayout->addWidget(cssEdit, 1);

		connect(expandCssBtn, &QPushButton::clicked, this, &LowerThirdSettingsDialog::onOpenCssEditorDialog);

		tplRow->addWidget(cssCard, 1);

		root->addLayout(tplRow, 1);
	}

	{
		buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
		auto *applyBtn = buttonBox->addButton(tr("Save && Apply"), QDialogButtonBox::AcceptRole);

		connect(buttonBox, &QDialogButtonBox::accepted, this, &LowerThirdSettingsDialog::onSaveAndApply);
		connect(buttonBox, &QDialogButtonBox::rejected, this, &LowerThirdSettingsDialog::reject);
		connect(applyBtn, &QPushButton::clicked, this, &LowerThirdSettingsDialog::onSaveAndApply);

		auto *bottomRow = new QHBoxLayout();

		importBtn = new QPushButton(this);
		importBtn->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
		importBtn->setCursor(Qt::PointingHandCursor);
		importBtn->setToolTip(tr("Import template from ZIP..."));

		exportBtn = new QPushButton(this);
		exportBtn->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
		exportBtn->setCursor(Qt::PointingHandCursor);
		exportBtn->setToolTip(tr("Export template to ZIP..."));

		bottomRow->addWidget(importBtn);
		bottomRow->addWidget(exportBtn);
		bottomRow->addStretch(1);
		bottomRow->addWidget(buttonBox);

		root->addLayout(bottomRow);

		connect(importBtn, &QPushButton::clicked, this, &LowerThirdSettingsDialog::onImportTemplateClicked);
		connect(exportBtn, &QPushButton::clicked, this, &LowerThirdSettingsDialog::onExportTemplateClicked);
	}

	connect(animInCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LowerThirdSettingsDialog::onAnimInChanged);
	connect(animOutCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LowerThirdSettingsDialog::onAnimOutChanged);

	connect(customAnimInEdit, &QLineEdit::textChanged, this, [this](const QString &) { updateCustomAnimFieldsVisibility(); });
	connect(customAnimOutEdit, &QLineEdit::textChanged, this, [this](const QString &) { updateCustomAnimFieldsVisibility(); });

	connect(fontCombo, &QFontComboBox::currentFontChanged, this, &LowerThirdSettingsDialog::onFontChanged);
	connect(hotkeyEdit, &QKeySequenceEdit::keySequenceChanged, this, &LowerThirdSettingsDialog::onHotkeyChanged);

	updateCustomAnimFieldsVisibility();
}

LowerThirdSettingsDialog::~LowerThirdSettingsDialog()
{
	delete currentBgColor;
	delete currentTextColor;
}

void LowerThirdSettingsDialog::setLowerThirdId(const QString &id)
{
	currentId = id;
	loadFromState();
}

void LowerThirdSettingsDialog::loadFromState()
{
	if (currentId.isEmpty())
		return;

	auto *cfg = smart_lt::get_by_id(currentId.toStdString());
	if (!cfg)
		return;

	titleEdit->setText(QString::fromStdString(cfg->title));
	subtitleEdit->setText(QString::fromStdString(cfg->subtitle));

	{
		const QString v = QString::fromStdString(cfg->anim_in);
		const int idx = animInCombo->findData(v);
		animInCombo->setCurrentIndex(idx >= 0 ? idx : 0);
	}
	{
		const QString v = QString::fromStdString(cfg->anim_out);
		const int idx = animOutCombo->findData(v);
		animOutCombo->setCurrentIndex(idx >= 0 ? idx : 0);
	}

	customAnimInEdit->setText(QString::fromStdString(cfg->custom_anim_in));
	customAnimOutEdit->setText(QString::fromStdString(cfg->custom_anim_out));
	updateCustomAnimFieldsVisibility();

	if (!cfg->font_family.empty())
		fontCombo->setCurrentFont(QFont(QString::fromStdString(cfg->font_family)));

	hotkeyEdit->setKeySequence(QKeySequence(QString::fromStdString(cfg->hotkey)));

	htmlEdit->setPlainText(QString::fromStdString(cfg->html_template));
	cssEdit->setPlainText(QString::fromStdString(cfg->css_template));

	delete currentBgColor;
	delete currentTextColor;
	currentBgColor = nullptr;
	currentTextColor = nullptr;

	QColor bg(QString::fromStdString(cfg->bg_color));
	QColor fg(QString::fromStdString(cfg->text_color));
	if (!bg.isValid())
		bg = QColor(17, 24, 39);
	if (!fg.isValid())
		fg = QColor(249, 250, 251);

	currentBgColor = new QColor(bg);
	currentTextColor = new QColor(fg);

	updateColorButton(bgColorBtn, *currentBgColor);
	updateColorButton(textColorBtn, *currentTextColor);

	{
		const QString v = QString::fromStdString(cfg->lt_position);
		const int idx = ltPosCombo->findData(v);
		ltPosCombo->setCurrentIndex(idx >= 0 ? idx : 0);
	}

	if (cfg->profile_picture.empty())
		profilePictureEdit->clear();
	else
		profilePictureEdit->setText(QString::fromStdString(cfg->profile_picture));

	pendingProfilePicturePath.clear();
}

void LowerThirdSettingsDialog::saveToState()
{
	if (currentId.isEmpty())
		return;

	auto *cfg = smart_lt::get_by_id(currentId.toStdString());
	if (!cfg)
		return;

	cfg->title = titleEdit->text().toStdString();
	cfg->subtitle = subtitleEdit->text().toStdString();

	cfg->anim_in = animInCombo->currentData().toString().toStdString();
	cfg->anim_out = animOutCombo->currentData().toString().toStdString();
	cfg->custom_anim_in = customAnimInEdit->text().toStdString();
	cfg->custom_anim_out = customAnimOutEdit->text().toStdString();

	cfg->font_family = fontCombo->currentFont().family().toStdString();
	cfg->hotkey = hotkeyEdit->keySequence().toString(QKeySequence::PortableText).toStdString();

	cfg->lt_position = ltPosCombo->currentData().toString().toStdString();

	if (currentBgColor)
		cfg->bg_color = currentBgColor->name(QColor::HexRgb).toStdString();
	if (currentTextColor)
		cfg->text_color = currentTextColor->name(QColor::HexRgb).toStdString();

	cfg->html_template = htmlEdit->toPlainText().toStdString();
	cfg->css_template = cssEdit->toPlainText().toStdString();

	if (!pendingProfilePicturePath.isEmpty() && smart_lt::has_output_dir()) {
		const QString outDir = QString::fromStdString(smart_lt::output_dir());
		QDir dir(outDir);

		if (!cfg->profile_picture.empty()) {
			const QString oldPath = dir.filePath(QString::fromStdString(cfg->profile_picture));
			if (QFile::exists(oldPath))
				QFile::remove(oldPath);
		}

		const QFileInfo fi(pendingProfilePicturePath);
		const QString ext = fi.suffix().toLower();
		const qint64 ts = QDateTime::currentSecsSinceEpoch();

		QString newFileName = QString("%1_%2").arg(QString::fromStdString(cfg->id)).arg(ts);
		if (!ext.isEmpty())
			newFileName += "." + ext;

		const QString destPath = dir.filePath(newFileName);

		if (QFile::copy(pendingProfilePicturePath, destPath)) {
			cfg->profile_picture = newFileName.toStdString();
			profilePictureEdit->setText(newFileName);
		} else {
			LOGW("Failed to copy profile picture '%s' -> '%s'",
			     pendingProfilePicturePath.toUtf8().constData(), destPath.toUtf8().constData());
		}

		pendingProfilePicturePath.clear();
	}

	smart_lt::apply_changes(smart_lt::ApplyMode::JsonOnly);
}

void LowerThirdSettingsDialog::onPickBgColor()
{
	QColor start = currentBgColor ? *currentBgColor : QColor(17, 24, 39);
	QColor c = QColorDialog::getColor(start, this, tr("Pick background color"));
	if (!c.isValid())
		return;

	if (!currentBgColor)
		currentBgColor = new QColor(c);
	else
		*currentBgColor = c;

	updateColorButton(bgColorBtn, c);
}

void LowerThirdSettingsDialog::onPickTextColor()
{
	QColor start = currentTextColor ? *currentTextColor : QColor(249, 250, 251);
	QColor c = QColorDialog::getColor(start, this, tr("Pick text color"));
	if (!c.isValid())
		return;

	if (!currentTextColor)
		currentTextColor = new QColor(c);
	else
		*currentTextColor = c;

	updateColorButton(textColorBtn, c);
}

void LowerThirdSettingsDialog::onBrowseProfilePicture()
{
	const QString filter = tr("Images (*.png *.jpg *.jpeg *.webp *.gif);;All Files (*.*)");
	const QString file = QFileDialog::getOpenFileName(this, tr("Select profile picture"), QString(), filter);
	if (file.isEmpty())
		return;

	pendingProfilePicturePath = file;
	profilePictureEdit->setText(file);
}

void LowerThirdSettingsDialog::onSaveAndApply()
{
	saveToState();
	accept();
}

void LowerThirdSettingsDialog::onExportTemplateClicked()
{
	if (currentId.isEmpty()) {
		QMessageBox::warning(this, tr("Export"), tr("No lower third selected to export."));
		return;
	}

	saveToState();

	auto *cfg = smart_lt::get_by_id(currentId.toStdString());
	if (!cfg) {
		QMessageBox::warning(this, tr("Export"), tr("Cannot find current lower third configuration."));
		return;
	}

	const QString suggestedName = QString::fromStdString(cfg->id) + "-template.zip";
	const QString zipPath = QFileDialog::getSaveFileName(this,
							    tr("Export Template"),
							    suggestedName,
							    tr("Template ZIP (*.zip)"));

	if (zipPath.isEmpty())
		return;

	zipFile zf = zipOpen(zipPath.toUtf8().constData(), 0);
	if (!zf) {
		QMessageBox::warning(this, tr("Export"), tr("Failed to create ZIP file."));
		return;
	}

	QJsonObject obj;
	obj["id"] = QString::fromStdString(cfg->id);
	obj["title"] = QString::fromStdString(cfg->title);
	obj["subtitle"] = QString::fromStdString(cfg->subtitle);
	obj["anim_in"] = QString::fromStdString(cfg->anim_in);
	obj["anim_out"] = QString::fromStdString(cfg->anim_out);
	obj["custom_anim_in"] = QString::fromStdString(cfg->custom_anim_in);
	obj["custom_anim_out"] = QString::fromStdString(cfg->custom_anim_out);
	obj["font_family"] = QString::fromStdString(cfg->font_family);
	obj["lt_position"] = QString::fromStdString(cfg->lt_position);
	obj["bg_color"] = QString::fromStdString(cfg->bg_color);
	obj["text_color"] = QString::fromStdString(cfg->text_color);
	obj["visible"] = cfg->visible;
	obj["hotkey"] = QString::fromStdString(cfg->hotkey);
	obj["profile_picture"] = QString::fromStdString(cfg->profile_picture);

	const QJsonDocument jsonDoc(obj);
	const QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::Indented);
	if (!zip_write_file(zf, "template.json", jsonBytes)) {
		zipClose(zf, nullptr);
		QMessageBox::warning(this, tr("Export"), tr("Failed to write template.json to ZIP."));
		return;
	}

	const QByteArray htmlBytes = QString::fromStdString(cfg->html_template).toUtf8();
	if (!zip_write_file(zf, "template.html", htmlBytes)) {
		zipClose(zf, nullptr);
		QMessageBox::warning(this, tr("Export"), tr("Failed to write template.html to ZIP."));
		return;
	}

	const QByteArray cssBytes = QString::fromStdString(cfg->css_template).toUtf8();
	if (!zip_write_file(zf, "template.css", cssBytes)) {
		zipClose(zf, nullptr);
		QMessageBox::warning(this, tr("Export"), tr("Failed to write template.css to ZIP."));
		return;
	}

	if (!cfg->profile_picture.empty() && smart_lt::has_output_dir()) {
		const QString outDir = QString::fromStdString(smart_lt::output_dir());
		QDir dir(outDir);

		const QString picFileName = QString::fromStdString(cfg->profile_picture);
		const QString srcPath = dir.filePath(picFileName);

		if (QFile::exists(srcPath)) {
			QFile picFile(srcPath);
			if (picFile.open(QIODevice::ReadOnly)) {
				const QByteArray picBytes = picFile.readAll();
				const QString ext = QFileInfo(picFileName).suffix().toLower();
				const QString internalName = ext.isEmpty()
					? QStringLiteral("profile")
					: QStringLiteral("profile.%1").arg(ext);

				if (!zip_write_file(zf, internalName.toUtf8().constData(), picBytes)) {
					zipClose(zf, nullptr);
					QMessageBox::warning(this, tr("Export"), tr("Failed to add profile picture to ZIP."));
					return;
				}
			}
		}
	}

	zipClose(zf, nullptr);
	QMessageBox::information(this, tr("Export"), tr("Template exported successfully."));
}

void LowerThirdSettingsDialog::onImportTemplateClicked()
{
	const QString zipPath = QFileDialog::getOpenFileName(this,
							    tr("Select template ZIP"),
							    QString(),
							    tr("Template ZIP (*.zip)"));

	if (zipPath.isEmpty())
		return;

	if (currentId.isEmpty()) {
		QMessageBox::warning(this, tr("Import"), tr("No lower third selected to import into."));
		return;
	}

	QTemporaryDir tempDir;
	if (!tempDir.isValid()) {
		QMessageBox::warning(this, tr("Error"), tr("Unable to create temp directory."));
		return;
	}

	QString htmlPath, cssPath, jsonPath, profilePicPath;
	if (!unzip_to_dir(zipPath, tempDir.path(), htmlPath, cssPath, jsonPath, profilePicPath)) {
		QMessageBox::warning(this, tr("Error"),
				     tr("ZIP must contain template.html, template.css and template.json."));
		return;
	}

	QFile jsonFile(jsonPath);
	if (!jsonFile.open(QIODevice::ReadOnly)) {
		QMessageBox::warning(this, tr("Error"), tr("Cannot read template.json."));
		return;
	}

	QJsonParseError err;
	const QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll(), &err);
	if (err.error != QJsonParseError::NoError || !doc.isObject()) {
		QMessageBox::warning(this, tr("Error"), tr("Invalid JSON file."));
		return;
	}

	auto *cfg = smart_lt::get_by_id(currentId.toStdString());
	if (!cfg)
		return;

	const QJsonObject obj = doc.object();

	cfg->title = obj.value("title").toString().toStdString();
	cfg->subtitle = obj.value("subtitle").toString().toStdString();
	cfg->anim_in = obj.value("anim_in").toString().toStdString();
	cfg->anim_out = obj.value("anim_out").toString().toStdString();
	cfg->custom_anim_in = obj.value("custom_anim_in").toString().toStdString();
	cfg->custom_anim_out = obj.value("custom_anim_out").toString().toStdString();
	cfg->font_family = obj.value("font_family").toString().toStdString();
	cfg->lt_position = obj.value("lt_position").toString().toStdString();
	cfg->bg_color = obj.value("bg_color").toString().toStdString();
	cfg->text_color = obj.value("text_color").toString().toStdString();
	cfg->visible = obj.value("visible").toBool(false);
	cfg->hotkey = obj.value("hotkey").toString().toStdString();

	{
		QFile f1(htmlPath);
		if (f1.open(QIODevice::ReadOnly))
			cfg->html_template = QString::fromUtf8(f1.readAll()).toStdString();
	}
	{
		QFile f2(cssPath);
		if (f2.open(QIODevice::ReadOnly))
			cfg->css_template = QString::fromUtf8(f2.readAll()).toStdString();
	}

	if (!profilePicPath.isEmpty() && smart_lt::has_output_dir()) {
		const QString outDir = QString::fromStdString(smart_lt::output_dir());
		if (!outDir.isEmpty()) {
			QDir dir(outDir);

			if (!cfg->profile_picture.empty()) {
				const QString oldPath = dir.filePath(QString::fromStdString(cfg->profile_picture));
				if (QFile::exists(oldPath))
					QFile::remove(oldPath);
			}

			const QString ext = QFileInfo(profilePicPath).suffix().toLower();
			const QString newName = ext.isEmpty()
				? QString("%1_profile").arg(QString::fromStdString(cfg->id))
				: QString("%1_profile.%2").arg(QString::fromStdString(cfg->id)).arg(ext);

			const QString dest = dir.filePath(newName);
			QFile::remove(dest);

			if (QFile::copy(profilePicPath, dest)) {
				cfg->profile_picture = newName.toStdString();
			}
		}
	}

	smart_lt::apply_changes(smart_lt::ApplyMode::JsonOnly);
	loadFromState();
	QMessageBox::information(this, tr("Imported"), tr("Template imported successfully."));
}

void LowerThirdSettingsDialog::openTemplateEditorDialog(const QString &title, QPlainTextEdit *sourceEdit)
{
	if (!sourceEdit)
		return;

	QDialog dlg(this);
	dlg.setWindowTitle(title);
	dlg.resize(900, 700);

	auto *layout = new QVBoxLayout(&dlg);
	auto *bigEdit = new QPlainTextEdit(&dlg);

	bigEdit->setPlainText(sourceEdit->toPlainText());
	bigEdit->setLineWrapMode(QPlainTextEdit::NoWrap);

	QFont monoFont = bigEdit->font();
#if defined(Q_OS_WIN)
	monoFont.setFamily(QStringLiteral("Consolas"));
#elif defined(Q_OS_MACOS)
	monoFont.setFamily(QStringLiteral("Menlo"));
#else
	monoFont.setFamily(QStringLiteral("Monospace"));
#endif
	bigEdit->setFont(monoFont);

	auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dlg);

	layout->addWidget(bigEdit, 1);
	layout->addWidget(box);

	connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
	connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

	if (dlg.exec() == QDialog::Accepted)
		sourceEdit->setPlainText(bigEdit->toPlainText());
}

void LowerThirdSettingsDialog::onOpenHtmlEditorDialog()
{
	openTemplateEditorDialog(tr("Edit HTML Template"), htmlEdit);
}

void LowerThirdSettingsDialog::onOpenCssEditorDialog()
{
	openTemplateEditorDialog(tr("Edit CSS Template"), cssEdit);
}

void LowerThirdSettingsDialog::updateCustomAnimFieldsVisibility()
{
	const QString customKey = QStringLiteral("custom");

	const bool inCustom = (animInCombo->currentData().toString() == customKey);
	const bool outCustom = (animOutCombo->currentData().toString() == customKey);

	customAnimInLabel->setVisible(inCustom);
	customAnimInEdit->setVisible(inCustom);
	customAnimOutLabel->setVisible(outCustom);
	customAnimOutEdit->setVisible(outCustom);
}

void LowerThirdSettingsDialog::onAnimInChanged(int)
{
	updateCustomAnimFieldsVisibility();
}

void LowerThirdSettingsDialog::onAnimOutChanged(int)
{
	updateCustomAnimFieldsVisibility();
}

void LowerThirdSettingsDialog::onFontChanged(const QFont &)
{
	// Font change is persisted on Save; no immediate apply here.
	// If you want immediate apply w/out rev bump, uncomment:
	// saveToState();
}

void LowerThirdSettingsDialog::onHotkeyChanged(const QKeySequence &)
{
	// Hotkey change is persisted on Save; no immediate apply here.
	// If you want immediate apply w/out rev bump, uncomment:
	// saveToState();
}

void LowerThirdSettingsDialog::onLtPosChanged(int)
{
	// Position change is persisted on Save; no immediate apply here.
	// If you want immediate apply w/out rev bump, uncomment:
	// saveToState();
}

void LowerThirdSettingsDialog::updateColorButton(QPushButton *btn, const QColor &color)
{
	const QString hex = color.name(QColor::HexRgb);
	btn->setStyleSheet(QString("background-color:%1;border:1px solid #333;min-width:64px;padding:2px 4px;").arg(hex));
	btn->setText(hex);
}
