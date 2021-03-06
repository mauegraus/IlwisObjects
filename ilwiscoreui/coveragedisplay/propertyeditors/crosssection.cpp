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

#include <QApplication>
#include "coverage.h"
#include "representation.h"
#include "attributemodel.h"
#include "visualattribute.h"
#include "visualpropertyeditor.h"
#include "raster.h"
#include "attributedefinition.h"
#include "table.h"
#include "crosssection.h"
#include "ilwiscontext.h"


using namespace Ilwis;
using namespace Ui;


CrossSectionPin::CrossSectionPin() : QObject()
{}

CrossSectionPin::CrossSectionPin(const QString& lbl, const Coordinate& crd, const IGeoReference& grf, QObject *parent) : QObject(parent),
_label(lbl),
_coord(crd.isValid() ? crd : Coordinate(0, 0))
{
    if (grf.isValid()) {
        if (crd.isValid())
            _pix = grf->coord2Pixel(crd);
        else
            _pix = Pixel(0, 0);
    }
 
}
double CrossSectionPin::x() const {
    return _coord.x;
}
double CrossSectionPin::y() const {
    return _coord.y;
}
QString CrossSectionPin::label() const {
    return _label;
}
void CrossSectionPin::x(double v) {
    _coord.x = v;
}
void CrossSectionPin::y(double v) {
    _coord.y = v;
}
void CrossSectionPin::label(const QString& v) {
    CrosssectionTool *tool = static_cast<CrosssectionTool *>(parent());
    if (!tool->labelExists(v)) {
        _label = v;
    }
}

int CrossSectionPin::row() const {
    return _pix.y;
}
int CrossSectionPin::column() const {
    return _pix.x;
}
void CrossSectionPin::row(int r) {
    _pix.y = r;
}
void CrossSectionPin::column(int c) {
    _pix.x = c;
}

void CrossSectionPin::update() {
    emit updatePin();
}
//-------------------------------------------
CrosssectionTool::CrosssectionTool()
{

}

CrosssectionTool::CrosssectionTool(VisualAttribute *p) :
    VisualPropertyEditor(p, "crosssectiontool",TR("Cross Section"),QUrl("CrossSectionTool.qml"))
{
    _pinData.prepare();
    _activePinData.prepare();
}

bool CrosssectionTool::canUse(const IIlwisObject& obj, const QString& name ) const
{
    if (!obj.isValid())
        return false;
    if(!hasType(obj->ilwisType(), itCOVERAGE))
        return false;
    return name == VisualAttribute::LAYER_ONLY;

}

VisualPropertyEditor *CrosssectionTool::create(VisualAttribute *p)
{
    return new CrosssectionTool(p);
}

void CrosssectionTool::prepare(const Ilwis::IIlwisObject& bj, const DataDefinition &datadef) {
    _panelCoverage = coverage();
    auto *lm = vpmodel()->layer()->layerManager();
    QString path = context()->ilwisFolder().absoluteFilePath();
    QUrl url = QUrl::fromLocalFile(path);
    /*if (_panelCoverage.isValid() && _panelCoverage->ilwisType() == itRASTER) {
        IRasterCoverage raster = _panelCoverage.as<RasterCoverage>();
        if (raster.isValid()) {
            _dataSource = new PinDataSource(raster->id(), this);
            emit bandsChanged();
        }
    }*/
    associatedUrl(url.toString() + "/qml/datapanel/visualization/propertyeditors/PostDrawerCrossSection.qml");
    lm->addPostDrawer(this);
}

bool CrosssectionTool::hasData() const {
	return _dataSource != 0;
}

bool CrosssectionTool::labelExists(const QString& newlabel) const{
    for (const auto *pin : _pins) {
        if (pin->label() == newlabel)
            return true;
    }
    return false;
}
QQmlListProperty<Ilwis::Ui::CrossSectionPin> Ilwis::Ui::CrosssectionTool::pins()
{
   /* QVariantList result;
    IGeoReference grf = vpmodel()->layer()->layerManager()->rootLayer()->screenGrf();
    if (_coverage.isValid()) {
            if (_coverage->ilwisType() == itRASTER) {
            IRasterCoverage raster = _coverage.as<RasterCoverage>();
            if (raster.isValid() && raster->size().zsize() > 1) {
                grf = raster->georeference();
            }
        }
    }*/

   return QQmlListProperty<CrossSectionPin>(this, _pins);
}

