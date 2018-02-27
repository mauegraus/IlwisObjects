#ifndef COVERAGELAYERMODEL_H
#define COVERAGELAYERMODEL_H

#include <QQmlListProperty>
#include "kernel.h"
#include "ilwisdata.h"
#include "coveragedisplay\layermodel.h"
#include "ilwiscoreui_global.h"

class LayersContext;

namespace Ilwis {
class Coverage;
typedef IlwisData<Coverage> ICoverage;
class Representation;
typedef IlwisData<Representation> IRepresentation;
class ColumnDefinition;

namespace Ui{
class LayerManager;
class VisualPropertyEditor;

class ILWISCOREUISHARED_EXPORT CoverageLayerModel : public LayerModel
{
    Q_OBJECT

public:
    Q_PROPERTY(QString activeAttribute READ activeAttributeName WRITE activeAttributeName NOTIFY activeAttributeChanged)

    Q_INVOKABLE QQmlListProperty<Ilwis::Ui::VisualPropertyEditor> vproperties(const QString& attrName);
	Q_INVOKABLE void setActiveAttribute(int idx);

	CoverageLayerModel();
    CoverageLayerModel(LayerManager *manager, QObject *parent, const QString &name, const QString &desc, const IOOptions& options);

    QString activeAttributeName() const;
	void activeAttributeName(const QString& pName);
	VisualAttribute *activeAttribute();

    virtual QVariant vproperty(const QString& pName) const;
    virtual void vproperty(const QString& pName, const QVariant& value);

    bool showInfo() const;
    void showInfo(bool yesno);

    virtual ICoverage coverage() const;
    virtual void coverage(const ICoverage& cov);

    QString url() const;
    QColor color(const IRepresentation &rpr, const QString& propName, double value, LayerModel::ColorValueMeaning cvm);
    virtual QString value2string(const QVariant& value, const QString& attrName="") const;
    QString layerData(const Coordinate &crdIn, const QString& attrName, QVariantList &items) const;
    bool isCoverageBased() const;

signals:
    void activeAttributeChanged();
    void vpropertiesChanged();

protected:
    void addVisualAttribute(const QString &attrName, const QString& propertyName, VisualPropertyEditor *newProp);
    bool needsCoordConversion() const;
    QString v2s(const ColumnDefinition &coldef, const QVariant &value) const;
private:
    quint32 layerIndex() const;
    VisualPropertyEditor *visualProperty(const QString& attrName, const QString &propertyName);


    bool _showInfo = true;
    bool _coordConversionNeeded = false;
    QString _activeAttribute;
    ICoverage _coverage;
};
}
}

typedef QQmlListProperty<Ilwis::Ui::CoverageLayerModel> CoverageLayerModelList;
Q_DECLARE_METATYPE(CoverageLayerModelList)

#endif // COVERAGELAYERMODEL_H
