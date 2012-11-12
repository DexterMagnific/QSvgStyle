#!/usr/bin/env python 
'''
Copyright (C) 2010 Said LANKRI

##QSvgStyle automatic new interior generation

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
'''

import inkex
import simplestyle, sys
from math import *

def draw_SVG_rect(x,y,w,h, fill, parent):
    style = { 'stroke': 'none', 'fill':fill}
    rect_attribs = {'style':simplestyle.formatStyle(style),
                    'x':str(x), 'y':str(y), 'width':str(w), 'height':str(h)}
    inkex.etree.SubElement(parent, inkex.addNS('rect','svg'), rect_attribs )

def draw_SVG_text(x,y, text, parent):
    text_attribs = {'x':str(x), 'y':str(y)}
    inkex.etree.SubElement(parent, inkex.addNS('text','svg'), text_attribs )

def draw_interior(x,y, w,h, parent,basename,status):
    g = inkex.etree.SubElement(parent, 'g', {'id':basename+'-'+status})
    draw_SVG_rect(x,y, w,h, '#00FF00', g)

class QS_geninterior(inkex.Effect):
    def __init__(self):
        inkex.Effect.__init__(self)

        self.OptionParser.add_option("--basename",
                        action="store", type="string", 
                        dest="basename", default="button",
                        help="Basename for frame element names")

        self.OptionParser.add_option("--normal",
                        action="store", type="inkbool", 
                        dest="normal", default=1,
                        help="Check to generate <basename>-normal elements")

        self.OptionParser.add_option("--focused",
                        action="store", type="inkbool", 
                        dest="focused", default=1,
                        help="Check to generate <basename>-focused elements")

        self.OptionParser.add_option("--pressed",
                        action="store", type="inkbool", 
                        dest="pressed", default=1,
                        help="Check to generate <basename>-pressed elements")

        self.OptionParser.add_option("--toggled",
                        action="store", type="inkbool", 
                        dest="toggled", default=1,
                        help="Check to generate <basename>-toggled elements")

        self.OptionParser.add_option("--disabled",
                        action="store", type="inkbool", 
                        dest="disabled", default=1,
                        help="Check to generate <basename>-disabled elements")

        self.OptionParser.add_option("--width",
                        action="store", type="int", 
                        dest="width", default=30,
                        help="Interior width")

        self.OptionParser.add_option("--height",
                        action="store", type="int", 
                        dest="height", default=30,
                        help="Interior height")

    def effect(self):
        w = 10
        h = 10
        
        x = self.view_center[0]
        y = self.view_center[1]

        if (self.options.normal):
            draw_interior(
                x,y,
                self.options.width,self.options.height, 
                self.current_layer,
                self.options.basename,'normal')

        x = x+self.options.width+10
        if (self.options.focused):
            draw_interior(
                x,y,
                self.options.width,self.options.height, 
                self.current_layer,
                self.options.basename,'focused')

        x = x+self.options.width+10
        if (self.options.pressed):
            draw_interior(
                x,y,
                self.options.width,self.options.height, 
                self.current_layer,
                self.options.basename,'pressed')

        x = x+self.options.width+10
        if (self.options.toggled):
            draw_interior(
                x,y,
                self.options.width,self.options.height, 
                self.current_layer,
                self.options.basename,'toggled')

        x = x+self.options.width+10
        if (self.options.disabled):
            draw_interior(
                x,y,
                self.options.width,self.options.height, 
                self.current_layer,
                self.options.basename,'disabled')

if __name__ == '__main__':
    e = QS_geninterior()
    e.affect()


# vim: expandtab shiftwidth=4 tabstop=8 softtabstop=4 encoding=utf-8 textwidth=99
