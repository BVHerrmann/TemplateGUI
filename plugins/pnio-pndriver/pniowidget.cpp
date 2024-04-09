#include "pniowidget.h"

#include "pnioplugin.h"
#include "pniostatewidget.h"
#include "pniobitstatewidget.h"
#include "pniotextwidget.h"
#include "pniorecordwidget.h"


PNIOWidget::PNIOWidget(PNIOPlugin *plugin, QWidget *parent) :
    QWidget(parent)
{
    _plugin = plugin;

    setupUi();

    _update_timer = new QTimer(this);
    _update_timer->start(100);
}

PNIOWidget::~PNIOWidget()
{
    _update_timer->stop();
}

void PNIOWidget::setupUi()
{
    QBoxLayout *layout = new QHBoxLayout();
    layout->setAlignment(Qt::AlignTop);
    setLayout(layout);

    QGroupBox *in_box = new QGroupBox(tr("Input"));
    QPalette pal_in = in_box->palette();
    pal_in.setColor(QPalette::Window, Qt::transparent);
    in_box->setPalette(pal_in);
    layout->addWidget(in_box);

    QScrollArea *in_area = new QScrollArea();
    in_area->setWidgetResizable(true);
    in_area->setFrameStyle(QFrame::Plain);
    in_box->setLayout(new QVBoxLayout());
    in_box->layout()->setContentsMargins(0, 0, 0, 0);
    in_box->layout()->addWidget(in_area);

    QWidget *in_widget = new QWidget();
    in_area->setWidget(in_widget);

    _in_layout = new QFormLayout();
    _in_layout->setLabelAlignment(Qt::AlignVCenter);
    _in_layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    _in_layout->setVerticalSpacing(0);
    in_widget->setLayout(_in_layout);


    QGroupBox *out_box = new QGroupBox(tr("Output"));
    QPalette pal_out = out_box->palette();
    pal_out.setColor(QPalette::Window, Qt::transparent);
    out_box->setPalette(pal_out);
    layout->addWidget(out_box);

    QScrollArea *out_area = new QScrollArea();
    out_area->setWidgetResizable(true);
    out_area->setFrameStyle(QFrame::Plain);
    out_box->setLayout(new QVBoxLayout());
    out_box->layout()->setContentsMargins(0, 0, 0, 0);
    out_box->layout()->addWidget(out_area);

    QWidget *out_widget = new QWidget();
    out_area->setWidget(out_widget);

    _out_layout = new QFormLayout();
    _out_layout->setLabelAlignment(Qt::AlignVCenter);
    _out_layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    _out_layout->setVerticalSpacing(0);
    out_widget->setLayout(_out_layout);
}

void PNIOWidget::setupWidget(PNIOInterface *pnio_interface)
{
    int last_in_slot = -1;
    for (const std::shared_ptr<PNIOInputValue> &config : pnio_interface->getInConfig()) {
        if ((int) config->slot() != last_in_slot) {
            if (last_in_slot != -1) {
                QFrame *line = new QFrame();
                line->setFrameShape(QFrame::HLine);
                line->setFrameShadow(QFrame::Sunken);
                line->setFixedHeight(20);
                line->setContentsMargins(0, 0, 0, 0);
                _in_layout->addRow(line);
            }
            last_in_slot = config->slot();
        }

        PNIOStateWidget *widget = newWidgetForConfig(config);
        _in_layout->addRow(config->name(), widget);
    }

    int last_out_slot = -1;
    for (const std::shared_ptr<PNIOOutputValue> &config : pnio_interface->getOutConfig()) {
        if ((int) config->slot() != last_out_slot) {
            if (last_out_slot != -1) {
                QFrame *line = new QFrame();
                line->setFrameShape(QFrame::HLine);
                line->setFrameShadow(QFrame::Sunken);
                line->setFixedHeight(20);
                line->setContentsMargins(0, 0, 0, 0);
                _out_layout->addRow(line);
            }
            last_out_slot = config->slot();
        }

        PNIOStateWidget *widget = newWidgetForConfig(config);
        _out_layout->addRow(config->name(), widget);
    }

	int last_read_slot = -1;
	addLine(_in_layout);
	for (const std::shared_ptr<PNIOReadRecord> &config : pnio_interface->getReadRecordConfig()) {
		if ((int)config->slot() != last_read_slot) {
			if (last_read_slot != -1) {
				addLine(_in_layout);
			}
			last_read_slot = config->slot();
		}

		PNIORecordWidget *widget = newWidgetForRecordConfig(config);
		_in_layout->addRow(config->name(), widget);
	}

	int last_write_slot = -1;
	addLine(_out_layout);
	for (const std::shared_ptr<PNIOWriteRecord> &config : pnio_interface->getWriteRecordConfig()) {
		if ((int)config->slot() != last_write_slot) {
			if (last_write_slot != -1) {
				addLine(_out_layout);
			}
			last_write_slot = config->slot();
		}

		PNIORecordWidget *widget = newWidgetForRecordConfig(config);
		_out_layout->addRow(config->name(), widget);
	}
}

PNIOStateWidget * PNIOWidget::newWidgetForConfig(const std::shared_ptr<PNIOValue> &config)
{
    PNIOStateWidget *widget = 0;
    
    switch(config->datatype()) {
    case QVariant::Bool:
        widget = new PNIOBitStateWidget(config);
        break;
    default:
        widget = new PNIOTextWidget(config);
        break;
    }
    
    connect(_update_timer, &QTimer::timeout, widget, &PNIOStateWidget::updateUi);

    return widget;
}

PNIORecordWidget* PNIOWidget::newWidgetForRecordConfig(const std::shared_ptr<PNIORecord> &config)
{
	PNIORecordWidget *widget = new PNIORecordWidget(config);
	connect(_update_timer, &QTimer::timeout, widget, &PNIORecordWidget::updateUi);

	return widget;
}

void PNIOWidget::addLine(QFormLayout * layout)
{
	QFrame *line = new QFrame();
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	line->setFixedHeight(20);
	line->setContentsMargins(0, 0, 0, 0);
	layout->addRow(line);
}
