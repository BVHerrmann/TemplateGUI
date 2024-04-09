#ifndef PNIORECORDWIDGET_H
#define PNIORECORDWIDGET_H

#include <QtCore>
#include <QtWidgets>

#include <logic.h>
#include <pniovalue.h>


class PNIORecordWidget : public QWidget
{
	Q_OBJECT
public:
	explicit PNIORecordWidget(const std::shared_ptr<PNIORecord> &config, QWidget *parent = nullptr);

signals:
	// CommunicationInterface
	void valueChanged(const QString &name, const QVariant &value);

public slots:
	void updateUi();

private:
	void setupUi();

	QWidget * contentWidget();
	void setValue(const QVariant &value);
	void setState(bool valid);

	QLabel *_label;

	bool _valid = false;
	QVariant _value;

	std::shared_ptr<PNIORecord> _config;
};

#endif // PNIORECORDWIDGET_H
