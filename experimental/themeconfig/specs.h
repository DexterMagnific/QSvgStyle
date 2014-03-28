/***************************************************************************
 *   Copyright (C) 2009 by Saïd LANKRI   *
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

#include <limits.h>

#include <QString>
#include <QDebug>

/** Maximum depth in inheritance lookup */
#define INHERIT_LOOKUP_DEPTH 2

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

  value_t () { present = false; };
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
};

/** Generic information about a theme */
typedef struct {
  value_t<QString> author;
  value_t<QString> comment;
  /* Animations enabled */
  value_t<bool> animated;
  /* Animation step in msecs */
  value_t<int> step;
} theme_spec_t;

/** Generic information about a frame */
typedef struct {
  /* Inherited Element */
  value_t<QString> inherits;
  /* Element name */
  value_t<QString> element;
  /* has frame ? */
  value_t<bool> hasFrame;
  /* Allow capsule grouping ? */
  value_t<bool> hasCapsule;
  /* frame size */
  value_t<int> top,bottom,left,right;
  /* widget position in a capsule, used internally */
  value_t<int> capsuleH,capsuleV; // 0 : middle, -1 : left,top, 1 : right,bottom, 2 : left+right,top+bottom
  /* positions where the frame is 'cut' (for each line), used internally */
  value_t<int> y0c0,y0c1, y1c0,y1c1, x0c0,x0c1, x1c0,x1c1;
  /* number of animations */
  value_t<int> animationFrames;
  /* loop animation */
  value_t<int> loopAnimation;
} frame_spec_t;

/** Generic information about a frame interior */
typedef struct {
  /* Inherited Element */
  value_t<QString> inherits;
  /* Element name */
  value_t<QString> element;
  /* has interior */
  value_t<bool> hasInterior;
  /* has margins ? */
  value_t<bool> hasMargin;
  /* pattern size */
  value_t<int> px,py;
  /* number of animations */
  value_t<int> animationFrames;
  /* loop animation */
  value_t<int> loopAnimation;
} interior_spec_t;

/** Generic information about widget indicators */
typedef struct {
  /* Inherited Element */
  value_t<QString> inherits;
  /* Element name */
  value_t<QString> element;
  /* size */
  value_t<int> size;
  /* number of animations */
  value_t<int> animationFrames;
  /* loop animation */
  value_t<int> loopAnimation;
} indicator_spec_t;

/** Generic information about the size of a widget */
typedef struct {
  /* min/fixed width or height. <= 0 : does not apply */
  value_t<int> minH,fixedH;
  value_t<int> minW,fixedW;
} size_spec_t;

/** Generic information about text and icons (labels) */
typedef struct {
  /* Inherited Element */
  value_t<QString> inherits;
  /* has shadow */
  value_t<bool> hasShadow;
  /* shadow shift */
  value_t<int> xshift,yshift;
  /* shadow color */
  value_t<int> r,g,b,a;
  /* shadow depth */
  value_t<int> depth;
  /* has margins ? */
  value_t<bool> hasMargin;
  /* text margins */
  value_t<int> top,bottom,left,right;
  /* text-icon spacing */
  value_t<int> tispace;
} label_spec_t;

/** Generic information about an element */
typedef struct {
  /* Inherited Element */
  value_t<QString> inherits;
  /* Other specs */
  frame_spec_t frame;
  interior_spec_t interior;
  indicator_spec_t indicator;
  size_spec_t size;
  label_spec_t label;
} element_spec_t;

/** Fills the frame spec with default values */
static inline void default_frame_spec(frame_spec_t &fspec) {
  fspec.hasFrame = false;
  fspec.hasCapsule = false;
  fspec.inherits = QString::null;
  fspec.element = QString::null;
  fspec.top = fspec.bottom = fspec.left = fspec.right = 0;
  fspec.capsuleH = fspec.capsuleV = 0;
  fspec.x0c0 = fspec.x0c1 = fspec.x1c0 = fspec.x1c1 = fspec.y0c0 = fspec.y0c1 =
    fspec.y1c0 = fspec.y1c1 = -1;
  fspec.animationFrames = 0;
  fspec.loopAnimation = true;
}

/** Fills the interior with default values */
static inline void default_interior_spec(interior_spec_t &ispec) {
  ispec.hasInterior = true;
  ispec.hasMargin = false;
  ispec.inherits = QString::null;
  ispec.element = QString::null;
  ispec.px = ispec.py = 0;
  ispec.animationFrames = 0;
  ispec.loopAnimation = true;
}

/** Fills the indicator spec with default values */
static inline void default_indicator_spec(indicator_spec_t &dspec) {
  dspec.inherits = QString::null;
  dspec.element = QString::null;
  dspec.size = 15;
  dspec.animationFrames = 0;
  dspec.loopAnimation = true;
}

/** Fills the label spec with default values */
static inline void default_label_spec(label_spec_t &lspec) {
  lspec.hasShadow = false;
  lspec.xshift = 0;
  lspec.yshift = 1;
  lspec.r = lspec.g = lspec.b = 0;
  lspec.a = 255;
  lspec.depth = 1;
  lspec.hasMargin = false;
  lspec.top = lspec.bottom = lspec.left = lspec.right = 0;
  lspec.tispace = 0;
}

/** Fills the size spec with default values */
static inline void default_size_spec(size_spec_t &sspec) {
  sspec.minH = sspec.fixedH = sspec.minW = sspec.fixedW = -1;
}

/** Fills the widget spec with default values */
static inline void default_element_spec(element_spec_t &espec) {
  espec.inherits = QString::null;
  default_frame_spec(espec.frame);
  default_interior_spec(espec.interior);
  default_indicator_spec(espec.indicator);
  default_label_spec(espec.label);
  default_size_spec(espec.size);
}

/** Fills the widget spec with default values */
static inline void default_theme_spec(theme_spec_t &tspec) {
  tspec.author = QString::null;
  tspec.comment = QString::null;
  tspec.animated = false;
  tspec.step = 250;
}

/** Printing function for frame spec */
static inline void print_frame_spec(const frame_spec_t &fspec) {
  qDebug() << "frame={ "
           << "hasFrame=" << fspec.hasFrame << ", "
           << "inherits=" << fspec.inherits << ", "
           << "element="  << fspec.element << ", "
           << "size(t,b,l,r)=("     << fspec.top << "," << fspec.bottom << "," << fspec.left << "," << fspec.right << "), "
           << "capsule(h,v)=(" << fspec.capsuleH << "," << fspec.capsuleV << "), "
           << "animFrames=" << fspec.animationFrames << ", "
           << "loopAnim=" << fspec.loopAnimation
           << " }";
}

/** Printing function for interior spec */
static inline void print_interior_spec(const interior_spec_t &ispec) {
  qDebug() << "interior={ "
           << "hasInterior=" << ispec.hasInterior << ", "
           << "inherits=" << ispec.inherits << ", "
           << "element="  << ispec.element << ", "
           << "hasMargins=" << ispec.hasMargin << ", "
           << "pattern(h,v)=("  << ispec.px << "," << ispec.py << "), "
           << "animFrames=" << ispec.animationFrames << ", "
           << "loopAnim=" << ispec.loopAnimation
           << " }";
}

#endif
