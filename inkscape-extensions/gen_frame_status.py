#!/usr/bin/env python 
'''
Copyright (C) 2010 Said LANKRI

##QSvgStyle status generation from existing normal frame

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
import simplestyle, sys, copy, simpletransform
from math import *

class QS_genframestatus(inkex.Effect):
    def __init__(self):
        inkex.Effect.__init__(self)

        self.OptionParser.add_option("--basename",
                        action="store", type="string", 
                        dest="basename", default="button",
                        help="Basename for existing normal frame")

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

    def copy_frame(self, basename,status, newbasename,newstatus, dx):
        current = self.getElementById(basename+'-'+status+'-top')
        if current != None:
            new = copy.deepcopy(current)
            new.set('id', newbasename+'-'+newstatus+'-top')
            if new.attrib.has_key('transform'):
                mat = simpletransform.parseTransform(new.get('transform'))
                mat[0][2] = mat[0][2]+dx
            else:
                mat = [[1,0,dx],[0,1,0]]
            new.set('transform', simpletransform.formatTransform(mat))
            current.getparent().append(new)

        current = self.getElementById(basename+'-'+status+'-bottom')
        if current != None:
            new = copy.deepcopy(current)
            new.set('id', newbasename+'-'+newstatus+'-bottom')
            if new.attrib.has_key('transform'):
                mat = simpletransform.parseTransform(new.get('transform'))
                mat[0][2] = mat[0][2]+dx
            else:
                mat = [[1,0,dx],[0,1,0]]
            new.set('transform', simpletransform.formatTransform(mat))
            current.getparent().append(new)

        current = self.getElementById(basename+'-'+status+'-left')
        if current != None:
            new = copy.deepcopy(current)
            new.set('id', newbasename+'-'+newstatus+'-left')
            if new.attrib.has_key('transform'):
                mat = simpletransform.parseTransform(new.get('transform'))
                mat[0][2] = mat[0][2]+dx
            else:
                mat = [[1,0,dx],[0,1,0]]
            new.set('transform', simpletransform.formatTransform(mat))
            current.getparent().append(new)

        current = self.getElementById(basename+'-'+status+'-right')
        if current != None:
            new = copy.deepcopy(current)
            new.set('id', newbasename+'-'+newstatus+'-right')
            if new.attrib.has_key('transform'):
                mat = simpletransform.parseTransform(new.get('transform'))
                mat[0][2] = mat[0][2]+dx
            else:
                mat = [[1,0,dx],[0,1,0]]
            new.set('transform', simpletransform.formatTransform(mat))
            current.getparent().append(new)

        current = self.getElementById(basename+'-'+status+'-topleft')
        if current != None:
            new = copy.deepcopy(current)
            new.set('id', newbasename+'-'+newstatus+'-topleft')
            if new.attrib.has_key('transform'):
                mat = simpletransform.parseTransform(new.get('transform'))
                mat[0][2] = mat[0][2]+dx
            else:
                mat = [[1,0,dx],[0,1,0]]
            new.set('transform', simpletransform.formatTransform(mat))
            current.getparent().append(new)

        current = self.getElementById(basename+'-'+status+'-topright')
        if current != None:
            new = copy.deepcopy(current)
            new.set('id', newbasename+'-'+newstatus+'-topright')
            if new.attrib.has_key('transform'):
                mat = simpletransform.parseTransform(new.get('transform'))
                mat[0][2] = mat[0][2]+dx
            else:
                mat = [[1,0,dx],[0,1,0]]
            new.set('transform', simpletransform.formatTransform(mat))
            current.getparent().append(new)

        current = self.getElementById(basename+'-'+status+'-bottomleft')
        if current != None:
            new = copy.deepcopy(current)
            new.set('id', newbasename+'-'+newstatus+'-bottomleft')
            if new.attrib.has_key('transform'):
                mat = simpletransform.parseTransform(new.get('transform'))
                mat[0][2] = mat[0][2]+dx
            else:
                mat = [[1,0,dx],[0,1,0]]
            new.set('transform', simpletransform.formatTransform(mat))
            current.getparent().append(new)

        current = self.getElementById(basename+'-'+status+'-bottomright')
        if current != None:
            new = copy.deepcopy(current)
            new.set('id', newbasename+'-'+newstatus+'-bottomright')
            if new.attrib.has_key('transform'):
                mat = simpletransform.parseTransform(new.get('transform'))
                mat[0][2] = mat[0][2]+dx
            else:
                mat = [[1,0,dx],[0,1,0]]
            new.set('transform', simpletransform.formatTransform(mat))
            current.getparent().append(new)

    def effect(self):

        dx = 30
        if self.options.focused:
            self.copy_frame(self.options.basename,'normal', self.options.basename,'focused', dx)

        if self.options.pressed:
            self.copy_frame(self.options.basename,'normal', self.options.basename,'pressed', 2*dx)

        if self.options.toggled:
            self.copy_frame(self.options.basename,'normal', self.options.basename,'toggled', 3*dx)

        if self.options.pressed:
            self.copy_frame(self.options.basename,'normal', self.options.basename,'disabled', 4*dx)


if __name__ == '__main__':
    e = QS_genframestatus()
    e.affect()


# vim: expandtab shiftwidth=4 tabstop=8 softtabstop=4 encoding=utf-8 textwidth=99
