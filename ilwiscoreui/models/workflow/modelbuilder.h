#ifndef WORKFLOWCATALOGMODEL_H
#define WORKFLOWCATALOGMODEL_H

#include <QObject>
#include <QQmlListProperty>
#include <functional>
#include <memory>
#include "ilwiscoreui_global.h"

namespace Ilwis{
class AnalysisPattern;
class ApplicationModel;

namespace Ui{
class ModelDesigner;
class AnalysisModel;
class ApplicationModelUI;
class ResourceModel;


typedef std::function<AnalysisModel *(Ilwis::AnalysisPattern *pattern)> CreateAnalysisModel;
typedef std::function<ApplicationModelUI *(Ilwis::ApplicationModel *,QObject *parent)> CreateAppModel;


class ILWISCOREUISHARED_EXPORT ModelBuilder : public QObject
{
    Q_OBJECT

    friend std::unique_ptr<ModelBuilder>& modelbuilder();
public:
    explicit ModelBuilder(QObject *parent = 0);

    Q_PROPERTY(QVariantList analysisModels READ analysisModels CONSTANT)

    Q_INVOKABLE Ilwis::Ui::ModelDesigner * createModel(Ilwis::Ui::ResourceModel *rmodel, QObject *parent);
    Q_INVOKABLE Ilwis::Ui::AnalysisModel * createAnalysisModel(Ilwis::AnalysisPattern *pattern);
  
    ApplicationModelUI *createApplicationModelUI(Ilwis::ApplicationModel *appmodel, QObject *parent);

    AnalysisModel *registerAnalysisModel(const QString& type, CreateAnalysisModel creator);
    ApplicationModelUI *registerApplicationModel(const QString& type, CreateAppModel creator);




private:
    QVariantList analysisModels() const;

    std::map<QString, CreateAnalysisModel>  _analysisModelCreators;
    std::map<QString, CreateAppModel>  _applicationModelCreators;

    static std::unique_ptr<ModelBuilder> _modelbuilder;


signals:


public slots:

};

inline std::unique_ptr<ModelBuilder>& modelbuilder(){
    if ( ModelBuilder::_modelbuilder.get() == 0) {
        ModelBuilder::_modelbuilder.reset( new ModelBuilder());
    }
    return ModelBuilder::_modelbuilder;
}
}
}

#endif // WORKFLOWCATALOGMODEL_H