QString CrosssectionTool::columnName(int index, const QString& coverageName) const {
    if (index < 0 || index >= _pins.size())
        return sUNDEF;
    QString ycolName = _pins[index]->label();
    if (_contineousMode)
        ycolName = "contineous_pin";
   /* if (_dataSource) {
        ycolName += coverageName;
    }*/
    ycolName = ycolName.replace(QRegExp("[/ .-,;:'\"]"), "_");

    return ycolName;
}
int Ilwis::Ui::CrosssectionTool::decimalsCrds() const
{
    if (!_panelCoverage.isValid())
        return 0;
    return _panelCoverage->coordinateSystem()->isLatLon() ? 7 : 3;
}
void CrosssectionTool::changePinData(int index, const Coordinate& crd) {
    if (!_pinData.isValid() || _dataSource == 0)
        return;
 

    std::vector<QVariant> empty(_pinData->recordCount());
    _pinData->column(index+1, empty);  // index == 0 is the band value which we dont want to change
    IRasterCoverage raster;
    raster.prepare(_dataSource->coverageId());
    
    if (raster.isValid()) {
        if (raster->datadef().domain()->ilwisType() == itNUMERICDOMAIN) {
            if (!raster->datadef().range<NumericRange>()->isValid()) {
                //QGuiApplication *app = static_cast<QGuiApplication *>(QApplication::instance());
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                raster->statistics(Ilwis::NumericStatistics::pBASIC);
                QApplication::restoreOverrideCursor();
            }
        }
  
        QString ycolName = columnName(index, raster->name());
        Pixel pix = raster->georeference()->coord2Pixel(crd);
        int col = 0;
        if ((col = _pinData->columnIndex(ycolName)) != iUNDEF) {
            int rec = 0;
            for (int z = 0; z < raster->size().zsize(); ++z) {
                if (_dataSource->active(z)) {
                    double v = raster->pix2value(Pixel(pix.x, pix.y, z));
                    _pinData->setCell(col, rec++, v);
                }
            }
        }
    }
}

void CrosssectionTool::changeCoords(int index, int c, int r, bool useScreenPixels)
{
    if (index >= 0 && index < _pins.size()) {
        if (_panelCoverage->ilwisType() == itRASTER) {
            IRasterCoverage raster = _panelCoverage.as<RasterCoverage>();
            if (raster.isValid()) {
                Coordinate crd;
                if (useScreenPixels) {
                    crd = vpmodel()->layer()->layerManager()->rootLayer()->screenGrf()->pixel2Coord(Pixel(c, r));
                    Pixel pix = raster->georeference()->coord2Pixel(crd);
                    c = pix.x;
                    r = pix.y;
                }
                crd = raster->georeference()->pixel2Coord(Pixel(c, r));
                _pins[index]->x(crd.x);
                _pins[index]->y(crd.y);
                _pins[index]->column(c);
                _pins[index]->row(r);
                _pins[index]->update();
                vpmodel()->layer()->layerManager()->updatePostDrawers();
                changePinData(index, crd); 
            }
        }
        
    }
}

void CrosssectionTool::changePixel(int index, double x, double y)
{
    if (index >= 0 && index < _pins.size()) {
        if (_panelCoverage->ilwisType() == itRASTER) {
            IRasterCoverage raster = _panelCoverage.as<RasterCoverage>();
            if (raster.isValid()) {
                Pixel pix = raster->georeference()->coord2Pixel(Coordinate(x, y));
                _pins[index]->x(x) ;
                _pins[index]->y(y);
                _pins[index]->column(pix.x);
                _pins[index]->row(pix.y);
                _pins[index]->update();
                vpmodel()->layer()->layerManager()->updatePostDrawers();
                changePinData(index, Coordinate(x,y)); 
            }
        }
    }
}

