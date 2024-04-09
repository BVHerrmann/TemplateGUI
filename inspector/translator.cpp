#include "translator.h"

#include "inspectordefines.h"


Translator::Translator(QObject *parent) :
    QObject(parent)
{

}

QStringList Translator::availableLanguages()
{
    QStringList languages;
    
    QDirIterator it(":/translations", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFileInfo fi(it.next());
        QStringList suffixParts = fi.completeSuffix().split(".");
    
        if (suffixParts.count()) {
            QString language = suffixParts.first();
            if (!languages.contains(language)) {
                languages << language;
            }
        }
    }
    languages.sort();
    
    return languages;
}

void Translator::updateTranslations()
{
    QSettings translator_settings;
    QLocale locale = QLocale(translator_settings.value(kLocale, "de").toString());
    
    QDirIterator it(":/translations", QDirIterator::Subdirectories);
    QStringList translationFiles;
    QList<QTranslator *> translators;
    
    while (it.hasNext()) {
        QFileInfo fi(it.next());
        if (!translationFiles.contains(fi.baseName())) {
            QTranslator *translator = new QTranslator(qApp);
            qApp->installTranslator(translator);
            bool result = translator->load(locale, fi.baseName(), ".", ":/translations");
            if (result) {
                _installedTranslators << translator;
                translators << translator;
            } else {
                qWarning() << "Translation" << fi.baseName() << "for" << locale.uiLanguages() << "not found!";
            }
            translationFiles << fi.baseName();
        }
    }
    
    for (QTranslator *translator : _installedTranslators) {
        if (!translators.contains(translator)) {
            qApp->removeTranslator(translator);
            translator->deleteLater();
        }
    }
    
    // update list of current translators
    _installedTranslators = translators;
}
