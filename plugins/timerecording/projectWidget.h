#pragma once

#include <QWidget>
#include <QPixmap>
#include "etrsplugin.h"
#include "contentboarditem.h"
#include "employee.h"
#include "bmessagebox.h"

class DialogStampIn;
class JobDescriptionWidget;
class ProjectWidget : public QWidget
{
   Q_OBJECT

   public:
        ProjectWidget(QWidget* parent,bool useTestsystem);
	    ~ProjectWidget();
        void updateWorkList(const QString& employeeID, const QString& employeeName);
        void viewWorklist();
        void checkClockStatus(const QString& employeeID, const QString& employeeName);

      public slots:
        void slotItemClicked(QListWidgetItem* item);
        void slotProjectClockIn();
        void slotProjectClockOut();

   private:
        void setupUi();
        ErpClient* _erpClient = nullptr;
        QList<workListItem> _workList;
        QString _employeeID;
        QString _employeeName;
        QListWidget* _listViewWorkList = nullptr;
        DialogStampIn* _dialogProjectInOut = nullptr;
        JobDescriptionWidget* _jobDescriptionWidget = nullptr;
        int _currentWorkListIndex = -1;
};

class ProjectHeaderWidget : public ContentBoardItem
{
    Q_OBJECT

  public:
    ProjectHeaderWidget(QWidget* parent = nullptr);
};

class DialogStampIn : public PopupDialog
{
    Q_OBJECT

  public:
    DialogStampIn(QWidget* parent) : PopupDialog(parent)
    {
        QBoxLayout* Hbox = new QHBoxLayout();
        QBoxLayout* Vbox = new QVBoxLayout();
        centralWidget()->setLayout(Vbox);

        setWindowTitle(tr("Project Stamp In?"));

        QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No);
        // button_box->button(QDialogButtonBox::Yes)->setObjectName(tr("Yes"));
        // button_box->button(QDialogButtonBox::No)->setObjectName(tr("No"));

        button_box->button(QDialogButtonBox::Yes)->setText(tr("Yes"));
        button_box->button(QDialogButtonBox::No)->setText(tr("No"));

        connect(button_box, &QDialogButtonBox::clicked, [=](QAbstractButton* button) {
            switch (button_box->standardButton(button)) {
                case QDialogButtonBox::Yes:
                    accept();
                    this->close();
                    break;
                case QDialogButtonBox::No:
                    reject();
                    this->close();
                    break;
                default:
                    break;
            }
        });

        Vbox->addWidget(button_box);
        Vbox->addLayout(Hbox);
    }
    ~DialogStampIn() {}
};
