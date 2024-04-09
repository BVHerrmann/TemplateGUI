#include "usercardpopupdialog.h"

#include <QtWidgets>


UserCardPopupDialog::UserCardPopupDialog(QWidget *parent) : PopupDialog(parent)
{
    setWindowTitle(tr("Add Transponder-ID"));
    
    QBoxLayout *box = new QVBoxLayout();
    centralWidget()->setLayout(box);
    
    box->addSpacing(16);
    
	box->addWidget(new QLabel(tr("Please hold the Transponder in front of the reader.")));
	QProgressBar* progressBar = new QProgressBar();
	progressBar->setRange(0, 0);
	progressBar->setAlignment(Qt::AlignHCenter);
	box->addWidget(progressBar);
    box->addSpacing(16);
    
	//Buttons
	QDialogButtonBox* buttonBox = new QDialogButtonBox(
		QDialogButtonBox::Cancel,
		Qt::Horizontal);
	QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	box->addWidget(buttonBox);
}
