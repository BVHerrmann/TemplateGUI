#include "accesscontroloptionpanel.h"

#include "usermanagement.h"
#include "userimportpopupdialog.h"


AccessControlOptionPanel::AccessControlOptionPanel(QWidget *parent) : OptionPanel(parent)
{
	setupUi();
}

void AccessControlOptionPanel::setupUi()
{
	QVBoxLayout* layout = new QVBoxLayout();
	setLayout(layout);

	layout->addSpacing(40);

	QPushButton* importButton = new QPushButton(tr("Import..."));
	connect(importButton, &QPushButton::clicked, this, &AccessControlOptionPanel::importUsers);
	layout->addWidget(importButton);

	QPushButton* exportButton = new QPushButton(tr("Export..."));
	connect(exportButton, &QPushButton::clicked, this, &AccessControlOptionPanel::exportUsers);
	layout->addWidget(exportButton);

	layout->addStretch();
}

void AccessControlOptionPanel::importUsers()
{
	UserImportPopupDialog m(qApp->activeWindow());
	bool ok = m.exec();
	UserManagement::UserImportOption option = m.option();

	if (ok) {
		QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), tr("CSV-File (*.csv)"));
		if (!fileName.isNull()) {
			UserManagement::instance().importUsers(option, fileName);
			emit updatedUsers();
		}
	}
}

void AccessControlOptionPanel::exportUsers()
{
	QSettings settings;

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + tr("User") + "-" + QDateTime::currentDateTime().toString("yyyy-MM-ddThh-mm-ss"), tr("CSV-File (*.csv)"));
	UserManagement::instance().exportUsers(fileName);
}