QVariantList CrosssectionTool::pinLocation4screen() const
{
    QVariantList result;
    for (auto *pin : _pins) {
        QVariantMap pos;
        Pixel pix = vpmodel()->layer()->layerManager()->rootLayer()->screenGrf()->coord2Pixel(Coordinate(pin->x(), pin->y()));
        
        pos["x"] = pix.x;
        pos["y"] = pix.y;
        pos["label"] = pin->label();
        result.push_back(pos);
    }
    return result;
}

QString Ilwis::Ui::CrosssectionTool::tableUrlPrivate()
{
    if (_pinData.isValid()) {
        return _pinData->resource().url().toString();
    }
    return QString();
}

int CrosssectionTool::maxR() const {
    if (_panelCoverage.isValid() && _panelCoverage->ilwisType() == itRASTER) {
        IRasterCoverage raster = _panelCoverage.as<RasterCoverage>();
        if (raster.isValid()) {
            return raster->georeference()->size().ysize();
        }
    }
    return 0;
}

int CrosssectionTool::maxC() const {
    if (_panelCoverage.isValid() && _panelCoverage->ilwisType() == itRASTER) {
        IRasterCoverage raster = _panelCoverage.as<RasterCoverage>();
        if (raster.isValid()) {
            return raster->georeference()->size().xsize();
        }
    }
    return 0;
}

double CrosssectionTool::minX() const {
    if (_panelCoverage.isValid()) {
        return _panelCoverage->envelope().min_corner().x;
    }
    return 0;
}

double CrosssectionTool::minY() const {
    if (_panelCoverage.isValid()) {
        return _panelCoverage->envelope().min_corner().y;
    }
    return 0;
}

double CrosssectionTool::maxX() const {
    if (_panelCoverage.isValid()) {
        return _panelCoverage->envelope().max_corner().x;
    }
    return 0;
}

double CrosssectionTool::maxY() const {
    if (_panelCoverage.isValid()) {
        return _panelCoverage->envelope().max_corner().y;
    }
    return 0;
}

int CrosssectionTool::pinRow(int index) const {
    if (index < _pins.size()) {
        return _pins[index]->row();
    }
    return 0;
}
int CrosssectionTool::pinColumn(int index) const{
    if (index < _pins.size()) {
        return _pins[index]->column();
    }
    return 0;
}
double CrosssectionTool::pinX(int index) const {
    if (index < _pins.size()) {
        return _pins[index]->x();
    }
    return 0;
}
double CrosssectionTool::pinY(int index) const {
    if (index < _pins.size()) {
        return _pins[index]->y();
    }
    return 0;
}

void Ilwis::Ui::CrosssectionTool::deletePin(int index)
{
    if (index < _pins.size() && index >= 0) {
        QString columnName = _pinData->columndefinition(index + 1).name();
        _pins.removeAt(index);
        vpmodel()->layer()->layerManager()->updatePostDrawers();
        _pinData->deleteColumn(columnName);
        emit pinsChanged();
        emit pinCountChanged();
    }
}

void CrosssectionTool::addPinPrivate() {
	if (!_dataSource)
		return;

    IRasterCoverage raster;
    raster.prepare(_dataSource->coverageId());

    if (_pinData->columnIndex("bands") == iUNDEF) { // empty table
        _pinData->addColumn(ColumnDefinition("bands", IDomain("count")));
        for (int z = 0; z < raster->size().zsize(); ++z) {
            if (_dataSource->active(z))
                _pinData->setCell(0, z, z+1);
        }
    }
    QString ycolName = columnName(_pins.size() - 1, raster->name());
    if (_pinData->columnIndex(ycolName) == iUNDEF) {
        _pinData->addColumn(ColumnDefinition(ycolName, raster->datadef(), _pinData->columnCount() - 1));
    }

    _pins.back()->update();
    vpmodel()->layer()->layerManager()->updatePostDrawers();
    emit pinsChanged();
    emit pinCountChanged();
}
void CrosssectionTool::addPin()
{
    _pins.push_back(new CrossSectionPin("pin_" + QString::number(_pins.size()), Coordinate(), vpmodel()->layer()->layerManager()->rootLayer()->screenGrf(), this));
    addPinPrivate();
}

