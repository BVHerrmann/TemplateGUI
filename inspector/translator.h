#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <QtCore>

#include "singleton.h"


class Translator : public QObject, public Singleton<Translator>
{
    Q_OBJECT
    friend class Singleton<Translator>;

public:
    explicit Translator(QObject *parent = 0);

    QStringList availableLanguages();
    void updateTranslations();
    
signals:
    
public slots:

private:
    QList<QTranslator *> _installedTranslators;
};

#endif // TRANSLATOR_H
