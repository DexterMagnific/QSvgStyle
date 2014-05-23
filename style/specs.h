/***************************************************************************
 *   Copyright (C) 2014 by Saïd LANKRI   *
 *   said.lankri@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef SPEC_H
#define SPEC_H

#include <QVariant>
#include <QDebug>

/** Helper class that behaves like the value it holds, but with
 * a possibility to tell if the value has been set or no
 * Variables of this type can be assigned, retrieved and referenced
 * <code>
 * value_t<QString> s; // s.present is false
 * s = "hello"; // s.present becomes true
 * QString str = s; // no problemo :)
 * </code>
 */
template <typename T> struct value_t {
private:
  T value;

public:
  bool present;

  value_t () : present(false) { };
  value_t (const QVariant &v) {
    present = !v.isNull();
    value = v.value<T>();
  };

  value_t<T> & operator = (const T &v) {
    present = true;
    value = v;
    return *this;
  }
  operator const T & () const {
    return value;
  }
  operator T & () {
    return value;
  }
  operator QVariant() const {
    if ( present )
      return QVariant(value);
    else
      return QVariant();
  }
};

/** Generic information about a theme */
typedef struct {
  value_t<QString> name;
  value_t<QString> author;
  value_t<QString> descr;
} theme_spec_t;

/** Generic information about a frame */
typedef struct frame_spec_t {
  frame_spec_t () {
    hasCapsule = false;
    capsuleH = capsuleV = 0;
  };

  /* Element name */
  value_t<QString> element;
  /* has frame ? */
  value_t<bool> hasFrame;
  /* frame size */
  value_t<int> width;

  /* Allow capsule grouping, used internally */
  bool hasCapsule;
  /* filled with width when read from config file, used internally */
  int top,bottom,left,right;
  /* widget position in a capsule, used internally */
  int capsuleH,capsuleV; // 0 : middle, -1 : left,top, 1 : right,bottom, 2 : left+right,top+bottom
  /* positions where the frame is 'cut' (for each line), used internally */
  int y0c0,y0c1, y1c0,y1c1, x0c0,x0c1, x1c0,x1c1;
} frame_spec_t;

/** Generic information about a frame interior */
typedef struct {
  /* Element name */
  value_t<QString> element;
  /* has interior */
  value_t<bool> hasInterior;
  /* pattern size */
  value_t<int> px,py;
} interior_spec_t;

/** Generic information about widget indicators */
typedef struct {
  /* Element name */
  value_t<QString> element;
  /* size */
  value_t<int> size;
} indicator_spec_t;

/** Generic information about text and icons (labels) */
typedef struct {
  /* has shadow */
  value_t<bool> hasShadow;
  /* shadow shift */
  value_t<int> xshift,yshift;
  /* shadow color */
  value_t<int> r,g,b,a;
  /* shadow depth */
  value_t<int> depth;
  /* text margins */
  value_t<int> hmargin,vmargin;
  /* text-icon spacing */
  value_t<int> tispace;

  int margin;

} label_spec_t;

/** Generic information about an element */
typedef struct {
  /* Inherited Element */
  value_t<QString> inherits;
  frame_spec_t frame;
  interior_spec_t interior;
  indicator_spec_t indicator;
  label_spec_t label;
} element_spec_t;

#endif