Ilwis::Ui::PinDataSource* CrosssectionTool::dataSource() 
{
    return _dataSource;
}


void Ilwis::Ui::CrosssectionTool::addDataSource(const QString & id)
{
    try {
        bool ok;
        quint64 objid = id.toULongLong(&ok);
        if (!ok)
            return;
        _dataSource = new PinDataSource(objid, this);
        emit dataSourceChanged();
        emit bandsChanged();
		emit hasDataChanged();
    }
    catch (const ErrorObject& err) {}
}

QVariantList Ilwis::Ui::CrosssectionTool::bands()
{
    if (_dataSource) {
        return _dataSource->bands();
    }
    return QVariantList();
}

void Ilwis::Ui::CrosssectionTool::setActive(int bandIndex, bool yesno)
{
    if (_dataSource) {
        _dataSource->active(bandIndex, yesno);
        int count = 0;
        int recs = _pinData->recordCount();
        for (int rec = 0; rec < recs; ++rec)
            _pinData->removeRecord(0); // clear table

        for (int rec = 0; rec < recs; ++rec) {
            if (_dataSource->active(rec)) 
                _pinData->setCell(0, count++, rec);
        }
        for (int pin = 0; pin < pinCount(); ++pin) {
                changePinData(pin, Coordinate(_pins[pin]->x(), _pins[pin]->y()));
        }
    }
}

QString Ilwis::Ui::CrosssectionTool::pinDataColumn(int index) const
{
    if (index < _pinData->columnCount()) {
        return _pinData->columndefinitionRef(index).name();
    }
    return QString();
}

int CrosssectionTool::pinCount() const {
    return _pins.size();
}

bool CrosssectionTool::contineousMode() const {
    return _contineousMode;
}
void CrosssectionTool::contineousMode(bool yesno) {
    _contineousMode = yesno;
}

int CrosssectionTool::addContineousPin() {
    _pins.push_back(new CrossSectionPin("contineous_pin", Coordinate(), vpmodel()->layer()->layerManager()->rootLayer()->screenGrf(), this));
    addPinPrivate();
    _pins.back()->update();
    vpmodel()->layer()->layerManager()->updatePostDrawers();
    emit pinCountChanged();
    return _pins.size() - 1;
}

//---------------------------------
PinDataSource::PinDataSource() {

}

PinDataSource::PinDataSource(quint64 objid, QObject *parent) : QObject(parent) {
    IRasterCoverage raster;
    raster.prepare(objid);
    if (!raster.isValid())
        return;
    _objid = objid;
    CrosssectionTool *tool = static_cast<CrosssectionTool *>(parent);
    IRasterCoverage rasterPanel = tool->coverage().as<RasterCoverage>();
    if (!rasterPanel.isValid()) {
        throw ErrorObject(TR("Crossection tool only works for raster coverages"));
    }
    if (!raster->georeference()->isCompatible(rasterPanel->georeference())) {
        throw ErrorObject(TR("Rasters must have compatible georeferences"));
    }
    const RasterStackDefinition&  stack = raster->stackDefinition();
    for (quint32 i = 0; i < stack.count(); ++i) {
        QString name = QString::number(i + 1); // stack.index(i+1);
        QVariantMap data;
        data["name"] = name;
        data["active"] = true;
        _actives.push_back(data);
    }
}

QVariantList PinDataSource::bands() const
{
    return _actives;
}

quint64 Ilwis::Ui::PinDataSource::coverageId() const
{
    return _objid;
}

QString PinDataSource::sourcePrivate() const
{
    ICoverage cov;
    cov.prepare(_objid);
    if (cov.isValid())
        return cov->resource().url().toString();

    return QString();
}

void PinDataSource::active(int index, bool yesno) {
    if (index < _actives.size()) {
        QVariantMap mp = _actives[index].toMap();
        mp["active"] = yesno;
        _actives[index] = mp;
    }
}

bool Ilwis::Ui::PinDataSource::active(int index) const
{
    if (index < _actives.size()) {
        QVariant v = _actives[index];
        QVariantMap mp = v.toMap();
        return mp["active"].toBool();
    }
    return false;
}



