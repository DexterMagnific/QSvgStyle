#ifndef GROUPS_H
#define GROUPS_H

#include <QString>
#include <QStyle>

/**
 * Returns the configuration group used fot the given PrimitiveElement
 */
QString PE_group(QStyle::PrimitiveElement element);

/**
 * Returns the configuration group used fot the given SubElement
 */
QString SE_group(QStyle::SubElement element);

/**
 * Returns the configuration group used fot the given ControlElement
 */
QString CE_group(QStyle::ControlElement element);

/**
 * Returns the configuration group used fot the given ComplexControl
 */
QString CC_group(QStyle::ComplexControl element);

/**
 * Returns the configuration group used fot the given ContentsType
 */
QString CT_group(QStyle::ContentsType type);

#endif
