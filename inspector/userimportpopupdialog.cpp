#include "userimportpopupdialog.h"

#include <QtWidgets>


UserImportPopupDialog::UserImportPopupDialog(QWidget *parent) : PopupDialog(parent)
{
    setWindowTitle(tr("Import Users"));
    
    QVBoxLayout *layout = new QVBoxLayout();
    centralWidget()->setLayout(layout);
    
	layout->addSpacing(16);
    
	QLabel* label = new QLabel(tr("Replace current users or append users?"));
	layout->addWidget(label);

	layout->addSpacing(16);
    
	//Buttons
	QPushButton* appendButton = new QPushButton(tr("Append"));
	QPushButton* replaceButton = new QPushButton(tr("Replace"));
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, Qt::Horizontal);
	buttonBox->addButton(replaceButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(appendButton, QDialogButtonBox::ActionRole);
	layout->addWidget(buttonBox);

	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(appendButton, &QPushButton::clicked, [&]()
	{
		_option = UserManagement::append;
		accept();
	});
	connect(replaceButton, &QPushButton::clicked, [&]()
	{
		_option = UserManagement::replace;
		accept();
	});
}
