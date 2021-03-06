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

#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlField>
#include <QColor>
#include "kernel.h"
#include "ilwisdata.h"
#include "range.h"
#include "itemrange.h"
#include "domain.h"
#include "itemdomain.h"
#include "domainitem.h"
#include "coloritem.h"
#include "identifieritem.h"
#include "thematicitem.h"
#include "interval.h"
#include "colorrange.h"
#include "numericrange.h"
#include "datadefinition.h"
#include "palettecolorlookup.h"

using namespace Ilwis;
PaletteColorLookUp::PaletteColorLookUp(){

}

PaletteColorLookUp::PaletteColorLookUp(const QString &definition){
    fromDefinition(definition);
}

PaletteColorLookUp::PaletteColorLookUp(const IDomain &dom, const QString &rprCode)
{
    _cyclic = hasType(dom->valueType(), itINDEXEDITEM | itNAMEDITEM | itSTRING);
    InternalDatabaseConnection db;
    QString query = QString("Select * from representation where code='%1'").arg(rprCode != "" ? rprCode : "primarycolors" );
    if (db.exec(query)) {
        if ( db.next()){
            QSqlRecord rec = db.record();
            QString  definition = rec.field("definition").value().toString();
            fromDefinition(definition, dom);
        }
    }
}

PaletteColorLookUp::PaletteColorLookUp(boost::container::flat_map<quint32, QColor> &newcolors) : _colors(newcolors)
{

}

QColor PaletteColorLookUp::value2color(double index, const NumericRange &, const NumericRange &) const
{
    if (index != iUNDEF) {
        int localIndex = index;
        if (_cyclic) {
            localIndex = localIndex % _colors.size();
        }
        auto iter = _colors.find(localIndex);
        if (iter != _colors.end()) {

            QColor clr = (*iter).second;
            return clr;
        }
    }
    return QColor();
}

void PaletteColorLookUp::fromDefinition(const QString &definition, const IDomain& dom){
    QStringList parts = definition.split("|");
    if ( dom.isValid()){
        if ( hasType(dom->valueType(), itTHEMATICITEM|itNUMERICITEM|itTIMEITEM)){
            IItemDomain itemdom = dom.as<ItemDomain<DomainItem>>();
            int index = 0;
            for(auto item : itemdom){
                QString part = parts[index % parts.size()];
                _colors[(quint32)item->raw()] = string2color(part);
                ++index;
            }
            return;
        }
    }
    int index = 0;
    _cyclic = true;
    for( QString part : parts){
        QColor color = string2color(part);
        _colors[index - 1] = color;
        ++index;
    }
}

ColorLookUp *PaletteColorLookUp::clone() const{
    PaletteColorLookUp *newpalette = new PaletteColorLookUp();
    newpalette->_colors = _colors;
    newpalette->_cyclic = _cyclic;
    return newpalette;
}

void PaletteColorLookUp::setColor(double value, const QColor &clr)
{
    _colors[(quint32)value] = clr;
}
