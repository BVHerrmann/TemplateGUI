#include "mainwidget.h"


MainWidget::MainWidget(QWidget *contentWidget, OptionPanel *optionPanel, QWidget *parent) : QWidget(parent)
{
    _content_widget = contentWidget;
    _option_panel = optionPanel;
    
    setupUi();
}

void MainWidget::setupUi()
{
    _stacked_layout = new QStackedLayout();
    _stacked_layout->setStackingMode(QStackedLayout::StackAll);
    
    _main_layout = new QHBoxLayout(this);
    _main_layout->setSpacing(0);
    _main_layout->setContentsMargins(0, 0, 0, 0);
    
    _showOptionPanelButton = new QPushButton("G");
    _showOptionPanelButton->setObjectName("ShowOptionPanel");
    
    if (_content_widget) {
        _main_layout->addWidget(_content_widget);
    }
    _main_layout->addWidget(_showOptionPanelButton);
    
    // main widget
    _main_widget = new QWidget(this);
    _main_widget->setLayout(_main_layout);
    _stacked_layout->addWidget(_main_widget);
    
    _option_widget = new QWidget(this);
    _option_layout = new QHBoxLayout();
    
    _option_widget->setLayout(_option_layout);
    _option_layout->setSpacing(0);
    _option_layout->setContentsMargins(0, 0, 0, 0);
    
    QPushButton *bar_widget = new QPushButton(QChar(0x203A));
    bar_widget->setObjectName("HideOptionPanel");
    
    QPushButton *close_option_panel_button = new QPushButton();
    close_option_panel_button->setObjectName("invisible");
    connect(close_option_panel_button, &QPushButton::clicked, this, &MainWidget::closeOptionPanel );
    _option_layout->addWidget(close_option_panel_button);
    
    _option_layout->addWidget(bar_widget);
    if (_option_panel) {
        _option_layout->addWidget(_option_panel);
    }
    
    _option_widget->setVisible(false);
    _stacked_layout->addWidget(_option_widget);
    
    connect(_showOptionPanelButton, &QPushButton::clicked, this, &MainWidget::openOptionPanel );
    connect(bar_widget, &QPushButton::clicked, this, &MainWidget::closeOptionPanel );
    
    setLayout(_stacked_layout);
}

void MainWidget::openOptionPanel()
{
    _stacked_layout->setCurrentWidget(_option_widget);
    _option_widget->setHidden(false);
}

void MainWidget::closeOptionPanel()
{
    _option_widget->setHidden(true);
    _stacked_layout->setCurrentWidget(_main_widget);
}

void MainWidget::update(QWidget *contentWidget, OptionPanel *optionPanel)
{
    if (_content_widget != contentWidget) {
        if (!_content_widget) {
            _main_layout->insertWidget(0, contentWidget);
        } else if (!contentWidget) {
            int idx = _main_layout->indexOf(_main_layout);
            if (idx != -1) {
                QLayoutItem *item = _main_layout->takeAt(0);
                delete item;
            }
        } else {
            QLayoutItem *item = _main_layout->replaceWidget(_content_widget, contentWidget);
            delete item;
        }
    }
    _content_widget = contentWidget;
    
    if (_option_panel != optionPanel) {
        if (!_option_panel) {
            _option_layout->addWidget(optionPanel);
            
        } else if (!optionPanel) {
            int idx = _option_layout->indexOf(_option_panel);
            if (idx != -1) {
                QLayoutItem *item = _option_layout->takeAt(idx);
                delete item;
            }
            
            _option_widget->setHidden(true);
            _stacked_layout->setCurrentWidget(_main_widget);
        } else {
            QLayoutItem *item = _option_layout->replaceWidget(_option_panel, optionPanel);
            delete item;
        }
        
        _option_panel = optionPanel;
    }
    _showOptionPanelButton->setHidden(_option_panel == nullptr);
}

void MainWidget::hideEvent(QHideEvent *event)
{
    (void)event;

    closeOptionPanel();
}

void MainWidget::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
