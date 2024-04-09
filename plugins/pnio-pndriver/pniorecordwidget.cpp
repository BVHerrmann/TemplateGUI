#include "pniorecordwidget.h"

PNIORecordWidget::PNIORecordWidget(const std::shared_ptr<PNIORecord> &config, QWidget *parent) :
	QWidget(parent)
{
	_label = new QLabel();

	_config = config;

	setupUi();
}

void PNIORecordWidget::setupUi()
{
	QBoxLayout *layout = new QHBoxLayout(this);
	layout->setMargin(layout->margin() / 4);

	layout->addStretch();

	QWidget *widget = contentWidget();
	layout->addWidget(widget);

	setState(false);
}

QWidget * PNIORecordWidget::contentWidget()
{
	return _label;
}

void PNIORecordWidget::setValue(const QVariant & value)
{
	_label->setText(value.toString());
}

void PNIORecordWidget::setState(bool valid)
{
	_label->setEnabled(valid);
}

void PNIORecordWidget::updateUi()
{
	bool changed = false;

	bool valid = _config->valid();
	if (valid != _valid) {
		setState(valid);
		_valid = valid;
		changed = true;
	}

	QVariant value = _config->displayValue();
	if (value != _value) {
		setValue(value);
		_value = value;
		changed = true;
	}

	if (changed)
		QWidget::update();
}
