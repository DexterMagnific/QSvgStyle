#!/usr/bin/env python 
'''
Copyright (C) 2010 Said LANKRI

##QSvgStyle automatic new frame generation

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

def draw_SVG_line(x1, y1, x2, y2, width, name, parent):
    style = { 'stroke': '#000000', 'stroke-width':str(width), 'fill': 'none' }
    line_attribs = {'style':simplestyle.formatStyle(style),
                    inkex.addNS('label','inkscape'):name,
                    'd':'M '+str(x1)+','+str(y1)+' L '+str(x2)+','+str(y2)}
    inkex.etree.SubElement(parent, inkex.addNS('path','svg'), line_attribs )
    
def draw_SVG_rect(x,y,w,h, fill, parent):
    style = { 'stroke': 'none', 'fill':fill}
    rect_attribs = {'style':simplestyle.formatStyle(style),
                    'x':str(x), 'y':str(y), 'width':str(w), 'height':str(h)}
    inkex.etree.SubElement(parent, inkex.addNS('rect','svg'), rect_attribs )

def draw_frame(x,y, top,bottom,left,right, w,h, parent,basename,status):
    rw = 1

    g = inkex.etree.SubElement(parent, 'g', {'id':basename+'-'+status+'-top'})
    for i in range(0,top):
        draw_SVG_rect(x+(left+1)*rw,y+rw*i, w,rw, '#FF0000', g)
        
    g = inkex.etree.SubElement(parent, 'g', {'id':basename+'-'+status+'-bottom'})
    for i in range(0,bottom):
        draw_SVG_rect(x+(left+1)*rw,y+(top+2)*rw+h+rw*i, w,rw,  '#FF0000', g)
        
    g = inkex.etree.SubElement(parent, 'g', {'id':basename+'-'+status+'-left'})
    for i in range(0,left):
        draw_SVG_rect(x+rw*i,y+rw*(top+1), rw,h,  '#FF0000', g)
        
    g = inkex.etree.SubElement(parent, 'g', {'id':basename+'-'+status+'-right'})
    for i in range(0,left):
        draw_SVG_rect(x+(left+2)*rw+w+rw*i,y+rw*(top+1), rw,h,  '#FF0000', g)

    g = inkex.etree.SubElement(parent, 'g', {'id':basename+'-'+status+'-topleft'})
    for i in range(0,left):
        for j in range(0,top):
            draw_SVG_rect(x+rw*i,y+rw*j, rw,rw,  '#FF0000', g)

    g = inkex.etree.SubElement(parent, 'g', {'id':basename+'-'+status+'-bottomleft'})
    for i in range(0,left):
        for j in range(0,bottom):
            draw_SVG_rect(x+rw*i,y+(top+2)*rw+h+rw*j, rw,rw,  '#FF0000', g)

    g = inkex.etree.SubElement(parent, 'g', {'id':basename+'-'+status+'-topright'})
    for i in range(0,left):
        for j in range(0,top):
            draw_SVG_rect(x+(left+2)*rw+w+rw*i,y+rw*j, rw,rw,  '#FF0000', g)

    g = inkex.etree.SubElement(parent, 'g', {'id':basename+'-'+status+'-bottomright'})
    for i in range(0,left):
        for j in range(0,bottom):
            draw_SVG_rect(x+(left+2)*rw+w+rw*i,y+(top+2)*rw+h+rw*j, rw,rw,  '#FF0000', g)

class QS_genframe(inkex.Effect):
    def __init__(self):
        inkex.Effect.__init__(self)
        self.OptionParser.add_option("--top",
                        action="store", type="int", 
                        dest="top", default=1,
                        help="Top frame width")

        self.OptionParser.add_option("--bottom",
                        action="store", type="int", 
                        dest="bottom", default=1,
                        help="Bottom frame width")

        self.OptionParser.add_option("--left",
                        action="store", type="int", 
                        dest="left", default=1,
                        help="Top frame width")

        self.OptionParser.add_option("--right",
                        action="store", type="int", 
                        dest="right", default=1,
                        help="Top frame width")
 
        self.OptionParser.add_option("--rounded",
                        action="store", type="inkbool", 
                        dest="rounded", default=False,
                        help="Generate rounded frame")

        self.OptionParser.add_option("--basename",
                        action="store", type="string", 
                        dest="basename", default="button",
                        help="Basename for frame element names")

        self.OptionParser.add_option("--normal",
                        action="store", type="inkbool", 
                        dest="normal", default=1,
                        help="Check to generate <basename>-<side>-normal elements")

        self.OptionParser.add_option("--focused",
                        action="store", type="inkbool", 
                        dest="focused", default=1,
                        help="Check to generate <basename>-<side>-focused elements")

        self.OptionParser.add_option("--pressed",
                        action="store", type="inkbool", 
                        dest="pressed", default=1,
                        help="Check to generate <basename>-<side>-pressed elements")

        self.OptionParser.add_option("--toggled",
                        action="store", type="inkbool", 
                        dest="toggled", default=1,
                        help="Check to generate <basename>-<side>-toggled elements")

        self.OptionParser.add_option("--disabled",
                        action="store", type="inkbool", 
                        dest="disabled", default=1,
                        help="Check to generate <basename>-<side>-disabled elements")

        self.OptionParser.add_option("--width",
                        action="store", type="int", 
                        dest="width", default=30,
                        help="Frame width")

        self.OptionParser.add_option("--height",
                        action="store", type="int", 
                        dest="height", default=30,
                        help="Frame height")


    def effect(self):
        w = 10
        h = 10
        
        x = self.view_center[0]
        y = self.view_center[1]

        if (self.options.normal):
            draw_frame(
                x,y,
                self.options.top,self.options.bottom,self.options.left,self.options.right, 
                self.options.width,self.options.height, 
                self.current_layer,
                self.options.basename,'normal')

        x = x+self.options.width+10
        if (self.options.focused):
            draw_frame(
                x,y,
                self.options.top,self.options.bottom,self.options.left,self.options.right, 
                self.options.width,self.options.height, 
                self.current_layer,
                self.options.basename,'focused')

        x = x+self.options.width+10
        if (self.options.pressed):
            draw_frame(
                x,y,
                self.options.top,self.options.bottom,self.options.left,self.options.right, 
                self.options.width,self.options.height, 
                self.current_layer,
                self.options.basename,'pressed')

        x = x+self.options.width+10
        if (self.options.toggled):
            draw_frame(
                x,y,
                self.options.top,self.options.bottom,self.options.left,self.options.right, 
                self.options.width,self.options.height, 
                self.current_layer,
                self.options.basename,'toggled')

        x = x+self.options.width+10
        if (self.options.disabled):
            draw_frame(
                x,y,
                self.options.top,self.options.bottom,self.options.left,self.options.right, 
                self.options.width,self.options.height, 
                self.current_layer,
                self.options.basename,'disabled')

if __name__ == '__main__':
    e = QS_genframe()
    e.affect()


# vim: expandtab shiftwidth=4 tabstop=8 softtabstop=4 encoding=utf-8 textwidth=99
