#ifndef GROUPS_H
#define GROUPS_H

#include <QString>
#include <QStyle>

extern "C" {

QString PE_group(QStyle::PrimitiveElement element);
QString SE_group(QStyle::SubElement element);
QString CE_group(QStyle::ControlElement element);
QString CC_group(QStyle::ComplexControl element);
QString CT_group(QStyle::ContentsType type);

}

#endif
