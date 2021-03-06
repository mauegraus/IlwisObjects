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

#include "coverage.h"
#include <QtQuick/qquickwindow.h>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLContext>
#include <QDir>
#include "table.h"
#include "layerdrawer.h"
#include "drawingcolor.h"
#include "layersrenderer.h"
#include "layersview.h"
#include "symboltable.h"
#include "operationExpression.h"
#include "operationmetadata.h"
#include "commandhandler.h"
#include "coveragedisplay/layermanager.h"
#include "coveragedisplay/coveragelayermodel.h"
#include "operation.h"

using namespace Ilwis;
using namespace Ui;

LayersRenderer::LayersRenderer(const QQuickFramebufferObject *fbo, bool active)
{
    if ( !_rootDrawer){
        _rootDrawer = new Ilwis::Geodrawer::RootDrawer(fbo, Ilwis::IOOptions());
        connect(_rootDrawer, &Ilwis::Geodrawer::BaseDrawer::updateRenderer,this, &LayersRenderer::updateRenderer );
        initializeOpenGLFunctions();
    }

    _rootDrawer->active(active);
}

LayersRenderer::~LayersRenderer()
{
    delete _rootDrawer;
}


void LayersRenderer::saveAsImage() const
{
    QImage image = _fbo->toImage();
    QImage vflip = image.transformed(QMatrix().scale(1,-1));
    QUrl url(_saveImagePath);
    QFileInfo inf(url.toLocalFile());
    QDir dir(inf.absoluteDir().absolutePath() );
    if ( !dir.exists())
        dir.mkdir(inf.absolutePath());
    vflip.save(inf.absoluteFilePath());
}

void LayersRenderer::render()
{
    try {

        if ( !_rootDrawer)
            return;

        if ( !_rootDrawer->isActive())
            return ;

        if ( _rootDrawer->is3D()){
            glEnable( GL_DEPTH_TEST);
            glDepthMask(true);
        }else
            glDisable( GL_DEPTH_TEST);

        QColor backgroundColor = _rootDrawer->attribute("backgroundcolor").value<QColor>();
        glClearColor(backgroundColor.redF(), backgroundColor.greenF(), backgroundColor.blueF(), 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);

        _rootDrawer->draw( );

        glDisable(GL_BLEND);

        if ( _saveImagePath != "" && _fbo){
            saveAsImage();
        }

        emit drawDone();
    }
    catch(const ErrorObject& ){}
    catch(const std::exception& ex){
        kernel()->issues()->log(ex.what());
    }
}

void LayersRenderer::cleanup()
{
    if ( _rootDrawer){
        _rootDrawer->cleanUp();
    }
}
void LayersRenderer::updateRenderer() {
    emit update();
}

void LayersRenderer::synchronize(QQuickFramebufferObject *item)
{
    try {

        bool needPrepare = false;
        LayersView *gdrawer = static_cast<LayersView *>(item);

        _viewPortSize =  QSize(gdrawer->width(), gdrawer->height());
        _windowSize = gdrawer->window()->size();
        _rootDrawer->pixelAreaSize(_viewPortSize);

        while(gdrawer->_attributerequests.size() > 0){
            auto pair = gdrawer->_attributerequests.front();
            gdrawer->_attributerequests.pop_front();
            QVariant var = _rootDrawer->attributeOfDrawer(pair.first, pair.second);
            gdrawer->_copiedAttributes[pair.first.toLower() + "|" + pair.second.toLower()] = var;
            needPrepare = true;
        }

        for(const Ilwis::OperationExpression& expr : gdrawer->_commands){
            quint64 id = commandhandler()->findOperationId(expr);
            if ( id != i64UNDEF) {
                QScopedPointer<OperationImplementation> oper(commandhandler()->create( expr));
                if ( !oper.isNull() && oper->isValid()) {
                    ExecutionContext ctx;
                    SymbolTable symTable;
                    QVariant v = qVariantFromValue((void *) _rootDrawer);
                    ctx._additionalInfo["rootdrawer"] = v;
                    if ( oper->execute(&ctx, symTable)){
                        Symbol sym = symTable.getSymbol("layerdrawer");
                        if ( sym.isValid()){
                            Ilwis::Geodrawer::DrawerInterface *drawer = static_cast<Ilwis::Geodrawer::DrawerInterface *>(sym._var.value<void *>());
                            if ( drawer){
                                ICoverage cov = drawer->attribute("coverage").value<ICoverage>();
                                if ( gdrawer->_manager){
                                    qDebug() << "render" << QThread::currentThread();
                                    emit addDataSource(cov->resource().url(),cov->ilwisType(),drawer);
                                }
                            }
                        }
                    }
                }
                needPrepare = true;
            }
        }
        gdrawer->_commands = std::deque<OperationExpression>();


        while( gdrawer->_attributeQueue.size() > 0){
            auto pair = gdrawer->_attributeQueue.front();
            gdrawer->_attributeQueue.pop_front();
            for(QVariantMap::const_iterator iter = pair.second.begin(); iter != pair.second.end(); ++iter) {
                if ( pair.first.toLower() == "view"){
                    handleRendererAttributes(iter.key(), iter.value())    ;
                }else {
                    _rootDrawer->drawerAttribute(pair.first.toLower(),iter.key(), iter.value());
                }
            }
            needPrepare = true;
        }
        if ( needPrepare){
            _rootDrawer->prepare(Ilwis::Geodrawer::DrawerInterface::ptALL,Ilwis::IOOptions());
            emit synchronizeDone();
        }
    } catch ( const ErrorObject& ){

    } catch ( const std::exception ex){

    }
}

void LayersRenderer::handleRendererAttributes(const QString& code, const QVariant& value){
    if ( code == "saveimage")    {
        _saveImagePath = value.toString();
    }
}

