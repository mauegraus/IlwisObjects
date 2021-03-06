/*IlwisObjects is a framework for analysis, processing and visualization of remote sensing and gis data
Copyright (C) 2018  52n North

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#include "kernel.h"
#include "ilwisdata.h"
#include "mastercatalog.h"
#include "symboltable.h"
#include "geos/geom/Coordinate.h"
#include "location.h"
#include "ilwiscoordinate.h"
#include "box.h"
#include "operationmetadata.h"
#include "workflownode.h"
#include "workflow.h"
#include "operationmetadata.h"
#include "workflowmodel.h"
#include "symboltable.h"
#include "commandhandler.h"
#include "operationcatalogmodel.h"
#include "ilwiscontext.h"
#include "ilwistypes.h"
#include "resourcemodel.h"
#include "applicationmodel.h"
#include "analysispattern.h"
#include "modeldesigner.h"
#include "modellerfactory.h"
#include "modelbuilder.h"

using namespace Ilwis;
using namespace Ui;

std::unique_ptr<ModelBuilder> ModelBuilder::_modelbuilder;



ModelBuilder::ModelBuilder(QObject *parent) : QObject(parent)
{

}

ModelDesigner *ModelBuilder::createModel(ResourceModel *rmodel,QObject *parent)
{
    try {
        return new ModelDesigner(rmodel, parent);
    }
    catch( const ErrorObject& ){}

    return 0;
}

AnalysisModel *ModelBuilder::createAnalysisModel(AnalysisPattern *pattern)
{
    auto iter = _analysisModelCreators.find(pattern->type());
    if ( iter != _analysisModelCreators.end()){
         return (*iter).second(pattern);
    }
    return 0;
}

ApplicationModelUI * ModelBuilder::createApplicationModelUI(ApplicationModel *appmodel, QObject *parent){
    for(auto creator : _applicationModelCreators){
        ApplicationModelUI *appmodelui = creator.second(appmodel, parent);
        if ( appmodel->name() == appmodelui->name()){
            return appmodelui;
        }
        delete appmodelui;
    }
    return 0;
}

AnalysisModel *ModelBuilder::registerAnalysisModel(const QString &type, CreateAnalysisModel creator)
{
    auto iter = _analysisModelCreators.find(type);
    if ( iter == _analysisModelCreators.end()){
        _analysisModelCreators[type] = creator;
    }
    return 0;
}

ApplicationModelUI *ModelBuilder::registerApplicationModel(const QString &type, CreateAppModel creator)
{
    auto iter = _applicationModelCreators.find(type);
    if ( iter == _applicationModelCreators.end()){
        _applicationModelCreators[type] = creator;
    }
    return 0;
}

QVariantList ModelBuilder::analysisModels() const
{
    QVariantList result;
    for (auto analysis : _analysisModelCreators) {
        QVariantMap mp;
        mp["name"] = analysis.first;
        result.push_back(mp);
    }
    return result;
}
